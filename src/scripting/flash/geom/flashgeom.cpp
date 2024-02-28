/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "scripting/abc.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/geom/Matrix3D.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/flash/display/BitmapContainer.h"

using namespace lightspark;
using namespace std;

void Rectangle::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	
	c->setDeclaredMethodByQName("left","",Class<IFunction>::getFunction(c->getSystemState(),_getLeft,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("left","",Class<IFunction>::getFunction(c->getSystemState(),_setLeft),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(c->getSystemState(),_getLeft,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(c->getSystemState(),_setLeft),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("right","",Class<IFunction>::getFunction(c->getSystemState(),_getRight,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("right","",Class<IFunction>::getFunction(c->getSystemState(),_setRight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_getWidth,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",Class<IFunction>::getFunction(c->getSystemState(),_setWidth),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("top","",Class<IFunction>::getFunction(c->getSystemState(),_getTop,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("top","",Class<IFunction>::getFunction(c->getSystemState(),_setTop),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(c->getSystemState(),_getTop,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(c->getSystemState(),_setTop),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("bottom","",Class<IFunction>::getFunction(c->getSystemState(),_getBottom,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bottom","",Class<IFunction>::getFunction(c->getSystemState(),_setBottom),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_getHeight,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",Class<IFunction>::getFunction(c->getSystemState(),_setHeight),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("bottomRight","",Class<IFunction>::getFunction(c->getSystemState(),_getBottomRight,0,Class<Point>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bottomRight","",Class<IFunction>::getFunction(c->getSystemState(),_setBottomRight),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("size","",Class<IFunction>::getFunction(c->getSystemState(),_getSize,0,Class<Point>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("size","",Class<IFunction>::getFunction(c->getSystemState(),_setSize),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("topLeft","",Class<IFunction>::getFunction(c->getSystemState(),_getTopLeft,0,Class<Point>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("topLeft","",Class<IFunction>::getFunction(c->getSystemState(),_setTopLeft),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(c->getSystemState(),clone,0,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("contains","",Class<IFunction>::getFunction(c->getSystemState(),contains,2,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("containsPoint","",Class<IFunction>::getFunction(c->getSystemState(),containsPoint,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("containsRect","",Class<IFunction>::getFunction(c->getSystemState(),containsRect,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("equals","",Class<IFunction>::getFunction(c->getSystemState(),equals,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("inflate","",Class<IFunction>::getFunction(c->getSystemState(),inflate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("inflatePoint","",Class<IFunction>::getFunction(c->getSystemState(),inflatePoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("intersection","",Class<IFunction>::getFunction(c->getSystemState(),intersection,1,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("intersects","",Class<IFunction>::getFunction(c->getSystemState(),intersects,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("isEmpty","",Class<IFunction>::getFunction(c->getSystemState(),isEmpty,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("offset","",Class<IFunction>::getFunction(c->getSystemState(),offset),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("offsetPoint","",Class<IFunction>::getFunction(c->getSystemState(),offsetPoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setEmpty","",Class<IFunction>::getFunction(c->getSystemState(),setEmpty),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("union","",Class<IFunction>::getFunction(c->getSystemState(),_union,1,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTo","",Class<IFunction>::getFunction(c->getSystemState(),setTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyFrom","",Class<IFunction>::getFunction(c->getSystemState(),copyFrom),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
}

const lightspark::RECT Rectangle::getRect() const
{
	return lightspark::RECT(x,x+width,y,y+height);
}

bool Rectangle::destruct()
{
	x=0;
	y=0;
	width=0;
	height=0;
	return destructIntern();
}
void Rectangle::addUser(DisplayObject* u)
{
	users.insert(u);
}

void Rectangle::removeUser(DisplayObject* u)
{
	users.erase(u);
}
void Rectangle::notifyUsers()
{
	for(auto it=users.begin();it!=users.end();it++)
		(*it)->updatedRect();
}

ASFUNCTIONBODY_ATOM(Rectangle,_constructor)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);

	if(argslen>=1)
		th->x=asAtomHandler::toNumber(args[0]);
	if(argslen>=2)
		th->y=asAtomHandler::toNumber(args[1]);
	if(argslen>=3)
		th->width=asAtomHandler::toNumber(args[2]);
	if(argslen>=4)
		th->height=asAtomHandler::toNumber(args[3]);
}

ASFUNCTIONBODY_ATOM(Rectangle,_getLeft)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,wrk,th->x);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setLeft)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	if (th->x != val)
	{
		th->x=val;
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,_getRight)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,wrk,th->x + th->width);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setRight)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	if (th->width != val-th->x)
	{
		th->width=val-th->x;
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,_getWidth)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,wrk,th->width);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setWidth)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	if (th->width != val)
	{
		th->width=val;
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,_getTop)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,wrk,th->y);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setTop)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	if (th->y != val)
	{
		th->y=val;
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,_getBottom)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,wrk,th->y + th->height);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setBottom)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	if (th->height != val-th->y)
	{
		th->height=val-th->y;
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,_getBottomRight)
{
	assert_and_throw(argslen==0);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* res = Class<Point>::getInstanceS(wrk,th->x + th->width, th->y + th->height);
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setBottomRight)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* br = asAtomHandler::as<Point>(args[0]);
	if (th->width != br->getX() - th->x || th->height != br->getY() - th->y)
	{
		th->width = br->getX() - th->x;
		th->height = br->getY() - th->y;
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,_getTopLeft)
{
	assert_and_throw(argslen==0);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* res = Class<Point>::getInstanceS(wrk,th->x, th->y);
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setTopLeft)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* br = asAtomHandler::as<Point>(args[0]);
	if (th->x != br->getX()|| th->y != br->getY())
	{
		th->x = br->getX();
		th->y = br->getY();
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,_getSize)
{
	assert_and_throw(argslen==0);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* res = Class<Point>::getInstanceS(wrk,th->width, th->height);
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setSize)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* br = asAtomHandler::as<Point>(args[0]);
	if (th->width != br->getX()|| th->height != br->getY())
	{
		th->width = br->getX();
		th->height = br->getY();
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,_getHeight)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,wrk,th->height);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setHeight)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	if (th->height != val)
	{
		th->height=val;
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,clone)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* res=Class<Rectangle>::getInstanceS(wrk);
	res->x=th->x;
	res->y=th->y;
	res->width=th->width;
	res->height=th->height;
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,contains)
{
	assert_and_throw(argslen == 2);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t x = asAtomHandler::toNumber(args[0]);
	number_t y = asAtomHandler::toNumber(args[1]);

	asAtomHandler::setBool(ret,th->x <= x && x <= th->x + th->width
						&& th->y <= y && y <= th->y + th->height );
}

ASFUNCTIONBODY_ATOM(Rectangle,containsPoint)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* br = asAtomHandler::as<Point>(args[0]);
	number_t x = br->getX();
	number_t y = br->getY();

	asAtomHandler::setBool(ret,th->x <= x && x <= th->x + th->width
						&& th->y <= y && y <= th->y + th->height );
}

ASFUNCTIONBODY_ATOM(Rectangle,containsRect)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* cr = asAtomHandler::as<Rectangle>(args[0]);

	asAtomHandler::setBool(ret,th->x <= cr->x && cr->x + cr->width <= th->x + th->width
						&& th->y <= cr->y && cr->y + cr->height <= th->y + th->height );
}

ASFUNCTIONBODY_ATOM(Rectangle,equals)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* co = asAtomHandler::as<Rectangle>(args[0]);

	asAtomHandler::setBool(ret,th->x == co->x && th->width == co->width
						&& th->y == co->y && th->height == co->height );
}

ASFUNCTIONBODY_ATOM(Rectangle,inflate)
{
	assert_and_throw(argslen == 2);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t dx = asAtomHandler::toNumber(args[0]);
	number_t dy = asAtomHandler::toNumber(args[1]);

	th->x -= dx;
	th->width += 2 * dx;
	th->y -= dy;
	th->height += 2 * dy;
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(Rectangle,inflatePoint)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* po = asAtomHandler::as<Point>(args[0]);
	number_t dx = po->getX();
	number_t dy = po->getY();

	th->x -= dx;
	th->width += 2 * dx;
	th->y -= dy;
	th->height += 2 * dy;
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(Rectangle,intersection)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* ti = asAtomHandler::as<Rectangle>(args[0]);
	Rectangle* res = Class<Rectangle>::getInstanceS(wrk);

	number_t thtop = th->y;
	number_t thleft = th->x;
	number_t thright = th->x + th->width;
	number_t thbottom = th->y + th->height;

	number_t titop = ti->y;
	number_t tileft = ti->x;
	number_t tiright = ti->x + ti->width;
	number_t tibottom = ti->y + ti->height;

	if ( thtop > tibottom || thright < tileft ||
						thbottom < titop || thleft > tiright )
	{
		// rectangles don't intersect
		res->x = 0;
		res->y = 0;
		res->width = 0;
		res->height = 0;
		ret = asAtomHandler::fromObject(res);
		return;
	}

	Rectangle* leftmost = ti;
	Rectangle* rightmost = th;

	// find left most
	if ( thleft < tileft )
	{
		leftmost = th;
		rightmost = ti;
	}

	Rectangle* topmost = ti;
	Rectangle* bottommost = th;

	// find top most
	if ( thtop < titop )
	{
		topmost = th;
		bottommost = ti;
	}

	res->x = rightmost->x;
	res->width = min(leftmost->x + leftmost->width, rightmost->x + rightmost->width) - rightmost->x;
	res->y = bottommost->y;
	res->height = min(topmost->y + topmost->height, bottommost->y + bottommost->height) - bottommost->y;

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,intersects)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* ti = asAtomHandler::as<Rectangle>(args[0]);

	number_t thtop = th->y;
	number_t thleft = th->x;
	number_t thright = th->x + th->width;
	number_t thbottom = th->y + th->height;

	number_t titop = ti->y;
	number_t tileft = ti->x;
	number_t tiright = ti->x + ti->width;
	number_t tibottom = ti->y + ti->height;

	asAtomHandler::setBool(ret,!(thtop > tibottom || thright < tileft ||
						thbottom < titop || thleft > tiright) );
}

ASFUNCTIONBODY_ATOM(Rectangle,isEmpty)
{
	assert_and_throw(argslen == 0);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);

	asAtomHandler::setBool(ret,th->width <= 0 || th->height <= 0 );
}

ASFUNCTIONBODY_ATOM(Rectangle,offset)
{
	assert_and_throw(argslen == 2);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);

	th->x += asAtomHandler::toNumber(args[0]);
	th->y += asAtomHandler::toNumber(args[1]);
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(Rectangle,offsetPoint)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* po = asAtomHandler::as<Point>(args[0]);

	th->x += po->getX();
	th->y += po->getY();
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(Rectangle,setEmpty)
{
	assert_and_throw(argslen == 0);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);

	th->x = 0;
	th->y = 0;
	th->width = 0;
	th->height = 0;
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(Rectangle,_union)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* ti = asAtomHandler::as<Rectangle>(args[0]);
	Rectangle* res = Class<Rectangle>::getInstanceS(wrk);

	res->x = th->x;
	res->y = th->y;
	res->width = th->width;
	res->height = th->height;

	if ( ti->width == 0 || ti->height == 0 )
	{
		ret = asAtomHandler::fromObject(res);
		return;
	}

	res->x = min(th->x, ti->x);
	res->y = min(th->y, ti->y);
	res->width = max(th->x + th->width, ti->x + ti->width);
	res->height = max(th->y + th->height, ti->y + ti->height);

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,_toString)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	tiny_string s = "(x=";
	s+= Number::toString(th->x);
	s+= ", y=";
	s+= Number::toString(th->y);
	s+= ", w=";
	s+= Number::toString(th->width);
	s+= ", h=";
	s+= Number::toString(th->height);
	s+= ")";
	ret = asAtomHandler::fromObject(abstract_s(wrk,s));
}

ASFUNCTIONBODY_ATOM(Rectangle,setTo)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t x,y,wi,h;
	ARG_CHECK(ARG_UNPACK(x)(y)(wi)(h));
	th->x = x;
	th->y = y;
	th->width = wi;
	th->height = h;
	th->notifyUsers();
}
ASFUNCTIONBODY_ATOM(Rectangle,copyFrom)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	_NR<Rectangle> sourcerect;
	ARG_CHECK(ARG_UNPACK(sourcerect));
	if (!sourcerect.isNull())
	{
		th->x = sourcerect->x;
		th->y = sourcerect->y;
		th->width = sourcerect->width;
		th->height = sourcerect->height;
		th->notifyUsers();
	}
}

ColorTransform::ColorTransform(ASWorker* wrk, Class_base* c, const CXFORMWITHALPHA& cx)
  : ASObject(wrk,c,T_OBJECT,SUBTYPE_COLORTRANSFORM)
{
	cx.getParameters(redMultiplier, greenMultiplier, blueMultiplier, alphaMultiplier,
					 redOffset, greenOffset, blueOffset, alphaOffset);
}

void ColorTransform::setProperties(const CXFORMWITHALPHA &cx)
{
	cx.getParameters(redMultiplier, greenMultiplier, blueMultiplier, alphaMultiplier,
					 redOffset, greenOffset, blueOffset, alphaOffset);
}

void ColorTransform::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;

	// properties
	c->setDeclaredMethodByQName("color","",Class<IFunction>::getFunction(c->getSystemState(),getColor,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("color","",Class<IFunction>::getFunction(c->getSystemState(),setColor),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("redMultiplier","",Class<IFunction>::getFunction(c->getSystemState(),getRedMultiplier,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("redMultiplier","",Class<IFunction>::getFunction(c->getSystemState(),setRedMultiplier),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("greenMultiplier","",Class<IFunction>::getFunction(c->getSystemState(),getGreenMultiplier,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("greenMultiplier","",Class<IFunction>::getFunction(c->getSystemState(),setGreenMultiplier),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("blueMultiplier","",Class<IFunction>::getFunction(c->getSystemState(),getBlueMultiplier,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blueMultiplier","",Class<IFunction>::getFunction(c->getSystemState(),setBlueMultiplier),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("alphaMultiplier","",Class<IFunction>::getFunction(c->getSystemState(),getAlphaMultiplier,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("alphaMultiplier","",Class<IFunction>::getFunction(c->getSystemState(),setAlphaMultiplier),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("redOffset","",Class<IFunction>::getFunction(c->getSystemState(),getRedOffset,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("redOffset","",Class<IFunction>::getFunction(c->getSystemState(),setRedOffset),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("greenOffset","",Class<IFunction>::getFunction(c->getSystemState(),getGreenOffset,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("greenOffset","",Class<IFunction>::getFunction(c->getSystemState(),setGreenOffset),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("blueOffset","",Class<IFunction>::getFunction(c->getSystemState(),getBlueOffset,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blueOffset","",Class<IFunction>::getFunction(c->getSystemState(),setBlueOffset),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("alphaOffset","",Class<IFunction>::getFunction(c->getSystemState(),getAlphaOffset,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("alphaOffset","",Class<IFunction>::getFunction(c->getSystemState(),setAlphaOffset),SETTER_METHOD,true);

	// methods
	c->setDeclaredMethodByQName("concat","",Class<IFunction>::getFunction(c->getSystemState(),concat),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
}

bool ColorTransform::destruct()
{
	redMultiplier=1.0;
	greenMultiplier=1.0;
	blueMultiplier=1.0;
	alphaMultiplier=1.0;
	redOffset=0.0;
	greenOffset=0.0;
	blueOffset=0.0;
	alphaOffset=0.0;
	return ASObject::destruct();
}

ASFUNCTIONBODY_ATOM(ColorTransform,_constructor)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	assert_and_throw(argslen<=8);
	if(0 < argslen)
		th->redMultiplier=asAtomHandler::toNumber(args[0]);
	else
		th->redMultiplier=1.0;
	if(1 < argslen)
		th->greenMultiplier=asAtomHandler::toNumber(args[1]);
	else
		th->greenMultiplier=1.0;
	if(2 < argslen)
		th->blueMultiplier=asAtomHandler::toNumber(args[2]);
	else
		th->blueMultiplier=1.0;
	if(3 < argslen)
		th->alphaMultiplier=asAtomHandler::toNumber(args[3]);
	else
		th->alphaMultiplier=1.0;
	if(4 < argslen)
		th->redOffset=asAtomHandler::toNumber(args[4]);
	else
		th->redOffset=0.0;
	if(5 < argslen)
		th->greenOffset=asAtomHandler::toNumber(args[5]);
	else
		th->greenOffset=0.0;
	if(6 < argslen)
		th->blueOffset=asAtomHandler::toNumber(args[6]);
	else
		th->blueOffset=0.0;
	if(7 < argslen)
		th->alphaOffset=asAtomHandler::toNumber(args[7]);
	else
		th->alphaOffset=0.0;
}

ASFUNCTIONBODY_ATOM(ColorTransform,setColor)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	assert_and_throw(argslen==1);
	uint32_t tmp=asAtomHandler::toInt(args[0]);
	//Setting multiplier to 0
	th->redMultiplier=0;
	th->greenMultiplier=0;
	th->blueMultiplier=0;
	//Setting offset to the input value
	th->redOffset=(tmp>>16)&0xff;
	th->greenOffset=(tmp>>8)&0xff;
	th->blueOffset=tmp&0xff;
}

ASFUNCTIONBODY_ATOM(ColorTransform,getColor)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);

	int ro, go, bo;
	ro = static_cast<int>(th->redOffset) & 0xff;
	go = static_cast<int>(th->greenOffset) & 0xff;
	bo = static_cast<int>(th->blueOffset) & 0xff;

	uint32_t color = (ro<<16) | (go<<8) | bo;

	asAtomHandler::setUInt(ret,wrk,color);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getRedMultiplier)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,wrk,th->redMultiplier);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setRedMultiplier)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->redMultiplier = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getGreenMultiplier)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,wrk,th->greenMultiplier);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setGreenMultiplier)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->greenMultiplier = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getBlueMultiplier)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,wrk,th->blueMultiplier);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setBlueMultiplier)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->blueMultiplier = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getAlphaMultiplier)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,wrk,th->alphaMultiplier);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setAlphaMultiplier)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->alphaMultiplier = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getRedOffset)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,wrk,th->redOffset);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setRedOffset)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->redOffset = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getGreenOffset)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,wrk,th->greenOffset);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setGreenOffset)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->greenOffset = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getBlueOffset)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,wrk,th->blueOffset);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setBlueOffset)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->blueOffset = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getAlphaOffset)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,wrk,th->alphaOffset);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setAlphaOffset)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->alphaOffset = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,concat)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	ColorTransform* ct=asAtomHandler::as<ColorTransform>(args[0]);
	th->redMultiplier *= ct->redMultiplier;
	th->redOffset = th->redOffset * ct->redMultiplier + ct->redOffset;
	th->greenMultiplier *= ct->greenMultiplier;
	th->greenOffset = th->greenOffset * ct->greenMultiplier + ct->greenOffset;
	th->blueMultiplier *= ct->blueMultiplier;
	th->blueOffset = th->blueOffset * ct->blueMultiplier + ct->blueOffset;
	th->alphaMultiplier *= ct->alphaMultiplier;
	th->alphaOffset = th->alphaOffset * ct->alphaMultiplier + ct->alphaOffset;
}

