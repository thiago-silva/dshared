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
  offset_ptr<sdict> sd;
} SDict;


PyObject* SDict_new(offset_ptr<sdict>);

static int
SDict_len(PyObject* self) {
  return 0;
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

    offset_ptr<sdict_value_t> val = sdict_get_item(self->sd.get(), strkey);
    switch(val->tag) {
      case sdict_value_t::NIL:
        Py_INCREF(Py_None);
        return Py_None;
      case sdict_value_t::STRING:
        return PyString_FromString(val->str.c_str());
      case sdict_value_t::NUMBER:
        return PyInt_FromLong(val->num);
      case sdict_value_t::PYDICT:
        return SDict_new((sdict*) val->d.get());
    }
    printf("TODO get");
    assert(0);
  } catch(...) { //out of range
    Py_INCREF(Py_None);
    return Py_None;
  }
  Py_INCREF(Py_None);
  return Py_None;
}


int
rec_store_item(sdict* sd, const char* strkey, PyObject* val) {
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
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    int ret;
    sdict* entry = manager->create_sdict();
    while (PyDict_Next(val, &pos, &key, &value)) {
      const char* strkey = PyString_AsString(key);
      if ((ret = rec_store_item(entry, strkey, value)) != 0) {
        return ret;
      }
    }
    sdict_set_sdict_item(sd, strkey, entry);
    return 0;
  } else if (1 /*sdic*/) {
    printf("TODO sdict");
    assert(0);
  } else if (1 /*fucking class*/) {
    printf("TODO class");
    assert(0);
  } else {
    PyErr_SetString(PyExc_TypeError,"value type not supported");
    return 1;
  }
  return 0;
}

int
SDict_set_item(PyObject* _self, PyObject* key, PyObject* val) {
  SDict* self = (SDict*) _self;
  if (!PyString_Check(key)) {
    PyErr_SetString(PyExc_TypeError,"only strings can index sdict");
    return 1;
  }
  const char* strkey = PyString_AsString(key);
  return rec_store_item(self->sd.get(), strkey, val);
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


  if (PyDict_Type.tp_init((PyObject *)self, args, kwds) < 0)
    return -1;

  self->sd = manager->create_sdict();
  return 0;
}

static PyMappingMethods sdict_mapping = {
    SDict_len,
    SDict_get_item,
    SDict_set_item
};


static PyTypeObject
SDictType = {
    PyObject_HEAD_INIT(NULL)
    0,                       /* ob_size */
    "dshared.Dict",          /* tp_name */
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
    0,                       /* tp_methods */
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
SDict_new(offset_ptr<sdict> sd) {
  SDict* raw = (SDict*) PyObject_CallObject((PyObject *) &SDictType, PyTuple_New(0));
  raw->sd = sd;
  return (PyObject*) raw;
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
