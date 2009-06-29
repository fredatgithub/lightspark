/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

//#define __STDC_LIMIT_MACROS
#include <llvm/Module.h>
#include <llvm/DerivedTypes.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Constants.h> 
#include <llvm/Support/IRBuilder.h> 
#include <llvm/Target/TargetData.h>
#include <math.h>
#include "abc.h"
#include "logger.h"
#include "swftypes.h"
#include <sstream>
#include "swf.h"
#include "flashevents.h"

llvm::ExecutionEngine* ABCVm::ex;

extern __thread SystemState* sys;

using namespace std;
long timeDiff(timespec& s, timespec& d);

void ignore(istream& i, int count);

DoABCTag::DoABCTag(RECORDHEADER h, std::istream& in):DisplayListTag(h,in),done(false)
{
	int dest=in.tellg();
	dest+=getSize();
	in >> Flags >> Name;
	LOG(CALLS,"DoABCTag Name: " << Name);

	vm=new ABCVm(sys,in);
	sys->currentVm=vm;

	if(dest!=in.tellg())
		LOG(ERROR,"Corrupted ABC data: missing " << dest-in.tellg());
}

int DoABCTag::getDepth() const
{
	//The first thing when encoutering showframe
	return -200;
}

void DoABCTag::Render()
{
	LOG(CALLS,"ABC Exec " << Name);
	if(done)
		return;
	pthread_create(&thread,NULL,(void* (*)(void*))ABCVm::Run,vm);
	done=true;
}

SymbolClassTag::SymbolClassTag(RECORDHEADER h, istream& in):DisplayListTag(h,in)
{
	LOG(TRACE,"SymbolClassTag");
	in >> NumSymbols;

	Tags.resize(NumSymbols);
	Names.resize(NumSymbols);

	for(int i=0;i<NumSymbols;i++)
	{
		in >> Tags[i] >> Names[i];
		
		pair < map<int, bind_candidates>::iterator, map<int, bind_candidates>::iterator> range=
			sys->bind_canditates_map.equal_range(Tags[i]);

		if(range.first==range.second)
			LOG(NOT_IMPLEMENTED,"SymbolClass refers to not named object " << Names[i] << " id " << Tags[i]);
		for(range.first;range.first!=range.second;range.first++)
		{
			to_bind.insert(*range.first);
		}
	}

	sys->bind_canditates_map.clear();
}

int SymbolClassTag::getDepth() const
{
	//After DoABCTag execution, but before any object
	return -100;
}

void SymbolClassTag::Render()
{
	//Should be a control tag
	if(done)
		return;
	LOG(TRACE,"SymbolClassTag Render");

	for(int i=0;i<NumSymbols;i++)
	{
		LOG(TRACE,Tags[i] << ' ' << Names[i]);
		if(Tags[i]==0)
		{
			sys->currentVm->addEvent(NULL,new BindClassEvent(sys,NULL,Names[i],"",NULL));
		}
		else
		{
			DictionaryTag* t=sys->dictionaryLookup(Tags[i]);
			ASObject* base=dynamic_cast<ASObject*>(t);
			if(base==NULL)
			{
				LOG(ERROR,"Base in not an ASObject");
				abort();
			}
			else
			{
				pair < map<int, bind_candidates>::iterator, map<int, bind_candidates>::iterator> range=
					to_bind.equal_range(Tags[i]);

				if(range.first==range.second)
				{
					ASObject* b2=dynamic_cast<ASObject*>(base->clone());
					if(b2==NULL)
					{
						LOG(ERROR,"WTF?");
						abort();
					}
					sys->currentVm->addEvent(NULL, new BindClassEvent(b2, NULL, Names[i], "", NULL));
				}
				for(range.first;range.first!=range.second;range.first++)
				{
					ASObject* b2=dynamic_cast<ASObject*>(base->clone());
					sys->currentVm->addEvent(NULL, new BindClassEvent(b2, 
						range.first->second.parent, Names[i], range.first->second.obj_name, 
						range.first->second.placed_by));
				}
			}
		}
	}
	//We stop execution until execution engine catches up
	sem_t s;
	sem_init(&s,0,0);
	sys->currentVm->addEvent(NULL, new SynchronizationEvent(&s));
	sem_wait(&s);
	//Now the bindings are effective

	done=true;
}

//Be careful, arguments nubering starts from 1
ISWFObject* ABCVm::argumentDumper(arguments* arg, uint32_t n)
{
	return arg->at(n-1);
}

void ABCVm::registerFunctions()
{
	vector<const llvm::Type*> sig;
	const llvm::Type* ptr_type=ex->getTargetData()->getIntPtrType();

	sig.push_back(llvm::IntegerType::get(32));
	llvm::FunctionType* FT=llvm::FunctionType::get(llvm::Type::VoidTy, sig, false);
	llvm::Function* F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"not_impl",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::not_impl);
	sig.clear();

	sig.push_back(llvm::PointerType::getUnqual(ptr_type));
	sig.push_back(llvm::IntegerType::get(32));
	FT=llvm::FunctionType::get(llvm::PointerType::getUnqual(ptr_type), sig, false);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"argumentDumper",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::argumentDumper);
	sig.clear();

	// (method_info*)
	sig.push_back(llvm::PointerType::getUnqual(ptr_type));
	FT=llvm::FunctionType::get(llvm::Type::VoidTy, sig, false);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"pushScope",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::pushScope);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"debug",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::debug);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"decRef",module);
	ex->addGlobalMapping(F,(void*)&ISWFObject::s_decRef);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"incRef",module);
	ex->addGlobalMapping(F,(void*)&ISWFObject::s_incRef);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"method_reset",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::method_reset);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"convert_d",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::convert_d);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"nextValue",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::nextValue);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"nextName",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::nextName);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"convert_b",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::convert_b);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"convert_i",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::convert_i);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"coerce_a",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::coerce_a);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"coerce_s",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::coerce_s);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"throw",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::_throw);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"getGlobalScope",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::getGlobalScope);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"decrement",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::decrement);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"decrement_i",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::decrement_i);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"increment",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::increment);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"increment_i",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::increment_i);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"pop",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::pop);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"lessThan",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::lessThan);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"greaterThan",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::greaterThan);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"not",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::_not);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"strictEquals",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::strictEquals);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"equals",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::equals);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"dup",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::dup);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"subtract",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::subtract);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"divide",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::divide);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"multiply",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::multiply);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"add",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::add);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"swap",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::swap);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"pushNaN",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::pushNaN);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"pushNull",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::pushNull);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"pushTrue",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::pushTrue);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"pushFalse",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::pushFalse);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"isTypelate",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::isTypelate);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"asTypelate",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::asTypelate);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"popScope",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::popScope);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"newActivation",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::newActivation);

	//Lazy pushing
	FT=llvm::FunctionType::get(llvm::PointerType::getUnqual(ptr_type), sig, false);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"pushUndefined",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::pushUndefined);
	//End of lazy pushing

	// (method_info*,int)
	sig.push_back(llvm::IntegerType::get(32));
	FT=llvm::FunctionType::get(llvm::Type::VoidTy, sig, false);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"ifNLT",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::ifNLT);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"ifNGE",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::ifNGE);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"ifLT",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::ifLT);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"ifStrictNE",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::ifStrictNE);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"ifNe",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::ifNe);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"ifStrictEq",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::ifStrictEq);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"ifEq",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::ifEq);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"ifTrue",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::ifTrue);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"ifFalse",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::ifFalse);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"newCatch",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::newCatch);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"setSuper",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::setSuper);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"newFunction",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::newFunction);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"newObject",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::newObject);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"deleteProperty",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::deleteProperty);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"jump",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::jump);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"getSlot",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::getSlot);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"setSlot",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::setSlot);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"getLocal",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::getLocal);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"setLocal",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::setLocal);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"call",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::call);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"coerce",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::coerce);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"getLex",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::getLex);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"findPropStrict",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::findPropStrict);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"pushDouble",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::pushDouble);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"pushInt",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::pushInt);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"pushShort",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::pushShort);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"pushByte",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::pushByte);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"incLocal_i",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::incLocal_i);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"getProperty",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::getProperty);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"setProperty",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::setProperty);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"findProperty",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::findProperty);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"construct",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::construct);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"constructSuper",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::constructSuper);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"newArray",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::newArray);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"newClass",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::newClass);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"pushString",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::pushString);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"initProperty",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::initProperty);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"kill",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::kill);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"getScopeObject",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::getScopeObject);

	// (method_info*,int,int)
	sig.push_back(llvm::IntegerType::get(32));
	FT=llvm::FunctionType::get(llvm::Type::VoidTy, sig, false);
	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"callPropVoid",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::callPropVoid);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"callSuper",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::callSuper);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"callSuperVoid",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::callSuperVoid);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"callProperty",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::callProperty);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"constructProp",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::constructProp);

	F=llvm::Function::Create(FT,llvm::Function::ExternalLinkage,"hasNext2",module);
	ex->addGlobalMapping(F,(void*)&ABCVm::hasNext2);
}

void ABCVm::registerClasses()
{
	//All valid_classes should be registered in the Global object
/*	map<string,int>::iterator it=valid_classes.begin();
	for(it;it!=valid_classes.end();it++)
	{
		Global.setVariableByName
	}*/

	//Register predefined types, ASObject are enough for not implemented classes
	Global.setVariableByName("Object",new ASObject);
	valid_classes["Object"]=-1;
	Global.setVariableByName(".int",new ASObject);
	valid_classes[".int"]=-1;
	Global.setVariableByName(".Boolean",new ASObject);
	valid_classes[".Boolean"]=-1;
	Global.setVariableByName("Number",new ASObject);
	valid_classes["Number"]=-1;
	Global.setVariableByName("String",new ASString);
	valid_classes["String"]=-1;
	Global.setVariableByName("flash.display.MovieClip",new ASMovieClip);
	valid_classes["flash.display.MovieClip"]=-1;
	Global.setVariableByName("flash.display.DisplayObject",new ASObject);
	valid_classes["flash.display.DisplayObject"]=-1;
	Global.setVariableByName("flash.display.SimpleButton",new ASObject);
	valid_classes["flash.display.SimpleButton"]=-1;
	Global.setVariableByName("flash.text.TextField",new ASObject);
	valid_classes["flash.text.TextField"]=-1;
	Global.setVariableByName("flash.xml.XMLDocument",new ASObject);
	valid_classes["flash.xml.XMLDocument"]=-1;
	Global.setVariableByName("flash.utils.Timer",new ASObject);
	valid_classes["flash.utils.Timer"]=-1;
	Global.setVariableByName("flash.text.TextFormat",new ASObject);
	valid_classes["flash.text.TextFormat"]=-1;
	Global.setVariableByName("flash.text.TextFieldType",new ASObject);
	valid_classes["flash.text.TextFieldType"]=-1;
	Global.setVariableByName("flash.utils.Dictionary",new ASObject);
	valid_classes["flash.utils.Dictionary"]=-1;
	Global.setVariableByName("flash.geom.Rectangle",new ASObject);
	valid_classes["flash.geom.Rectangle"]=-1;
	Global.setVariableByName("Array",new ASArray);
	valid_classes["Array"]=-1;
	Global.setVariableByName("undefined",new Undefined);
	valid_classes["undefined"]=-1;

//	Global.setVariableByName(".Error",new ASObject);
	Global.setVariableByName("Math",new Math);

	Global.setVariableByName("flash.events.EventDispatcher",new ASObject);
	Global.setVariableByName("flash.display.InteractiveObject",new ASObject);
	Global.setVariableByName("flash.display.DisplayObjectContainer",new ASObject);
	Global.setVariableByName("flash.display.Sprite",new ASObject);
	Global.setVariableByName("flash.events.Event",new Event(""));
	Global.setVariableByName("flash.events.MouseEvent",new MouseEvent);
	Global.setVariableByName("flash.net.LocalConnection",new ASObject);
	Global.setVariableByName("flash.utils.Proxy",new ASObject);
	Global.setVariableByName("flash.events.ProgressEvent",new ASObject);
}