ASFUNCTIONBODY_ATOM(ColorTransform,_toString)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	char buf[1024];
	snprintf(buf,1024,"(redOffset=%f, redMultiplier=%f, greenOffset=%f, greenMultiplier=%f blueOffset=%f blueMultiplier=%f alphaOffset=%f, alphaMultiplier=%f)",
			th->redOffset, th->redMultiplier, th->greenOffset, th->greenMultiplier, th->blueOffset, th->blueMultiplier, th->alphaOffset, th->alphaMultiplier);
	
	ret = asAtomHandler::fromObject(abstract_s(wrk,buf));
}

void Point::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(c->getSystemState(),_getX,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(c->getSystemState(),_getY,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),_getlength,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",Class<IFunction>::getFunction(c->getSystemState(),_setX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",Class<IFunction>::getFunction(c->getSystemState(),_setY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("interpolate","",Class<IFunction>::getFunction(c->getSystemState(),interpolate,3,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("distance","",Class<IFunction>::getFunction(c->getSystemState(),distance,2,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("add","",Class<IFunction>::getFunction(c->getSystemState(),add,1,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("subtract","",Class<IFunction>::getFunction(c->getSystemState(),subtract,1,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(c->getSystemState(),clone,0,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("equals","",Class<IFunction>::getFunction(c->getSystemState(),equals,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("normalize","",Class<IFunction>::getFunction(c->getSystemState(),normalize),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("offset","",Class<IFunction>::getFunction(c->getSystemState(),offset),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("polar","",Class<IFunction>::getFunction(c->getSystemState(),polar,2,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("setTo","",Class<IFunction>::getFunction(c->getSystemState(),setTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyFrom","",Class<IFunction>::getFunction(c->getSystemState(),copyFrom),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY_ATOM(Point,_toString)
{
	Point* th=asAtomHandler::as<Point>(obj);
	char buf[512];
	snprintf(buf,512,"(a=%f, b=%f)",th->x,th->y);
	ret = asAtomHandler::fromObject(abstract_s(wrk,buf));
}

number_t Point::lenImpl(number_t x, number_t y)
{
	return sqrt(x*x + y*y);
}

bool Point::destruct()
{
	x=0;
	y=0;
	return ASObject::destruct();
}

number_t Point::len() const
{
	return lenImpl(x, y);
}

ASFUNCTIONBODY_ATOM(Point,_constructor)
{
	Point* th=asAtomHandler::as<Point>(obj);
	if(argslen>=1)
		th->x=asAtomHandler::toNumber(args[0]);
	if(argslen>=2)
		th->y=asAtomHandler::toNumber(args[1]);
}

ASFUNCTIONBODY_ATOM(Point,_getX)
{
	Point* th=asAtomHandler::as<Point>(obj);
	asAtomHandler::setNumber(ret,wrk,th->x);
}

ASFUNCTIONBODY_ATOM(Point,_getY)
{
	Point* th=asAtomHandler::as<Point>(obj);
	asAtomHandler::setNumber(ret,wrk,th->y);
}

ASFUNCTIONBODY_ATOM(Point,_setX)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==1);
	th->x = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Point,_setY)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==1);
	th->y = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Point,_getlength)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==0);
	asAtomHandler::setNumber(ret,wrk,th->len());
}

ASFUNCTIONBODY_ATOM(Point,interpolate)
{
	assert_and_throw(argslen==3);
	Point* pt1=asAtomHandler::as<Point>(args[0]);
	Point* pt2=asAtomHandler::as<Point>(args[1]);
	number_t f=asAtomHandler::toNumber(args[2]);
	Point* res=Class<Point>::getInstanceS(wrk);
	res->x = pt2->x - (pt2->x - pt1->x) * f;
	res->y = pt2->y - (pt2->y - pt1->y) * f;
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Point,distance)
{
	assert_and_throw(argslen==2);
	Point* pt1=asAtomHandler::as<Point>(args[0]);
	Point* pt2=asAtomHandler::as<Point>(args[1]);
	number_t res=lenImpl(pt2->x - pt1->x, pt2->y - pt1->y);
	asAtomHandler::setNumber(ret,wrk,res);
}

ASFUNCTIONBODY_ATOM(Point,add)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==1);
	Point* v=asAtomHandler::as<Point>(args[0]);
	Point* res=Class<Point>::getInstanceS(wrk);
	res->x = th->x + v->x;
	res->y = th->y + v->y;
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Point,subtract)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==1);
	Point* v=asAtomHandler::as<Point>(args[0]);
	Point* res=Class<Point>::getInstanceS(wrk);
	res->x = th->x - v->x;
	res->y = th->y - v->y;
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Point,clone)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==0);
	Point* res=Class<Point>::getInstanceS(wrk);
	res->x = th->x;
	res->y = th->y;
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Point,equals)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==1);
	Point* toCompare=asAtomHandler::as<Point>(args[0]);
	asAtomHandler::setBool(ret,(th->x == toCompare->x) & (th->y == toCompare->y));
}

ASFUNCTIONBODY_ATOM(Point,normalize)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen<2);
	number_t thickness = argslen > 0 ? asAtomHandler::toNumber(args[0]) : 1.0;
	number_t len = th->len();
	th->x = len == 0 ? 0 : th->x * thickness / len;
	th->y = len == 0 ? 0 : th->y * thickness / len;
}

ASFUNCTIONBODY_ATOM(Point,offset)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==2);
	number_t dx = asAtomHandler::toNumber(args[0]);
	number_t dy = asAtomHandler::toNumber(args[1]);
	th->x += dx;
	th->y += dy;
}

ASFUNCTIONBODY_ATOM(Point,polar)
{
	assert_and_throw(argslen==2);
	number_t len = asAtomHandler::toNumber(args[0]);
	number_t angle = asAtomHandler::toNumber(args[1]);
	Point* res=Class<Point>::getInstanceS(wrk);
	res->x = len * cos(angle);
	res->y = len * sin(angle);
	ret = asAtomHandler::fromObject(res);
}
ASFUNCTIONBODY_ATOM(Point,setTo)
{
	Point* th=asAtomHandler::as<Point>(obj);
	number_t x;
	number_t y;
	ARG_CHECK(ARG_UNPACK(x)(y));
	th->x = x;
	th->y = y;
}
ASFUNCTIONBODY_ATOM(Point,copyFrom)
{
	Point* th=asAtomHandler::as<Point>(obj);
	_NR<Point> sourcepoint;
	ARG_CHECK(ARG_UNPACK(sourcepoint));
	if (!sourcepoint.isNull())
	{
		th->x = sourcepoint->x;
		th->y = sourcepoint->y;
	}
}

Transform::Transform(ASWorker* wrk, Class_base* c):ASObject(wrk,c),perspectiveProjection(Class<PerspectiveProjection>::getInstanceSNoArgs(wrk))
{
}
Transform::Transform(ASWorker* wrk,Class_base* c, _R<DisplayObject> o):ASObject(wrk,c),owner(o),perspectiveProjection(Class<PerspectiveProjection>::getInstanceSNoArgs(wrk))
{
}

bool Transform::destruct()
{
	owner.reset();
	perspectiveProjection.reset();
	matrix3D.reset();
	return destructIntern();
}

void Transform::finalize()
{
	owner.reset();
	perspectiveProjection.reset();
	matrix3D.reset();
}

void Transform::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	if (owner)
		owner->prepareShutdown();
	if (perspectiveProjection)
		perspectiveProjection->prepareShutdown();
	if (matrix3D)
		matrix3D->prepareShutdown();
}

