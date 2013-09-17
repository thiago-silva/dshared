// Copyright (c) 2013 Thiago B. L. Silva <thiago@metareload.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include <Python.h>
#include <longobject.h>
#include "dshared.hpp"
#include <iostream>

//single static shared memory [sorry]
MManager* manager = NULL;

const char*
py_unicode_to_string(PyObject* unic) {
  Py_ssize_t sz = PyUnicode_GetSize(unic);
  Py_UNICODE* unistr = PyUnicode_AS_UNICODE(unic);
  PyObject* ustr = PyUnicode_Encode(unistr, sz, "utf-8", "replace");
  return PyString_AsString(ustr);
}

typedef std::map<PyObject*, offset_ptr<smap> > visited_map_t;

int
rec_store_item(offset_ptr<smap> sd, PyObject* key,
               PyObject* val, visited_map_t visited = visited_map_t());

int
rec_store_item(offset_ptr<smap> sd, Py_ssize_t key,
               PyObject* val, visited_map_t visited = visited_map_t());

int
do_rec_store_item(offset_ptr<smap> sd, const char* key,
                  PyObject* val, visited_map_t visited = visited_map_t());

static PyObject*
PyObject_from_variant(offset_ptr<smap_value_t> val);



/** dshared.list  **/

typedef struct {
  PyObject_HEAD //PyListObject list;
  offset_ptr<smap> sm;
} SList;

typedef struct {
  PyObject_HEAD
  SList* collection;
  smap::iterator current;
} SList_Iter;


static int
SList_print(PyObject *self, FILE *file, int flags);

static int
SList_compare(PyObject *o1, PyObject *o2);

static PyObject*
SList_rich_compare(PyObject *a, PyObject *b, int op);

static int
SList_len(PyObject* self);

static PyObject*
SList_append(PyObject* o1, PyObject* o2);

static PyObject*
SList_insert(PyObject* self, PyObject* args);

static PyObject*
SList_pop(PyObject* self, PyObject* args);

static PyObject*
SList_concat(PyObject* o1, PyObject* o2);

static int
SList_set_item(PyObject* _self, Py_ssize_t key, PyObject* val);

static PyObject*
SList_get_item(PyObject* _self, Py_ssize_t key);

static PyObject*
SList_slice(PyObject *o, Py_ssize_t i1, Py_ssize_t i2);

static int
SList_contains(PyObject *self, PyObject *value);

static PyObject*
SList_iter(PyObject* self);


static PyObject*
SList_str(PyObject *self);

static int
SList_init(SList *self, PyObject *args, PyObject *kwds);

static PyObject*
SList_create_list(offset_ptr<smap_value_t>);

static PyObject*
SList_Iter_iternext(PyObject *_self);

static PyObject*
SList_add(PyObject* _self, PyObject* args);

static int
populate_smap_with_list(offset_ptr<smap> entry, PyObject* val, visited_map_t visited = visited_map_t());

static PyObject*
smap_entry_as_SList(offset_ptr<smap_value_t> val);

static PySequenceMethods
slist_seq = {
  SList_len,/* sq_length */
  SList_concat,/* sq_concat */
  0,/* sq_repeat */
  SList_get_item,/* sq_item */
  SList_slice,/* sq_slice */
  SList_set_item,/* sq_ass_item */
  0,/* sq_ass_slice */
  SList_contains,/* sq_contains*/
  0, /* sq_inplace_concat*/
  0 /* sq_inplace_repeat */
};

static PyNumberMethods
slist_nums = {
  SList_add,   /* binaryfunc nb_add;          __add__ */
    0,            /* binaryfunc nb_subtract;     __sub__ */
    0,            /* binaryfunc nb_multiply;     __mul__ */
    0,            /* binaryfunc nb_divide;       __div__ */
    0,            /* binaryfunc nb_remainder;    __mod__ */
    0,            /* binaryfunc nb_divmod;       __divmod__ */
    0,            /* ternaryfunc nb_power;       __pow__ */
    0,            /* unaryfunc nb_negative;      __neg__ */
    0,            /* unaryfunc nb_positive;      __pos__ */
    0,            /* unaryfunc nb_absolute;      __abs__ */
    0,            /* inquiry nb_nonzero;         __nonzero__ */
    0,            /* unaryfunc nb_invert;        __invert__ */
    0,            /* binaryfunc nb_lshift;       __lshift__ */
    0,            /* binaryfunc nb_rshift;       __rshift__ */
    0,            /* binaryfunc nb_and;          __and__ */
    0,            /* binaryfunc nb_xor;          __xor__ */
    0,            /* binaryfunc nb_or;           __or__ */
    0,            /* coercion nb_coerce;         __coerce__ */
    0,            /* unaryfunc nb_int;           __int__ */
    0,            /* unaryfunc nb_long;          __long__ */
    0,            /* unaryfunc nb_float;         __float__ */
    0,            /* unaryfunc nb_oct;           __oct__ */
    0,            /* unaryfunc nb_hex;           __hex__ */
};

static PyMethodDef
SList_methods[] = {
  {"append", (PyCFunction)SList_append, METH_VARARGS,
   PyDoc_STR("append")},
  {"insert", (PyCFunction)SList_insert, METH_VARARGS,
   PyDoc_STR("insert")},
  {"pop", (PyCFunction)SList_pop, METH_VARARGS,
   PyDoc_STR("pop")},
    // {"__add__", (PyCFunction)SList_add, METH_VARARGS,
    //  PyDoc_STR("__add__")},
    // {"__radd__", (PyCFunction)SList_radd, METH_VARARGS,
    //  PyDoc_STR("__radd__")},
    {NULL, NULL},
};


static PyTypeObject
SListType = {
    PyObject_HEAD_INIT(NULL)
    0,                       /* ob_size */
    "dshared.list",          /* tp_name */
    sizeof(SList),           /* tp_basicsize */
    0,                       /* tp_itemsize */
    0,                       /* tp_dealloc */
    SList_print,             /* tp_print */
    0,                       /* tp_getattr */
    0,                       /* tp_setattr */
    SList_compare,           /* tp_compare */
    0,                       /* tp_repr */
    &slist_nums,             /* tp_as_number */
    &slist_seq,              /* tp_as_sequence */
    0,                       /* tp_as_mapping */
    0,                       /* tp_hash */
    0,                       /* tp_call */
    SList_str,                /* tp_str */
    0,                       /* tp_getattro */
    0,                       /* tp_setattro */
    0,                       /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT       /* tp_flags */
      | Py_TPFLAGS_HAVE_SEQUENCE_IN
      | Py_TPFLAGS_HAVE_ITER
      | Py_TPFLAGS_CHECKTYPES
      | Py_TPFLAGS_HAVE_RICHCOMPARE,

    0,                       /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    SList_rich_compare,      /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    SList_iter,              /* tp_iter */
    0,                       /* tp_iternext */
    SList_methods,           /* tp_methods */
    0,                       /* tp_members */
    0,                       /* tp_getset */
    0,                       /* tp_base */
    0,                       /* tp_dict */
    0,                       /* tp_descr_get */
    0,                       /* tp_descr_set */
    0,                       /* tp_dictoffset */
    (initproc)SList_init,    /* tp_init */
    0,                       /* tp_alloc */
    0,                       /* tp_new */
};

