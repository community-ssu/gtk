/*
    pygame - Python Game Library
    Copyright (C) 2000-2001  Pete Shinners

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Pete Shinners
    pete@shinners.org
*/

/*
 *  Python Rect Object -- useful 2d rectangle class
 */
#define PYGAMEAPI_RECT_INTERNAL
#include "pygame.h"
#include "structmember.h"


staticforward PyTypeObject PyRect_Type;
#define PyRect_Check(x) ((x)->ob_type == &PyRect_Type)

static PyObject* rect_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static int rect_init(PyRectObject *self, PyObject *args, PyObject *kwds);



GAME_Rect* GameRect_FromObject(PyObject* obj, GAME_Rect* temp)
{
	int val;
	int length;

	if(PyRect_Check(obj))
		return &((PyRectObject*)obj)->r;
	if(PySequence_Check(obj) && (length=PySequence_Length(obj))>0)
	{
		if(length == 4)
		{
			if(!IntFromObjIndex(obj, 0, &val)) return NULL; temp->x = val;
			if(!IntFromObjIndex(obj, 1, &val)) return NULL; temp->y = val;
			if(!IntFromObjIndex(obj, 2, &val)) return NULL; temp->w = val;
			if(!IntFromObjIndex(obj, 3, &val)) return NULL; temp->h = val;
			return temp;
		}
		if(length == 2)
		{
			PyObject* sub = PySequence_GetItem(obj, 0);
			if(!sub || !PySequence_Check(sub) || PySequence_Length(sub)!=2)
				{Py_XDECREF(sub); return NULL;}
			if(!IntFromObjIndex(sub, 0, &val)) {Py_DECREF(sub); return NULL;} temp->x = val;
			if(!IntFromObjIndex(sub, 1, &val)) {Py_DECREF(sub); return NULL;} temp->y = val;
			Py_DECREF(sub);
			sub = PySequence_GetItem(obj, 1);
			if(!sub || !PySequence_Check(sub) || PySequence_Length(sub)!=2)
				{Py_XDECREF(sub); return NULL;}
			if(!IntFromObjIndex(sub, 0, &val)) {Py_DECREF(sub); return NULL;} temp->w = val;
			if(!IntFromObjIndex(sub, 1, &val)) {Py_DECREF(sub); return NULL;} temp->h = val;
			Py_DECREF(sub);
			return temp;
		}
		if(PyTuple_Check(obj) && length == 1) /*looks like an arg?*/
		{
			PyObject* sub = PyTuple_GET_ITEM(obj, 0);
			if(sub)
				return GameRect_FromObject(sub, temp);
		}
	}
    if(PyObject_HasAttrString(obj, "rect"))
    {
		PyObject *rectattr;
        GAME_Rect *returnrect;
		rectattr = PyObject_GetAttrString(obj, "rect");
        if(PyCallable_Check(rectattr)) /*call if it's a method*/
        {
            PyObject *rectresult = PyObject_CallObject(rectattr, NULL);
            Py_DECREF(rectattr);
            if(!rectresult)
                return NULL;
            rectattr = rectresult;
        }
        returnrect = GameRect_FromObject(rectattr, temp);
        Py_DECREF(rectattr);
        return returnrect;
    }
	return NULL;
}

PyObject* PyRect_New(SDL_Rect* r)
{
	PyRectObject* rect;
	rect = (PyRectObject *)PyRect_Type.tp_new(&PyRect_Type, NULL, NULL);
        if(rect)
        {
            rect->r.x = r->x;
            rect->r.y = r->y;
            rect->r.w = r->w;
            rect->r.h = r->h;
        }
	return (PyObject*)rect;
}

PyObject* PyRect_New4(int x, int y, int w, int h)
{
	PyRectObject* rect;
	rect = (PyRectObject *)PyRect_Type.tp_new(&PyRect_Type, NULL, NULL);
        if(rect)
        {
            rect->r.x = x;
            rect->r.y = y;
            rect->r.w = w;
            rect->r.h = h;
        }
	return (PyObject*)rect;
}

static int DoRectsIntersect(GAME_Rect *A, GAME_Rect *B)
{
	return ((A->x >= B->x && A->x < B->x+B->w)  ||
		    (B->x >= A->x && B->x < A->x+A->w)) &&
		   ((A->y >= B->y && A->y < B->y+B->h)	||
		    (B->y >= A->y && B->y < A->y+A->h));
}



    /*DOC*/ static char doc_normalize[] =
    /*DOC*/    "Rect.normalize() -> None\n"
    /*DOC*/    "corrects negative sizes\n"
    /*DOC*/    "\n"
    /*DOC*/    "If the rectangle has a a negative size in width or\n"
    /*DOC*/    "height, this will flip that axis so the sizes are\n"
    /*DOC*/    "positive, and the rectangle remains in the same\n"
    /*DOC*/    "place.\n"
    /*DOC*/ ;

static PyObject* rect_normalize(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	if(!PyArg_ParseTuple(args, ""))
		return NULL;

	if(self->r.w < 0)
	{
		self->r.x += self->r.w;
		self->r.w = -self->r.w;
	}
	if(self->r.h < 0)
	{
		self->r.y += self->r.h;
		self->r.h = -self->r.h;
	}

	RETURN_NONE
}


    /*DOC*/ static char doc_move[] =
    /*DOC*/    "Rect.move(x, y) -> Rect\n"
    /*DOC*/    "new rectangle with position changed\n"
    /*DOC*/    "\n"
    /*DOC*/    "Returns a new rectangle which is the base\n"
    /*DOC*/    "rectangle moved by the given amount.\n"
    /*DOC*/ ;

static PyObject* rect_move(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	int x, y;

	if(!TwoIntsFromObj(args, &x, &y))
		return RAISE(PyExc_TypeError, "argument must contain two numbers");

	return PyRect_New4(self->r.x+x, self->r.y+y, self->r.w, self->r.h);
}

    /*DOC*/ static char doc_move_ip[] =
    /*DOC*/    "Rect.move_ip(x, y) -> None\n"
    /*DOC*/    "move the Rect by the given offset\n"
    /*DOC*/    "\n"
    /*DOC*/    "Moves the rectangle which by the given amount.\n"
    /*DOC*/ ;

static PyObject* rect_move_ip(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	int x, y;

	if(!TwoIntsFromObj(args, &x, &y))
		return RAISE(PyExc_TypeError, "argument must contain two numbers");

	self->r.x += x;
	self->r.y += y;
	RETURN_NONE
}



    /*DOC*/ static char doc_inflate[] =
    /*DOC*/    "Rect.inflate(x, y) -> Rect\n"
    /*DOC*/    "new rectangle with size changed\n"
    /*DOC*/    "\n"
    /*DOC*/    "Returns a new rectangle which has the sizes\n"
    /*DOC*/    "changed by the given amounts. The rectangle\n"
    /*DOC*/    "shrinks and expands around the rectangle's center.\n"
    /*DOC*/    "Negative values will shrink the rectangle.\n"
    /*DOC*/ ;

static PyObject* rect_inflate(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	int x, y;

	if(!TwoIntsFromObj(args, &x, &y))
		return RAISE(PyExc_TypeError, "argument must contain two numbers");

	return PyRect_New4(self->r.x-x/2, self->r.y-y/2, self->r.w+x, self->r.h+y);
}


    /*DOC*/ static char doc_inflate_ip[] =
    /*DOC*/    "Rect.inflate_ip(x, y) -> None\n"
    /*DOC*/    "changes the Rect size\n"
    /*DOC*/    "\n"
    /*DOC*/    "Changes the Rect by the given amounts. The rectangle\n"
    /*DOC*/    "shrinks and expands around the rectangle's center.\n"
    /*DOC*/    "Negative values will shrink the rectangle.\n"
    /*DOC*/ ;