bool Transform::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = ASObject::countCylicMemberReferences(gcstate);
	if (owner)
		ret = owner->countAllCylicMemberReferences(gcstate) || ret;
	if (perspectiveProjection)
		ret = perspectiveProjection->countAllCylicMemberReferences(gcstate) || ret;
	if (matrix3D)
		ret = matrix3D->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void Transform::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("colorTransform","",Class<IFunction>::getFunction(c->getSystemState(),_getColorTransform,0,Class<Transform>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("colorTransform","",Class<IFunction>::getFunction(c->getSystemState(),_setColorTransform),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("matrix","",Class<IFunction>::getFunction(c->getSystemState(),_setMatrix),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("matrix","",Class<IFunction>::getFunction(c->getSystemState(),_getMatrix,0,Class<Matrix>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("concatenatedMatrix","",Class<IFunction>::getFunction(c->getSystemState(),_getConcatenatedMatrix,0,Class<Matrix>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER(c, perspectiveProjection);
	REGISTER_GETTER_SETTER(c, matrix3D);
}
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(Transform, perspectiveProjection)
ASFUNCTIONBODY_GETTER_SETTER_CB(Transform, matrix3D,onSetMatrix3D)

void Transform::onSetMatrix3D(_NR<Matrix3D> oldValue)
{
	owner->setMatrix3D(this->matrix3D);
}

ASFUNCTIONBODY_ATOM(Transform,_constructor)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	// it's not in the specs but it seems to be possible to construct a Transform object with an owner as argment
	ARG_CHECK(ARG_UNPACK(th->owner,NullRef));
}

ASFUNCTIONBODY_ATOM(Transform,_getMatrix)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	assert_and_throw(argslen==0);
	if (th->owner->matrix.isNull())
	{
		asAtomHandler::setNull(ret);
		return;
	}
	const MATRIX& res=th->owner->getMatrix();
	ret = asAtomHandler::fromObject(Class<Matrix>::getInstanceS(wrk,res));
}

ASFUNCTIONBODY_ATOM(Transform,_setMatrix)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	_NR<Matrix> m;
	ARG_CHECK(ARG_UNPACK(m));
	th->owner->setMatrix(m);
}

ASFUNCTIONBODY_ATOM(Transform,_getColorTransform)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	if (th->owner->colorTransform.isNull())
		th->owner->colorTransform = _NR<ColorTransform>(Class<ColorTransform>::getInstanceS(wrk));
	th->owner->colorTransform->incRef();
	ret = asAtomHandler::fromObject(th->owner->colorTransform.getPtr());
}

ASFUNCTIONBODY_ATOM(Transform,_setColorTransform)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	_NR<ColorTransform> ct;
	ARG_CHECK(ARG_UNPACK(ct));
	if (ct.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "colorTransform");
		return;
	}

	th->owner->colorTransform = ct;
	th->owner->hasChanged=true;
	th->owner->setNeedsCachedBitmapRecalculation();
	if (th->owner->isOnStage())
		th->owner->requestInvalidation(wrk->getSystemState());
	else
		th->owner->requestInvalidationFilterParent();
}

ASFUNCTIONBODY_ATOM(Transform,_getConcatenatedMatrix)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	ret = asAtomHandler::fromObject(Class<Matrix>::getInstanceS(wrk,th->owner->getConcatenatedMatrix()));
}