string ABCVm::getMultinameString(unsigned int mi, method_info* th) const
{
	const multiname_info* m=&constant_pool.multinames[mi];
	string ret;
	switch(m->kind)
	{
		case 0x07:
		{
			const namespace_info* n=&constant_pool.namespaces[m->ns];
			if(n->name)
			{
				ret=getString(n->name);
				if(ret=="")
					ret=getString(m->name);
				else
					ret+="."+getString(m->name);
			}
			else
				ret=getString(m->name);
			break;
		}
		case 0x09:
		{
			const ns_set_info* s=&constant_pool.ns_sets[m->ns_set];
			if(s->count!=1)
			{
				LOG(ERROR,"Multiname on namespace set not really supported yet");
				//printNamespaceSet(s);
				ret=getString(m->name);
			}
			else
			{
				const namespace_info* n=&constant_pool.namespaces[s->ns[0]];
				ret=getString(n->name);
				if(ret=="")
					ret=getString(m->name);
				else
					ret+="."+getString(m->name);
			}
			break;
		}
		case 0x1b:
		{
			string name;
			if(th!=NULL)
			{
				ISWFObject* n=th->runtime_stack_pop();
				/*if(n->getObjectType()!=T_STRING)
				{
					LOG(ERROR,"Name on the stack should be a string");
					name="<Invalid>";
				}
				else*/
				name=n->toString();
				n->decRef();
			}
			else
				name="<Invalid>";
			//We currently assume that a null namespace is good
			const ns_set_info* s=&constant_pool.ns_sets[m->ns_set];
			LOG(ERROR,"Multiname on namespace set not really supported yet");
			//printNamespaceSet(s);
			return name;
			break;
		}
/*		case 0x0d:
			LOG(CALLS, "QNameA");
			break;
		case 0x0f:
			LOG(CALLS, "RTQName");
			break;
		case 0x10:
			LOG(CALLS, "RTQNameA");
			break;
		case 0x11:
			LOG(CALLS, "RTQNameL");
			break;
		case 0x12:
			LOG(CALLS, "RTQNameLA");
			break;
		case 0x0e:
			LOG(CALLS, "MultinameA");
			break;
		case 0x1c:
			LOG(CALLS, "MultinameLA");
			break;*/
		default:
			LOG(ERROR,"Multiname to String not yet implemented for this kind " << hex << m->kind);
			break;
	}
	return ret;
}

ABCVm::ABCVm(SystemState* s,istream& in):shutdown(false),m_sys(s),running(false)
{
	in >> minor >> major;
	LOG(CALLS,"ABCVm version " << major << '.' << minor);
	in >> constant_pool;

	in >> method_count;
	methods.resize(method_count);
	for(int i=0;i<method_count;i++)
		in >> methods[i];

	in >> metadata_count;
	metadata.resize(metadata_count);
	for(int i=0;i<metadata_count;i++)
		in >> metadata[i];

	in >> class_count;
	instances.resize(class_count);
	for(int i=0;i<class_count;i++)
	{
		in >> instances[i];
		//Link instance names with classes
		valid_classes[getMultinameString(instances[i].name)]=i;
	}
	classes.resize(class_count);
	for(int i=0;i<class_count;i++)
	{
		in >> classes[i];
/*		LOG(CALLS,"Declared class " << getMultinameString(instances[i].name));
		for(int j=0;j<classes[i].trait_count;j++)
			LOG(CALLS,getMultinameString(classes[i].traits[j].name));
		for(int j=0;j<instances[i].trait_count;j++)
			LOG(CALLS,getMultinameString(instances[i].traits[j].name));*/
	}

	in >> script_count;
	scripts.resize(script_count);
	for(int i=0;i<script_count;i++)
		in >> scripts[i];

	in >> method_body_count;
	method_body.resize(method_body_count);
	for(int i=0;i<method_body_count;i++)
	{
		in >> method_body[i];
		//Link method body with method signature
		if(methods[method_body[i].method].body!=NULL)
			LOG(ERROR,"Duplicate body assigment")
		else
			methods[method_body[i].method].body=&method_body[i];
	}

	sem_init(&mutex,0,1);
	sem_init(&sem_event_count,0,0);
}

void ABCVm::handleEvent()
{
	sem_wait(&mutex);
	pair<InteractiveObject*,Event*> e=events_queue.front();
	events_queue.pop_front();
	if(e.first)
		e.first->handleEvent(e.second);
	else
	{
		//Should be handled by the Vm itself
		switch(e.second->getEventType())
		{
			case BIND_CLASS:
			{
				BindClassEvent* ev=dynamic_cast<BindClassEvent*>(e.second);
				arguments args;
				args.incRef();
				args.push(new Null);
				ISWFObject* o=buildNamedClass(ev->class_name,ev->base,&args);
				if(ev->parent)
				{
					ev->parent->setVariableByName(ev->obj_name,o);
					ev->placed_by->setWrapped(o);
				}

				break;
			}
			case SHUTDOWN:
				shutdown=true;
				break;
			case SYNC:
			{
				SynchronizationEvent* ev=dynamic_cast<SynchronizationEvent*>(e.second);
				ev->sync();
				break;
			}
			default:
				LOG(ERROR,"Not supported event");
				abort();
		}
	}
	sem_post(&mutex);
}

void ABCVm::addEvent(InteractiveObject* obj ,Event* ev)
{
	sem_wait(&mutex);
	events_queue.push_back(pair<InteractiveObject*,Event*>(obj, ev));
	sem_post(&sem_event_count);
	sem_post(&mutex);
}

void ABCVm::method_reset(method_info* th)
{
	th->stack_index=0;
	th->scope_stack.clear();
}

void dumpClasses(map<string,int>& maps)
{
	map<string,int>::iterator it=maps.begin();
	for(it;it!=maps.end();it++)
		cout << it->first << endl;
}

ISWFObject* ABCVm::buildNamedClass(const string& s, ASObject* base,arguments* args)
{
	map<string,int>::iterator it=valid_classes.find(s);
	if(it!=valid_classes.end())
		LOG(CALLS,"buildNameClass: Class " << s << " found")
	else
	{
		LOG(ERROR,"Class " << s << " not found");
		return new ASObject;
	}

	//TODO: Really build class
	int index=it->second;
	if(index==-1)
	{
		bool found;
		ISWFObject* r=Global.getVariableByName(it->first,found);
		if(!found)
		{
			LOG(ERROR,"Class " << it->first << " not found in global");
			abort();
		}
		if(r->getObjectType()==T_DEFINABLE)
			LOG(ERROR,"Class " << it->first << " is not yet valid");
		return r->clone();
	}
	else
	{
		LOG(CALLS,"Setting class name to " << s);
		base->class_name=s;
		bool found;
		ISWFObject* r=Global.getVariableByName(it->first,found);
		if(!found)
		{
			LOG(ERROR,"Class " << it->first << " not found in global");
			abort();
		}
		if(r->getObjectType()==T_DEFINABLE)
		{
			LOG(CALLS,"Class " << it->first << " is not yet valid");
			Definable* d=dynamic_cast<Definable*>(r);
			d->define(&Global);
			LOG(CALLS,"End of deferred init");
			r=Global.getVariableByName(it->first,found);
			if(!found)
			{
				LOG(ERROR,"Class " << it->first << " not found in global");
				abort();
			}
		}

		ASObject* ro=dynamic_cast<ASObject*>(r);
		if(ro==NULL)
		{
			LOG(ERROR,"Class is not as ASObject");
			abort();
		}
		base->prototype=ro;

		method_info* m=&methods[instances[index].init];
		synt_method(m);
		LOG(CALLS,"Building instance traits");
		for(int i=0;i<instances[index].trait_count;i++)
			buildTrait(base,&instances[index].traits[i]);

		LOG(CALLS,"Calling Instance init on " << s);
		//module.dump();
		if(m->f)
		{
			Function::as_function FP=(Function::as_function)ex->getPointerToFunction(m->f);
			args->incRef();
			FP(base,args);
			args->decRef();
		}
		return base;
	}
}

inline method_info* ABCVm::get_method(unsigned int m)
{
	if(m<method_count)
		return &methods[m];
	else
	{
		LOG(ERROR,"Requested invalid method");
		return NULL;
	}
}

void ABCVm::divide(method_info* th)
{
	ISWFObject* val2=th->runtime_stack_pop();
	ISWFObject* val1=th->runtime_stack_pop();

	Number num2(val2);
	Number num1(val1);

	val1->decRef();
	val2->decRef();
	th->runtime_stack_push(new Number(num1/num2));
	LOG(CALLS,"divide "  << num1 << '/' << num2);
}

void ABCVm::getGlobalScope(method_info* th)
{
	LOG(CALLS,"getGlobalScope");
	th->runtime_stack_push(th->scope_stack[0]);
	th->scope_stack[0]->incRef();
}

void ABCVm::decrement(method_info* th)
{
	LOG(NOT_IMPLEMENTED,"decrement");
}

void ABCVm::decrement_i(method_info* th)
{
	LOG(NOT_IMPLEMENTED,"decrement_i");
}

void ABCVm::increment(method_info* th)
{
	LOG(NOT_IMPLEMENTED,"increment");
}

void ABCVm::increment_i(method_info* th)
{
	LOG(NOT_IMPLEMENTED,"increment_i");
}

void ABCVm::subtract(method_info* th)
{
	ISWFObject* val2=th->runtime_stack_pop();
	ISWFObject* val1=th->runtime_stack_pop();

	Number num2(val2);
	Number num1(val1);
	val1->decRef();
	val2->decRef();
	th->runtime_stack_push(new Number(num1-num2));
	LOG(CALLS,"subtract " << num1 << '-' << num2);
}

void ABCVm::multiply(method_info* th)
{
	ISWFObject* val2=th->runtime_stack_pop();
	ISWFObject* val1=th->runtime_stack_pop();

	Number num2(val2);
	Number num1(val1);
	val1->decRef();
	val2->decRef();
	th->runtime_stack_push(new Number(num1*num2));
	LOG(CALLS,"multiply "  << num1 << '*' << num2);
}

void ABCVm::add(method_info* th)
{
	//Implement ECMA add algorithm, for XML and default
	ISWFObject* val2=th->runtime_stack_pop();
	ISWFObject* val1=th->runtime_stack_pop();
	if(val1->getObjectType()==T_NUMBER && val2->getObjectType()==T_NUMBER)
	{
		double num2=val2->toNumber();
		double num1=val1->toNumber();
		val1->decRef();
		val2->decRef();
		th->runtime_stack_push(new Number(num1+num2));
		LOG(CALLS,"add " << num1 << '+' << num2);
	}
	else if(val1->getObjectType()==T_STRING && val2->getObjectType()==T_STRING)
	{
		string a=val1->toString();
		string b=val2->toString();
		th->runtime_stack_push(new ASString(a+b));
		LOG(CALLS,"add " << a << '+' << b);
	}
	else if(val1->getObjectType()==T_NUMBER && val2->getObjectType()==T_INTEGER)
	{
		double num2=val2->toNumber();
		double num1=val1->toNumber();
		val1->decRef();
		val2->decRef();
		th->runtime_stack_push(new Number(num1+num2));
		LOG(CALLS,"add " << num1 << '+' << num2);
	}
	else
	{
		LOG(NOT_IMPLEMENTED,"Add between types " << val1->getObjectType() << ' ' << val2->getObjectType());
		th->runtime_stack_push(new Undefined);
	}

}

void ABCVm::isTypelate(method_info* th)
{
	LOG(NOT_IMPLEMENTED,"isTypelate");
}

void ABCVm::asTypelate(method_info* th)
{
	LOG(NOT_IMPLEMENTED,"asTypelate");
}

void ABCVm::nextValue(method_info* th)
{
	LOG(NOT_IMPLEMENTED,"nextValue");
}

void ABCVm::nextName(method_info* th)
{
	LOG(CALLS,"nextName");
	Integer* i=dynamic_cast<Integer*>(th->runtime_stack_pop());
	if(i==NULL)
	{
		LOG(ERROR,"Type mismatch");
		abort();
	}

	ISWFObject* obj=th->runtime_stack_pop();
	th->runtime_stack_push(new ASString(obj->getNameAt(*i-1)));

	i->decRef();
	obj->decRef();
}

void ABCVm::swap(method_info* th)
{
	LOG(NOT_IMPLEMENTED,"swap");
}

void ABCVm::newActivation(method_info* th)
{
	LOG(CALLS,"newActivation");
	//TODO: Should create a real activation object
	//TODO: Should method traits be added to the activation context?
	ASObject* act=new ASObject;
	for(int i=0;i<th->body->trait_count;i++)
		th->vm->buildTrait(act,&th->body->traits[i]);

	th->runtime_stack_push(act);
}

void ABCVm::popScope(method_info* th)
{
	LOG(NOT_IMPLEMENTED,"popScope");
}

void ABCVm::constructProp(method_info* th, int n, int m)
{
	string name=th->vm->getMultinameString(n);
	LOG(CALLS,"constructProp "<<name << ' ' << m);
	arguments args;
	args.resize(m);
	for(int i=0;i<m;i++)
		args.at(m-i-1)=th->runtime_stack_pop();

	ISWFObject* obj=th->runtime_stack_pop();
	bool found;
	ISWFObject* o=obj->getVariableByName(name,found);
	if(!found)
	{
		LOG(ERROR,"Could not resolve property");
		abort();
	}

	if(o->getObjectType()==T_DEFINABLE)
	{
		LOG(CALLS,"Deferred definition of property " << name);
		Definable* d=dynamic_cast<Definable*>(o);
		d->define(&th->vm->Global);
		o=obj->getVariableByName(name,found);
		LOG(CALLS,"End of deferred definition of property " << name);
	}

	LOG(CALLS,"Constructing");
	ISWFObject* ret=th->vm->buildNamedClass(name,new ASObject,&args);
	LOG(CALLS,"End of constructing");

	th->runtime_stack_push(ret);
}