static PyObject* rect_inflate_ip(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	int x, y;

	if(!TwoIntsFromObj(args, &x, &y))
		return RAISE(PyExc_TypeError, "argument must contain two numbers");

	self->r.x -= x/2;
	self->r.y -= y/2;
	self->r.w += x;
	self->r.h += y;
	RETURN_NONE
}



    /*DOC*/ static char doc_union[] =
    /*DOC*/    "Rect.union(rectstyle) -> Rect\n"
    /*DOC*/    "makes new rectangle covering both inputs\n"
    /*DOC*/    "\n"
    /*DOC*/    "Returns a new Rect to completely cover the\n"
    /*DOC*/    "given input. There may be area inside the new\n"
    /*DOC*/    "Rect that is not covered by either input.\n"
    /*DOC*/ ;

static PyObject* rect_union(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	GAME_Rect *argrect, temp;
	int x, y, w, h;
	if(!(argrect = GameRect_FromObject(args, &temp)))
		return RAISE(PyExc_TypeError, "Argument must be rect style object");

	x = min(self->r.x, argrect->x);
	y = min(self->r.y, argrect->y);
	w = max(self->r.x+self->r.w, argrect->x+argrect->w) - x;
	h = max(self->r.y+self->r.h, argrect->y+argrect->h) - y;
	return PyRect_New4(x, y, w, h);
}




    /*DOC*/ static char doc_union_ip[] =
    /*DOC*/    "Rect.union_ip(rectstyle) -> None\n"
    /*DOC*/    "rectangle covering both input\n"
    /*DOC*/    "\n"
    /*DOC*/    "Resizes the Rect to completely cover the\n"
    /*DOC*/    "given input. There may be area inside the new\n"
    /*DOC*/    "dimensions that is not covered by either input.\n"
    /*DOC*/ ;

static PyObject* rect_union_ip(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	GAME_Rect *argrect, temp;
	int x, y, w, h;
	if(!(argrect = GameRect_FromObject(args, &temp)))
		return RAISE(PyExc_TypeError, "Argument must be rect style object");

	x = min(self->r.x, argrect->x);
	y = min(self->r.y, argrect->y);
	w = max(self->r.x+self->r.w, argrect->x+argrect->w) - x;
	h = max(self->r.y+self->r.h, argrect->y+argrect->h) - y;
	self->r.x = x;
	self->r.y = y;
	self->r.w = w;
	self->r.h = h;
	RETURN_NONE
}


    /*DOC*/ static char doc_unionall[] =
    /*DOC*/    "Rect.unionall(sequence_of_rectstyles) -> Rect\n"
    /*DOC*/    "rectangle covering all inputs\n"
    /*DOC*/    "\n"
    /*DOC*/    "Returns a new rectangle that completely covers all the\n"
    /*DOC*/    "given inputs. There may be area inside the new\n"
    /*DOC*/    "rectangle that is not covered by the inputs.\n"
    /*DOC*/ ;

static PyObject* rect_unionall(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	GAME_Rect *argrect, temp;
	int loop, size;
	PyObject* list, *obj;
	int t, l, b, r;

	if(!PyArg_ParseTuple(args, "O", &list))
		return NULL;
	if(!PySequence_Check(list))
		return RAISE(PyExc_TypeError, "Argument must be a sequence of rectstyle objects.");

	l = self->r.x;
	t = self->r.y;
	r = self->r.x + self->r.w;
	b = self->r.y + self->r.h;
	size = PySequence_Length(list); /*warning, size could be -1 on error?*/
	if(size < 1)
		return PyRect_New4(l, t, r-l, b-t);

	for(loop = 0; loop < size; ++loop)
	{
		obj = PySequence_GetItem(list, loop);
		if(!obj || !(argrect = GameRect_FromObject(obj, &temp)))
		{
			RAISE(PyExc_TypeError, "Argument must be a sequence of rectstyle objects.");
			Py_XDECREF(obj);
			break;
		}
		l = min(l, argrect->x);
		t = min(t, argrect->y);
		r = max(r, argrect->x+argrect->w);
		b = max(b, argrect->y+argrect->h);
		Py_DECREF(obj);
	}
	return PyRect_New4(l, t, r-l, b-t);
}


    /*DOC*/ static char doc_unionall_ip[] =
    /*DOC*/    "Rect.unionall_ip(sequence_of_rectstyles) -> None\n"
    /*DOC*/    "rectangle covering all inputs\n"
    /*DOC*/    "\n"
    /*DOC*/    "Resizes the rectangle to completely cover all the\n"
    /*DOC*/    "given inputs. There may be area inside the new\n"
    /*DOC*/    "rectangle that is not covered by the inputs.\n"
    /*DOC*/ ;

static PyObject* rect_unionall_ip(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	GAME_Rect *argrect, temp;
	int loop, size;
	PyObject* list, *obj;
	int t, l, b, r;

	if(!PyArg_ParseTuple(args, "O", &list))
		return NULL;
	if(!PySequence_Check(list))
		return RAISE(PyExc_TypeError, "Argument must be a sequence of rectstyle objects.");

	l = self->r.x;
	t = self->r.y;
	r = self->r.x + self->r.w;
	b = self->r.y + self->r.h;

	size = PySequence_Length(list); /*warning, size could be -1 on error?*/
	if(size < 1)
		return PyRect_New4(l, t, r-l, b-t);

	for(loop = 0; loop < size; ++loop)
	{
		obj = PySequence_GetItem(list, loop);
		if(!obj || !(argrect = GameRect_FromObject(obj, &temp)))
		{
			RAISE(PyExc_TypeError, "Argument must be a sequence of rectstyle objects.");
			Py_XDECREF(obj);
			break;
		}
		l = min(l, argrect->x);
		t = min(t, argrect->y);
		r = max(r, argrect->x+argrect->w);
		b = max(b, argrect->y+argrect->h);
		Py_DECREF(obj);
	}

	self->r.x = l;
	self->r.y = t;
	self->r.w = r-l;
	self->r.h = b-t;
	RETURN_NONE
}


    /*DOC*/ static char doc_collidepoint[] =
    /*DOC*/    "Rect.collidepoint(x, y) -> bool\n"
    /*DOC*/    "point inside rectangle\n"
    /*DOC*/    "\n"
    /*DOC*/    "Returns true if the given point position is inside\n"
    /*DOC*/    "the rectangle. If a point is on the border, it is\n"
    /*DOC*/    "counted as inside.\n"
    /*DOC*/ ;

static PyObject* rect_collidepoint(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	int x, y;
	int inside;

	if(!TwoIntsFromObj(args, &x, &y))
		return RAISE(PyExc_TypeError, "argument must contain two numbers");

	inside = x>=self->r.x && x<self->r.x+self->r.w &&
				y>=self->r.y && y<self->r.y+self->r.h;

	return PyInt_FromLong(inside);
}



    /*DOC*/ static char doc_colliderect[] =
    /*DOC*/    "Rect.colliderect(rectstyle) -> bool\n"
    /*DOC*/    "check overlapping rectangles\n"
    /*DOC*/    "\n"
    /*DOC*/    "Returns true if any area of the two rectangles\n"
    /*DOC*/    "overlaps.\n"
    /*DOC*/ ;