static PyTypeObject
SList_IterType = {
    PyObject_HEAD_INIT(NULL)
    0,                       /* ob_size */
    "dshared.list-iterator", /* tp_name */
    sizeof(SList_Iter),      /* tp_basicsize */
    0,                       /* tp_itemsize */
    0,                       /* tp_dealloc */
    0,                       /* tp_print */
    0,                       /* tp_getattr */
    0,                       /* tp_setattr */
    0,                       /* tp_compare */
    0,                       /* tp_repr */
    0,                       /* tp_as_number */
    0,                       /* tp_as_sequence */
    0,                       /* tp_as_mapping */
    0,                       /* tp_hash */
    0,                       /* tp_call */
    0,                       /* tp_str */
    0,                       /* tp_getattro */
    0,                       /* tp_setattro */
    0,                       /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
     Py_TPFLAGS_HAVE_ITER,   /* tp_flags */
    0,                       /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    PyObject_SelfIter,       /* tp_iter */
    SList_Iter_iternext,     /* tp_iternext */
    0,                       /* tp_methods */
    0,                       /* tp_members */
    0,                       /* tp_getset */
    0,                       /* tp_base */
    0,                       /* tp_dict */
    0,                       /* tp_descr_get */
    0,                       /* tp_descr_set */
    0,                       /* tp_dictoffset */
    0,                       /* tp_init */
    0,                       /* tp_alloc */
    0,                       /* tp_new */
};

int
SList_print(PyObject *self, FILE *file, int flags) {
  std::stringstream s;
  s << "<dshared.list" << " object at " << self << ">";
  fputs(s.str().c_str(), file);
  return 0;
}

int
SList_compare(PyObject *_self, PyObject *o2) { // 0 = True, 1/-1 = False
  //std::cout << "SList_compare\n";
  if (_self == o2) {
    //std::cout << "SList_compare: pointers are the same: 0\n";
    return 0;
  }
  if (!PyObject_TypeCheck(o2, &SListType) &&
      !PyList_CheckExact(o2)) {
    //std::cout << "SList_compare: other is not list or slist: 1\n";
    return 1;
  }
  if (!PyObject_TypeCheck(_self, &SListType)) {
    PyErr_SetString(PyExc_TypeError,"SList_compare: why self is not a slist?");
    return -1;
  }
  SList* self = (SList*) _self;
  Py_ssize_t tsize =  SList_len(_self);
  if (tsize == -1) {
    return -1;
  }
  Py_ssize_t osize = PyList_CheckExact(o2) ? PyList_Size(o2) : SList_len(o2);
  if (osize == -1) {
    return -1;
  }
  //std::cout << "SList_compare: sizes: " << tsize << ":" << osize <<"\n";
  if (tsize != osize) {
    //std::cout << "SList_compare: sizes differ: 1\n";
    return 1;
  }
  //std::cout << "....continuing..\n";
  for (smap::size_type i = 0; i < self->sm->size(); i++) {
    std::stringstream s;
    s << i;
    //std::cout << "getting variant for key " << i << "\n";
    offset_ptr<smap_value_t> val = smap_get_item(self->sm, s.str().c_str());
    PyObject* other1 = PyObject_from_variant(val);

    PyObject* other2 = PyList_CheckExact(o2) ? PyList_GetItem(o2, i) : SList_get_item(o2,i);

    //std::cout << "SList_compare: comparing item " << i << " - " << other1 << ":" << other2 << "\n";
    if (other1 == other2) {
      continue;
    } else if (PyObject_RichCompare(other1, other2, Py_EQ) != Py_True) {
      //std::cout << "SList_compare: item " << i << " not equal, returning 1\n";
      return 1;
    }
  }
  //std::cout << "SList_compare: equal!\n";
  return 0;
}



PyObject*
SList_rich_compare(PyObject *self, PyObject *other, int op) {
  //std::cout << "SList_rich_compare...\n";
  if (op != Py_EQ) {
    PyErr_SetString(PyExc_TypeError," TODO: comparision operator not supported");
    return NULL; //I guess it should be Py_NotImplemented, so we don't break things up
  }
  if (SList_compare(self, other) == 0) {
    //std::cout << "SList_rich_compare: true\n";
    Py_INCREF(Py_True);
    return Py_True;
  } else {
    //std::cout << "SList_rich_compare: false\n";
    Py_INCREF(Py_False);
    return Py_False;
  }
}

int
SList_len(PyObject* self) {
  return ((SList*)self)->sm->size();
}

PyObject*
_SList_add(SList* lhs, PyListObject* _rhs) {
  PyObject* rhs = (PyObject*) _rhs;
  //std::cout << "_SList_add[1]\n";

  Py_ssize_t size = lhs->sm->size();

  Py_ssize_t rsize = PyList_Size(rhs);
  size += rsize;

  PyObject* lst = PyList_New(size);
  smap::size_type i;
  for (i = 0; i < lhs->sm->size(); i++) {
    std::stringstream s;
    s << i;
    offset_ptr<smap_value_t> val = smap_get_item(lhs->sm, s.str().c_str());
    PyObject* o = PyObject_from_variant(val);
    PyList_SetItem(lst, i, o);
  }

  for (Py_ssize_t j = 0; j < rsize; i++, j++) {
    PyObject* o = PyList_GetItem(rhs, j);
    PyList_SetItem(lst, i, o);
  }
  Py_INCREF(lst);
  return lst;
}

PyObject*
_SList_add(PyListObject* _lhs, SList* rhs) {
  PyObject* lhs = (PyObject*) _lhs;
  //std::cout << "_SList_add[2]\n";

  Py_ssize_t lsize = PyList_Size(lhs);
  Py_ssize_t rsize = rhs->sm->size();
  Py_ssize_t size = lsize + rsize;

  PyObject* lst = PyList_New(size);
  Py_ssize_t i;
  for (i = 0; i < lsize; i++) {
    PyObject* o = PyList_GetItem(lhs, i);
    PyList_SetItem(lst, i, o);
  }

  for (Py_ssize_t j = 0; j < rsize; i++, j++) {
    std::stringstream s;
    s << j;
    offset_ptr<smap_value_t> val = smap_get_item(rhs->sm, s.str().c_str());
    PyObject* o = PyObject_from_variant(val);
    PyList_SetItem(lst, i, o);
  }
  Py_INCREF(lst);
  return lst;
}

PyObject*
_SList_add(SList* lhs, SList* rhs) {
  PyErr_SetString(PyExc_TypeError," TODO");
  return NULL;
}

static PyObject*
SList_add(PyObject* self, PyObject* other) {
  if (PyObject_TypeCheck(self, &SListType) &&
      PyList_CheckExact(other)) {
    return _SList_add((SList*)self, (PyListObject*)other);
  } else if (PyObject_TypeCheck(other, &SListType) &&
             PyList_CheckExact(self)) {
    return _SList_add((PyListObject*)self, (SList*)other);
  } if (PyObject_TypeCheck(self, &SListType) &&
        PyObject_TypeCheck(other, &SListType)) {
    return _SList_add((SList*)self, (SList*)other);
  } else {
    PyErr_SetString(PyExc_TypeError," can't concatenate slist with unknown type");
    return NULL;
  }
}

PyObject*
SList_concat(PyObject *self, PyObject *other) {
  //std::cout << "slists::concat\n";
  return SList_add(self, other);
}