Matrix::Matrix(ASWorker* wrk, Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_MATRIX)
{
}

Matrix::Matrix(ASWorker* wrk,Class_base* c, const MATRIX& m):ASObject(wrk,c,T_OBJECT,SUBTYPE_MATRIX),matrix(m)
{
}

void Matrix::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable=true;
	//Properties
	c->setDeclaredMethodByQName("a","",Class<IFunction>::getFunction(c->getSystemState(),_get_a,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("b","",Class<IFunction>::getFunction(c->getSystemState(),_get_b,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("c","",Class<IFunction>::getFunction(c->getSystemState(),_get_c,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("d","",Class<IFunction>::getFunction(c->getSystemState(),_get_d,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("tx","",Class<IFunction>::getFunction(c->getSystemState(),_get_tx,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("ty","",Class<IFunction>::getFunction(c->getSystemState(),_get_ty,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	
	c->setDeclaredMethodByQName("a","",Class<IFunction>::getFunction(c->getSystemState(),_set_a),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("b","",Class<IFunction>::getFunction(c->getSystemState(),_set_b),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("c","",Class<IFunction>::getFunction(c->getSystemState(),_set_c),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("d","",Class<IFunction>::getFunction(c->getSystemState(),_set_d),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("tx","",Class<IFunction>::getFunction(c->getSystemState(),_set_tx),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("ty","",Class<IFunction>::getFunction(c->getSystemState(),_set_ty),SETTER_METHOD,true);
	
	//Methods 
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(c->getSystemState(),clone,0,Class<Matrix>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("concat","",Class<IFunction>::getFunction(c->getSystemState(),concat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyFrom","",Class<IFunction>::getFunction(c->getSystemState(),copyFrom),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createBox","",Class<IFunction>::getFunction(c->getSystemState(),createBox),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createGradientBox","",Class<IFunction>::getFunction(c->getSystemState(),createGradientBox),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("deltaTransformPoint","",Class<IFunction>::getFunction(c->getSystemState(),deltaTransformPoint,1,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("identity","",Class<IFunction>::getFunction(c->getSystemState(),identity),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("invert","",Class<IFunction>::getFunction(c->getSystemState(),invert),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("rotate","",Class<IFunction>::getFunction(c->getSystemState(),rotate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("scale","",Class<IFunction>::getFunction(c->getSystemState(),scale),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTo","",Class<IFunction>::getFunction(c->getSystemState(),setTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("transformPoint","",Class<IFunction>::getFunction(c->getSystemState(),transformPoint,1,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("translate","",Class<IFunction>::getFunction(c->getSystemState(),translate),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY_ATOM(Matrix,_constructor)
{
	assert_and_throw(argslen <= 6);
	
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	
	//Mapping to cairo_matrix_t
	//a -> xx
	//b -> yx
	//c -> xy
	//d -> yy
	//tx -> x0
	//ty -> y0
	
	if (argslen >= 1)
		th->matrix.xx = asAtomHandler::toNumber(args[0]);
	if (argslen >= 2)
		th->matrix.yx = asAtomHandler::toNumber(args[1]);
	if (argslen >= 3)
		th->matrix.xy = asAtomHandler::toNumber(args[2]);
	if (argslen >= 4)
		th->matrix.yy = asAtomHandler::toNumber(args[3]);
	if (argslen >= 5)
		th->matrix.x0 = asAtomHandler::toNumber(args[4]);
	if (argslen == 6)
		th->matrix.y0 = asAtomHandler::toNumber(args[5]);
}

ASFUNCTIONBODY_ATOM(Matrix,_toString)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	char buf[512];
	snprintf(buf,512,"(a=%f, b=%f, c=%f, d=%f, tx=%f, ty=%f)",
			th->matrix.xx, th->matrix.yx, th->matrix.xy, th->matrix.yy, th->matrix.x0, th->matrix.y0);
	ret = asAtomHandler::fromObject(abstract_s(wrk,buf));
}

MATRIX Matrix::getMATRIX() const
{
	return matrix;
}

bool Matrix::destruct()
{
	matrix = MATRIX();
	return destructIntern();
}

ASFUNCTIONBODY_ATOM(Matrix,_get_a)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,wrk,th->matrix.xx);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_a)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.xx = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,_get_b)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,wrk,th->matrix.yx);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_b)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.yx = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,_get_c)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,wrk,th->matrix.xy);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_c)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.xy = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,_get_d)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,wrk,th->matrix.yy);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_d)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.yy = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,_get_tx)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,wrk,th->matrix.x0);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_tx)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.x0 = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,_get_ty)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,wrk,th->matrix.y0);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_ty)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.y0 = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,clone)
{
	assert_and_throw(argslen==0);

	Matrix* th=asAtomHandler::as<Matrix>(obj);
	Matrix* res=Class<Matrix>::getInstanceS(wrk,th->matrix);
	ret =asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Matrix,concat)
{
	assert_and_throw(argslen==1);

	Matrix* th=asAtomHandler::as<Matrix>(obj);
	Matrix* m=asAtomHandler::as<Matrix>(args[0]);

	//Premultiply, which is flash convention
	cairo_matrix_multiply(&th->matrix,&th->matrix,&m->matrix);
}

ASFUNCTIONBODY_ATOM(Matrix,identity)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==0);
	
	cairo_matrix_init_identity(&th->matrix);
}

ASFUNCTIONBODY_ATOM(Matrix,invert)
{
	assert_and_throw(argslen==0);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	th->matrix=th->matrix.getInverted();
}

ASFUNCTIONBODY_ATOM(Matrix,translate)
{
	assert_and_throw(argslen==2);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	number_t dx = asAtomHandler::toNumber(args[0]);
	number_t dy = asAtomHandler::toNumber(args[1]);

	th->matrix.translate(dx,dy);
}

ASFUNCTIONBODY_ATOM(Matrix,rotate)
{
	assert_and_throw(argslen==1);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	number_t angle = asAtomHandler::toNumber(args[0]);

	th->matrix.rotate(angle);
}

ASFUNCTIONBODY_ATOM(Matrix,scale)
{
	assert_and_throw(argslen==2);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	number_t sx = asAtomHandler::toNumber(args[0]);
	number_t sy = asAtomHandler::toNumber(args[1]);

	th->matrix.scale(sx, sy);
}

void Matrix::_createBox (number_t scaleX, number_t scaleY, number_t angle, number_t x, number_t y) {
	/*
	 * sequence written in the spec:
	 *      identity();
	 *      rotate(angle);
	 *      scale(scaleX, scaleY);
	 *      translate(x, y);
	 */

	//Initialize using rotation
	cairo_matrix_init_rotate(&matrix,angle);

	matrix.scale(scaleX,scaleY);
	matrix.translate(x,y);
}

ASFUNCTIONBODY_ATOM(Matrix,createBox)
{
	assert_and_throw(argslen>=2 && argslen <= 5);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	number_t scaleX = asAtomHandler::toNumber(args[0]);
	number_t scaleY = asAtomHandler::toNumber(args[1]);

	number_t angle = 0;
	if ( argslen > 2 ) angle = asAtomHandler::toNumber(args[2]);

	number_t translateX = 0;
	if ( argslen > 3 ) translateX = asAtomHandler::toNumber(args[3]);

	number_t translateY = 0;
	if ( argslen > 4 ) translateY = asAtomHandler::toNumber(args[4]);

	th->_createBox(scaleX, scaleY, angle, translateX, translateY);
}

ASFUNCTIONBODY_ATOM(Matrix,createGradientBox)
{
	assert_and_throw(argslen>=2 && argslen <= 5);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	number_t width  = asAtomHandler::toNumber(args[0]);
	number_t height = asAtomHandler::toNumber(args[1]);

	number_t angle = 0;
	if ( argslen > 2 ) angle = asAtomHandler::toNumber(args[2]);

	number_t translateX = width/2.0;
	if ( argslen > 3 ) translateX += asAtomHandler::toNumber(args[3]);

	number_t translateY = height/2.0;
	if ( argslen > 4 ) translateY += asAtomHandler::toNumber(args[4]);

	th->_createBox(width / 1638.4, height / 1638.4, angle, translateX, translateY);
}

ASFUNCTIONBODY_ATOM(Matrix,transformPoint)
{
	assert_and_throw(argslen==1);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	Point* pt=asAtomHandler::as<Point>(args[0]);

	number_t ttx = pt->getX();
	number_t tty = pt->getY();
	cairo_matrix_transform_point(&th->matrix,&ttx,&tty);
	ret = asAtomHandler::fromObject(Class<Point>::getInstanceS(wrk,ttx, tty));
}

ASFUNCTIONBODY_ATOM(Matrix,deltaTransformPoint)
{
	assert_and_throw(argslen==1);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	Point* pt=asAtomHandler::as<Point>(args[0]);

	number_t ttx = pt->getX();
	number_t tty = pt->getY();
	cairo_matrix_transform_distance(&th->matrix,&ttx,&tty);
	ret = asAtomHandler::fromObject(Class<Point>::getInstanceS(wrk,ttx, tty));
}

ASFUNCTIONBODY_ATOM(Matrix,setTo)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	
	//Mapping to cairo_matrix_t
	//a -> xx
	//b -> yx
	//c -> xy
	//d -> yy
	//tx -> x0
	//ty -> y0
	
	ARG_CHECK(ARG_UNPACK(th->matrix.xx)(th->matrix.yx)(th->matrix.xy)(th->matrix.yy)(th->matrix.x0)(th->matrix.y0));
}
ASFUNCTIONBODY_ATOM(Matrix,copyFrom)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	
	_NR<Matrix> sourceMatrix;
	ARG_CHECK(ARG_UNPACK(sourceMatrix));
	th->matrix.xx = sourceMatrix->matrix.xx;
	th->matrix.yx = sourceMatrix->matrix.yx;
	th->matrix.xy = sourceMatrix->matrix.xy;
	th->matrix.yy = sourceMatrix->matrix.yy;
	th->matrix.x0 = sourceMatrix->matrix.x0;
	th->matrix.y0 = sourceMatrix->matrix.y0;
}



void PerspectiveProjection::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	REGISTER_GETTER_SETTER(c, fieldOfView);
	REGISTER_GETTER_SETTER(c, focalLength);
	REGISTER_GETTER_SETTER(c, projectionCenter);
	
}

bool PerspectiveProjection::destruct()
{
	fieldOfView = 0;
	focalLength= 0;
	projectionCenter.reset();
	return destructIntern();
}

void PerspectiveProjection::finalize()
{
	projectionCenter.reset();
}

void PerspectiveProjection::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	if (projectionCenter)
		projectionCenter->prepareShutdown();
	
}

ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(PerspectiveProjection, fieldOfView)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(PerspectiveProjection, focalLength)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(PerspectiveProjection, projectionCenter)

ASFUNCTIONBODY_ATOM(PerspectiveProjection,_constructor)
{
	PerspectiveProjection* th=asAtomHandler::as<PerspectiveProjection>(obj);
	th->projectionCenter = _MR(Class<Point>::getInstanceSNoArgs(wrk));
	LOG(LOG_NOT_IMPLEMENTED,"PerspectiveProjection is not implemented");
}