static PyObject* rect_colliderect(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	GAME_Rect *argrect, temp;
	if(!(argrect = GameRect_FromObject(args, &temp)))
		return RAISE(PyExc_TypeError, "Argument must be rect style object");

	return PyInt_FromLong(DoRectsIntersect(&self->r, argrect));
}



    /*DOC*/ static char doc_collidelist[] =
    /*DOC*/    "Rect.collidelist(rectstyle list) -> int index\n"
    /*DOC*/    "find overlapping rectangle\n"
    /*DOC*/    "\n"
    /*DOC*/    "Returns the index of the first rectangle in the\n"
    /*DOC*/    "list to overlap the base rectangle. Once an\n"
    /*DOC*/    "overlap is found, this will stop checking the\n"
    /*DOC*/    "remaining list. If no overlap is found, it will\n"
    /*DOC*/    "return -1.\n"
    /*DOC*/ ;

static PyObject* rect_collidelist(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	GAME_Rect *argrect, temp;
	int loop, size;
	PyObject* list, *obj;
	PyObject* ret = NULL;

	if(!PyArg_ParseTuple(args, "O", &list))
		return NULL;

	if(!PySequence_Check(list))
		return RAISE(PyExc_TypeError, "Argument must be a sequence of rectstyle objects.");

	size = PySequence_Length(list); /*warning, size could be -1 on error?*/
	for(loop = 0; loop < size; ++loop)
	{
		obj = PySequence_GetItem(list, loop);
		if(!obj || !(argrect = GameRect_FromObject(obj, &temp)))
		{
			RAISE(PyExc_TypeError, "Argument must be a sequence of rectstyle objects.");
			Py_XDECREF(obj);
			break;
		}
		if(DoRectsIntersect(&self->r, argrect))
		{
			ret = PyInt_FromLong(loop);
			Py_DECREF(obj);
			break;
		}
		Py_DECREF(obj);
	}
	if(loop == size)
		ret = PyInt_FromLong(-1);

	return ret;
}



    /*DOC*/ static char doc_collidelistall[] =
    /*DOC*/    "Rect.collidelistall(rectstyle list) -> index list\n"
    /*DOC*/    "find all overlapping rectangles\n"
    /*DOC*/    "\n"
    /*DOC*/    "Returns a list of the indexes that contain\n"
    /*DOC*/    "rectangles overlapping the base rectangle. If no\n"
    /*DOC*/    "overlap is found, it will return an empty\n"
    /*DOC*/    "sequence.\n"
    /*DOC*/ ;

static PyObject* rect_collidelistall(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	GAME_Rect *argrect, temp;
	int loop, size;
	PyObject* list, *obj;
	PyObject* ret = NULL;

	if(!PyArg_ParseTuple(args, "O", &list))
		return NULL;

	if(!PySequence_Check(list))
		return RAISE(PyExc_TypeError, "Argument must be a sequence of rectstyle objects.");

	ret = PyList_New(0);
	if(!ret)
		return NULL;

	size = PySequence_Length(list); /*warning, size could be -1?*/
	for(loop = 0; loop < size; ++loop)
	{
		obj = PySequence_GetItem(list, loop);

		if(!obj || !(argrect = GameRect_FromObject(obj, &temp)))
		{
			Py_XDECREF(obj);
			Py_DECREF(ret);
			return RAISE(PyExc_TypeError, "Argument must be a sequence of rectstyle objects.");
		}

		if(DoRectsIntersect(&self->r, argrect))
		{
			PyObject* num = PyInt_FromLong(loop);
			if(!num)
			{
				Py_DECREF(obj);
				return NULL;
			}
			PyList_Append(ret, num);
			Py_DECREF(num);
		}
		Py_DECREF(obj);
	}

	return ret;
}


    /*DOC*/ static char doc_collidedict[] =
    /*DOC*/    "Rect.collidedict(dict if rectstyle keys) -> key/value pair\n"
    /*DOC*/    "find overlapping rectangle in a dictionary\n"
    /*DOC*/    "\n"
    /*DOC*/    "Returns the key/value pair of the first rectangle key\n"
    /*DOC*/    "in the dict that overlaps the base rectangle. Once an\n"
    /*DOC*/    "overlap is found, this will stop checking the\n"
    /*DOC*/    "remaining list. If no overlap is found, it will\n"
    /*DOC*/    "return None.\n"
    /*DOC*/    "\n"
    /*DOC*/    "Remember python dictionary keys must be immutable,\n"
    /*DOC*/    "Rects are not immutable, so they cannot directly be,\n"
    /*DOC*/    "dictionary keys. You can convert the Rect to a tuple\n"
    /*DOC*/    "with the tuple() builtin command.\n"
    /*DOC*/ ;

static PyObject* rect_collidedict(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	GAME_Rect *argrect, temp;
	int loop=0;
	PyObject* dict, *key, *val;
	PyObject* ret = NULL;

	if(!PyArg_ParseTuple(args, "O", &dict))
		return NULL;
	if(!PyDict_Check(dict))
		return RAISE(PyExc_TypeError, "Argument must be a dict with rectstyle keys.");

        while(PyDict_Next(dict, &loop, &key, &val))
	{
		if(!(argrect = GameRect_FromObject(key, &temp)))
		{
			RAISE(PyExc_TypeError, "Argument must be a dict with rectstyle keys.");
			break;
		}
		if(DoRectsIntersect(&self->r, argrect))
		{
			ret = Py_BuildValue("(OO)", key, val);
			break;
		}
	}

        if(!ret)
        {
            Py_INCREF(Py_None);
            ret = Py_None;
        }
	return ret;
}


    /*DOC*/ static char doc_collidedictall[] =
    /*DOC*/    "Rect.collidedictall(rectstyle list) -> key/val list\n"
    /*DOC*/    "find all overlapping rectangles\n"
    /*DOC*/    "\n"
    /*DOC*/    "Returns a list of the indexes that contain\n"
    /*DOC*/    "rectangles overlapping the base rectangle. If no\n"
    /*DOC*/    "overlap is found, it will return an empty\n"
    /*DOC*/    "sequence.\n"
    /*DOC*/    "\n"
    /*DOC*/    "Remember python dictionary keys must be immutable,\n"
    /*DOC*/    "Rects are not immutable, so they cannot directly be,\n"
    /*DOC*/    "dictionary keys. You can convert the Rect to a tuple\n"
    /*DOC*/    "with the tuple() builtin command.\n"
    /*DOC*/ ;