void ABCVm::callProperty(method_info* th, int n, int m)
{
	//Should be called after arguments are popped
	string name=th->vm->getMultinameString(n);
	LOG(CALLS,"callProperty " << name << ' ' << m);
	arguments args;
	args.resize(m);
	for(int i=0;i<m;i++)
		args.at(m-i-1)=th->runtime_stack_pop();
	ISWFObject* obj=th->runtime_stack_pop();
	bool found;
	ISWFObject* o=obj->getVariableByName(name,found);
	if(found)
	{
		//If o is already a function call it, otherwise find the Call method
		if(o->getObjectType()==T_FUNCTION)
		{
			IFunction* f=dynamic_cast<IFunction*>(o);
			ISWFObject* ret=f->call(obj,&args);
			th->runtime_stack_push(ret);
		}
		else if(o->getObjectType()==T_UNDEFINED)
		{
			LOG(NOT_IMPLEMENTED,"We got a Undefined function");
			th->runtime_stack_push(new Undefined);
		}
		else if(o->getObjectType()==T_DEFINABLE)
		{
			LOG(NOT_IMPLEMENTED,"We got a function not yet valid");
			th->runtime_stack_push(new Undefined);
		}
		else
		{
			IFunction* f=dynamic_cast<IFunction*>(o->getVariableByName("Call",found));
			ISWFObject* ret=f->call(o,&args);
			th->runtime_stack_push(ret);
		}
	}
	else
	{
		LOG(NOT_IMPLEMENTED,"Calling an undefined function");
		th->runtime_stack_push(new Undefined);
	}
	obj->decRef();
}

void ABCVm::hasNext2(method_info* th, int n, int m)
{
	LOG(CALLS,"hasNext2 " << n << ' ' << m);
	ISWFObject* obj=th->locals[n];
	int cur_index=th->locals[m]->toInt();
	if(cur_index+1<=obj->numVariables())
	{
		th->locals[m]->decRef();
		th->locals[m]=new Integer(cur_index+1);
		th->runtime_stack_push(new Boolean(true));
	}
	else
	{
		obj->decRef();
		th->locals[n]=new Null;
		th->locals[m]->decRef();
		th->locals[m]=new Integer(0);
		th->runtime_stack_push(new Boolean(false));
	}
}

void ABCVm::callSuper(method_info* th, int n, int m)
{
	string name=th->vm->getMultinameString(n); 
	LOG(NOT_IMPLEMENTED,"callSuper " << name << ' ' << m);
	th->runtime_stack_push(new Undefined);
}

void ABCVm::callSuperVoid(method_info* th, int n, int m)
{
	string name=th->vm->getMultinameString(n); 
	LOG(NOT_IMPLEMENTED,"callSuperVoid " << name << ' ' << m);
}

void ABCVm::callPropVoid(method_info* th, int n, int m)
{
	string name=th->vm->getMultinameString(n); 
	LOG(CALLS,"callPropVoid " << name << ' ' << m);
	arguments* args=new arguments;
	args->resize(m);
	for(int i=0;i<m;i++)
		args->at(m-i-1)=th->runtime_stack_pop();
	ISWFObject* obj=th->runtime_stack_pop();
	bool found;
	ISWFObject* o=obj->getVariableByName(name,found);
	if(found)
	{
		//If o is already a function call it, otherwise find the Call method
		if(o->getObjectType()==T_FUNCTION)
		{
			IFunction* f=dynamic_cast<IFunction*>(o);
			f->call(obj,args);
		}
		else if(o->getObjectType()==T_UNDEFINED)
		{
			LOG(NOT_IMPLEMENTED,"We got a Undefined function");
			th->runtime_stack_push(new Undefined);
		}
		else if(o->getObjectType()==T_DEFINABLE)
		{
			LOG(NOT_IMPLEMENTED,"We got a function not yet valid");
			th->runtime_stack_push(new Undefined);
		}
		else
		{
			IFunction* f=dynamic_cast<IFunction*>(o->getVariableByName(".Call",found));
			f->call(obj,args);
		}
	}
	else
		LOG(NOT_IMPLEMENTED,"Calling an undefined function");
	args->decRef();
	obj->decRef();
}

void ABCVm::jump(method_info* th, int offset)
{
	LOG(CALLS,"jump " << offset);
}

void ABCVm::ifTrue(method_info* th, int offset)
{
	LOG(CALLS,"ifTrue " << offset);

	ISWFObject* obj1=th->runtime_stack_pop();
	th->runtime_stack_push((ISWFObject*)new uintptr_t(Boolean_concrete(obj1)));
	obj1->decRef();
}

void ABCVm::ifFalse(method_info* th, int offset)
{
	LOG(CALLS,"ifFalse " << offset);

	ISWFObject* obj1=th->runtime_stack_pop();
	th->runtime_stack_push((ISWFObject*)new uintptr_t(!Boolean_concrete(obj1)));
	obj1->decRef();
}

//We follow the Boolean() algorithm, but return a concrete result, not a Boolean object
bool Boolean_concrete(ISWFObject* obj)
{
	if(obj->getObjectType()==T_STRING)
	{
		LOG(CALLS,"String to bool");
		string s=obj->toString();
		if(s.empty())
			return false;
		else
			return true;
	}
	else if(obj->getObjectType()==T_BOOLEAN)
	{
		LOG(CALLS,"Boolean to bool");
		Boolean* b=dynamic_cast<Boolean*>(obj);
		return b->val;
	}
	else if(obj->getObjectType()==T_OBJECT)
	{
		LOG(CALLS,"Object to bool");
		return true;
	}
	else if(obj->getObjectType()==T_UNDEFINED)
	{
		LOG(CALLS,"Undefined to bool");
		return false;
	}
	else
		return false;
}

void ABCVm::ifStrictNE(method_info* th, int offset)
{
	LOG(NOT_IMPLEMENTED,"ifStrictNE " << offset);
	abort();
}

void ABCVm::ifNGE(method_info* th, int offset)
{
	LOG(CALLS,"ifNGE " << offset);
	abort();
}

void ABCVm::ifNLT(method_info* th, int offset)
{
	LOG(CALLS,"ifNLT " << offset);
	ISWFObject* obj2=th->runtime_stack_pop();
	ISWFObject* obj1=th->runtime_stack_pop();

	//Real comparision demanded to object
	if(obj1->isLess(obj2))
		th->runtime_stack_push((ISWFObject*)new uintptr_t(0));
	else
		th->runtime_stack_push((ISWFObject*)new uintptr_t(1));

	obj2->decRef();
	obj1->decRef();
}

void ABCVm::ifLT(method_info* th, int offset)
{
	LOG(CALLS,"ifLT " << offset);
	ISWFObject* obj2=th->runtime_stack_pop();
	ISWFObject* obj1=th->runtime_stack_pop();

	//Real comparision demanded to object
	if(obj1->isLess(obj2))
		th->runtime_stack_push((ISWFObject*)new uintptr_t(1));
	else
		th->runtime_stack_push((ISWFObject*)new uintptr_t(0));

	obj2->decRef();
	obj1->decRef();
}

void ABCVm::ifNe(method_info* th, int offset)
{
	LOG(CALLS,"ifNe " << offset);

	ISWFObject* obj1=th->runtime_stack_pop();
	ISWFObject* obj2=th->runtime_stack_pop();

	//Real comparision demanded to object
	if(!obj1->isEqual(obj2))
		th->runtime_stack_push((ISWFObject*)new uintptr_t(1));
	else
		th->runtime_stack_push((ISWFObject*)new uintptr_t(0));
}

void ABCVm::lessThan(method_info* th)
{
	LOG(CALLS,"lessThan");

	ISWFObject* obj1=th->runtime_stack_pop();
	ISWFObject* obj2=th->runtime_stack_pop();

	//Real comparision demanded to object
	if(obj1->isLess(obj2))
		th->runtime_stack_push(new Boolean(true));
	else
		th->runtime_stack_push(new Boolean(false));
}

void ABCVm::greaterThan(method_info* th)
{
	LOG(CALLS,"greaterThan");

	ISWFObject* obj1=th->runtime_stack_pop();
	ISWFObject* obj2=th->runtime_stack_pop();

	//Real comparision demanded to object
	if(obj1->isGreater(obj2))
		th->runtime_stack_push(new Boolean(true));
	else
		th->runtime_stack_push(new Boolean(false));
}

void ABCVm::ifStrictEq(method_info* th, int offset)
{
	LOG(CALLS,"ifStrictEq " << offset);
	abort();

	//CHECK
	ISWFObject* obj1=th->runtime_stack_pop();
	ISWFObject* obj2=th->runtime_stack_pop();

	//Real comparision demanded to object
	if(obj1->isEqual(obj2))
		th->runtime_stack_push((ISWFObject*)new uintptr_t(1));
	else
		th->runtime_stack_push((ISWFObject*)new uintptr_t(0));
}

void ABCVm::ifEq(method_info* th, int offset)
{
	LOG(CALLS,"ifEq " << offset);

	ISWFObject* obj1=th->runtime_stack_pop();
	ISWFObject* obj2=th->runtime_stack_pop();

	//Real comparision demanded to object
	if(obj1->isEqual(obj2))
		th->runtime_stack_push((ISWFObject*)new uintptr_t(1));
	else
		th->runtime_stack_push((ISWFObject*)new uintptr_t(0));
}

void ABCVm::deleteProperty(method_info* th, int n)
{
	LOG(NOT_IMPLEMENTED,"deleteProperty " << n);
}

void ABCVm::call(method_info* th, int n)
{
	LOG(NOT_IMPLEMENTED,"call " << n);
}

void ABCVm::coerce(method_info* th, int n)
{
	string name=th->vm->getMultinameString(n); 
	LOG(NOT_IMPLEMENTED,"coerce " << name);
}

void ABCVm::newCatch(method_info* th, int n)
{
	LOG(NOT_IMPLEMENTED,"newCatch " << n);
}

void ABCVm::setSuper(method_info* th, int n)
{
	ISWFObject* value=th->runtime_stack_pop();
	string name=th->vm->getMultinameString(n); 

	LOG(NOT_IMPLEMENTED,"setSuper " << name);

	ISWFObject* obj=th->runtime_stack_pop();
	obj->setVariableByName(name,value);
}

void ABCVm::newFunction(method_info* th, int n)
{
	LOG(NOT_IMPLEMENTED,"newFunction " << n);
	th->runtime_stack_push(new Undefined);
}

void ABCVm::newObject(method_info* th, int n)
{
	LOG(CALLS,"newObject " << n);
	ISWFObject* ret=new ASObject;
	for(int i=0;i<n;i++)
	{
		ISWFObject* value=th->runtime_stack_pop();
		ISWFObject* name=th->runtime_stack_pop();
		ret->setVariableByName(name->toString(),value);
		name->decRef();
	}

	th->runtime_stack_push(ret);
}

void ABCVm::setSlot(method_info* th, int n)
{
	LOG(CALLS,"setSlot DONE: " << n);
	ISWFObject* value=th->runtime_stack_pop();
	ISWFObject* obj=th->runtime_stack_pop();
	
	obj->setSlot(n,value);
}

void ABCVm::getSlot(method_info* th, int n)
{
	LOG(CALLS,"getSlot DONE: " << n);
	ISWFObject* obj=th->runtime_stack_pop();
	th->runtime_stack_push(obj->getSlot(n));
	obj->getSlot(n)->incRef();
}

void ABCVm::getLocal(method_info* th, int n)
{
	LOG(CALLS,"getLocal: DONE " << n);
}

void ABCVm::setLocal(method_info* th, int n)
{
	LOG(CALLS,"setLocal: DONE " << n);
}

void ABCVm::convert_d(method_info* th)
{
	LOG(NOT_IMPLEMENTED, "convert_d" );
}

void ABCVm::convert_b(method_info* th)
{
	LOG(NOT_IMPLEMENTED, "convert_b" );
}

void ABCVm::convert_i(method_info* th)
{
	LOG(NOT_IMPLEMENTED, "convert_i" );
}

void ABCVm::coerce_a(method_info* th)
{
	LOG(NOT_IMPLEMENTED, "coerce_a" );
}

void ABCVm::coerce_s(method_info* th)
{
	LOG(NOT_IMPLEMENTED, "coerce_s" );
}

void ABCVm::pop(method_info* th)
{
	LOG(CALLS, "pop: DONE" );
}

void ABCVm::_not(method_info* th)
{
	LOG(NOT_IMPLEMENTED, "not" );
}

void ABCVm::not_impl(int n)
{
	LOG(CALLS, "not implement opcode 0x" << hex << n );
	abort();
}

void ABCVm::_throw(method_info* th)
{
	LOG(NOT_IMPLEMENTED, "throw" );
}

