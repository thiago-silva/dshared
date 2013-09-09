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

/** dshared.List  **/

typedef struct {
    PyListObject list;
    int state;
} SList;

static PyObject *
SList_increment(SList *self, PyObject *unused) {
    self->state++;
    return PyInt_FromLong(self->state);
}

static int
SList_init(SList *self, PyObject *args, PyObject *kwds) {
    if (PyList_Type.tp_init((PyObject *)self, args, kwds) < 0)
        return -1;
    self->state = 0;
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
    int state;
} SDict;

static PyObject *
SDict_increment(SDict *self, PyObject *unused)
{
    self->state++;
    return PyInt_FromLong(self->state);
}


static int
SDict_init(SDict *self, PyObject *args, PyObject *kwds)
{
    if (PyList_Type.tp_init((PyObject *)self, args, kwds) < 0)
        return -1;
    self->state = 0;
    return 0;
}

static PyMethodDef
SDict_methods[] = {
    {"increment", (PyCFunction)SDict_increment, METH_NOARGS,
     PyDoc_STR("increment state counter")},
    {NULL, NULL},
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


/** END: dshared.Dict  **/

/** dshared functions  **/

static PyObject *
dshared_init(PyObject *self, PyObject *size)
{
  if (!PyInt_Check(size)) {
    PyErr_SetString(PyExc_TypeError,"expected a number");
    return NULL;
  }
  long value = PyLong_AsLong(size);
  //...
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
dshared_dict(PyObject *self, PyObject *dict)
{
  if (!PyDict_Check(dict)) {
    PyErr_SetString(PyExc_TypeError,"expected a dictionary");
    return NULL;
  }

  Py_INCREF(dict);
  return dict;
}

static PyMethodDef
DSharedMethods[] = {
  {"init",  dshared_init, METH_O,
   "Initialize shared memory."},
  {"dict",  dshared_dict, METH_O,
   "create a shared dict."},
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

  Py_INCREF(&SListType);
  PyModule_AddObject(m, "List", (PyObject *) &SListType);

  Py_INCREF(&SDictType);
  PyModule_AddObject(m, "Dict", (PyObject *) &SDictType);
}