static PyObject* rect_collidedictall(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	GAME_Rect *argrect, temp;
	int loop=0;
	PyObject* dict, *key, *val;
	PyObject* ret = NULL;

	if(!PyArg_ParseTuple(args, "O", &dict))
		return NULL;
	if(!PyDict_Check(dict))
		return RAISE(PyExc_TypeError, "Argument must be a dict with rectstyle keys.");

	ret = PyList_New(0);
	if(!ret)
		return NULL;

        while(PyDict_Next(dict, &loop, &key, &val))
	{
		if(!(argrect = GameRect_FromObject(key, &temp)))
		{
			Py_DECREF(ret);
			return RAISE(PyExc_TypeError, "Argument must be a dict with rectstyle keys.");
		}

		if(DoRectsIntersect(&self->r, argrect))
		{
			PyObject* num = Py_BuildValue("(OO)", key, val);
			if(!num)
				return NULL;
			PyList_Append(ret, num);
			Py_DECREF(num);
		}
	}

	return ret;
}



    /*DOC*/ static char doc_clip[] =
    /*DOC*/    "Rect.clip(rectstyle) -> Rect\n"
    /*DOC*/    "rectangle cropped inside another\n"
    /*DOC*/    "\n"
    /*DOC*/    "Returns a new rectangle that is the given\n"
    /*DOC*/    "rectangle cropped to the inside of the base\n"
    /*DOC*/    "rectangle. If the two rectangles do not overlap to\n"
    /*DOC*/    "begin with, you will get a rectangle with 0 size.\n"
    /*DOC*/ ;

static PyObject* rect_clip(PyObject* self, PyObject* args)
{
	GAME_Rect *A, *B, temp;
	int x, y, w, h;

	A = &((PyRectObject*)self)->r;
	if(!(B = GameRect_FromObject(args, &temp)))
		return RAISE(PyExc_TypeError, "Argument must be rect style object");

	/* Left */
	if((A->x >= B->x) && (A->x < (B->x+B->w)))
		x = A->x;
	else if((B->x >= A->x) && (B->x < (A->x+A->w)))
		x = B->x;
	else
		goto nointersect;
	/* Right */
	if(((A->x+A->w) > B->x) && ((A->x+A->w) <= (B->x+B->w)))
		w = (A->x+A->w) - x;
	else if(((B->x+B->w) > A->x) && ((B->x+B->w) <= (A->x+A->w)))
		w = (B->x+B->w) - x;
	else
		goto nointersect;

	/* Top */
	if((A->y >= B->y) && (A->y < (B->y+B->h)))
		y = A->y;
	else if((B->y >= A->y) && (B->y < (A->y+A->h)))
		y = B->y;
	else
		goto nointersect;
	/* Bottom */
	if (((A->y+A->h) > B->y) && ((A->y+A->h) <= (B->y+B->h)))
		h = (A->y+A->h) - y;
	else if(((B->y+B->h) > A->y) && ((B->y+B->h) <= (A->y+A->h)))
		h = (B->y+B->h) - y;
	else
		goto nointersect;

	return PyRect_New4(x, y, w, h);

nointersect:
	return PyRect_New4(A->x, A->y, 0, 0);
}


    /*DOC*/ static char doc_contains[] =
    /*DOC*/    "Rect.contains(rectstyle) -> bool\n"
    /*DOC*/    "check if rectangle fully inside another\n"
    /*DOC*/    "\n"
    /*DOC*/    "Returns true when the given rectangle is entirely\n"
    /*DOC*/    "inside the base rectangle.\n"
    /*DOC*/ ;

static PyObject* rect_contains(PyObject* oself, PyObject* args)
{
	int contained;
	PyRectObject* self = (PyRectObject*)oself;
	GAME_Rect *argrect, temp;
	if(!(argrect = GameRect_FromObject(args, &temp)))
		return RAISE(PyExc_TypeError, "Argument must be rect style object");

	contained = (self->r.x <= argrect->x) && (self->r.y <= argrect->y) &&

	            (self->r.x + self->r.w >= argrect->x + argrect->w) &&

	            (self->r.y + self->r.h >= argrect->y + argrect->h) &&

	            (self->r.x + self->r.w > argrect->x) &&

	            (self->r.y + self->r.h > argrect->y);


	return PyInt_FromLong(contained);
}


    /*DOC*/ static char doc_clamp[] =
    /*DOC*/    "Rect.clamp(rectstyle) -> Rect\n"
    /*DOC*/    "move rectangle inside another\n"
    /*DOC*/    "\n"
    /*DOC*/    "Returns a new rectangle that is moved to be\n"
    /*DOC*/    "completely inside the argument rectangle. If the base\n"
    /*DOC*/    "rectangle is too large for the argument rectangle in\n"
    /*DOC*/    "an axis, it will be centered on that axis.\n"
    /*DOC*/ ;

static PyObject* rect_clamp(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	GAME_Rect *argrect, temp;
	int x, y;
	if(!(argrect = GameRect_FromObject(args, &temp)))
		return RAISE(PyExc_TypeError, "Argument must be rect style object");

	if(self->r.w >= argrect->w)
		x = argrect->x + argrect->w / 2 - self->r.w / 2;
	else if(self->r.x < argrect->x)
		x = argrect->x;
	else if(self->r.x + self->r.w > argrect->x + argrect->w)
		x = argrect->x + argrect->w - self->r.w;
	else
		x = self->r.x;

	if(self->r.h >= argrect->h)
		y = argrect->y + argrect->h / 2 - self->r.h / 2;
	else if(self->r.y < argrect->y)
		y = argrect->y;
	else if(self->r.y + self->r.h > argrect->y + argrect->h)
		y = argrect->y + argrect->h - self->r.h;
	else
		y = self->r.y;

	return PyRect_New4(x, y, self->r.w, self->r.h);
}



    /*DOC*/ static char doc_fit[] =
    /*DOC*/    "Rect.fit(rectstyle) -> Rect\n"
    /*DOC*/    "Fill as much of the argument as possible, maintains aspect ratio\n"
    /*DOC*/    "\n"
    /*DOC*/    "Returns a new rectangle that fills as much of the rectangle\n"
    /*DOC*/    "argumement as possible. The new rectangle maintains the same\n"
    /*DOC*/    "aspect ratio as the original rectangle. This can be used to\n"
    /*DOC*/    "find scaling sizes that fill an area but won't distort the image.\n"
    /*DOC*/ ;

static PyObject* rect_fit(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	GAME_Rect *argrect, temp;
	int w, h, x, y;
	float xratio, yratio, maxratio;
	if(!(argrect = GameRect_FromObject(args, &temp)))
		return RAISE(PyExc_TypeError, "Argument must be rect style object");

	xratio = (float)self->r.w / (float)argrect->w;
	yratio = (float)self->r.h / (float)argrect->h;
	maxratio = (xratio>yratio)?xratio:yratio;
	
	{
		w = (int)(self->r.w / maxratio);
		h = (int)(self->r.h / maxratio);
	}

	
	x = argrect->x + (argrect->w - w)/2;
	y = argrect->y + (argrect->h - h)/2;

	return PyRect_New4(x, y, w, h);
}


    /*DOC*/ static char doc_clamp_ip[] =
    /*DOC*/    "Rect.clamp_ip(rectstyle) -> None\n"
    /*DOC*/    "moves the rectangle inside another\n"
    /*DOC*/    "\n"
    /*DOC*/    "Moves the Rect to be\n"
    /*DOC*/    "completely inside the argument rectangle. If the given\n"
    /*DOC*/    "rectangle is too large for the argument rectangle in\n"
    /*DOC*/    "an axis, it will be centered on that axis.\n"
    /*DOC*/ ;

