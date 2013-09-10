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


/** dshared.List  **/

typedef struct {
  PyListObject list;
  offset_ptr<sdict> d;
} SList;

static PyObject *
SList_increment(SList *self, PyObject *unused) {
    return PyInt_FromLong(1);
}

static int
SList_init(SList *self, PyObject *args, PyObject *kwds) {
  if (PyList_Type.tp_init((PyObject *)self, args, kwds) < 0)
    return -1;
  return 0;
}

static PyMethodDef
SList_methods[] = {
    {"increment", (PyCFunction)SList_increment, METH_NOARGS,
     PyDoc_STR("increment state counter")},
    {NULL, NULL},
};


static PyTypeObject
SListType = {
    PyObject_HEAD_INIT(NULL)
    0,                       /* ob_size */
    "dshared.List",          /* tp_name */
    sizeof(SList),           /* tp_basicsize */
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
      Py_TPFLAGS_BASETYPE,   /* tp_flags */
    0,                       /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
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


/** END: dshared.List  **/


/** dshared.Dict  **/

typedef struct {
  PyDictObject dict;
  sdict* sd;
} SDict;

static PyObject*
SDict_create(offset_ptr<sdict_value_t>);

static PyObject*
SDict_for_sdict(offset_ptr<sdict_value_t> val) {
  if (val->has_pyobj_cache()) {
    return (PyObject*)val->cache();
  } else {
    return SDict_create(val);
  }
}

static PyObject*
PyObject_from_sdict(offset_ptr<sdict_value_t> val) {
  switch(val->tag) {
    case sdict_value_t::NIL:
      Py_INCREF(Py_None);
      return Py_None;
    case sdict_value_t::STRING:
      return PyString_FromString(val->str.c_str());
    case sdict_value_t::NUMBER:
      return PyInt_FromLong(val->num);
    case sdict_value_t::PYDICT:
      return SDict_for_sdict(val);
  }
  PyErr_SetString(PyExc_TypeError,"unknown type tag");
  return NULL;
}

static int
SDict_len(PyObject* self) {
  return ((SDict*)self)->sd->size();
}

static PyObject*
SDict_get_item(PyObject* _self, PyObject* key) {
  SDict* self = (SDict*) _self;

  if (!PyString_Check(key)) {
    PyErr_SetString(PyExc_TypeError,"only strings can index sdict");
    return NULL;
  }

  const char* strkey = PyString_AsString(key);

  try {

    offset_ptr<sdict_value_t> val = sdict_get_item(self->sd, strkey);
    return PyObject_from_sdict(val);
  } catch(...) { //out of range
    PyErr_SetString(PyExc_KeyError,strkey);
    return NULL;
  }
  Py_INCREF(Py_None);
  return Py_None;
}

typedef std::map<PyObject*, sdict*> visited_map_t;

static int
rec_store_item(sdict* sd, const char* strkey,
               PyObject* val, visited_map_t visited = visited_map_t());

static int
SDict_set_item(PyObject* _self, PyObject* key, PyObject* val) {
  SDict* self = (SDict*) _self;
  if (!PyString_Check(key)) {
    PyErr_SetString(PyExc_TypeError,"only strings can index sdict");
    return 1;
  }
  const char* strkey = PyString_AsString(key);
  return rec_store_item(self->sd, strkey, val);
}




typedef struct {
  PyObject_HEAD
  SDict* collection;
  sdict::iterator current;
} SDict_Iter;

PyObject*
SDict_Iter_iternext(PyObject *_self) {
  SDict_Iter* self = (SDict_Iter*)_self;

  if (self->current == self->collection->sd->end()) {
    PyErr_SetNone(PyExc_StopIteration);
    return NULL;
  } else {
    const char* strkey = self->current->first.c_str();
    sdict_value_t* val = (sdict_value_t*) self->current->second.get();
    PyObject* tp = PyTuple_New(2);
    PyTuple_SetItem(tp, 0, PyString_FromString(strkey));
    PyTuple_SetItem(tp, 1, PyObject_from_sdict(val));
    self->current++;
    return tp;
  }
}

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

static PyObject*
SDict_iteritems(PyObject* self, PyObject* args) {
  SDict_Iter* iterator = (SDict_Iter*)  PyObject_New(SDict_Iter, &SDict_IterType);
  if (!iterator) return NULL;
  if (!PyObject_Init((PyObject *)iterator, &SDict_IterType)) {
    Py_DECREF(iterator);
    return NULL;
  }

  iterator->collection = (SDict*) self;
  iterator->current = iterator->collection->sd->begin();
  Py_INCREF(self);
  Py_INCREF(iterator);
  return (PyObject*) iterator;
}

static int
SDict_init(SDict *self, PyObject *args, PyObject *kwds)
{
  if (manager == NULL) {
    PyErr_SetString(PyExc_RuntimeError,"init() was not called");
    return -1;
  }

  PyObject *py_dict;
  if (PyTuple_Size(args) == 0) {
    py_dict = PyDict_New();
  } else {
    if (!PyArg_UnpackTuple(args, "__init__", 1, 1, &py_dict)) {
      PyErr_SetString(PyExc_TypeError,"expected one argument");
      return -1;
    }
  }

  if (!PyDict_Check(py_dict)) {
    PyErr_SetString(PyExc_TypeError,"expected a dictionary");
    return -1;
  }


  if (PyDict_Type.tp_init((PyObject *)self, PyTuple_New(0), PyDict_New()) < 0)
    return -1;

  self->sd = manager->create_sdict();
  return 0;
}


static PyObject*
SDict_iter(PyObject *o) {
  return NULL;
}

static PyMappingMethods
sdict_mapping = {
    SDict_len,
    SDict_get_item,
    SDict_set_item
};

static PyMethodDef
SDict_methods[] = {
    {"iteritems", (PyCFunction)SDict_iteritems, METH_NOARGS,
     PyDoc_STR("iterator for items")},
    {NULL, NULL},
};

static PyTypeObject
SDictType = {
    PyObject_HEAD_INIT(NULL)
    0,                       /* ob_size */
    "dshared.dict",          /* tp_name */
    sizeof(SDict),           /* tp_basicsize */
    0,                       /* tp_itemsize */
    0,                       /* tp_dealloc */
    0,                       /* tp_print */
    0,                       /* tp_getattr */
    0,                       /* tp_setattr */
    0,                       /* tp_compare */
    0,                       /* tp_repr */
    0,                       /* tp_as_number */
    0,                       /* tp_as_sequence */
    &sdict_mapping,          /* tp_as_mapping */
    0,                       /* tp_hash */
    0,                       /* tp_call */
    0,                       /* tp_str */
    0,                       /* tp_getattro */
    0,                       /* tp_setattro */
    0,                       /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
      Py_TPFLAGS_BASETYPE,   /* tp_flags */
    0,                       /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
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


static PyObject*
SDict_create(offset_ptr<sdict_value_t> variant) {
  SDict* obj = (SDict*) PyObject_CallObject((PyObject *) &SDictType, PyTuple_New(0));
  Py_INCREF(obj); //obj should be cached. I'm not sure if an additional is necessary.

  variant.get()->cache_obj(obj);
  obj->sd = (sdict*) variant->d.get();
  return (PyObject*) obj;
}

int
rec_store_item(sdict* sd, const char* strkey, PyObject* val, visited_map_t  visited) {
  if (val == Py_None) {
    sdict_set_null_item(sd, strkey);
    return 0;
  } else if (PyString_Check(val)) {
    sdict_set_string_item(sd, strkey, PyString_AsString(val));
    return 0;
  } else if (PyInt_Check(val)) {
    long r = PyInt_AsLong(val);
    if (PyErr_Occurred()) {
      return 1;
    }
    sdict_set_number_item(sd, strkey, r);
    return 0;
  } else if (PyDict_CheckExact(val)) {
    visited_map_t::iterator it = visited.find(val);
    if (it != visited.end()) { //recursive structure
      sdict_set_sdict_item(sd, strkey, it->second);
      return 0;
    } else {
      PyObject *key, *value;
      Py_ssize_t pos = 0;
      int ret;
      sdict* entry = manager->create_sdict();
      visited[val] = entry;
      while (PyDict_Next(val, &pos, &key, &value)) {
        const char* strkey = PyString_AsString(key);
        if ((ret = rec_store_item(entry, strkey, value, visited)) != 0) {
          return ret;
        }
      }
      sdict_set_sdict_item(sd, strkey, entry);
      return 0;
    }
  } else if (PyObject_TypeCheck(val, &SDictType)) {
    std::cout << "HERE\n";
    sdict* other = ((SDict*)val)->sd;
    sdict_set_sdict_item(sd, strkey, other);
    return 0;
  } else if (1 /*fucking class*/) {
    PyErr_SetString(PyExc_TypeError,"TODO class instance");
    return 1;
  } else {
    PyErr_SetString(PyExc_TypeError,"value type not supported");
    return 1;
  }
  return 0;
}

/** END: dshared.Dict  **/

/** dshared functions  **/

static PyObject *
dshared_init(PyObject *self, PyObject *args)
{
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
  if (!PyInt_Check(py_size)) {
    PyErr_SetString(PyExc_TypeError,"expected integer as second argument");
    return NULL;
  }

  long size = PyLong_AsLong(py_size);
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
  SListType.tp_base = &PyList_Type;
  if (PyType_Ready(&SListType) < 0)
    return;

  SDictType.tp_base = &PyDict_Type;
  if (PyType_Ready(&SDictType) < 0)
    return;

  PyObject *m = Py_InitModule("dshared", DSharedMethods);

  if (m == NULL)
    return;

  // Py_INCREF(&SListType);
  // PyModule_AddObject(m, "List", (PyObject *) &SListType);
  Py_INCREF(&SDictType);
  PyModule_AddObject(m, "dict", (PyObject *) &SDictType);
}