void ABCVm::strictEquals(method_info* th)
{
	LOG(NOT_IMPLEMENTED, "strictEquals" );
}

void ABCVm::equals(method_info* th)
{
	LOG(NOT_IMPLEMENTED, "equals" );
}

void ABCVm::dup(method_info* th)
{
	LOG(CALLS, "dup: DONE" );
}

void ABCVm::pushTrue(method_info* th)
{
	LOG(CALLS, "pushTrue" );
	th->runtime_stack_push(new Boolean(true));
}

void ABCVm::pushFalse(method_info* th)
{
	LOG(CALLS, "pushFalse" );
	th->runtime_stack_push(new Boolean(false));
}

void ABCVm::pushNaN(method_info* th)
{
	LOG(CALLS, "pushNaN DONE" );
	th->runtime_stack_push(new Undefined);
}

ISWFObject* ABCVm::pushUndefined(method_info* th)
{
	LOG(CALLS, "pushUndefined DONE" );
	return new Undefined;
}

void ABCVm::pushNull(method_info* th)
{
	LOG(CALLS, "pushNull DONE" );
	th->runtime_stack_push(new Null);
}

void ABCVm::pushScope(method_info* th)
{
	ISWFObject* t=th->runtime_stack_pop();
	LOG(CALLS, "pushScope: DONE " << t );
	th->scope_stack.push_back(t);
}

void ABCVm::pushInt(method_info* th, int n)
{
	s32 i=th->vm->constant_pool.integer[n];
	LOG(CALLS, "pushInt [" << dec << n << "] " << i);
	th->runtime_stack_push(new Integer(i));
}

void ABCVm::pushDouble(method_info* th, int n)
{
	d64 d=th->vm->constant_pool.doubles[n];
	LOG(CALLS, "pushDouble [" << dec << n << "] " << d);
	th->runtime_stack_push(new Number(d));
}

void ABCVm::pushShort(method_info* th, int n)
{
	LOG(CALLS, "pushShort " << n );
	th->runtime_stack_push(new Integer(n));
}

void ABCVm::pushByte(method_info* th, int n)
{
	LOG(CALLS, "pushByte " << n );
	th->runtime_stack_push(new Integer(n));
}

void ABCVm::incLocal_i(method_info* th, int n)
{
	LOG(CALLS, "incLocal_i " << n );
	if(th->locals[n]->getObjectType()==T_INTEGER)
	{
		Integer* i=dynamic_cast<Integer*>(th->locals[n]);
		i->val++;
	}
	else
	{
		LOG(NOT_IMPLEMENTED,"Cannot increment type " << th->locals[n]->getObjectType());
	}

}

void ABCVm::construct(method_info* th, int n)
{
	LOG(NOT_IMPLEMENTED, "construct " << n );
}

void ABCVm::constructSuper(method_info* th, int n)
{
	LOG(NOT_IMPLEMENTED, "constructSuper " << n );
}

void ABCVm::setProperty(method_info* th, int n)
{
	ISWFObject* value=th->runtime_stack_pop();
	string name=th->vm->getMultinameString(n,th);
	LOG(CALLS,"setProperty " << name);

	ISWFObject* obj=th->runtime_stack_pop();

	//Check to see if a proper setter method is available
	bool found;
	IFunction* f=obj->getSetterByName(name,found);
	if(found)
	{
		//Call the setter
		LOG(CALLS,"Calling the setter");
		arguments args;
		args.push(value);
		value->incRef();
		f->call(obj,&args);
		LOG(CALLS,"End of setter");
	}
	else
	{
		//No setter, just assign the variable
		obj->setVariableByName(name,value);
	}
}

void ABCVm::getProperty(method_info* th, int n)
{
	string name=th->vm->getMultinameString(n,th);
	LOG(CALLS, "getProperty " << name );

	ISWFObject* obj=th->runtime_stack_pop();

	bool found;
	//Check to see if a proper getter method is available
	IFunction* f=obj->getGetterByName(name,found);
	if(found)
	{
		//Call the getter
		LOG(CALLS,"Calling the getter");
		//arguments args;
		//args.push(value);
		//value->incRef();
		th->runtime_stack_push(f->call(obj,NULL));
		LOG(CALLS,"End of getter");
		return;
	}
	
	ISWFObject* ret=obj->getVariableByName(name,found);
	if(!found)
	{
		LOG(NOT_IMPLEMENTED,"Property not found " << name);
		th->runtime_stack_push(new Undefined);
	}
	else
	{
		if(ret->getObjectType()==T_DEFINABLE)
		{
			LOG(ERROR,"Property " << name << " is not yet valid");
			abort();
		}
		if(name=="clip")
			cout << ret->class_name << endl;
		th->runtime_stack_push(ret);
		ret->incRef();
	}
}

void ABCVm::findProperty(method_info* th, int n)
{
	string name=th->vm->getMultinameString(n);
	LOG(CALLS, "findProperty " << name );

	vector<ISWFObject*>::reverse_iterator it=th->scope_stack.rbegin();
	bool found=false;
	for(it;it!=th->scope_stack.rend();it++)
	{
		(*it)->getVariableByName(name,found);
		if(found)
		{
			//We have to return the object, not the property
			th->runtime_stack_push(*it);
			(*it)->incRef();
			break;
		}
	}
	if(!found)
	{
		LOG(CALLS, "NOT found, pushing global" );
		th->runtime_stack_push(&th->vm->Global);
		th->vm->Global.incRef();
	}
}

void ABCVm::findPropStrict(method_info* th, int n)
{
	string name=th->vm->getMultinameString(n);
	LOG(CALLS, "findPropStrict " << name );

	vector<ISWFObject*>::reverse_iterator it=th->scope_stack.rbegin();
	bool found=false;
	for(it;it!=th->scope_stack.rend();it++)
	{
		(*it)->getVariableByName(name,found);
		if(found)
		{
			//We have to return the object, not the property
			th->runtime_stack_push(*it);
			(*it)->incRef();
			break;
		}
	}
	if(!found)
	{
		LOG(CALLS, "NOT found, trying Global" );
		bool found2;
		th->vm->Global.getVariableByName(name,found2);
		if(found2)
		{
			th->runtime_stack_push(&th->vm->Global);
			th->vm->Global.incRef();
		}
		else
		{
			LOG(CALLS, "NOT found, pushing Undefined" );
			th->runtime_stack_push(new Undefined);
		}
	}
}

void ABCVm::initProperty(method_info* th, int n)
{
	string name=th->vm->getMultinameString(n);
	LOG(CALLS, "initProperty " << name );
	ISWFObject* value=th->runtime_stack_pop();

	ISWFObject* obj=th->runtime_stack_pop();

	//TODO: Should we make a copy or pass the reference
	obj->setVariableByName(name,value);
}

void ABCVm::newArray(method_info* th, int n)
{
	LOG(CALLS, "newArray " << n );
	ASArray* ret=new ASArray;
	ret->resize(n);
	for(int i=0;i<n;i++)
		ret->at(n-i-1)=th->runtime_stack_pop();

	th->runtime_stack_push(ret);
}

void ABCVm::newClass(method_info* th, int n)
{
	LOG(CALLS, "newClass " << n );
	ISWFObject* ret=th->runtime_stack_pop()->clone();
	method_info* m=&th->vm->methods[th->vm->classes[n].cinit];
	th->vm->synt_method(m);
	LOG(CALLS,"Building class traits");
	for(int i=0;i<th->vm->classes[n].trait_count;i++)
		th->vm->buildTrait(ret,&th->vm->classes[n].traits[i]);

	//TODO: should add Constructor the the class methods
	LOG(CALLS,"Calling Class init");
	ret->class_name="newclass";
	if(m->f)
	{
		Function::as_function FP=(Function::as_function)ex->getPointerToFunction(m->f);
		FP(ret,NULL);
	}
	th->runtime_stack_push(ret);
}

void ABCVm::getScopeObject(method_info* th, int n)
{
	th->runtime_stack_push(th->scope_stack[n]);
	th->scope_stack[n]->incRef();
	LOG(CALLS, "getScopeObject: DONE " << th->scope_stack[n] );
}

void ABCVm::debug(method_info* i)
{
	LOG(CALLS, "debug " << i->locals[1] );
}

void ABCVm::getLex(method_info* th, int n)
{
	string name=th->vm->getMultinameString(n);
	LOG(CALLS, "getLex DONE: " << name );
	vector<ISWFObject*>::reverse_iterator it=th->scope_stack.rbegin();
	bool found=false;
	for(it;it!=th->scope_stack.rend();it++)
	{
		ISWFObject* o=(*it)->getVariableByName(name,found);
		if(found)
		{
			//If we are getting a function object attach the the current scope
			if(o->getObjectType()==T_FUNCTION)
			{
				LOG(CALLS,"Attaching this to function " << name);
				Function* f=dynamic_cast<Function*>(o);
				f->closure_this=th->locals[0];
			}
			else if(o->getObjectType()==T_DEFINABLE)
			{
				LOG(CALLS,"Deferred definition of property " << name);
				Definable* d=dynamic_cast<Definable*>(o);
				d->define(&th->vm->Global);
				o=(*it)->getVariableByName(name,found);
			}
			th->runtime_stack_push(o);
			o->incRef();
			break;
		}
	}
	if(!found)
	{
		LOG(CALLS, "NOT found, trying Global" );
		bool found2;
		ISWFObject* o2=th->vm->Global.getVariableByName(name,found2);
		if(found2)
		{
			if(o2->getObjectType()==T_DEFINABLE)
			{
				LOG(CALLS,"Deferred definition of property " << name);
				Definable* d=dynamic_cast<Definable*>(o2);
				d->define(&th->vm->Global);
				o2=th->vm->Global.getVariableByName(name,found);
			}

			th->runtime_stack_push(o2);
			o2->incRef();
		}
		else
		{
			LOG(CALLS, "NOT found, pushing Undefined" );
			th->runtime_stack_push(new Undefined);
		}
	}

}

void ABCVm::pushString(method_info* th, int n)
{
	string s=th->vm->getString(n); 
	LOG(CALLS, "pushString " << s );
	th->runtime_stack_push(new ASString(s));
}

void ABCVm::kill(method_info* th, int n)
{
	LOG(NOT_IMPLEMENTED, "kill " << n );
}

void method_info::runtime_stack_push(ISWFObject* s)
{
	stack[stack_index++]=s;
}

void method_info::setStackLength(const llvm::ExecutionEngine* ex, int l)
{
	const llvm::Type* ptr_type=ex->getTargetData()->getIntPtrType();
	//TODO: We add this huge safety margin because not implemented instruction do not clean the stack as they should
	stack=new ISWFObject*[ l*10 ];
	max_stack_index=l*10;
	llvm::Constant* constant = llvm::ConstantInt::get(ptr_type, (uintptr_t)stack);
	//TODO: Now the stack is considered an array of pointer to int64
	dynamic_stack = llvm::ConstantExpr::getIntToPtr(constant, llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(ptr_type)));
	constant = llvm::ConstantInt::get(ptr_type, (uintptr_t)&stack_index);
	dynamic_stack_index = llvm::ConstantExpr::getIntToPtr(constant, llvm::PointerType::getUnqual(llvm::IntegerType::get(32)));

}

llvm::Value* method_info::llvm_stack_pop(llvm::IRBuilder<>& builder) const 
{
	//decrement stack index
	llvm::Value* index=builder.CreateLoad(dynamic_stack_index);
	llvm::Constant* constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), 1);
	llvm::Value* index2=builder.CreateSub(index,constant);
	builder.CreateStore(index2,dynamic_stack_index);

	llvm::Value* dest=builder.CreateGEP(dynamic_stack,index2);
	return builder.CreateLoad(dest);
}

llvm::Value* method_info::llvm_stack_peek(llvm::IRBuilder<>& builder) const
{
	llvm::Value* index=builder.CreateLoad(dynamic_stack_index);
	llvm::Constant* constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), 1);
	llvm::Value* index2=builder.CreateSub(index,constant);
	llvm::Value* dest=builder.CreateGEP(dynamic_stack,index2);
	return builder.CreateLoad(dest);
}

void method_info::llvm_stack_push(llvm::ExecutionEngine* ex, llvm::IRBuilder<>& builder, llvm::Value* val)
{
	llvm::Value* index=builder.CreateLoad(dynamic_stack_index);
	llvm::Value* dest=builder.CreateGEP(dynamic_stack,index);
	builder.CreateStore(val,dest);
	builder.CreateCall(ex->FindFunctionNamed("incRef"), val);

	//increment stack index
	llvm::Constant* constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), 1);
	llvm::Value* index2=builder.CreateAdd(index,constant);
	builder.CreateStore(index2,dynamic_stack_index);
}

ISWFObject* method_info::runtime_stack_pop()
{
	if(stack_index==0)
	{
		LOG(ERROR,"Empty stack");
		abort();
	}
	ISWFObject* ret=stack[--stack_index];
	return ret;
}