static PyObject* rect_clamp_ip(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
	GAME_Rect *argrect, temp;
	int x, y;
	if(!(argrect = GameRect_FromObject(args, &temp)))
		return RAISE(PyExc_TypeError, "Argument must be rect style object");

	if(self->r.w >= argrect->w)
		x = argrect->x + argrect->w / 2 - self->r.w / 2;
	else if(self->r.x < argrect->x)
		x = argrect->x;
	else if(self->r.x + self->r.w > argrect->x + argrect->w)
		x = argrect->x + argrect->w - self->r.w;
	else
		x = self->r.x;

	if(self->r.h >= argrect->h)
		y = argrect->y + argrect->h / 2 - self->r.h / 2;
	else if(self->r.y < argrect->y)
		y = argrect->y;
	else if(self->r.y + self->r.h > argrect->y + argrect->h)
		y = argrect->y + argrect->h - self->r.h;
	else
		y = self->r.y;

	self->r.x = x;
	self->r.y = y;
	RETURN_NONE
}


/* for pickling */
static PyObject* rect_reduce(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
        return Py_BuildValue("(O(iiii))", oself->ob_type,
                    (int)self->r.x, (int)self->r.y, (int)self->r.w, (int)self->r.h);
}

/* for copy module */
static PyObject* rect_copy(PyObject* oself, PyObject* args)
{
	PyRectObject* self = (PyRectObject*)oself;
        return PyRect_New4(self->r.x, self->r.y, self->r.w, self->r.h);
}




static struct PyMethodDef rect_methods[] =
{
	{"normalize",		(PyCFunction)rect_normalize,	1, doc_normalize},
	{"clip",			(PyCFunction)rect_clip, 		1, doc_clip},
	{"clamp",			(PyCFunction)rect_clamp,		1, doc_clamp},
	{"clamp_ip",			 (PyCFunction)rect_clamp_ip,		  1,	    doc_clamp_ip},
	{"fit",			(PyCFunction)rect_fit,		1, doc_fit},

	{"move",			(PyCFunction)rect_move, 		1, doc_move},
	{"inflate",			(PyCFunction)rect_inflate,		1, doc_inflate},
	{"union",			(PyCFunction)rect_union,		1, doc_union},
	{"unionall",		(PyCFunction)rect_unionall,		1, doc_unionall},

	{"move_ip",			(PyCFunction)rect_move_ip,		1, doc_move_ip},
	{"inflate_ip",		(PyCFunction)rect_inflate_ip,	1, doc_inflate_ip},
	{"union_ip",		(PyCFunction)rect_union_ip,		1, doc_union_ip},
	{"unionall_ip", 	(PyCFunction)rect_unionall_ip,	1, doc_unionall_ip},

	{"collidepoint",	(PyCFunction)rect_collidepoint, 1, doc_collidepoint},
	{"colliderect", 	(PyCFunction)rect_colliderect,	1, doc_colliderect},
	{"collidelist", 	(PyCFunction)rect_collidelist,	1, doc_collidelist},
	{"collidelistall",	(PyCFunction)rect_collidelistall,1,doc_collidelistall},
	{"collidedict", 	(PyCFunction)rect_collidedict,	1, doc_collidedict},
	{"collidedictall",	(PyCFunction)rect_collidedictall,1,doc_collidedictall},
	{"contains",		(PyCFunction)rect_contains,		1, doc_contains},

        {"__reduce__",          (PyCFunction)rect_reduce, 0, NULL},
        {"__copy__",            (PyCFunction)rect_copy, 0, NULL},

	{NULL,		NULL}
};




/* sequence functions */

static int rect_length(PyRectObject *self)
{
	return 4;
}

static PyObject* rect_item(PyRectObject *self, int i)
{
	int* data = (int*)&self->r;
	if(i<0 || i>3)
		return RAISE(PyExc_IndexError, "Invalid rect Index");

	return PyInt_FromLong(data[i]);
}

static int rect_ass_item(PyRectObject *self, int i, PyObject *v)
{
	int val;
	int* data = (int*)&self->r;
	if(i<0 || i>3)
	{
		RAISE(PyExc_IndexError, "Invalid rect Index");
		return -1;
	}
	if(!IntFromObj(v, &val))
	{
		RAISE(PyExc_TypeError, "Must assign numeric values");
		return -1;
	}
	data[i] = val;
	return 0;
}


static PyObject* rect_slice(PyRectObject *self, int ilow, int ihigh)
{
	PyObject *list;
	int* data = (int*)&self->r;
	int numitems, loop, l = 4;

	if (ihigh < 0) ihigh += l;
	if (ilow  < 0) ilow  += l;
	if (ilow < 0) ilow = 0;
	else if (ilow > l) ilow = l;
	if (ihigh < 0) ihigh = 0;
	else if (ihigh > l) ihigh = l;
	if (ihigh < ilow) ihigh = ilow;

	numitems = ihigh - ilow;
	list = PyList_New(numitems);
	for(loop = 0; loop < numitems; ++loop)
		PyList_SET_ITEM(list, loop, PyInt_FromLong(data[loop+ilow]));

	return list;
}



static int rect_ass_slice(PyRectObject *self, int ilow, int ihigh, PyObject *v)
{
	int* data = (int*)&self->r;
	int numitems, loop, l = 4;
	int val;

	if(!PySequence_Check(v))
	{
		RAISE(PyExc_TypeError, "Assigned slice must be a sequence");
		return -1;
	}

	if (ihigh < 0) ihigh += l;
	if (ilow  < 0) ilow  += l;
	if (ilow < 0) ilow = 0;
	else if (ilow > l) ilow = l;
	if (ihigh < 0) ihigh = 0;
	else if (ihigh > l) ihigh = l;
	if (ihigh < ilow) ihigh = ilow;

	numitems = ihigh - ilow;
	if(numitems != PySequence_Length(v))
	{
		RAISE(PyExc_ValueError, "Assigned slice must be same length");
		return -1;
	}

	for(loop = 0; loop < numitems; ++loop)
	{
		if(!IntFromObjIndex(v, loop, &val)) return -1;
		data[loop+ilow] = val;
	}

	return 0;
}

static PySequenceMethods rect_as_sequence = {
	(inquiry)rect_length,				/*length*/
	(binaryfunc)NULL,					/*concat*/
	(intargfunc)NULL,					/*repeat*/
	(intargfunc)rect_item,				/*item*/
	(intintargfunc)rect_slice,			/*slice*/
	(intobjargproc)rect_ass_item,		/*ass_item*/
	(intintobjargproc)rect_ass_slice,	/*ass_slice*/
};



/* numeric functions */

static int rect_nonzero(PyRectObject *self)
{
	return self->r.w != 0 && self->r.h != 0;
}

static int rect_coerce(PyObject** o1, PyObject** o2)
{
	PyObject* new1;
	PyObject* new2;
	GAME_Rect* r, temp;

	if(PyRect_Check(*o1))
	{
		new1 = *o1;
		Py_INCREF(new1);
	}
	else if((r = GameRect_FromObject(*o1, &temp)))
		new1 = PyRect_New4(r->x, r->y, r->w, r->h);
	else
		return 1;

	if(PyRect_Check(*o2))
	{
		new2 = *o2;
		Py_INCREF(new2);
	}
	else if((r = GameRect_FromObject(*o2, &temp)))
		new2 = PyRect_New4(r->x, r->y, r->w, r->h);
	else
	{
		Py_DECREF(new1);
		return 1;
	}

	*o1 = new1;
	*o2 = new2;
	return 0;
}