PyObject*
SList_append(PyObject* _self, PyObject* args) {

  PyObject *arg;
  if (PyTuple_Size(args) != 1) {
      PyErr_SetString(PyExc_TypeError,"expected one argument");
      return NULL;
  } else {
    if (!PyArg_UnpackTuple(args, "append", 1, 1, &arg)) {
      PyErr_SetString(PyExc_TypeError,"expected one argument");
      return NULL;
    }
  }

  SList* self = (SList*) _self;
  smap::size_type size = self->sm->size();
  std::stringstream s;
  s << size;
  if (do_rec_store_item(self->sm, s.str().c_str(), arg) != 0) {
    return NULL;
  }
  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject*
SList_insert(PyObject* _self, PyObject* args) {
  PyObject *pos, *lst;
  if (PyTuple_Size(args) != 2) {
      PyErr_SetString(PyExc_TypeError,"expected two arguments");
      return NULL;
  } else {
    if (!PyArg_UnpackTuple(args, "insert", 2, 2, &pos, &lst)) {
      PyErr_SetString(PyExc_TypeError,"expected two arguments");
      return NULL;
    }
  }

  if (!PyLong_Check(pos) && !PyInt_Check(pos)) {
    PyErr_SetString(PyExc_TypeError,"pos arg must be a number");
    return NULL;
  }

  SList* self = (SList*)_self;

  long other_size;
  if (PyObject_TypeCheck(lst, &SListType)) {
    other_size = SList_len(lst);
  } else if (PyList_CheckExact(lst)) {
    other_size = PyList_Size(lst);
  } else {
    PyErr_SetString(PyExc_TypeError,"list arg must be [s]list");
    return NULL;
  }
  //std::cout << "other len: " << other_size << "\n";

  long self_size = self->sm->size();
  //std::cout << "self len: " << self_size << "\n";
  long from = PyInt_AsLong(pos);
  //std::cout << "from: " << from << "\n";;
  if ((from > self_size) || (from < 0)) {
    PyErr_SetString(PyExc_TypeError,"pos arg must be in range [0,size]");
    return NULL;
  }

  long i, j;
  //PyObject *obj;
  std::list<PyObject*> objs;
  for (i = 0; i < from; i++) {
    std::stringstream s;
    s << j;
    //std::cout << "acc self " << i << "\n";
    objs.push_back(SList_get_item(_self, i));
  }
  for (j = 0; j < other_size; j++) {
    if (PyList_CheckExact(lst)) {
      //std::cout << "acc other list's " << j << "\n";
      objs.push_back(PyList_GetItem(lst, j));
    } else {
      //std::cout << "acc other slist's " << j << "\n";
      objs.push_back(SList_get_item(lst, j));
    }
  }

  for (j = from ; j < self_size; j++) {
    std::stringstream s;
    s << j;
    //std::cout << "acc self " << j << "\n";
    objs.push_back(SList_get_item(_self, j));
  }

  //std::cout << "overwriting all now..\n";
  i = 0;
  for (std::list<PyObject*>::iterator it = objs.begin(); it != objs.end(); it++, i++) {
    if (i < self_size) {
      std::stringstream s;
      s << i;
      //std::cout << "overwriting self " << i << "\n";
      if (do_rec_store_item(self->sm, s.str().c_str(), *it)) {
        return NULL;
      }
    } else {
      //std::cout << "appending self " << i << "\n";
      PyObject* tpl = PyTuple_New(1);
      PyTuple_SetItem(tpl, 0, *it);
      if (SList_append(_self, tpl) == NULL) {
        return NULL;
      }
    }
  }
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
SList_pop(PyObject* _self, PyObject* args) {
  SList* self = (SList*) _self;
  smap::size_type size = self->sm->size();
  if (size == 0) {
    PyErr_SetString(PyExc_IndexError,"pop from empty slist");
    return NULL;
  }
  std::stringstream s;
  s << (size-1);
  const char* strkey = s.str().c_str();
  offset_ptr<smap_value_t> val = smap_get_item(self->sm, strkey);
  smap_delete_item(self->sm, strkey);
  PyObject* obj = PyObject_from_variant(val);
  if (obj) {
    Py_INCREF(obj);
    return obj;
  } else {
    return NULL;
  }
}

int
SList_set_item(PyObject* _self, Py_ssize_t key, PyObject* val) {
  SList* self = (SList*) _self;
  return rec_store_item(self->sm, key, val);
}


PyObject*
SList_get_item(PyObject* _self, Py_ssize_t key) {
  //std::cout << "SList_get_item\n";

  SList* self = (SList*) _self;

  std::stringstream s;
  s << key;
  const char* strkey = s.str().c_str();
  try {
    //std::cout << "slist::get_item " << strkey << "\n";
    offset_ptr<smap_value_t> val = smap_get_item(self->sm, strkey);
    //std::cout << "  slist::get_item variant " << val <<"|" << val.get() << "\n";
    PyObject* obj = PyObject_from_variant(val);
    if (obj) {
      Py_INCREF(obj);
    }
    //std::cout << "  slist::get_item returning " << obj << "\n";
    return obj;
  } catch(...) { //out of range
    PyErr_SetString(PyExc_KeyError,strkey);
    return NULL;
  }
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
SList_slice(PyObject *_self, Py_ssize_t begin, Py_ssize_t end) {
  SList* self = (SList*) _self;
  //std::cout << "SList_slice " << begin << ":" << end << "\n";
  if (end > SList_len(_self)) {
    end = self->sm->size();
  }
  Py_ssize_t size = end - begin;
  PyObject* lst;
  if (size <= 0) {
    //std::cout << "SList_slice: empty\n";
    lst = PyList_New(0);
  } else {
    //std::cout << "SList_slice: slicing " << begin << ":" << end << "\n";
    lst = PyList_New(size);
    for (Py_ssize_t i = begin, j = 0; i < end; i++, j++) {
      //std::cout << "getting item " << i << " to put on " << j << "\n";
      std::stringstream s;
      s << i;
      offset_ptr<smap_value_t> val = smap_get_item(self->sm, s.str().c_str());
      //std::cout << "setting item to " << val << "\n";
      PyObject* o = PyObject_from_variant(val);
      PyList_SetItem(lst, j, o);
    }
  }
  Py_INCREF(lst);
  return lst;
}


int
SList_contains(PyObject *self, PyObject *value) {
  offset_ptr<smap> sm = ((SList*)self)->sm;
  //std::cout << "SList_contains\n";

  for (smap::size_type i = 0; i < sm->size(); i++) {
    PyObject* entry = SList_get_item(self, i);
    if (entry == NULL) {
      return -1;
    }
    //std::cout << "comparing " << entry << ":" << value << "\n";
    if (entry == value) {
      //std::cout << "exactly the same\n";
      return 1;
    } else {
      int ret = PyObject_RichCompareBool(entry, value, Py_EQ);
      //std::cout << "index " << i << " eqs value? " << ret << "\n";
      if (ret == 1) {
        return 0;
      }
    }
  }
  return 0;
}

PyObject*
SList_iter(PyObject* self) {
  //std::cout << "slist::__iter__\n";
  SList_Iter* iterator = (SList_Iter*)  PyObject_New(SList_Iter, &SList_IterType);
  if (!iterator) return NULL;
  //std::cout << "initing iter\n";
  if (!PyObject_Init((PyObject *)iterator, &SList_IterType)) {
    Py_DECREF(iterator);
    return NULL;
  }

  //std::cout << "setting col\n";
  iterator->collection = (SList*) self;
  iterator->current = iterator->collection->sm->begin();
  //std::cout << "returning iter with items: " << iterator->collection->sm->size() << "\n";
  Py_INCREF(iterator);
  return (PyObject*) iterator;
}

PyObject*
SList_str(PyObject *self) {
  std::stringstream s;
  s << "<dshared.list" << " object at " << self << ">";
  return PyString_FromString(s.str().c_str());
}


int
SList_init(SList *self, PyObject *args, PyObject *kwds) {
  Py_INCREF(self);
  //std::cout << "SList_init\n";
  if (manager == NULL) {
    PyErr_SetString(PyExc_RuntimeError,"init() was not called");
    return -1;
  }

  PyObject *arg;
  if (PyTuple_Size(args) == 0) {
    arg = PyList_New(0);
  } else {
    if (!PyArg_UnpackTuple(args, "__init__", 1, 1, &arg)) {
      PyErr_SetString(PyExc_TypeError,"expected one argument");
      return -1;
    }
  }

  if (PyList_CheckExact(arg)) {
    self->sm = manager->create_smap();
    if (populate_smap_with_list(self->sm, arg) != 0) {
      return -1;
    }
  } else if (PyObject_TypeCheck(arg, &SListType)) {
    self->sm = ((SList*)arg)->sm;
  } else {
    PyErr_SetString(PyExc_TypeError,"expected a list or dshared.list");
    return -1;
  }

  // if (PyList_Type.tp_init((PyObject *)self, PyTuple_New(0), PyDict_New()) < 0) {
  //   return -1;
  // }
  return 0;

}

PyObject*
SList_Iter_iternext(PyObject *_self) {
  //std::cout << "SDict_Iter_iternext\n";
  SList_Iter* self = (SList_Iter*)_self;

  if (self->current == self->collection->sm->end()) {
    PyErr_SetNone(PyExc_StopIteration);
    //std::cout << "  SDict_Iter_iternext: StopIteration\n";
    return NULL;
  } else {
    offset_ptr<smap_value_t> val = self->current->second;
    //std::cout << "  SDict_Iter_iternext: iter variant for " << self->current->first << ": " << val << "|" << val.get() << "\n";
    //std::cout << "  SDict_Iter_iternext: sneaking .d: " << val->d << "|" << val->d.get() << "tag: " << val->tag <<"\n";
    //std::cout << "  SDict_Iter_iternext: sneaking ._odict_: " << val->_odict_ << "|" << val->_odict_.get() <<"\n";
    PyObject* obj;
    if ((obj = PyObject_from_variant(val)) == NULL) {
      //std::cout << "  SDict_Iter_iternext: from variant is NULL!!!\n";
      return NULL;
    }
    Py_INCREF(obj);
    //std::cout << "  SDict_Iter_iternext: created set tuple value\n";
    self->current++;
    //std::cout << "  SDict_Iter_iternext: returning tp\n";
    return obj;
  }
}

int
populate_smap_with_list(offset_ptr<smap> entry, PyObject* val, visited_map_t visited) {
  PyObject *key, *value;
  int ret;
  visited[val] = entry;
  Py_ssize_t size = PyList_Size(val);
  for (int i = 0; i < size; i++) {
    value = PyList_GetItem(val,i);
    std::stringstream s;                       //I...
    s << i;                                    //hate...
    key = PyString_FromString(s.str().c_str());//c++!
    if ((ret = rec_store_item(entry, key, value, visited)) != 0) {
      return ret;
    }
  }
  return 0;
}



PyObject*
smap_entry_as_SList(offset_ptr<smap_value_t> val) {
  //std::cout << "Slist_for_smap\n";
  //std::cout << "   Slist_for_smap: has in cache?" << val->has_cache() << "\n";
  PyObject* obj;
  if (val->has_cache()) {
    //std::cout << "smap_entry_as_SList has cache...\n";
    obj = (PyObject*)val->cache();
    //std::cout << "   Slist_for_smap: cached obj to return: " << obj << "\n";
  } else {
    //std::cout << "  SList_for_smap: creating pylist...\n";
    obj = SList_create_list(val);
  }
  Py_INCREF(obj);
  return obj;
}

PyObject*
SList_create_list(offset_ptr<smap_value_t> variant) {
  //std::cout << "SList_create_dict:: for variant: " << variant << "|" << variant.get() << "\n";
  SList* obj = (SList*) PyObject_CallObject((PyObject *) &SListType, PyTuple_New(0));

  //std::cout << "SList_create_dict:: created list pyobject : " << obj << ". Caching (and crashing?)\n";
  variant->cache_obj(obj);
  //std::cout << "SUCC. SDict_create_dict:: setting pyobj-sd: " << variant->d << "|" << variant->d.get() << "\n";

  //std::cout << "CAST[before]  >>>" << variant->d << "|" << variant->d.get() << "\n";
  obj->sm = offset_ptr<smap>(static_cast<smap*>(variant->d.get()));
  //std::cout << "CAST :: >>>>>> : " << obj->sm <<"|"<< obj->sm.get() << "\n";
  return (PyObject*) obj;
}




/** END: dshared.list  **/


/** dshared.dict  **/



typedef struct {
  PyDictObject dict;
  offset_ptr<smap> sm;
} SDict;

typedef struct {
  PyObject_HEAD
  SDict* collection;
  smap::iterator current;
} SDict_Iter;

static PyObject*
smap_entry_as_SDICT(offset_ptr<smap_value_t> val);

static PyObject*
smap_entry_as_PYOBJ(offset_ptr<smap_value_t> val);

static PyObject*
PyObject_from_variant(offset_ptr<smap_value_t> val);

static int
SDict_contains(PyObject *self, PyObject *value);

static int
SDict_set_item(PyObject* _self, PyObject* key, PyObject* val);

static PyObject*
SDict_get_item(PyObject* _self, PyObject* key);

static int
populate_smap_with_dict(offset_ptr<smap> entry, PyObject* val, visited_map_t visited = visited_map_t());

static PyObject*
SDict_Iter_iternext(PyObject *_self);

static PyObject*
SDict_iteritems(PyObject* self, PyObject* args);

static PyObject*
SDict_items(PyObject* _self, PyObject* args);

static PyObject*
SDict_keys(PyObject* _self, PyObject* args);

static PyObject*
SDict_values(PyObject* _self, PyObject* args);

static PyObject*
SDict_update(PyObject* _self, PyObject* args);

static PyObject*
SDict_iter(PyObject* self);

static PyObject*
SDict_append(PyObject* _self, PyObject* args);

static int
SDict_init(SDict *self, PyObject *args, PyObject *kwds);

static PyObject*
SDict_str(PyObject *self);

static int
SDict_print(PyObject *self, FILE *file, int flags);

static Py_ssize_t
SDict_len(PyObject* self);

static int
SDict_init(SDict *self, PyObject *args, PyObject *kwds);

static PyObject*
SDict_create_dict(offset_ptr<smap_value_t> variant);

static PyObject*
SDict_create_obj(offset_ptr<smap_value_t> variant);

static PyObject*
SDict_rich_compare(PyObject *a, PyObject *b, int op);


static PyMappingMethods
SDict_mapping = {
    SDict_len,
    SDict_get_item,
    SDict_set_item
};

static PyMethodDef
SDict_methods[] = {
    {"iteritems", (PyCFunction)SDict_iteritems, METH_NOARGS,
     PyDoc_STR("iterator for items")},
    {"items", (PyCFunction)SDict_items, METH_NOARGS,
     PyDoc_STR("dict items")},
    {"append", (PyCFunction)SDict_append, METH_VARARGS,
     PyDoc_STR("append")},
    {"keys", (PyCFunction)SDict_keys, METH_VARARGS,
     PyDoc_STR("keys")},
    {"values", (PyCFunction)SDict_values, METH_VARARGS,
     PyDoc_STR("values")},
    {"update", (PyCFunction)SDict_update, METH_VARARGS,
     PyDoc_STR("update")},
    {NULL, NULL},
};

static PySequenceMethods
SDict_seq = {
  0,/* sq_length */
  0,/* sq_concat */
  0,/* sq_repeat */
  0,/* sq_item */
  0,/* sq_slice */
  0,/* sq_ass_item */
  0,/* sq_ass_slice */
  (objobjproc) SDict_contains,/* sq_contains*/
  0, /* sq_inplace_concat*/
  0 /* sq_inplace_repeat */
};

static PyTypeObject
SDict_IterType = {
    PyObject_HEAD_INIT(NULL)
    0,                       /* ob_size */
    "dshared.dict-iterator", /* tp_name */
    sizeof(SDict_Iter),      /* tp_basicsize */
    0,                       /* tp_itemsize */
    0,                       /* tp_dealloc */
    0,                       /* tp_print */
    0,                       /* tp_getattr */
    0,                       /* tp_setattr */
    0,                       /* tp_compare */
    0,                       /* tp_repr */
    0,                       /* tp_as_number */
    0,                       /* tp_as_sequence */
    0,                       /* tp_as_mapping */
    0,                       /* tp_hash */
    0,                       /* tp_call */
    0,                       /* tp_str */
    0,                       /* tp_getattro */
    0,                       /* tp_setattro */
    0,                       /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
     Py_TPFLAGS_HAVE_ITER,   /* tp_flags */
    0,                       /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    PyObject_SelfIter,       /* tp_iter */
    SDict_Iter_iternext,     /* tp_iternext */
    0,                       /* tp_methods */
    0,                       /* tp_members */
    0,                       /* tp_getset */
    0,                       /* tp_base */
    0,                       /* tp_dict */
    0,                       /* tp_descr_get */
    0,                       /* tp_descr_set */
    0,                       /* tp_dictoffset */
    0,                       /* tp_init */
    0,                       /* tp_alloc */
    0,                       /* tp_new */
};

static PyTypeObject
SDictType = {
    PyObject_HEAD_INIT(NULL)
    0,                       /* ob_size */
    "dshared.dict",          /* tp_name */
    sizeof(SDict),           /* tp_basicsize */
    0,                       /* tp_itemsize */
    0,                       /* tp_dealloc */
    SDict_print,             /* tp_print */
    0,                       /* tp_getattr */
    0,                       /* tp_setattr */
    0,                       /* tp_compare */
    0,                       /* tp_repr */
    0,                       /* tp_as_number */
    &SDict_seq,              /* tp_as_sequence */
    &SDict_mapping,          /* tp_as_mapping */
    0,                       /* tp_hash */
    0,                       /* tp_call */
    SDict_str,               /* tp_str */
    0,                       /* tp_getattro */
    0,                       /* tp_setattro */
    0,                       /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT
    | Py_TPFLAGS_HAVE_RICHCOMPARE
    | Py_TPFLAGS_HAVE_SEQUENCE_IN /* tp_flags */
    | Py_TPFLAGS_HAVE_ITER,
    0,                       /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    SDict_rich_compare,      /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    SDict_iter,              /* tp_iter */
    0,                       /* tp_iternext */
    SDict_methods,           /* tp_methods */
    0,                       /* tp_members */
    0,                       /* tp_getset */
    0,                       /* tp_base */
    0,                       /* tp_dict */
    0,                       /* tp_descr_get */
    0,                       /* tp_descr_set */
    0,                       /* tp_dictoffset */
    (initproc)SDict_init,    /* tp_init */
    0,                       /* tp_alloc */
    0,                       /* tp_new */
};


PyObject*
smap_entry_as_SDICT(offset_ptr<smap_value_t> val) {
  //std::cout << "smap_entry_as_SDICT\n";
  PyObject* obj;
  if (val->has_cache()) {
    //std::cout << "smap_entry_as_SDICT: val has cache, getting...\n";
    obj = (PyObject*)val->cache();
    //std::cout << "smap_entry_as_SDICT: returning cache " << obj << "\n";
  } else {
    //std::cout << "smap_entry_as_SDICT: NO CACHE...creating\n";
    obj = SDict_create_dict(val);
    //std::cout << "smap_entry_as_SDICT: returning new dict " << obj << "\n";
  }
  Py_INCREF(obj);
  return obj;
}

PyObject*
smap_entry_as_PYOBJ(offset_ptr<smap_value_t> val) {
  //std::cout << "SDict_for_pyobj has cache? " << val->has_cache() << "\n";
  PyObject* obj = NULL;
  if (val->has_cache()) {
    obj = (PyObject*)val->cache();
    //std::cout << "  returning cache: " << obj << "\n";
  } else {
    //std::cout << "SDict_for_pyobj has cache?no! creating instance\n";
    obj = SDict_create_obj(val);
    //std::cout << "SDict_for_pyobj created for this pid: " << obj << "\n";
  }
  return obj;
}

PyObject*
PyObject_from_variant(offset_ptr<smap_value_t> val) {
  //std::cout << "PyObject_from_variant: tag " << val->tag << "\n";
  switch(val->tag) {
    case smap_value_t::NIL:
      return Py_None;
    case smap_value_t::BOOL:
      return PyBool_FromLong(val->num);
    case smap_value_t::STRING:
      return PyString_FromString(val->str.c_str());
    case smap_value_t::NUMBER:
      return PyInt_FromLong(val->num);
    case smap_value_t::SDICT:
      return smap_entry_as_SDICT(val);
    case smap_value_t::SLIST:
      return smap_entry_as_SList(val);
    case smap_value_t::PYOBJ:
      return smap_entry_as_PYOBJ(val);
  }
  //std::cout << "WTF tag: " << val->tag << "\n";
  PyErr_SetString(PyExc_TypeError,"unknown type tag ");
  return NULL;
}

int
SDict_contains(PyObject *self, PyObject *value) {
  offset_ptr<smap> sd = ((SDict*)self)->sm;

  if (PyString_Check(value)) {
    const char* strkey = PyString_AsString(value);
    return smap_has_item(sd, strkey);
  } else if (PyUnicode_Check(value)) {
    const char* strkey = py_unicode_to_string(value);
    return smap_has_item(sd, strkey);
  } else {
    PyErr_SetString(PyExc_TypeError,"unknown type for smap index");
    return -1;
  }
}

int
SDict_set_item(PyObject* _self, PyObject* key, PyObject* val) {
  SDict* self = (SDict*) _self;
  return rec_store_item(self->sm, key, val);
}

PyObject*
SDict_get_item(PyObject* _self, PyObject* key) {
  SDict* self = (SDict*) _self;

  const char* strkey;
  if (PyString_Check(key)) {
    strkey = PyString_AsString(key);
  } else if (PyUnicode_Check(key)) {
    strkey = py_unicode_to_string(key);
  } else {
    PyErr_SetString(PyExc_TypeError,"only strings can index smap");
    return NULL;
  }

  strkey = PyString_AsString(key);
  //std::cout << "SDict_get_item " << strkey << "\n";

  try {
    //std::cout << "get_item " << strkey << "\n";
    offset_ptr<smap_value_t> val = smap_get_item(self->sm, strkey);
    //std::cout << "  get_item variant " << val <<"|" << val.get() << "\n";
    PyObject* obj = PyObject_from_variant(val);
    if (obj) {
      Py_INCREF(obj);
    }
    //std::cout << "  get_item returning " << obj << "\n";
    return obj;
  } catch(...) { //out of range
    PyErr_SetString(PyExc_KeyError,strkey);
    return NULL;
  }
  Py_INCREF(Py_None);
  return Py_None;
}

int
populate_smap_with_dict(offset_ptr<smap> entry, PyObject* val, visited_map_t visited) {
  PyObject *key, *value;
  Py_ssize_t pos = 0;
  int ret;
  visited[val] = entry;
  //std::cout << "populate_smap_with_dict\n";
  while (PyDict_Next(val, &pos, &key, &value)) {
    //std::cout << "populate_smap_with_dict:: rec_storing...\n";
    if ((ret = rec_store_item(entry, key, value, visited)) != 0) {
      return ret;
    }
  }
  return 0;
}

PyObject*
SDict_Iter_iternext(PyObject *_self) {
  //std::cout << "SDict_Iter_iternext\n";
  SDict_Iter* self = (SDict_Iter*)_self;

  if (self->current == self->collection->sm->end()) {
    PyErr_SetNone(PyExc_StopIteration);
    //std::cout << "  SDict_Iter_iternext: StopIteration\n";
    return NULL;
  } else {
    const char* strkey = self->current->first.c_str();
    offset_ptr<smap_value_t> val = self->current->second;
    //std::cout << "  SDict_Iter_iternext: iter variant for " << self->current->first << ": " << val << "|" << val.get() << "\n";
    //std::cout << "  SDict_Iter_iternext: sneaking .d: " << val->d << "|" << val->d.get() << "tag: " << val->tag <<"\n";
    //std::cout << "  SDict_Iter_iternext: sneaking ._odict_: " << val->_odict_ << "|" << val->_odict_.get() <<"\n";
    PyObject* tp;
    if ((tp= PyTuple_New(2)) == NULL) {
      return NULL;
    }
    //std::cout << "  SDict_Iter_iternext: created tuple. seting key: " << strkey << "\n";

    if (PyTuple_SetItem(tp, 0, PyString_FromString(strkey)) != 0) {
      return NULL;
    }
    //std::cout << "  SDict_Iter_iternext: created tuple, getting obj from variant\n";
    PyObject* obj;
    if ((obj = PyObject_from_variant(val)) == NULL) {
      //std::cout << "  SDict_Iter_iternext: from variant is NULL!!!\n";
      return NULL;
    }
    Py_INCREF(obj);
    //std::cout << "  SDict_Iter_iternext: add to tuple object: " << obj << "\n";
    if (PyTuple_SetItem(tp, 1, obj) != 0) {
        return NULL;
    }
    //std::cout << "  SDict_Iter_iternext: created set tuple value\n";
    self->current++;
    //std::cout << "  SDict_Iter_iternext: returning tp\n";
    return tp;
  }
}

PyObject*
SDict_iteritems(PyObject* self, PyObject* args) {
  SDict_Iter* iterator = (SDict_Iter*)  PyObject_New(SDict_Iter, &SDict_IterType);
  if (!iterator) return NULL;
  if (!PyObject_Init((PyObject *)iterator, &SDict_IterType)) {
    Py_DECREF(iterator);
    return NULL;
  }

  iterator->collection = (SDict*) self;
  iterator->current = iterator->collection->sm->begin();
  Py_INCREF(iterator);
  return (PyObject*) iterator;
}

PyObject*
SDict_items(PyObject* _self, PyObject* args) {
  SDict* self = (SDict*) _self;

  PyObject *lst;
  PyObject *tpl;
  PyObject *py_key;
  PyObject *py_val;

  if ((lst = PyList_New(self->sm->size())) == NULL) {
    return NULL;
  }
  int i = 0;
  for (smap::iterator it = self->sm->begin();
       it != self->sm->end(); i++, it++) {
    if ((tpl = PyTuple_New(2)) == NULL) {
      return NULL;
    }
    if ((py_key = PyString_FromString(it->first.c_str())) == NULL) {
      return NULL;
    }
    if (PyTuple_SetItem(tpl, 0, py_key) != 0) {
      return NULL;
    }

    offset_ptr<smap_value_t> sdval = smap_get_item(self->sm, it->first.c_str());
    if ((py_val = PyObject_from_variant(sdval)) == NULL) {
      return NULL;
    }
    if (PyTuple_SetItem(tpl, 1, py_val) != 0) {
      return NULL;
    }
    if (PyList_SetItem(lst, i, tpl) == -1) {
      return NULL;
    }
  }
  Py_INCREF(lst);
  return lst;
}

PyObject*
SDict_keys(PyObject* _self, PyObject* args) {
  SDict* self = (SDict*) _self;

  PyObject *lst;
  PyObject *py_key;

  if ((lst = PyList_New(self->sm->size())) == NULL) {
    return NULL;
  }
  int i = 0;
  for (smap::iterator it = self->sm->begin();
       it != self->sm->end(); i++, it++) {
    if ((py_key = PyString_FromString(it->first.c_str())) == NULL) {
      return NULL;
    }
    if (PyList_SetItem(lst, i, py_key) == -1) {
      return NULL;
    }
  }
  Py_INCREF(lst);
  return lst;
}

static PyObject*
SDict_update(PyObject* _self, PyObject* args) {
  PyObject *arg;
  if (!PyArg_UnpackTuple(args, "update", 1, 1, &arg)) {
    PyErr_SetString(PyExc_TypeError,"expected one argument");
    return NULL;
  }

  if (!PyDict_CheckExact(arg) && !PyObject_TypeCheck(arg, &SDictType)) {
    PyErr_SetString(PyExc_TypeError,"expected a dict or dshared.dict");
    return NULL;
  }

  SDict* self = (SDict*) _self;
  if (PyDict_CheckExact(arg)) {
    if (populate_smap_with_dict(self->sm, arg) != 0) {
      return NULL;
    }
  } else { //SDict
    SDict* other = (SDict*) arg;
    for (smap::iterator it = other->sm->begin();
         it != other->sm->end(); it++) {
      PyObject* py_key;
      if ((py_key = PyString_FromString(it->first.c_str())) == NULL) {
        return NULL;
      }

      PyObject* val;
      if ((val = SDict_get_item(arg, py_key)) == NULL) {
        return NULL;
      }
      do_rec_store_item(self->sm, it->first.c_str(), val);
    }
  }
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject*
SDict_values(PyObject* _self, PyObject* args) {
  //std::cout << "\n\n\n\n++++++++++>>>>>>>>>>>>>>>>>>>>>>> SDict_values\n";
  SDict* self = (SDict*) _self;

  PyObject *lst;
  PyObject *py_val;


  // SDict* obj = (SDict*) PyObject_CallObject((PyObject *) &SDictType, PyTuple_New(0));

  if ((lst = PyList_New(self->sm->size())) == NULL) {
    return NULL;
  }
  int i = 0;
  for (smap::iterator it = self->sm->begin();
       it != self->sm->end(); i++, it++) {
    //std::cout << "TOP FOR : " << self << " | " << self->sm << " -- size: " << self->sm->size() << "\n";
    offset_ptr<smap_value_t> sdval = it->second;

    //std::cout << "variant " << i << " is " << sdval << "\n";
    //offset_ptr<smap_value_t> sdval2 = smap_get_item(self->sm, it->first.c_str());
    //std::cout << "variant2 " << i << " is " << sdval2 << "\n";
    if ((py_val = PyObject_from_variant(sdval)) == NULL) {
      //std::cout << "error getting py from variant\n";
      return NULL;
    }
    if (PyList_SetItem(lst, i, py_val) == -1) {
      //std::cout << "error setting py to result list\n";
      return NULL;
    }
  }
  //std::cout << "returning list\n";
  Py_INCREF(lst);
  return lst;
}

PyObject*
SDict_iter(PyObject* self) {
  //std::cout << "smap::__iter__\n";
  SDict_Iter* iterator = (SDict_Iter*)  PyObject_New(SDict_Iter, &SDict_IterType);
  if (!iterator) return NULL;
  //std::cout << "initing iter\n";
  if (!PyObject_Init((PyObject *)iterator, &SDict_IterType)) {
    Py_DECREF(iterator);
    return NULL;
  }

  //std::cout << "setting col\n";
  iterator->collection = (SDict*) self;
  iterator->current = iterator->collection->sm->begin();
  //std::cout << "returning iter with items: " << iterator->collection->sm->size() << "\n";
  Py_INCREF(iterator);
  return (PyObject*) iterator;
}

PyObject*
SDict_append(PyObject* _self, PyObject* args) {
  SDict* self = (SDict*) _self;

  PyObject *arg;
  if (!PyArg_UnpackTuple(args, "append", 1, 1, &arg)) {
    PyErr_SetString(PyExc_TypeError,"expected one argument");
    return NULL;
  }

  int size = self->sm->size();
  std::stringstream s;
  s << size;
  if (rec_store_item(self->sm, PyString_FromString(s.str().c_str()), arg) != 0) {
    return NULL;
  }
  Py_INCREF(Py_None);
  return Py_None;
}


PyObject*
SDict_str(PyObject *self) {
  std::stringstream s;
  s << "<dshared.dict" << " object at " << self << ">";
  return PyString_FromString(s.str().c_str());
}

int
SDict_print(PyObject *self, FILE *file, int flags) {
  std::stringstream s;
  s << "<dshared.dict" << " object at " << self << ">";
  fputs(s.str().c_str(), file);
  return 0;
}

Py_ssize_t
SDict_len(PyObject* self) {
  return ((SDict*)self)->sm->size();
}




int
SDict_init(SDict *self, PyObject *args, PyObject *kwds) {
  Py_INCREF(self);
  if (manager == NULL) {
    PyErr_SetString(PyExc_RuntimeError,"init() was not called");
    return -1;
  }

  PyObject *arg;
  if (PyTuple_Size(args) == 0) {
    arg = PyDict_New();
  } else {
    if (!PyArg_UnpackTuple(args, "__init__", 1, 1, &arg)) {
      PyErr_SetString(PyExc_TypeError,"expected one argument");
      return -1;
    }
  }

  if (PyDict_CheckExact(arg)) {
    self->sm = manager->create_smap();
    if (populate_smap_with_dict(self->sm, arg) != 0) {
      return -1;
    }
  } else if (PyObject_TypeCheck(arg, &SDictType)) {
    self->sm = ((SDict*)arg)->sm;
  } else {
    PyErr_SetString(PyExc_TypeError,"expected a dict or dshared.dict");
    return -1;
  }

  if (PyDict_Type.tp_init((PyObject *)self, PyTuple_New(0), PyDict_New()) < 0) {
    return -1;
  }
  return 0;
}

PyObject*
SDict_create_dict(offset_ptr<smap_value_t> variant) {
  //std::cout << "SDict_create_dict:: for variant: " << variant << "|" << variant.get() << "\n";
  SDict* obj = (SDict*) PyObject_CallObject((PyObject *) &SDictType, PyTuple_New(0));

  //std::cout << "SDict_create_dict:: created dict pyobject : " << obj << ". Caching (and crashing?)\n";
  variant->cache_obj(obj);
  //std::cout << "SUCC. SDict_create_dict:: setting pyobj-sd: " << variant->d << "|" << variant->d.get() << "\n";
  //obj->sm = offset_ptr<smap>((smap*)variant->d.get());

  //std::cout << "CAST[before]  >>>" << variant->d << "|" << variant->d.get() << "\n";
  obj->sm = offset_ptr<smap>(static_cast<smap*>(variant->d.get()));
  //std::cout << "CAST :: >>>>>> : " << obj->sm <<"|"<< obj->sm.get() << "\n";
  return (PyObject*) obj;
}

PyObject*
SDict_create_obj(offset_ptr<smap_value_t> variant) {
  //std::cout << "SDict_create_obj:: creating py _dict_\n";
  SDict* _dict_;
  if ((_dict_ = (SDict*) PyObject_CallObject((PyObject *) &SDictType, PyTuple_New(0))) == NULL) {
    //std::cout << "  UOPS, could not create a SDict for obj.__dict__\n";
    return NULL;
  }

  //std::cout << "  setting _dict_'s sd to stored __dict__: " << variant->_odict_ <<"|"<<variant->_odict_.get() << "\n";
  //std::cout << ".CAST[before]  >>>" << variant->_odict_ << "|" << variant->_odict_.get() << "\n";
  _dict_->sm = offset_ptr<smap>(static_cast<smap*>(variant->_odict_.get()));
  _dict_->sm = offset_ptr<smap>((smap*)variant->_odict_.get());
  //std::cout << ".CAST  >>>" << _dict_->sm << "|" << _dict_->sm.get() << "\n";

  PyObject* klass = (PyObject*)variant->pyclass;

  //std::cout << "  creating 'proxy' instance for pyclass: " << klass << "\n";
  PyObject* obj;
  if (PyClass_Check(klass)) { //old style class
    //std::cout << "  creating old-style raw\n";
    if ((obj = PyInstance_NewRaw(klass, (PyObject*)_dict_)) == NULL) {
      //std::cout << "  ERROR creating proxy for old-style class\n";
      return NULL;
    }
  } else { //new style class
    PyObject* __new__;
    if ((__new__ = PyObject_GetAttrString(klass, "__new__")) == NULL) {
      //std::cout << "  ERROR getting __new__\n";
      return NULL;
    }
    PyObject* arg;
    if ((arg= PyTuple_New(1)) == NULL) {
      return NULL;
    }
    if (PyTuple_SetItem(arg, 0, klass) != 0) {
      return NULL;
    }

    if ((obj = PyObject_CallObject(__new__, arg)) == NULL) {
      //std::cout << "  ERROR creating proxy instance for new-style class " << variant->pyclass << "\n";
      return NULL;
    }
  }
  //std::cout << "  setting __dict__\n";
  if (PyObject_SetAttrString(obj, "__dict__", (PyObject*)_dict_) != 0) {
    //std::cout << "  ERROR setting __dict__ of new-style instance\n";
    return NULL;
  }
  //std::cout << "  setting __class__\n";
  if (PyObject_SetAttrString(obj, "__class__", klass) != 0) {
    //std::cout << "  ERROR setting __dict__ of new-style instance\n";
    return NULL;
  }

  variant->cache_obj(obj);
  return (PyObject*) obj;
}

int
rec_store_item(offset_ptr<smap> sd, PyObject* key, PyObject* val, visited_map_t  visited) {
  const char* strkey;
  if (PyString_Check(key)) {
    strkey = PyString_AsString(key);
  } else if (PyUnicode_Check(key)) {
    strkey = py_unicode_to_string(key);
  } else {
    PyErr_SetString(PyExc_TypeError,"only strings can index smap");
    return 1;
  }
  return do_rec_store_item(sd, strkey, val, visited);
}

PyObject*
SDict_rich_compare(PyObject *self, PyObject *other, int op) {
  //std::cout << "SDict_rich_compare: " << self << ":" << other << "\n";
  if (op != Py_EQ && op != Py_NE) {
    PyErr_SetString(PyExc_TypeError," TODO: comparision operator not supported");
    return NULL;  //I guess it should be Py_NotImplemented, so we don't break things up
  }

  bool val = (self == other && op == Py_EQ) ||
    (self != other && op == Py_NE);

  if (val) {
    Py_INCREF(Py_True);
    return Py_True;
  } else {
    Py_INCREF(Py_False);
    return Py_False;
  }
}



/** END: dshared.dict  **/



int
rec_store_item(offset_ptr<smap> sd, Py_ssize_t key, PyObject* val, visited_map_t  visited) {
  std::stringstream s;
  s << key;

  return do_rec_store_item(sd, s.str().c_str(), val, visited);
}

int
do_rec_store_item(offset_ptr<smap> sd, const char* strkey, PyObject* val, visited_map_t  visited) {
  //std::cout << "rec_store_item " << strkey << "\n";
  if (val == 0) { // delete item
    if (!smap_has_item(sd, strkey)) {
      PyErr_SetString(PyExc_TypeError,strkey);
      return 1;
    } else {
      smap_delete_item(sd, strkey);
      return 0;
    }
  } else if (val == Py_None) {
    smap_set_null_item(sd, strkey);
    return 0;
  } else if (PyBool_Check(val)) {
    smap_set_bool_item(sd, strkey, val == Py_True);
    return 0;
  } else if (PyString_Check(val)) {
    smap_set_string_item(sd, strkey, PyString_AsString(val));
    return 0;
  } else if (PyUnicode_Check(val)) {
    smap_set_string_item(sd, strkey, py_unicode_to_string(val));
    return 0;
  } else if (PyInt_Check(val) || PyLong_Check(val)) {
    long r = PyInt_AsLong(val);
    if (PyErr_Occurred()) {
      return 1;
    }
    smap_set_number_item(sd, strkey, r);
    return 0;
  } else if (PyList_CheckExact(val)) {
    //std::cout << "rec_store_item:: exact py list() " << strkey << "\n";
    visited_map_t::iterator it = visited.find(val);
    if (it != visited.end()) { //recursive structure
      smap_set_sdict_item(sd, strkey, it->second);
      return 0;
    } else {
      offset_ptr<smap> entry = manager->create_smap();
      //std::cout << "rec_store_item:: created SDICT: " << entry << "|" << entry.get() << "\n";

      int ret;
      if ((ret = populate_smap_with_list(entry, val, visited)) != 0) {
        return ret;
      }
      //std::cout << "rec_store_item:: slist: populated it, storing...\n";
      smap_set_slist_item(sd, strkey, entry);
      return 0;
    }
  } else if (PyDict_CheckExact(val)) {
    //std::cout << "rec_store_item::exact py dict() " << strkey << "\n";
    visited_map_t::iterator it = visited.find(val);
    if (it != visited.end()) { //recursive structure
      smap_set_sdict_item(sd, strkey, it->second);
      return 0;
    } else {
      offset_ptr<smap> entry = manager->create_smap();
      //std::cout << "rec_store_item:: created smap: " << entry << "|" << entry.get() << "\n";

      int ret;
      if ((ret = populate_smap_with_dict(entry, val, visited)) != 0) {
        return ret;
      }
      //std::cout << "rec_store_item:: populated it, storing...\n";
      smap_set_sdict_item(sd, strkey, entry);
      return 0;
    }
  } else if (PyObject_TypeCheck(val, &SDictType)) {
    //std::cout << "rec_store_item:: slist " << val << " \n";
    offset_ptr<smap> other = ((SDict*)val)->sm;
    smap_set_sdict_item(sd, strkey, other, val);
    return 0;
  } else if (PyObject_TypeCheck(val, &SListType)) {
    //std::cout << "rec_store_item:: slist\n";
    offset_ptr<smap> other = ((SList*)val)->sm;
    smap_set_slist_item(sd, strkey, other, val);
    return 0;
  } else if (PyObject_HasAttrString(val, "__class__")) {
    //std::cout << "SETTING OBJECT " << strkey << ": " << val << "\n";
    PyObject* _dict_;
    if ((_dict_ = PyObject_GetAttrString(val, "__dict__")) == NULL) {
      return 1;
    }
    //std::cout << "  OBJ: the __dict__: " << _dict_ << "\n";


    PyObject* pyclass;
    if ((pyclass = PyObject_GetAttrString(val, "__class__")) == NULL) {
      return 1;
    }
    //std::cout << "  OBJ: the  __class__ is " << pyclass << "\n";

    Py_INCREF(val);
    //std::cout << &SDictType << "::" << PyObject_Type(_dict_) << "\n";
    if (PyObject_TypeCheck(_dict_, &SDictType)) { // blessed
      //std::cout << "  OBJ: is SHARED already. Just set sd on our dict: " << ((SDict*)_dict_)->sm << "\n";
      smap_set_obj_item(sd, strkey,  ((SDict*)_dict_)->sm, pyclass, val);
      return 0;
    } else { //not blessed
      //std::cout << "  OBJ: is NOT shared. Creating SDict\n";
      SDict* new_obj_dict;
      if ((new_obj_dict = (SDict*)PyObject_CallObject((PyObject *) &SDictType, PyTuple_New(0))) == NULL) {
        return 1;
      }
      int ret;
      //std::cout << "  OBJ: populating our blessed __dict__: " << (void*)new_obj_dict <<"\n";
      if ((ret = populate_smap_with_dict(new_obj_dict->sm, _dict_)) != 0) {
        return ret;
      }

      //std::cout << "  OBJ: setting new smap as __dict__\n";
      if (PyObject_SetAttrString(val, "__dict__", (PyObject*)new_obj_dict) != 0) {
        return 1;
      }

      //std::cout << "  OBJ: the __dict__'s sd " << new_obj_dict->sm << " will be set to our entry: " << strkey << "\n";
      smap_set_obj_item(sd, strkey, new_obj_dict->sm, pyclass, val);
      return 0;
    }
  } else {
    PyErr_SetString(PyExc_TypeError,"value type not supported");
    return 1;
  }
  PyErr_SetString(PyExc_TypeError,"object not supported");
  return 1;
}

/** dshared functions  **/

static PyObject *
dshared_init(PyObject *self, PyObject *args) {
  if (manager != NULL) {
    PyErr_SetString(PyExc_RuntimeError,"init() already called");
    return NULL;
  }

  PyObject *py_name = NULL;
  PyObject *py_size = NULL;
  if (!PyArg_UnpackTuple(args, "init", 2, 2, &py_name, &py_size)) {
    PyErr_SetString(PyExc_TypeError,"expected (string,int) args ");
    return NULL;
  }
  if (!PyString_Check(py_name)) {
    PyErr_SetString(PyExc_TypeError,"expected string as first argument");
    return NULL;
  }

  if (!PyInt_Check(py_size) && !PyLong_Check(py_size)) {
    PyErr_SetString(PyExc_TypeError,"expected integer as second argument");
    return NULL;
  }

  PY_LONG_LONG size = PyLong_AsLongLong(py_size);
  const char* name = PyString_AsString(py_name);

  try {
    shared_memory_object::remove(name);
    manager = new MManager(name, size);
  } catch(...) {
    PyErr_SetString(PyExc_MemoryError,"error initializing shared memory");
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

static PyMethodDef
DSharedMethods[] = {
  {"init",  dshared_init, METH_VARARGS,
   "Initialize shared memory."},
  {NULL, NULL, 0, NULL}        /* Sentinel */
};



PyMODINIT_FUNC
initdshared(void)
{
  // SListType.tp_base = &PyList_Type;
  SListType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&SListType) < 0)
    return;

  SDictType.tp_base = &PyDict_Type;   //SDict has to be a dict
  if (PyType_Ready(&SDictType) < 0)   //so python doesn't complain
    return;                           //when we obj.__class__ = <SDict>

  PyObject *m = Py_InitModule("dshared", DSharedMethods);

  if (m == NULL)
    return;

  Py_INCREF(&SListType);
  PyModule_AddObject(m, "list", (PyObject *) &SListType);
  Py_INCREF(&SDictType);
  PyModule_AddObject(m, "dict", (PyObject *) &SDictType);
}