ISWFObject* method_info::runtime_stack_peek()
{
	if(stack_index==0)
	{
		LOG(ERROR,"Empty stack");
		return NULL;
	}
	return stack[stack_index-1];
}

inline ABCVm::stack_entry ABCVm::static_stack_pop(llvm::IRBuilder<>& builder, vector<ABCVm::stack_entry>& static_stack, const method_info* m) 
{
	//try to get the tail value from the static stack
	if(!static_stack.empty())
	{
		stack_entry ret=static_stack.back();
		static_stack.pop_back();
		return ret;
	}
	//try to pop the tail value of the dynamic stack
	return stack_entry(m->llvm_stack_pop(builder),STACK_OBJECT);
}

inline ABCVm::stack_entry ABCVm::static_stack_peek(llvm::IRBuilder<>& builder, vector<ABCVm::stack_entry>& static_stack, const method_info* m) 
{
	//try to get the tail value from the static stack
	if(!static_stack.empty())
		return static_stack.back();
	//try to load the tail value of the dynamic stack
	return stack_entry(m->llvm_stack_peek(builder),STACK_OBJECT);
}

inline void ABCVm::static_stack_push(vector<ABCVm::stack_entry>& static_stack, const ABCVm::stack_entry& e)
{
	static_stack.push_back(e);
}

inline void ABCVm::syncStacks(llvm::ExecutionEngine* ex,llvm::IRBuilder<>& builder, bool jitted,std::vector<stack_entry>& static_stack,method_info* m)
{
	if(jitted)
	{
		for(int i=0;i<static_stack.size();i++)
		{
			if(static_stack[i].second!=STACK_OBJECT)
				LOG(ERROR,"Conversion not yet implemented");
			m->llvm_stack_push(ex,builder,static_stack[i].first);
		}
		static_stack.clear();
	}
}

llvm::FunctionType* ABCVm::synt_method_prototype()
{
	//The pointer size compatible int type will be useful
	const llvm::Type* ptr_type=ex->getTargetData()->getIntPtrType();

	//Initialize LLVM representation of method
	vector<const llvm::Type*> sig;
	sig.push_back(llvm::PointerType::getUnqual(ptr_type));
	sig.push_back(llvm::PointerType::getUnqual(ptr_type));

	return llvm::FunctionType::get(llvm::Type::VoidTy, sig, false);
}