static PyNumberMethods rect_as_number = {
	(binaryfunc)NULL,		/*add*/
	(binaryfunc)NULL,		/*subtract*/
	(binaryfunc)NULL,		/*multiply*/
	(binaryfunc)NULL,		/*divide*/
	(binaryfunc)NULL,		/*remainder*/
	(binaryfunc)NULL,		/*divmod*/
	(ternaryfunc)NULL,		/*power*/
	(unaryfunc)NULL,		/*negative*/
	(unaryfunc)NULL,		/*pos*/
	(unaryfunc)NULL,		/*abs*/
	(inquiry)rect_nonzero,	/*nonzero*/
	(unaryfunc)NULL,		/*invert*/
	(binaryfunc)NULL,		/*lshift*/
	(binaryfunc)NULL,		/*rshift*/
	(binaryfunc)NULL,		/*and*/
	(binaryfunc)NULL,		/*xor*/
	(binaryfunc)NULL,		/*or*/
	(coercion)rect_coerce,	/*coerce*/
	(unaryfunc)NULL,		/*int*/
	(unaryfunc)NULL,		/*long*/
	(unaryfunc)NULL,		/*float*/
	(unaryfunc)NULL,		/*oct*/
	(unaryfunc)NULL,		/*hex*/
};


/* object type functions */
static void rect_dealloc(PyRectObject *self)
{
        if(self->weakreflist)
            PyObject_ClearWeakRefs((PyObject*)self);
	self->ob_type->tp_free((PyObject*)self);
}


static PyObject *rect_repr(PyRectObject *self)
{
	char string[256];
	sprintf(string, "<rect(%d, %d, %d, %d)>", self->r.x, self->r.y, self->r.w, self->r.h);
	return PyString_FromString(string);
}


static PyObject *rect_str(PyRectObject *self)
{
	return rect_repr(self);
}


static int rect_compare(PyRectObject *self, PyObject *other)
{
	GAME_Rect *orect, temp;

	orect = GameRect_FromObject(other, &temp);
	if(!orect)
	{
		RAISE(PyExc_TypeError, "must compare rect with rect style object");
		return -1;
	}

	if(self->r.x != orect->x)
		return self->r.x < orect->x ? -1 : 1;
	if(self->r.y != orect->y)
		return self->r.y < orect->y ? -1 : 1;
	if(self->r.w != orect->w)
		return self->r.w < orect->w ? -1 : 1;
	if(self->r.h != orect->h)
		return self->r.h < orect->h ? -1 : 1;

	return 0;
}

/*width*/
static PyObject* rect_getwidth(PyRectObject *self, void *closure) {
    return PyInt_FromLong(self->r.w);
}
static int rect_setwidth(PyRectObject *self, PyObject* value, void *closure) {
    int val1;
    if(!IntFromObj(value, &val1)) return -1;
    self->r.w = val1;
    return 0;
}

/*height*/
static PyObject* rect_getheight(PyRectObject *self, void *closure) {
    return PyInt_FromLong(self->r.h);
}
static int rect_setheight(PyRectObject *self, PyObject* value, void *closure) {
    int val1;
    if(!IntFromObj(value, &val1)) {RAISE(PyExc_TypeError, "invalid rect assignment");return -1;}
    self->r.h = val1;
    return 0;
}

/*top*/
static PyObject* rect_gettop(PyRectObject *self, void *closure) {
    return PyInt_FromLong(self->r.y);
}
static int rect_settop(PyRectObject *self, PyObject* value, void *closure) {
    int val1;
    if(!IntFromObj(value, &val1)) {RAISE(PyExc_TypeError, "invalid rect assignment");return -1;}
    self->r.y = val1;
    return 0;
}

/*left*/
static PyObject* rect_getleft(PyRectObject *self, void *closure) {
    return PyInt_FromLong(self->r.x);
}
static int rect_setleft(PyRectObject *self, PyObject* value, void *closure) {
    int val1;
    if(!IntFromObj(value, &val1)) {RAISE(PyExc_TypeError, "invalid rect assignment");return -1;};
    self->r.x = val1;
    return 0;
}

/*right*/
static PyObject* rect_getright(PyRectObject *self, void *closure) {
    return PyInt_FromLong(self->r.x + self->r.w);
}
static int rect_setright(PyRectObject *self, PyObject* value, void *closure) {
    int val1;
    if(!IntFromObj(value, &val1)) {RAISE(PyExc_TypeError, "invalid rect assignment");return -1;}
    self->r.x = val1-self->r.w;
    return 0;
}

/*bottom*/
static PyObject* rect_getbottom(PyRectObject *self, void *closure) {
    return PyInt_FromLong(self->r.y + self->r.h);
}
static int rect_setbottom(PyRectObject *self, PyObject* value, void *closure) {
    int val1;
    if(!IntFromObj(value, &val1)) {RAISE(PyExc_TypeError, "invalid rect assignment");return -1;}
    self->r.y = val1-self->r.h;
    return 0;
}

/*centerx*/
static PyObject* rect_getcenterx(PyRectObject *self, void *closure) {
    return PyInt_FromLong(self->r.x + (self->r.w>>1));
}
static int rect_setcenterx(PyRectObject *self, PyObject* value, void *closure) {
    int val1;
    if(!IntFromObj(value, &val1)) {RAISE(PyExc_TypeError, "invalid rect assignment");return -1;}
    self->r.x = val1-(self->r.w>>1);
    return 0;
}

/*centery*/
static PyObject* rect_getcentery(PyRectObject *self, void *closure) {
    return PyInt_FromLong(self->r.y + (self->r.h>>1));
}
static int rect_setcentery(PyRectObject *self, PyObject* value, void *closure) {
    int val1;
    if(!IntFromObj(value, &val1)) {RAISE(PyExc_TypeError, "invalid rect assignment");return -1;}
    self->r.y = val1-(self->r.h>>1);
    return 0;
}

/*topleft*/
static PyObject* rect_gettopleft(PyRectObject *self, void *closure) {
    return Py_BuildValue("(ii)", self->r.x, self->r.y);
}
static int rect_settopleft(PyRectObject *self, PyObject* value, void *closure) {
    int val1, val2;
    if(!TwoIntsFromObj(value, &val1, &val2)) {RAISE(PyExc_TypeError, "invalid rect assignment");return -1;}
    self->r.x = val1; self->r.y = val2;
    return 0;
}

/*topright*/
static PyObject* rect_gettopright(PyRectObject *self, void *closure) {
    return Py_BuildValue("(ii)", self->r.x+self->r.w, self->r.y);
}
static int rect_settopright(PyRectObject *self, PyObject* value, void *closure) {
    int val1, val2;
    if(!TwoIntsFromObj(value, &val1, &val2)) {RAISE(PyExc_TypeError, "invalid rect assignment");return -1;}
    self->r.x = val1-self->r.w;; self->r.y = val2;
    return 0;
}

/*bottomleft*/
static PyObject* rect_getbottomleft(PyRectObject *self, void *closure) {
    return Py_BuildValue("(ii)", self->r.x, self->r.y+self->r.h);
}
static int rect_setbottomleft(PyRectObject *self, PyObject* value, void *closure) {
    int val1, val2;
    if(!TwoIntsFromObj(value, &val1, &val2)) {RAISE(PyExc_TypeError, "invalid rect assignment");return -1;}
    self->r.x = val1; self->r.y = val2-self->r.h;
    return 0;
}

/*bottomright*/
static PyObject* rect_getbottomright(PyRectObject *self, void *closure) {
    return Py_BuildValue("(ii)", self->r.x+self->r.w, self->r.y+self->r.h);
}
static int rect_setbottomright(PyRectObject *self, PyObject* value, void *closure) {
    int val1, val2;
    if(!TwoIntsFromObj(value, &val1, &val2)) {RAISE(PyExc_TypeError, "invalid rect assignment");return -1;}
    self->r.x = val1-self->r.w; self->r.y = val2-self->r.h;
    return 0;
}

/*midtop*/
static PyObject* rect_getmidtop(PyRectObject *self, void *closure) {
    return Py_BuildValue("(ii)", self->r.x+(self->r.w>>1), self->r.y);
}
static int rect_setmidtop(PyRectObject *self, PyObject* value, void *closure) {
    int val1, val2;
    if(!TwoIntsFromObj(value, &val1, &val2)) {RAISE(PyExc_TypeError, "invalid rect assignment");return -1;}
    self->r.x += val1-(self->r.x+(self->r.w>>1)); self->r.y = val2;
    return 0;
}

/*midleft*/
static PyObject* rect_getmidleft(PyRectObject *self, void *closure) {
    return Py_BuildValue("(ii)", self->r.x, self->r.y+(self->r.h>>1));
}
static int rect_setmidleft(PyRectObject *self, PyObject* value, void *closure) {
    int val1, val2;
    if(!TwoIntsFromObj(value, &val1, &val2)) {RAISE(PyExc_TypeError, "invalid rect assignment");return -1;}
    self->r.x = val1; self->r.y += val2-(self->r.y+(self->r.h>>1));
    return 0;
}

/*midbottom*/
static PyObject* rect_getmidbottom(PyRectObject *self, void *closure) {
    return Py_BuildValue("(ii)", self->r.x+(self->r.w>>1), self->r.y+self->r.h);
}
static int rect_setmidbottom(PyRectObject *self, PyObject* value, void *closure) {
    int val1, val2;
    if(!TwoIntsFromObj(value, &val1, &val2)) {RAISE(PyExc_TypeError, "invalid rect assignment");return -1;}
    self->r.x += val1-(self->r.x+(self->r.w>>1)); self->r.y = val2-self->r.h;
    return 0;
}

/*midright*/
static PyObject* rect_getmidright(PyRectObject *self, void *closure) {
    return Py_BuildValue("(ii)", self->r.x+self->r.w, self->r.y+(self->r.h>>1));
}
static int rect_setmidright(PyRectObject *self, PyObject* value, void *closure) {
    int val1, val2;
    if(!TwoIntsFromObj(value, &val1, &val2)) {RAISE(PyExc_TypeError, "invalid rect assignment");return -1;}
    self->r.x = val1-self->r.w; self->r.y += val2-(self->r.y+(self->r.h>>1));
    return 0;
}

/*center*/
static PyObject* rect_getcenter(PyRectObject *self, void *closure) {
    return Py_BuildValue("(ii)", self->r.x+(self->r.w>>1), self->r.y+(self->r.h>>1));
}
static int rect_setcenter(PyRectObject *self, PyObject* value, void *closure) {
    int val1, val2;
    if(!TwoIntsFromObj(value, &val1, &val2)) {RAISE(PyExc_TypeError, "invalid rect assignment");return -1;}
    self->r.x += val1-(self->r.x+(self->r.w>>1)); self->r.y += val2-(self->r.y+(self->r.h>>1));
    return 0;
}

/*size*/
static PyObject* rect_getsize(PyRectObject *self, void *closure) {
    return Py_BuildValue("(ii)", self->r.w, self->r.h);
}
static int rect_setsize(PyRectObject *self, PyObject* value, void *closure) {
    int val1, val2;
    if(!TwoIntsFromObj(value, &val1, &val2)) {RAISE(PyExc_TypeError, "invalid rect assignment");return -1;}
    self->r.w = val1; self->r.h = val2;
    return 0;
}

static PyObject* rect_getsafepickle(PyRectObject *self, void *closure) {
    return PyInt_FromLong(1); /* TODO, should return True*/
}

static PyGetSetDef rect_getsets[] = {
    {"x", (getter)rect_getleft, (setter)rect_setleft, NULL, NULL},
    {"y", (getter)rect_gettop, (setter)rect_settop, NULL, NULL},
    {"w", (getter)rect_getwidth, (setter)rect_setwidth, NULL, NULL},
    {"h", (getter)rect_getheight, (setter)rect_setheight, NULL, NULL},
    {"width", (getter)rect_getwidth, (setter)rect_setwidth, NULL, NULL},
    {"height", (getter)rect_getheight, (setter)rect_setheight, NULL, NULL},
    {"top", (getter)rect_gettop, (setter)rect_settop, NULL, NULL},
    {"left", (getter)rect_getleft, (setter)rect_setleft, NULL, NULL},
    {"bottom", (getter)rect_getbottom, (setter)rect_setbottom, NULL, NULL},
    {"right", (getter)rect_getright, (setter)rect_setright, NULL, NULL},
    {"centerx", (getter)rect_getcenterx, (setter)rect_setcenterx, NULL, NULL},
    {"centery", (getter)rect_getcentery, (setter)rect_setcentery, NULL, NULL},
    {"topleft", (getter)rect_gettopleft, (setter)rect_settopleft, NULL, NULL},
    {"topright", (getter)rect_gettopright, (setter)rect_settopright, NULL, NULL},
    {"bottomleft", (getter)rect_getbottomleft, (setter)rect_setbottomleft, NULL, NULL},
    {"bottomright", (getter)rect_getbottomright, (setter)rect_setbottomright, NULL, NULL},
    {"midtop", (getter)rect_getmidtop, (setter)rect_setmidtop, NULL, NULL},
    {"midleft", (getter)rect_getmidleft, (setter)rect_setmidleft, NULL, NULL},
    {"midbottom", (getter)rect_getmidbottom, (setter)rect_setmidbottom, NULL, NULL},
    {"midright", (getter)rect_getmidright, (setter)rect_setmidright, NULL, NULL},
    {"size", (getter)rect_getsize, (setter)rect_setsize, NULL, NULL},
    {"center", (getter)rect_getcenter, (setter)rect_setcenter, NULL, NULL},

    {"__safe_for_unpickling__", (getter)rect_getsafepickle, NULL, NULL, NULL},
    {NULL}  /* Sentinel */
};



    /*DOC*/ static char doc_Rect_MODULE[] =
    /*DOC*/    "The rectangle object is a useful object\n"
    /*DOC*/    "representing a rectangle area. Rectangles are\n"
    /*DOC*/    "created from the pygame.Rect() function. This routine\n"
    /*DOC*/    "is also in the locals module, so importing the locals\n"
    /*DOC*/    "into your namespace allows you to just use Rect().\n"
    /*DOC*/    "\n"
    /*DOC*/    "Rect contains helpful methods, as well as a list of\n"
    /*DOC*/    "modifiable members:\n"
#if 1
  /*NODOC*/    "top, bottom, left, right, topleft, topright,\n"
  /*NODOC*/    "bottomleft, bottomright, size, width, height,\n"
  /*NODOC*/    "center, centerx, centery, midleft, midright, midtop,\n"
  /*NODOC*/    "midbottom.\n"
#else
    /*DOC*/    "<table border="0" cellspacing=0 cellpadding=0 width=66%><tr valign=top><td align=left><ul><li>top<li>bottom<li>left<li>right</ul></td>\n"
    /*DOC*/    "<td align=left><ul><li>topleft<li>topright<li>bottomleft<li>bottomright</ul></td>\n"
    /*DOC*/    "<td align=left><ul><li>midleft<li>midright<li>midtop<li>midbottom</ul></td>\n"
    /*DOC*/    "<td align=left><ul><li>center<li>centerx<li>centery</ul></td>\n"
    /*DOC*/    "<td align=left><ul><li>size<li>width<li>height</ul></td>\n"
    /*DOC*/    "</tr></table><br>\n"