llvm::Function* ABCVm::synt_method(method_info* m)
{
	if(m->f)
		return m->f;

	if(!m->body)
	{
		string n=getString(m->name);
		LOG(CALLS,"Method " << n << " should be intrinsic");
		return NULL;
	}
	stringstream code(m->body->code);
	m->vm=this;
	llvm::FunctionType* method_type=synt_method_prototype();
	m->f=llvm::Function::Create(method_type,llvm::Function::ExternalLinkage,"method",module);

	//The pointer size compatible int type will be useful
	const llvm::Type* ptr_type=ex->getTargetData()->getIntPtrType();
	
	llvm::BasicBlock *BB = llvm::BasicBlock::Create("entry", m->f);
	llvm::IRBuilder<> Builder;

	//We define a couple of variables that will be used a lot
	llvm::Constant* constant;
	llvm::Constant* constant2;
	llvm::Value* value;
	//let's give access to method data to llvm
	constant = llvm::ConstantInt::get(ptr_type, (uintptr_t)m);
	llvm::Value* th = llvm::ConstantExpr::getIntToPtr(constant, llvm::PointerType::getUnqual(ptr_type));

	//let's give access to local data storage
	m->locals=new ISWFObject*[m->body->local_count];
	constant = llvm::ConstantInt::get(ptr_type, (uintptr_t)m->locals);
	llvm::Value* locals = llvm::ConstantExpr::getIntToPtr(constant, 
			llvm::PointerType::getUnqual(llvm::PointerType::getUnqual(ptr_type)));

	//the stack is statically handled as much as possible to allow llvm optimizations
	//on branch and on interpreted/jitted code transition it is synchronized with the dynamic one
	vector<stack_entry> static_stack;
	static_stack.reserve(m->body->max_stack);
	m->setStackLength(ex,m->body->max_stack);

	//the scope stack is not accessible to llvm code
	
	//Creating a mapping between blocks and starting address
	map<int,llvm::BasicBlock*> blocks;
	blocks.insert(pair<int,llvm::BasicBlock*>(0,BB));

	bool jitted=false;

	//This is initialized to true so that on first iteration the entry block is used
	bool last_is_branch=true;

	Builder.SetInsertPoint(BB);
	Builder.CreateCall(ex->FindFunctionNamed("method_reset"), th);
	//We fill locals with function arguments
	//First argument is the 'this'
	constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), 0);
	llvm::Value* t=Builder.CreateGEP(locals,constant);
	llvm::Function::ArgumentListType::iterator it=m->f->getArgumentList().begin();
	llvm::Value* arg=it;
	Builder.CreateStore(arg,t);
	//Second argument is the arguments pointer
	it++;
	for(int i=0;i<m->param_count;i++)
	{
		constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), i+1);
		t=Builder.CreateGEP(locals,constant);
		arg=Builder.CreateCall2(ex->FindFunctionNamed("argumentDumper"), it, constant);
		Builder.CreateStore(arg,t);
	}

	//Each case block builds the correct parameters for the interpreter function and call it
	u8 opcode;
	while(1)
	{
		code >> opcode;
		if(code.eof())
			break;
		//Check if we are expecting a new block start
		map<int,llvm::BasicBlock*>::iterator it=blocks.find(int(code.tellg())-1);
		if(it!=blocks.end())
		{
			//A new block starts, the last instruction should have been a branch?
			if(!last_is_branch)
			{
				LOG(CALLS, "Last instruction before a new block was not a branch.");
				Builder.CreateBr(it->second);
			}
			LOG(CALLS,"Starting block at "<<int(code.tellg())-1);
			Builder.SetInsertPoint(it->second);
			last_is_branch=false;
		}
		else if(last_is_branch)
		{
			//TODO: Check this. It seems that there may be invalid code after
			//block end
			LOG(CALLS,"Ignoring at " << int(code.tellg())-1);
			continue;
			/*LOG(CALLS,"Inserting block at "<<int(code.tellg())-1);
			abort();
			llvm::BasicBlock* A=llvm::BasicBlock::Create("fall", m->f);
			Builder.CreateBr(A);
			Builder.SetInsertPoint(A);*/
		}

		switch(opcode)
		{
			case 0x03:
			{
				//throw
				LOG(TRACE, "synt throw" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("throw"), th);
				Builder.CreateRetVoid();
				break;
			}
			case 0x05:
			{
				//setsuper
				LOG(TRACE, "synt setsuper" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("setSuper"), th, constant);
				break;
			}
			case 0x08:
			{
				//kill
				LOG(TRACE, "synt kill" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("kill"), th, constant);
				break;
			}
			case 0x09:
			{
				//label
				syncStacks(ex,Builder,jitted,static_stack,m);
				//Create a new block and insert it in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}
				Builder.CreateBr(A);
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x0c:
			{
				//ifnlt
				LOG(TRACE, "synt ifnlt" );
				//TODO: implement common data comparison
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				last_is_branch=true;
				s24 t;
				code >> t;

				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
				//Make comparision
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("ifNLT"), th, constant);
			
				//Pop the stack, we are surely going to pop from the dynamic one
				//ifNLT pushed a pointer to integer
				llvm::Value* cond_ptr=static_stack_pop(Builder,static_stack,m).first;
				llvm::Value* cond=Builder.CreateLoad(cond_ptr);
				llvm::Value* cond1=Builder.CreateTrunc(cond,llvm::IntegerType::get(1));
				Builder.CreateCondBr(cond1,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x0f:
			{
				//ifnge
				LOG(TRACE, "synt ifnge");
				//TODO: implement common data comparison
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				last_is_branch=true;
				s24 t;
				code >> t;
				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
				//Make comparision
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("ifNGE"), th, constant);
			
				//Pop the stack, we are surely going to pop from the dynamic one
				//ifNGE pushed a pointer to integer
				llvm::Value* cond_ptr=static_stack_pop(Builder,static_stack,m).first;
				llvm::Value* cond=Builder.CreateLoad(cond_ptr);
				llvm::Value* cond1=Builder.CreateTrunc(cond,llvm::IntegerType::get(1));
				Builder.CreateCondBr(cond1,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x10:
			{
				//jump
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				last_is_branch=true;

				s24 t;
				code >> t;
				LOG(TRACE, "synt jump " << t );
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("jump"), th, constant);

				//Create a block for the fallthrough code and insert it in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}
				//Create a block for the landing code and insert it in the mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("jump_land", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}

				Builder.CreateBr(B);
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x11:
			{
				//iftrue
				LOG(TRACE, "synt iftrue" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;

				last_is_branch=true;
				s24 t;
				code >> t;

				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
			
				//Make comparision
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("ifTrue"), th, constant);

				//Pop the stack, we are surely going to pop from the dynamic one
				//ifTrue pushed a pointer to integer
				llvm::Value* cond_ptr=static_stack_pop(Builder,static_stack,m).first;
				llvm::Value* cond=Builder.CreateLoad(cond_ptr);
				llvm::Value* cond1=Builder.CreateTrunc(cond,llvm::IntegerType::get(1));
				Builder.CreateCondBr(cond1,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x12:
			{
				//iffalse
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;

				last_is_branch=true;
				s24 t;
				code >> t;
				LOG(TRACE, "synt iffalse " << t );

				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
			
				//Make comparision
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("ifFalse"), th, constant);

				//Pop the stack, we are surely going to pop from the dynamic one
				//ifFalse pushed a pointer to integer
				llvm::Value* cond_ptr=static_stack_pop(Builder,static_stack,m).first;
				llvm::Value* cond=Builder.CreateLoad(cond_ptr);
				llvm::Value* cond1=Builder.CreateTrunc(cond,llvm::IntegerType::get(1));
				Builder.CreateCondBr(cond1,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x13:
			{
				//ifeq
				LOG(TRACE, "synt ifeq" );
				//TODO: implement common data comparison
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;

				last_is_branch=true;
				s24 t;
				code >> t;
				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
			
				//Make comparision
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("ifEq"), th, constant);

				//Pop the stack, we are surely going to pop from the dynamic one
				//ifEq pushed a pointer to integer
				llvm::Value* cond_ptr=static_stack_pop(Builder,static_stack,m).first;
				llvm::Value* cond=Builder.CreateLoad(cond_ptr);
				llvm::Value* cond1=Builder.CreateTrunc(cond,llvm::IntegerType::get(1));
				Builder.CreateCondBr(cond1,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x14:
			{
				//ifne
				//TODO: implement common data comparison
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;

				last_is_branch=true;
				s24 t;
				code >> t;
				LOG(TRACE, "synt ifne " << t );
				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
			
				//Make comparision
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("ifNe"), th, constant);

				//Pop the stack, we are surely going to pop from the dynamic one
				//ifEq pushed a pointer to integer
				llvm::Value* cond_ptr=static_stack_pop(Builder,static_stack,m).first;
				llvm::Value* cond=Builder.CreateLoad(cond_ptr);
				llvm::Value* cond1=Builder.CreateTrunc(cond,llvm::IntegerType::get(1));
				Builder.CreateCondBr(cond1,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x15:
			{
				//iflt
				LOG(TRACE, "synt iflt" );
				//TODO: implement common data comparison
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				last_is_branch=true;
				s24 t;
				code >> t;
				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
				//Make comparision
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("ifLT"), th, constant);
			
				//Pop the stack, we are surely going to pop from the dynamic one
				//ifEq pushed a pointer to integer
				llvm::Value* cond_ptr=static_stack_pop(Builder,static_stack,m).first;
				llvm::Value* cond=Builder.CreateLoad(cond_ptr);
				llvm::Value* cond1=Builder.CreateTrunc(cond,llvm::IntegerType::get(1));
				Builder.CreateCondBr(cond1,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x19:
			{
				//ifstricteq
				LOG(TRACE, "synt ifstricteq" );
				//TODO: implement common data comparison
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;

				last_is_branch=true;
				s24 t;
				code >> t;
				//Create a block for the fallthrough code and insert in the mapping
				llvm::BasicBlock* A;
				map<int,llvm::BasicBlock*>::iterator it=blocks.find(code.tellg());
				if(it!=blocks.end())
					A=it->second;
				else
				{
					A=llvm::BasicBlock::Create("fall", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(code.tellg(),A));
				}

				//And for the branch destination, if they are not in the blocks mapping
				llvm::BasicBlock* B;
				it=blocks.find(int(code.tellg())+t);
				if(it!=blocks.end())
					B=it->second;
				else
				{
					B=llvm::BasicBlock::Create("then", m->f);
					blocks.insert(pair<int,llvm::BasicBlock*>(int(code.tellg())+t,B));
				}
			
				//Make comparision
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("ifStrictEq"), th, constant);

				//Pop the stack, we are surely going to pop from the dynamic one
				//ifStrictEq pushed a pointer to integer
				llvm::Value* cond_ptr=static_stack_pop(Builder,static_stack,m).first;
				llvm::Value* cond=Builder.CreateLoad(cond_ptr);
				llvm::Value* cond1=Builder.CreateTrunc(cond,llvm::IntegerType::get(1));
				Builder.CreateCondBr(cond1,B,A);
				//Now start populating the fallthrough block
				Builder.SetInsertPoint(A);
				break;
			}
			case 0x1a:
			{
				//ifstrictne
				LOG(TRACE, "synt ifstrictne" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				s24 t;
				code >> t;
				//Make comparision
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("ifStrictNE"), th, constant);
				break;
			}
			case 0x1b:
			{
				//lookupswitch
				LOG(TRACE, "synt lookupswitch" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				s24 t;
				code >> t;
				LOG(TRACE,"default " << int(t));
				u30 count;
				code >> count;
				LOG(TRACE,"count "<< int(count));
				for(int i=0;i<count+1;i++)
					code >> t;
				break;
			}
			case 0x1d:
			{
				//popscope
				LOG(TRACE, "synt popscope" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("popScope"), th);
				break;
			}
			case 0x1e:
			{
				//nextname
				LOG(TRACE, "synt nextname" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("nextName"), th);
				break;
			}
			case 0x20:
			{
				//pushnull
				LOG(TRACE, "synt pushnull" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("pushNull"), th);
				break;
			}
			case 0x21:
			{
				//pushundefined
				LOG(TRACE, "synt pushundefined" );
				value=Builder.CreateCall(ex->FindFunctionNamed("pushUndefined"), th);
				static_stack_push(static_stack,stack_entry(value,STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x23:
			{
				//nextvalue
				LOG(TRACE, "synt nextvalue" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("nextValue"), th);
				break;
			}
			case 0x24:
			{
				//pushbyte
				LOG(TRACE, "synt pushbyte" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				uint8_t t;
				code.read((char*)&t,1);
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("pushByte"), th, constant);
				break;
			}
			case 0x25:
			{
				//pushshort
				LOG(TRACE, "synt pushshort" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("pushShort"), th, constant);
				break;
			}
			case 0x26:
			{
				//pushtrue
				LOG(TRACE, "synt pushtrue" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("pushTrue"), th);
				break;
			}
			case 0x27:
			{
				//pushfalse
				LOG(TRACE, "synt pushfalse" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("pushFalse"), th);
				break;
			}
			case 0x28:
			{
				//pushnan
				LOG(TRACE, "synt pushnan" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("pushNaN"), th);
				break;
			}
			case 0x29:
			{
				//pop
				LOG(TRACE, "synt pop" );
				jitted=true;
				Builder.CreateCall(ex->FindFunctionNamed("pop"), th);
				stack_entry e=static_stack_pop(Builder,static_stack,m);
				break;
			}
			case 0x2a:
			{
				//dup
				LOG(TRACE, "synt dup" );
				jitted=true;
				Builder.CreateCall(ex->FindFunctionNamed("dup"), th);
				stack_entry e=static_stack_peek(Builder,static_stack,m);
				static_stack_push(static_stack,e);
				break;
			}
			case 0x2b:
			{
				//swap
				LOG(TRACE, "synt swap" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("swap"), th);
				break;
			}
			case 0x2c:
			{
				//pushstring
				LOG(TRACE, "synt pushstring" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("pushString"), th, constant);
				break;
			}
			case 0x2d:
			{
				//pushint
				LOG(TRACE, "synt pushint" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("pushInt"), th, constant);
				break;
			}
			case 0x2f:
			{
				//pushdouble
				LOG(TRACE, "synt pushdouble" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("pushDouble"), th, constant);
				break;
			}
			case 0x30:
			{
				//pushscope
				LOG(TRACE, "synt pushscope" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("pushScope"), th);
				break;
			}
			case 0x32:
			{
				//hasnext2
				//TODO: Implement static resolution where possible
				LOG(TRACE, "synt hasnext2" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				code >> t;
				constant2 = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall3(ex->FindFunctionNamed("hasNext2"), th, constant, constant2);
				break;
			}
			case 0x40:
			{
				//newfunction
				LOG(TRACE, "synt newfunction" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("newFunction"), th, constant);
				break;
			}
			case 0x41:
			{
				//call
				LOG(TRACE, "synt call" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("call"), th, constant);
				break;
			}
			case 0x42:
			{
				//construct
				LOG(TRACE, "synt construct" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("construct"), th, constant);
				break;
			}
			case 0x45:
			{
				//callsuper
				LOG(TRACE, "synt callsuper" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				code >> t;
				constant2 = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall3(ex->FindFunctionNamed("callSuper"), th, constant, constant2);
				break;
			}
			case 0x46:
			{
				//callproperty
				//TODO: Implement static resolution where possible
				LOG(TRACE, "synt callproperty" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				code >> t;
				constant2 = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);

/*				//Pop the stack arguments
				vector<llvm::Value*> args(t+1);
				for(int i=0;i<t;i++)
					args[t-i]=static_stack_pop(Builder,static_stack,m).first;*/
				//Call the function resolver, static case could be resolved at this time (TODO)
				Builder.CreateCall3(ex->FindFunctionNamed("callProperty"), th, constant, constant2);
/*				//Pop the function object, and then the object itself
				llvm::Value* fun=static_stack_pop(Builder,static_stack,m).first;

				llvm::Value* fun2=Builder.CreateBitCast(fun,synt_method_prototype(t));
				args[0]=static_stack_pop(Builder,static_stack,m).first;
				Builder.CreateCall(fun2,args.begin(),args.end());*/

				break;
			}
			case 0x47:
			{
				//returnvoid
				LOG(TRACE, "synt returnvoid" );
				last_is_branch=true;
				Builder.CreateRetVoid();
				break;
			}
			case 0x48:
			{
				//returnvalue
				//TODO: Should coerce the return type to the expected one
				LOG(TRACE, "synt returnvalue" );
				last_is_branch=true;
				stack_entry e=static_stack_pop(Builder,static_stack,m);
				Builder.CreateRet(e.first);
				break;
			}
			case 0x49:
			{
				//constructsuper
				LOG(TRACE, "synt constructsuper" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("constructSuper"), th, constant);
				break;
			}
			case 0x4a:
			{
				//constructprop
				LOG(TRACE, "synt constructprop" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				code >> t;
				constant2 = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall3(ex->FindFunctionNamed("constructProp"), th, constant, constant2);
				break;
			}
			case 0x4e:
			{
				//callsupervoid
				LOG(TRACE, "synt callsupervoid" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				code >> t;
				constant2 = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall3(ex->FindFunctionNamed("callSuperVoid"), th, constant, constant2);
				break;
			}
			case 0x4f:
			{
				//callpropvoid
				LOG(TRACE, "synt callpropvoid" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				code >> t;
				constant2 = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall3(ex->FindFunctionNamed("callPropVoid"), th, constant, constant2);
				break;
			}
			case 0x55:
			{
				//newobject
				LOG(TRACE, "synt newobject" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("newObject"), th, constant);
				break;
			}
			case 0x56:
			{
				//newarray
				LOG(TRACE, "synt newarray" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("newArray"), th, constant);
				break;
			}
			case 0x57:
			{
				//newactivation
				LOG(TRACE, "synt newactivation" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("newActivation"), th);
				break;
			}
			case 0x58:
			{
				//newclass
				LOG(TRACE, "synt newclass" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("newClass"), th, constant);
				break;
			}
			case 0x5a:
			{
				//newcatch
				LOG(TRACE, "synt newcatch" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("newCatch"), th, constant);
				break;
			}
			case 0x5d:
			{
				//findpropstrict
				LOG(TRACE, "synt findpropstrict" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("findPropStrict"), th, constant);
				break;
			}
			case 0x5e:
			{
				//findproperty
				LOG(TRACE, "synt findproperty" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("findProperty"), th, constant);
				break;
			}
			case 0x60:
			{
				//getlex
				LOG(TRACE, "synt getlex" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("getLex"), th, constant);
				break;
			}
			case 0x61:
			{
				//setproperty
				LOG(TRACE, "synt setproperty" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("setProperty"), th, constant);
				break;
			}
			case 0x62:
			{
				//getlocal
				LOG(TRACE, "synt getlocal" );
				u30 i;
				code >> i;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), i);
				Builder.CreateCall2(ex->FindFunctionNamed("getLocal"), th, constant);
				llvm::Value* t=Builder.CreateGEP(locals,constant);
				static_stack_push(static_stack,stack_entry(Builder.CreateLoad(t,"stack"),STACK_OBJECT));
				jitted=true;
				break;
			}
			case 0x63:
			{
				//setlocal
				LOG(TRACE, "synt setlocal" );
				u30 i;
				code >> i;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), i);
				Builder.CreateCall2(ex->FindFunctionNamed("setLocal"), th, constant);
				llvm::Value* t=Builder.CreateGEP(locals,constant);
				stack_entry e=static_stack_pop(Builder,static_stack,m);
				if(e.second!=STACK_OBJECT)
					LOG(ERROR,"conversion not yet implemented");
				Builder.CreateStore(e.first,t);
				jitted=true;
				break;
			}
			case 0x64:
			{
				//getglobalscope
				LOG(TRACE, "synt getglobalscope" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("getGlobalScope"), th);
				break;
			}
			case 0x65:
			{
				//getscopeobject
				LOG(TRACE, "synt getscopeobject" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("getScopeObject"), th, constant);
				break;
			}
			case 0x66:
			{
				//getproperty
				LOG(TRACE, "synt getproperty" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("getProperty"), th, constant);
				break;
			}
			case 0x68:
			{
				//initproperty
				LOG(TRACE, "synt initproperty" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("initProperty"), th, constant);
				break;
			}
			case 0x6a:
			{
				//deleteproperty
				LOG(TRACE, "synt deleteproperty" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("deleteProperty"), th, constant);
				break;
			}
			case 0x6c:
			{
				//getslot
				LOG(TRACE, "synt getslot" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("getSlot"), th, constant);
				break;
			}
			case 0x6d:
			{
				//setslot
				LOG(TRACE, "synt setslot" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("setSlot"), th, constant);
				break;
			}
			case 0x73:
			{
				//convert_i
				LOG(TRACE, "synt convert_i" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("convert_i"), th);
				break;
			}
			case 0x75:
			{
				//convert_d
				LOG(TRACE, "synt convert_d" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("convert_d"), th);
				break;
			}
			case 0x76:
			{
				//convert_b
				LOG(TRACE, "synt convert_b" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("convert_b"), th);
				break;
			}
			case 0x80:
			{
				//corce
				LOG(TRACE, "synt coerce" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("coerce"), th, constant);
				break;
			}
			case 0x82:
			{
				//coerce_a
				LOG(TRACE, "synt coerce_a" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("coerce_a"), th);
				break;
			}
			case 0x85:
			{
				//coerce_s
				LOG(TRACE, "synt coerce_s" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("coerce_s"), th);
				break;
			}
			case 0x87:
			{
				//astypelate
				LOG(TRACE, "synt astypelate" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("asTypelate"), th);
				break;
			}
			case 0x91:
			{
				//increment
				LOG(TRACE, "synt increment" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("increment"), th);
				break;
			}
			case 0x93:
			{
				//decrement
				LOG(TRACE, "synt decrement" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("decrement"), th);
				break;
			}
			case 0x96:
			{
				//not
				LOG(TRACE, "synt not" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("not"), th);
				break;
			}
			case 0xa0:
			{
				//add
				LOG(TRACE, "synt add" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("add"), th);
				break;
			}
			case 0xa1:
			{
				//subtract
				LOG(TRACE, "synt subtract" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("subtract"), th);
				break;
			}
			case 0xa2:
			{
				//multiply
				LOG(TRACE, "synt multiply" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("multiply"), th);
				break;
			}
			case 0xa3:
			{
				//divide
				LOG(TRACE, "synt divide" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("divide"), th);
				break;
			}
			case 0xab:
			{
				//equals
				LOG(TRACE, "synt equals" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("equals"), th);
				break;
			}
			case 0xac:
			{
				//strictequals
				LOG(TRACE, "synt strictequals" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("strictEquals"), th);
				break;
			}
			case 0xad:
			{
				//lessthan
				LOG(TRACE, "synt lessthan" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("lessThan"), th);
				break;
			}
			case 0xaf:
			{
				//greaterthan
				LOG(TRACE, "synt greaterthan" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("greaterThan"), th);
				break;
			}
			case 0xb3:
			{
				//istypelate
				LOG(TRACE, "synt istypelate" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("isTypelate"), th);
				break;
			}
			case 0xc0:
			{
				//increment_i
				LOG(TRACE, "synt increment_i" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("increment_i"), th);
				break;
			}
			case 0xc1:
			{
				//decrement_i
				LOG(TRACE, "synt decrement_i" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				Builder.CreateCall(ex->FindFunctionNamed("decrement_i"), th);
				break;
			}
			case 0xc2:
			{
				//inclocal_i
				LOG(TRACE, "synt inclocal_i" );
				syncStacks(ex,Builder,jitted,static_stack,m);
				jitted=false;
				u30 t;
				code >> t;
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), t);
				Builder.CreateCall2(ex->FindFunctionNamed("incLocal_i"), th, constant);
				break;
			}
			case 0xd0:
			case 0xd1:
			case 0xd2:
			case 0xd3:
			{
				//getlocal_n
				LOG(TRACE, "synt getlocal" );
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), opcode&3);
				Builder.CreateCall2(ex->FindFunctionNamed("getLocal"), th, constant);
				llvm::Value* t=Builder.CreateGEP(locals,constant);
				static_stack_push(static_stack,stack_entry(Builder.CreateLoad(t,"stack"),STACK_OBJECT));
				jitted=true;

				break;
			}
			case 0xd5:
			case 0xd6:
			case 0xd7:
			{
				//setlocal_n
				LOG(TRACE, "synt setlocal" );
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), opcode&3);
				llvm::Value* t=Builder.CreateGEP(locals,constant);
				stack_entry e=static_stack_pop(Builder,static_stack,m);
				if(e.second!=STACK_OBJECT)
					LOG(ERROR,"conversion not yet implemented");
				Builder.CreateStore(e.first,t);
				Builder.CreateCall2(ex->FindFunctionNamed("setLocal"), th, constant);
				jitted=true;
				break;
			}
			default:
				LOG(ERROR,"Not implemented instruction @" << code.tellg());
				u8 a,b,c;
				code >> a >> b >> c;
				LOG(ERROR,"dump " << hex << (unsigned int)opcode << ' ' << (unsigned int)a << ' ' 
						<< (unsigned int)b << ' ' << (unsigned int)c);
				constant = llvm::ConstantInt::get(llvm::IntegerType::get(32), opcode);
				Builder.CreateCall(ex->FindFunctionNamed("not_impl"), constant);
				Builder.CreateRetVoid();
				return m->f;
		}
	}

	map<int,llvm::BasicBlock*>::iterator it2=blocks.begin();
	for(it2;it2!=blocks.end();it2++)
	{
		if(it2->second->getTerminator()==NULL)
		{
			cout << "start at " << it2->first << endl;
			abort();
		}
	}
	return m->f;
}

void ABCVm::Run(ABCVm* th)
{
	sem_wait(&th->mutex);
	if(th->running)
	{
		sem_post(&th->mutex);
		return;
	}
	else
		th->running=true;

	sys=th->m_sys;
	th->module=new llvm::Module("abc jit");
	if(!ex)
		ex=llvm::ExecutionEngine::create(th->module);
	sem_post(&th->mutex);

	th->registerFunctions();
	th->registerClasses();
	//Set register 0 to Global
	//registers[0]=SWFObject(&Global);
	th->Global.class_name="Global";
	
	//Take script entries and declare their traits
	int i=0;
	for(i;i<th->scripts.size()-1;i++)
	{
		LOG(CALLS, "Script N: " << i );
		method_info* m=th->get_method(th->scripts[i].init);
		LOG(CALLS, "Building entry script traits: " << th->scripts[i].trait_count );
		for(int j=0;j<th->scripts[i].trait_count;j++)
			th->buildTrait(&th->Global,&th->scripts[i].traits[j]);
		th->synt_method(m);
		Function::as_function sinit=NULL;
		if(m->f)
			sinit=(Function::as_function)ex->getPointerToFunction(m->f);

		for(int j=0;j<th->scripts[i].trait_count;j++)
			th->buildTrait(&th->Global,&th->scripts[i].traits[j],sinit);
	}
	//The last script entry has to be run
	LOG(CALLS, "Last script (Entry Point)");
	method_info* m=th->get_method(th->scripts[i].init);
	LOG(CALLS, "Building entry script traits: " << th->scripts[i].trait_count );
	for(int j=0;j<th->scripts[i].trait_count;j++)
		th->buildTrait(&th->Global,&th->scripts[i].traits[j]);
	th->synt_method(m);
	for(int j=0;j<th->scripts[i].trait_count;j++)
		th->buildTrait(&th->Global,&th->scripts[i].traits[j]);
	if(m->f)
	{
		Function::as_function sinit=(Function::as_function)ex->getPointerToFunction(m->f);
		sinit(&th->Global,NULL);
	}
	LOG(CALLS, "End of Entry Point");

	while(!th->shutdown)
	{
		timespec ts,td;
		clock_gettime(CLOCK_REALTIME,&ts);
		sem_wait(&th->sem_event_count);
		th->handleEvent();
		sys->fps_prof->event_count++;

		clock_gettime(CLOCK_REALTIME,&td);
		sys->fps_prof->event_time+=timeDiff(ts,td);
	}
	//module.dump();
}

string ABCVm::getString(unsigned int s) const
{
	if(s)
		return constant_pool.strings[s];
	else
		return "";
}

void ABCVm::buildTrait(ISWFObject* obj, const traits_info* t, Function::as_function deferred_initialization)
{
	string name=getMultinameString(t->name);
	switch(t->kind&0xf)
	{
		case traits_info::Class:
		{
		//	LOG(CALLS,"Registering trait " << name);
		//	obj->setVariableByName(name, buildClass(t->classi));
			if(deferred_initialization)
				obj->setVariableByName(name, new Definable(deferred_initialization));
			else
				obj->setVariableByName(name, new Undefined);
			break;
		}
		case traits_info::Getter:
		{
			LOG(CALLS,"Getter trait: " << name);
			//syntetize method and create a new LLVM function object
			method_info* m=&methods[t->method];
			llvm::Function* f=synt_method(m);
			Function::as_function f2=(Function::as_function)ex->getPointerToFunction(f);
			obj->setGetterByName(name, new Function(f2));
			LOG(CALLS,"End Getter trait: " << name);
			break;
		}
		case traits_info::Setter:
		{
			LOG(CALLS,"Setter trait: " << name);
			//syntetize method and create a new LLVM function object
			method_info* m=&methods[t->method];
			llvm::Function* f=synt_method(m);
			Function::as_function f2=(Function::as_function)ex->getPointerToFunction(f);
			obj->setSetterByName(name, new Function(f2));
			LOG(CALLS,"End Setter trait: " << name);
			break;
		}
		case traits_info::Method:
		{
			LOG(CALLS,"Method trait: " << name);
			//syntetize method and create a new LLVM function object
			method_info* m=&methods[t->method];
			llvm::Function* f=synt_method(m);
			Function::as_function f2=(Function::as_function)ex->getPointerToFunction(f);
			obj->setVariableByName(name, new Function(f2));
			break;
		}
		case traits_info::Const:
		{
			//TODO: Not so const right now
			if(deferred_initialization)
				obj->setVariableByName(name, new Definable(deferred_initialization));
			else
				obj->setVariableByName(name, new Undefined);
			break;
		}
		case traits_info::Slot:
		{
			if(t->vindex)
			{
				switch(t->vkind)
				{
					case 0x01: //String
					{
						ISWFObject* ret=obj->setVariableByName(name, 
							new ASString(constant_pool.strings[t->vindex]));
						if(t->slot_id)
							obj->setSlot(t->slot_id, ret);
						break;
					}
					case 0x0a: //False
					{
						ISWFObject* ret=obj->setVariableByName(name, new Boolean(false));
						if(t->slot_id)
							obj->setSlot(t->slot_id, ret);
						break;
					}
					case 0x0c: //Null
					{
						ISWFObject* ret=obj->setVariableByName(name, new Null);
						if(t->slot_id)
							obj->setSlot(t->slot_id, ret);
						break;
					}
					default:
					{
						//fallthrough
						LOG(ERROR,"Slot kind " << hex << t->vkind);
						LOG(ERROR,"Trait not supported " << name << " " << t->kind);
						obj->setVariableByName(name, new Undefined);
						return;
					}
				}
				LOG(CALLS,"Slot "<<getMultinameString(t->name)<<" type "<<getMultinameString(t->type_name));
				break;
			}
			else
			{
				//else fallthrough
				LOG(CALLS,"Slot vindex 0 "<<name<<" type "<<getMultinameString(t->type_name));
				bool found;
				ISWFObject* ret=obj->getVariableByName(name,found);
				if(!found)
				{
					//ret=obj->setVariableByName(name, 
					//		buildNamedClass(new ASObject,getMultinameString(t->type_name)));
					//ret->_register();
					if(deferred_initialization)
						obj->setVariableByName(name, new Definable(deferred_initialization));
					else
					{
						LOG(CALLS,"Not deferred");
						ret=obj->setVariableByName(name,new Undefined);
					}
				}
				else
					LOG(CALLS,"Not resetting variable " << name);
				if(t->slot_id)
					obj->setSlot(t->slot_id, ret);
				break;
			}
		}
		default:
			LOG(ERROR,"Trait not supported " << name << " " << t->kind);
			obj->setVariableByName(name, new Undefined);
	}
}

void ABCVm::printTrait(const traits_info* t) const
{
	printMultiname(t->name);
	switch(t->kind&0xf)
	{
		case traits_info::Slot:
			LOG(CALLS,"Slot trait");
			LOG(CALLS,"id: " << t->slot_id << " vindex " << t->vindex << " vkind " << t->vkind);
			break;
		case traits_info::Method:
			LOG(CALLS,"Method trait");
			LOG(CALLS,"method: " << t->method);
			break;
		case traits_info::Getter:
			LOG(CALLS,"Getter trait");
			LOG(CALLS,"method: " << t->method);
			break;
		case traits_info::Setter:
			LOG(CALLS,"Setter trait");
			LOG(CALLS,"method: " << t->method);
			break;
		case traits_info::Class:
			LOG(CALLS,"Class trait: slot "<< t->slot_id);
			printClass(t->classi);
			break;
		case traits_info::Function:
			LOG(CALLS,"Function trait");
			break;
		case traits_info::Const:
			LOG(CALLS,"Const trait");
			break;
	}
}

void ABCVm::printMultiname(int mi) const
{
	if(mi==0)
	{
		LOG(CALLS, "Any (*)");
		return;
	}
	const multiname_info* m=&constant_pool.multinames[mi];
//	LOG(CALLS, "NameID: " << m->name );
	switch(m->kind)
	{
		case 0x07:
			LOG(CALLS, "QName: " << getString(m->name) );
			printNamespace(m->ns);
			break;
		case 0x0d:
			LOG(CALLS, "QNameA");
			break;
		case 0x0f:
			LOG(CALLS, "RTQName");
			break;
		case 0x10:
			LOG(CALLS, "RTQNameA");
			break;
		case 0x11:
			LOG(CALLS, "RTQNameL");
			break;
		case 0x12:
			LOG(CALLS, "RTQNameLA");
			break;
		case 0x09:
		{
			LOG(CALLS, "Multiname: " << getString(m->name));
			const ns_set_info* s=&constant_pool.ns_sets[m->ns_set];
			printNamespaceSet(s);
			break;
		}
		case 0x0e:
			LOG(CALLS, "MultinameA");
			break;
		case 0x1b:
			LOG(CALLS, "MultinameL");
			break;
		case 0x1c:
			LOG(CALLS, "MultinameLA");
			break;
	}
}

void ABCVm::printNamespace(int n) const
{
	if(n==0)
	{
		LOG(CALLS,"Any (*)");
		return;
	}

	const namespace_info* m=&constant_pool.namespaces[n];
	switch(m->kind)
	{
		case 0x08:
			LOG(CALLS, "Namespace " << getString(m->name));
			break;
		case 0x16:
			LOG(CALLS, "PackageNamespace " << getString(m->name));
			break;
		case 0x17:
			LOG(CALLS, "PackageInternalNs " << getString(m->name));
			break;
		case 0x18:
			LOG(CALLS, "ProtectedNamespace " << getString(m->name));
			break;
		case 0x19:
			LOG(CALLS, "ExplicitNamespace " << getString(m->name));
			break;
		case 0x1a:
			LOG(CALLS, "StaticProtectedNamespace " << getString(m->name));
			break;
		case 0x05:
			LOG(CALLS, "PrivateNamespace " << getString(m->name));
			break;
	}
}

void ABCVm::printNamespaceSet(const ns_set_info* m) const
{
	for(int i=0;i<m->count;i++)
	{
		printNamespace(m->ns[i]);
	}
}

ISWFObject* ABCVm::buildClass(int m) 
{
	const class_info* c=&classes[m];
	const instance_info* i=&instances[m];
	string name=getString(constant_pool.multinames[i->name].name);

	if(c->trait_count)
		LOG(NOT_IMPLEMENTED,"Should add class traits for " << name);

/*	//Run class initialization
	method_info* mi=get_method(c->cinit);
	synt_method(mi);
	void* f_ptr=ex->getPointerToFunction(mi->f);
	void (*FP)() = (void (*)())f_ptr;
	LOG(CALLS,"Class init lenght" << mi->body->code_length);
	FP();*/

//	LOG(CALLS,"Building class " << name);
	if(i->supername)
	{
		string super=getString(constant_pool.multinames[i->supername].name);
//		LOG(NOT_IMPLEMENTED,"Inheritance not supported: super " << super);
	}
	if(i->trait_count)
	{
//		LOG(NOT_IMPLEMENTED,"Should add instance traits");
/*		for(int j=0;j<i->trait_count;j++)
			printTrait(&i->traits[j]);*/
	}
/*	//Run instance initialization
	method_info* mi=get_method(i->init);
	if(synt_method(mi))
	{
		void* f_ptr=ex->getPointerToFunction(mi->f);
		void (*FP)() = (void (*)())f_ptr;
		LOG(CALLS,"instance init lenght" << mi->body->code_length);
		FP();
	}*/
	return new ASObject();
}

void ABCVm::printClass(int m) const
{
	const instance_info* i=&instances[m];
	LOG(CALLS,"Class name: " << getMultinameString(i->name));
	LOG(CALLS,"Class supername:");
	printMultiname(i->supername);
	LOG(CALLS,"Flags " <<hex << i->flags);
	LOG(CALLS,"Instance traits n: " <<i->trait_count);
	for(int j=0;j<i->trait_count;j++)
		printTrait(&i->traits[j]);
	LOG(CALLS,"Instance init");
	const method_info* mi=&methods.at(i->init);
	printMethod(mi);

	const class_info* c=&classes[m];
	LOG(CALLS,"Class traits n: " <<c->trait_count);
	for(int j=0;j<c->trait_count;j++)
		printTrait(&c->traits[j]);
	LOG(CALLS,"Class init");
	mi=&methods[c->cinit];
	printMethod(mi);
}

void ABCVm::printMethod(const method_info* m) const
{
	string n=getString(m->name);
	LOG(CALLS,"Method " << n);
	LOG(CALLS,"Params n: " << m->param_count);
	LOG(CALLS,"Return " << m->return_type);
	LOG(CALLS,"Flags " << m->flags);
	if(m->body)
		LOG(CALLS,"Body Lenght " << m->body->code_length)
	else
		LOG(CALLS,"No Body")
}

istream& operator>>(istream& in, u32& v)
{
	int i=0;
	uint8_t t,t2;
	v.val=0;
	do
	{
		in.read((char*)&t,1);
		t2=t;
		t&=0x7f;

		v.val|=(t<<i);
		i+=7;
		if(i==35)
		{
			if(t>15)
				LOG(ERROR,"parsing u32");
			break;
		}
	}
	while(t2&0x80);
	return in;
}

istream& operator>>(istream& in, s32& v)
{
	int i=0;
	uint8_t t,t2;
	v.val=0;
	do
	{
		in.read((char*)&t,1);
		t2=t;
		t&=0x7f;

		v.val|=(t<<i);
		i+=7;
		if(i==35)
		{
			if(t>15)
				LOG(ERROR,"parsing s32");
			break;
		}
	}
	while(t2&0x80);
	if(t2&0x40)
	{
		//Sign extend
		for(i;i<32;i++)
			v.val|=(1<<i);
	}
	return in;
}

istream& operator>>(istream& in, s24& v)
{
	int i=0;
	v.val=0;
	uint8_t t;
	for(i=0;i<24;i+=8)
	{
		in.read((char*)&t,1);
		v.val|=(t<<i);
	}

	if(t&0x80)
	{
		//Sign extend
		for(i;i<32;i++)
			v.val|=(1<<i);
	}
	return in;
}

istream& operator>>(istream& in, u30& v)
{
	int i=0;
	uint8_t t,t2;
	v.val=0;
	do
	{
		in.read((char*)&t,1);
		t2=t;
		t&=0x7f;

		v.val|=(t<<i);
		i+=7;
		if(i>29)
			LOG(ERROR,"parsing u30");
	}
	while(t2&0x80);
	if(v.val&0xc0000000)
			LOG(ERROR,"parsing u30");
	return in;
}

istream& operator>>(istream& in, u8& v)
{
	uint8_t t;
	in.read((char*)&t,1);
	v.val=t;
	return in;
}

istream& operator>>(istream& in, u16& v)
{
	uint16_t t;
	in.read((char*)&t,2);
	v.val=t;
	return in;
}

istream& operator>>(istream& in, d64& v)
{
	//Should check if this is right
	in.read((char*)&v.val,8);
	return in;
}

istream& operator>>(istream& in, string_info& v)
{
	in >> v.size;
	//TODO: String are expected to be UTF-8 encoded.
	//This temporary implementation assume ASCII, so fail if high bit is set
	uint8_t t;
	v.val.reserve(v.size);
	for(int i=0;i<v.size;i++)
	{
		in.read((char*)&t,1);
		v.val.push_back(t);
		if(t&0x80)
			LOG(NOT_IMPLEMENTED,"Multibyte not handled");
	}
	return in;
}

istream& operator>>(istream& in, namespace_info& v)
{
	in >> v.kind >> v.name;
	if(v.kind!=0x05 && v.kind!=0x08 && v.kind!=0x16 && v.kind!=0x17 && v.kind!=0x18 && v.kind!=0x19 && v.kind!=0x1a)
		LOG(ERROR,"Unexpected namespace kind");
	return in;
}

istream& operator>>(istream& in, method_body_info& v)
{
	in >> v.method >> v.max_stack >> v.local_count >> v.init_scope_depth >> v.max_scope_depth >> v.code_length;
	v.code.resize(v.code_length);
	for(int i=0;i<v.code_length;i++)
		in.read(&v.code[i],1);

	in >> v.exception_count;
	v.exceptions.resize(v.exception_count);
	for(int i=0;i<v.exception_count;i++)
		in >> v.exceptions[i];

	in >> v.trait_count;
	v.traits.resize(v.trait_count);
	for(int i=0;i<v.trait_count;i++)
		in >> v.traits[i];
	return in;
}

istream& operator>>(istream& in, ns_set_info& v)
{
	in >> v.count;

	v.ns.resize(v.count);
	for(int i=0;i<v.count;i++)
	{
		in >> v.ns[i];
		if(v.ns[i]==0)
			LOG(ERROR,"0 not allowed");
	}
	return in;
}

istream& operator>>(istream& in, multiname_info& v)
{
	in >> v.kind;

	switch(v.kind)
	{
		case 0x07:
		case 0x0d:
			in >> v.ns >> v.name;
			break;
		case 0x0f:
		case 0x10:
			in >> v.name;
			break;
		case 0x11:
		case 0x12:
			break;
		case 0x09:
		case 0x0e:
			in >> v.name >> v.ns_set;
			break;
		case 0x1b:
		case 0x1c:
			in >> v.ns_set;
			break;
		default:
			LOG(ERROR,"Unexpected multiname kind");
			break;
	}
	return in;
}

istream& operator>>(istream& in, method_info& v)
{
	in >> v.param_count;
	in >> v.return_type;

	v.param_type.resize(v.param_count);
	for(int i=0;i<v.param_count;i++)
		in >> v.param_type[i];
	
	in >> v.name >> v.flags;
	if(v.flags&0x08)
	{
		in >> v.option_count;
		v.options.resize(v.option_count);
		for(int i=0;i<v.option_count;i++)
		{
			in >> v.options[i].val >> v.options[i].kind;
			if(v.options[i].kind>0x1a)
				LOG(ERROR,"Unexpected options type");
		}
	}
	if(v.flags&0x80)
	{
		LOG(ERROR,"Params names not supported");
		abort();
	}
	return in;
}

istream& operator>>(istream& in, script_info& v)
{
	in >> v.init >> v.trait_count;
	v.traits.resize(v.trait_count);
	for(int i=0;i<v.trait_count;i++)
		in >> v.traits[i];
	return in;
}

istream& operator>>(istream& in, class_info& v)
{
	in >> v.cinit >> v.trait_count;
	v.traits.resize(v.trait_count);
	for(int i=0;i<v.trait_count;i++)
	{
		in >> v.traits[i];
	}
	return in;
}

istream& operator>>(istream& in, metadata_info& v)
{
	in >> v.name;
	in >> v.item_count;

	v.items.resize(v.item_count);
	for(int i=0;i<v.item_count;i++)
	{
		in >> v.items[i].key >> v.items[i].value;
	}
	return in;
}

istream& operator>>(istream& in, traits_info& v)
{
	in >> v.name >> v.kind;
	switch(v.kind&0xf)
	{
		case traits_info::Slot:
		case traits_info::Const:
			in >> v.slot_id >> v.type_name >> v.vindex;
			if(v.vindex)
				in >> v.vkind;
			break;
		case traits_info::Class:
			in >> v.slot_id >> v.classi;
			break;
		case traits_info::Function:
			in >> v.slot_id >> v.function;
			break;
		case traits_info::Getter:
		case traits_info::Setter:
		case traits_info::Method:
			in >> v.disp_id >> v.method;
			break;
		default:
			LOG(ERROR,"Unexpected kind " << v.kind);
			break;
	}

	if(v.kind&traits_info::Metadata)
	{
		in >> v.metadata_count;
		v.metadata.resize(v.metadata_count);
		for(int i=0;i<v.metadata_count;i++)
			in >> v.metadata[i];
	}
	return in;
}

istream& operator>>(istream& in, exception_info& v)
{
	in >> v.from >> v.to >> v.target >> v.exc_type >> v.var_name;
	return in;
}

istream& operator>>(istream& in, instance_info& v)
{
	in >> v.name >> v.supername >> v.flags;
	if(v.flags&instance_info::ClassProtectedNs)
		in >> v.protectedNs;

	in >> v.interface_count;
	v.interfaces.resize(v.interface_count);
	for(int i=0;i<v.interface_count;i++)
	{
		in >> v.interfaces[i];
		if(v.interfaces[i]==0)
			abort();
	}

	in >> v.init;

	in >> v.trait_count;
	v.traits.resize(v.trait_count);
	for(int i=0;i<v.trait_count;i++)
		in >> v.traits[i];
	return in;
}

istream& operator>>(istream& in, cpool_info& v)
{
	in >> v.int_count;
	v.integer.resize(v.int_count);
	for(int i=1;i<v.int_count;i++)
		in >> v.integer[i];

	in >> v.uint_count;
	v.uinteger.resize(v.uint_count);
	for(int i=1;i<v.uint_count;i++)
		in >> v.uinteger[i];

	in >> v.double_count;
	v.doubles.resize(v.double_count);
	for(int i=1;i<v.double_count;i++)
		in >> v.doubles[i];

	in >> v.string_count;
	v.strings.resize(v.string_count);
	for(int i=1;i<v.string_count;i++)
		in >> v.strings[i];

	in >> v.namespace_count;
	v.namespaces.resize(v.namespace_count);
	for(int i=1;i<v.namespace_count;i++)
		in >> v.namespaces[i];

	in >> v.ns_set_count;
	v.ns_sets.resize(v.ns_set_count);
	for(int i=1;i<v.ns_set_count;i++)
		in >> v.ns_sets[i];

	in >> v.multiname_count;
	v.multinames.resize(v.multiname_count);
	for(int i=1;i<v.multiname_count;i++)
		in >> v.multinames[i];

	return in;
}

ISWFObject* parseInt(ISWFObject* obj,arguments* args)
{
	return new Integer(0);
}

Math::Math()
{
	setVariableByName("PI",new Number(M_PI));
	setVariableByName("sqrt",new Function(Math::sqrt));
	setVariableByName("atan2",new Function(Math::atan2));
	setVariableByName("floor",new Function(Math::floor));
}

ASFUNCTIONBODY(Math,atan2)
{
	Number* n1=dynamic_cast<Number*>(args->at(0));
	Number* n2=dynamic_cast<Number*>(args->at(1));
	if(n1 && n2)
		return new Number(::atan2(*n1,*n2));
	else
	{
		LOG(CALLS, "Invalid argument type ("<<args->at(0)->getObjectType()<<','<<args->at(1)->getObjectType()<<')' );
		abort();
	}
}

ASFUNCTIONBODY(Math,floor)
{
	Number* n=dynamic_cast<Number*>(args->at(0));
	if(n)
		return new Number(::floor(*n));
	else
	{
		LOG(TRACE,"Invalid argument");
		abort();
	}
}

ASFUNCTIONBODY(Math,sqrt)
{
	Number* n=dynamic_cast<Number*>(args->at(0));
	if(n)
		return new Number(::sqrt(*n));
	else
	{
		LOG(TRACE,"Invalid argument");
		abort();
	}
}