#endif
    /*DOC*/    "When changing these members, the rectangle\n"
    /*DOC*/    "will be moved to the given assignment. (except when\n"
    /*DOC*/    "changing the size, width, or height member, which will\n"
    /*DOC*/    "resize the rectangle from the topleft corner)\n"
    /*DOC*/    "\n"
    /*DOC*/    "The rectstyle arguments used frequently with the\n"
    /*DOC*/    "Rect object (and elsewhere in pygame) is one of\n"
    /*DOC*/    "the following things.\n"
#if 1
  /*NODOC*/    "First, an actual Rect\n"
  /*NODOC*/    "object. Second, a sequence of [xpos, ypos, width,\n"
  /*NODOC*/    "height]. Lastly, a pair of sequences, representing\n"
  /*NODOC*/    "the position and size [[xpos, ypos], [width,\n"
  /*NODOC*/    "height]]. Also, if a method takes a rectstyle\n"
  /*NODOC*/    "argument as its only argument, you can simply pass\n"
  /*NODOC*/    "four arguments representing xpos, ypos, width,\n"
  /*NODOC*/    "height. A rectstyle argument can also be _any_ python\n"
  /*NODOC*/    "object with an attribute named 'rect'.\n"
#else
    /*DOC*/    "<table border=0 cellspacing=0 cellpadding=0 width=80%>\n"
    /*DOC*/    "<tr align=left valign=top><td align=left valign=middle width=20%><blockquote> </blockquote></td><td align=left valign=top><ul>\n"
    /*DOC*/    "<li>an actual Rect object. \n"
    /*DOC*/    "<li>a sequence of [xpos, ypos, width, height]. \n"
    /*DOC*/    "<li>a pair of sequences, representing the position and size [[xpos, ypos], [width,height]]. \n"
    /*DOC*/    "<li>if a method takes a rectstyle argument <b>as its <i>only</i> argument</b>, you can simply pass four arguments representing xpos, ypos, width, height. \n"
    /*DOC*/    "</ul>and perhaps most importantly:\n"
    /*DOC*/    "<ul><li>A rectstyle argument can also be <b><strong>_any_ python object</b> with an attribute named <b>'rect'.</b></strong>\n"
    /*DOC*/    "</ul></td></tr></table>\n"
#endif
    /*DOC*/ ;

static PyTypeObject PyRect_Type = {
	PyObject_HEAD_INIT(0)
	0,					/*size*/
	"pygame.Rect", 				/*name*/
	sizeof(PyRectObject),		        /*basicsize*/
	0,					/*itemsize*/
	/* methods */
	(destructor)rect_dealloc,	        /*dealloc*/
	(printfunc)NULL,			/*print*/
        NULL,	                                /*getattr*/
        NULL,	                                /*setattr*/
	(cmpfunc)rect_compare,		        /*compare*/
	(reprfunc)rect_repr,		        /*repr*/
	&rect_as_number,			/*as_number*/
	&rect_as_sequence,			/*as_sequence*/
	NULL,					/*as_mapping*/
	(hashfunc)NULL, 			/*hash*/
	(ternaryfunc)NULL,			/*call*/
	(reprfunc)rect_str,			/*str*/

	/* Space for future expansion */
	0L,0L,0L,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	doc_Rect_MODULE,                        /* Documentation string */
	0,					/* tp_traverse */
	0,					/* tp_clear */
	0,					/* tp_richcompare */
	offsetof(PyRectObject, weakreflist),    /* tp_weaklistoffset */
	0,					/* tp_iter */
	0,					/* tp_iternext */
	rect_methods,			        /* tp_methods */
	0,				        /* tp_members */
	rect_getsets,				/* tp_getset */
	0,					/* tp_base */
	0,					/* tp_dict */
	0,					/* tp_descr_get */
	0,					/* tp_descr_set */
	0,					/* tp_dictoffset */
	(initproc)rect_init,			/* tp_init */
	0,					/* tp_alloc */
	rect_new,			        /* tp_new */
};



/*module globals*/
#if 0
    /*DOC*/ static char doc_Rect[] =
    /*DOC*/    "pygame.Rect(rectstyle) -> Rect\n"
    /*DOC*/    "create a new rectangle\n"
    /*DOC*/    "\n"
    /*DOC*/    "Creates a new rectangle object. The given\n"
    /*DOC*/    "rectstyle represents one of the various ways of\n"
    /*DOC*/    "representing rectangle data. This is usually a\n"
    /*DOC*/    "sequence of x and y position for the topleft\n"
    /*DOC*/    "corner, and the width and height.\n"
    /*DOC*/    "\n"
    /*DOC*/    "For some of the Rect methods there are two version.\n"
    /*DOC*/    "For example, there is move() and move_ip(). The methods\n"
    /*DOC*/    "witht the '_ip' suffix on the name are the 'in-place'\n"
    /*DOC*/    "version of those functions. They effect the actual\n"
    /*DOC*/    "source object, instead of returning a new Rect object.\n"
    /*DOC*/ ;
#endif

static PyObject* rect_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyRectObject *self;
    self = (PyRectObject *)type->tp_alloc(type, 0);
    if (self)
    {
        self->r.x = self->r.y = 0;
        self->r.w = self->r.h = 0;
        self->weakreflist = NULL;
    }
    return (PyObject*)self;
}

static int rect_init(PyRectObject *self, PyObject *args, PyObject *kwds)
{
    GAME_Rect *argrect, temp;
    if(!(argrect = GameRect_FromObject(args, &temp)))
    {
            RAISE(PyExc_TypeError, "Argument must be rect style object");
            return -1;
    }

    self->r.x = argrect->x;
    self->r.y = argrect->y;
    self->r.w = argrect->w;
    self->r.h = argrect->h;
    return 0;
}



static PyMethodDef rect__builtins__[] =
{
	{NULL, NULL}
};




    /*DOC*/ static char rectangle_doc[] =
    /*DOC*/    "Module for the rectangle object\n";

PYGAME_EXPORT
void initrect(void)
{
	PyObject *module, *dict, *apiobj;
	static void* c_api[PYGAMEAPI_RECT_NUMSLOTS];

	/* Create the module and add the functions */
	PyType_Init(PyRect_Type);
        if (PyType_Ready(&PyRect_Type) < 0)
            return;


	module = Py_InitModule3("rect", rect__builtins__, rectangle_doc);
	dict = PyModule_GetDict(module);

	PyDict_SetItemString(dict, "RectType", (PyObject *)&PyRect_Type);
	PyDict_SetItemString(dict, "Rect", (PyObject *)&PyRect_Type);

	/* export the c api */
	c_api[0] = &PyRect_Type;
	c_api[1] = PyRect_New;
	c_api[2] = PyRect_New4;
	c_api[3] = GameRect_FromObject;
	apiobj = PyCObject_FromVoidPtr(c_api, NULL);
	PyDict_SetItemString(dict, PYGAMEAPI_LOCAL_ENTRY, apiobj);
	Py_DECREF(apiobj);

	/*imported needed apis*/
	import_pygame_base();
}
