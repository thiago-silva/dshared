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

// typedef struct {
//   PyListObject list;
//   offset_ptr<sdict> d;
// } SList;

// static PyObject *
// SList_increment(SList *self, PyObject *unused) {
//     return PyInt_FromLong(1);
// }

// static int
// SList_init(SList *self, PyObject *args, PyObject *kwds) {
//   if (PyList_Type.tp_init((PyObject *)self, args, kwds) < 0)
//     return -1;
//   return 0;
// }

// static PyMethodDef
// SList_methods[] = {
//     {"increment", (PyCFunction)SList_increment, METH_NOARGS,
//      PyDoc_STR("increment state counter")},
//     {NULL, NULL},
// };


// static PyTypeObject
// SListType = {
//     PyObject_HEAD_INIT(NULL)
//     0,                       /* ob_size */
//     "dshared.List",          /* tp_name */
//     sizeof(SList),           /* tp_basicsize */
//     0,                       /* tp_itemsize */
//     0,                       /* tp_dealloc */
//     0,                       /* tp_print */
//     0,                       /* tp_getattr */
//     0,                       /* tp_setattr */
//     0,                       /* tp_compare */
//     0,                       /* tp_repr */
//     0,                       /* tp_as_number */
//     0,                       /* tp_as_sequence */
//     0,                       /* tp_as_mapping */
//     0,                       /* tp_hash */
//     0,                       /* tp_call */
//     0,                       /* tp_str */
//     0,                       /* tp_getattro */
//     0,                       /* tp_setattro */
//     0,                       /* tp_as_buffer */
//     Py_TPFLAGS_DEFAULT |
//       Py_TPFLAGS_BASETYPE,   /* tp_flags */
//     0,                       /* tp_doc */
//     0,                       /* tp_traverse */
//     0,                       /* tp_clear */
//     0,                       /* tp_richcompare */
//     0,                       /* tp_weaklistoffset */
//     0,                       /* tp_iter */
//     0,                       /* tp_iternext */
//     SList_methods,           /* tp_methods */
//     0,                       /* tp_members */
//     0,                       /* tp_getset */
//     0,                       /* tp_base */
//     0,                       /* tp_dict */
//     0,                       /* tp_descr_get */
//     0,                       /* tp_descr_set */
//     0,                       /* tp_dictoffset */
//     (initproc)SList_init,    /* tp_init */
//     0,                       /* tp_alloc */
//     0,                       /* tp_new */
// };


/** END: dshared.List  **/


/** dshared.Dict  **/

typedef struct {
  PyDictObject dict;
  offset_ptr<sdict> sd;
} SDict;

static PyObject*
SDict_create_dict(offset_ptr<sdict_value_t>);

static PyObject*
SDict_create_obj(offset_ptr<sdict_value_t>);

static PyObject*
SDict_for_sdict(offset_ptr<sdict_value_t> val) {
  //std::cout << "SDict_for_sdict\n";
  //std::cout << "   SDict_for_sdict: has in cache?" << val->has_cache() << "\n";
  PyObject* obj;
  if (val->has_cache()) {
    obj = (PyObject*)val->cache();
    //std::cout << "   SDict_for_sdict: cached obj: " << obj << "\n";
  } else {
    //std::cout << "  SDict_for_sdict: creating pydict...\n";
    obj = SDict_create_dict(val);
  }
  Py_INCREF(obj);
  return obj;
}

static PyObject*
SDict_for_pyobj(offset_ptr<sdict_value_t> val) {
  //std::cout << "SDict_for_pyobj has cache? " << val->has_cache() << "\n";
  PyObject* obj;
  if (val->has_cache()) {
    obj = (PyObject*)val->cache();
    //std::cout << "  returning cache cache: " << obj << "\n";
  } else {
    //std::cout << "SDict_for_pyobj has cache?no! creating instance\n";
    PyObject* obj = SDict_create_obj(val);
    //std::cout << "SDict_for_pyobj created for this pid: " << obj << "\n";
  }
  Py_INCREF(obj);
  return obj;
}

static PyObject*
PyObject_from_variant(offset_ptr<sdict_value_t> val) {
  switch(val->tag) {
    case sdict_value_t::NIL:
      return Py_None;
    case sdict_value_t::STRING:
      return PyString_FromString(val->str.c_str());
    case sdict_value_t::NUMBER:
      return PyInt_FromLong(val->num);
    case sdict_value_t::SDICT:
      return SDict_for_sdict(val);
    case sdict_value_t::PYOBJ:
      return SDict_for_pyobj(val);
  }
  //std::cout << "WTF: " << val->tag << "\n";
  PyErr_SetString(PyExc_TypeError,"unknown type tag ");
  return NULL;
}

static int
SDict_len(PyObject* self) {
  return ((SDict*)self)->sd->size();
}

int
SDict_contains(PyObject *self, PyObject *value) {
  offset_ptr<sdict> sd = ((SDict*)self)->sd;

  const char* strkey = PyString_AsString(value);
  return sdict_has_item(sd, strkey);
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
    PyObject* obj = PyObject_from_variant(val);
    if (obj == NULL) {
      return NULL;
    }
    return obj;
  } catch(...) { //out of range
    PyErr_SetString(PyExc_KeyError,strkey);
    return NULL;
  }
  Py_INCREF(Py_None);
  return Py_None;
}

typedef std::map<PyObject*, offset_ptr<sdict> > visited_map_t;

static int
rec_store_item(offset_ptr<sdict> sd, PyObject* key,
               PyObject* val, visited_map_t visited = visited_map_t());

static int
populate_sdict(offset_ptr<sdict> entry, PyObject* val, visited_map_t visited = visited_map_t()) {
  PyObject *key, *value;
  Py_ssize_t pos = 0;
  int ret;
  visited[val] = entry;
  while (PyDict_Next(val, &pos, &key, &value)) {
    if ((ret = rec_store_item(entry, key, value, visited)) != 0) {
      return ret;
    }
  }
  return 0;
}

static int
SDict_set_item(PyObject* _self, PyObject* key, PyObject* val) {
  SDict* self = (SDict*) _self;
  return rec_store_item(self->sd, key, val);
}




typedef struct {
  PyObject_HEAD
  SDict* collection;
  sdict::iterator current;
} SDict_Iter;

PyObject*
SDict_Iter_iternext(PyObject *_self) {
  //std::cout << "SDict_Iter_iternext\n";
  SDict_Iter* self = (SDict_Iter*)_self;

  if (self->current == self->collection->sd->end()) {
    PyErr_SetNone(PyExc_StopIteration);
    return NULL;
  } else {
    const char* strkey = self->current->first.c_str();
    offset_ptr<sdict_value_t> val = self->current->second;
    //std::cout << "  SDict_Iter_iternext: iter variant for " << self->current->first << ": " << val << "|" << val.get() << "\n";
    //std::cout << "  SDict_Iter_iternext: sneaking .d: " << val->d << "|" << val->d.get() << "tag: " << val->tag <<"\n";
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
  Py_INCREF(iterator);
  return (PyObject*) iterator;
}

static PyObject*
SDict_items(PyObject* _self, PyObject* args) {
  SDict* self = (SDict*) _self;

  PyObject *lst;
  PyObject *tpl;
  PyObject *py_key;
  PyObject *py_val;

  if ((lst = PyList_New(self->sd->size())) == NULL) {
    return NULL;
  }
  int i = 0;
  for (sdict::iterator it = self->sd->begin();
       it != self->sd->end(); i++, it++) {
    if ((tpl = PyTuple_New(2)) == NULL) {
      return NULL;
    }
    if ((py_key = PyString_FromString(it->first.c_str())) == NULL) {
      return NULL;
    }
    if (PyTuple_SetItem(tpl, 0, py_key) != 0) {
      return NULL;
    }

    offset_ptr<sdict_value_t> sdval = sdict_get_item(self->sd, it->first.c_str());
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

static int
SDict_init(SDict *self, PyObject *args, PyObject *kwds);


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
    {"items", (PyCFunction)SDict_items, METH_NOARGS,
     PyDoc_STR("dict items")},
    {NULL, NULL},
};

static PySequenceMethods
sdict_seq = {
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
    &sdict_seq,              /* tp_as_sequence */
    &sdict_mapping,          /* tp_as_mapping */
    0,                       /* tp_hash */
    0,                       /* tp_call */
    SDict_str,               /* tp_str */
    0,                       /* tp_getattro */
    0,                       /* tp_setattro */
    0,                       /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
      Py_TPFLAGS_HAVE_SEQUENCE_IN, /* tp_flags */
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


int
SDict_init(SDict *self, PyObject *args, PyObject *kwds) {
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
    self->sd = manager->create_sdict();
    if (populate_sdict(self->sd, arg) != 0) {
      return -1;
    }
  } else if (PyObject_TypeCheck(arg, &SDictType)) {
    self->sd = ((SDict*)arg)->sd;
  } else {
    PyErr_SetString(PyExc_TypeError,"expected a dict or dshared.dict");
    return -1;
  }

  if (PyDict_Type.tp_init((PyObject *)self, PyTuple_New(0), PyDict_New()) < 0) {
    return -1;
  }
  return 0;
}

static PyObject*
SDict_create_dict(offset_ptr<sdict_value_t> variant) {
  //std::cout << "SDict_create_dict:: for variant: " << variant << "|" << variant.get() << "\n";
  SDict* obj = (SDict*) PyObject_CallObject((PyObject *) &SDictType, PyTuple_New(0));

  //std::cout << "SDict_create_dict:: created dict pyobject : " << obj << ". Caching (and crashing?)\n";
  variant->cache_obj(obj);
  //std::cout << "SUCC. SDict_create_dict:: setting pyobj-sd: " << variant->d << "|" << variant->d.get() << "\n";
  //obj->sd = offset_ptr<sdict>((sdict*)variant->d.get());

  obj->sd = offset_ptr<sdict>(static_cast<sdict*>(variant->d.get()));

  //std::cout << ".SDict_create_dict:: >>>>>> : " << obj->sd <<"|"<< obj->sd.get() << "\n";
  return (PyObject*) obj;
}

static PyObject*
SDict_create_obj(offset_ptr<sdict_value_t> variant) {
  //std::cout << "SDict_create_obj:: creating Py _dict_\n";
  SDict* _dict_;
  if ((_dict_ = (SDict*) PyObject_CallObject((PyObject *) &SDictType, PyTuple_New(0))) == NULL) {
    //std::cout << "  UOPS, could not create a SDict for obj.__dict__\n";
    return NULL;
  }

  //std::cout << "  setting _dict_'s sd to stored __dict__: " << variant->_odict_ <<"|"<<variant->_odict_.get() << "\n";
  _dict_->sd = offset_ptr<sdict>((sdict*)variant->_odict_.get());
  //std::cout << "  >>>" << _dict_->sd << "|" << _dict_->sd.get() << "\n";

  //std::cout << "  creating 'proxy' instance for pyclass: " << variant->pyclass << "\n";
  PyObject* obj;
  if ((obj = PyObject_CallObject((PyObject*)variant->pyclass, PyTuple_New(0))) == NULL) {
    //std::cout << "  ERROR creating proxy instance\n";
    return NULL;
  }
  //std::cout << "  setting 'proxy's __dict__\n";
  if (PyObject_SetAttrString(obj, "__dict__", (PyObject*)_dict_) == -1) {
    //std::cout << "  ERROR setting __dict__\n";
    return NULL;
  }

  variant->cache_obj(obj);
  return (PyObject*) obj;
}

int
rec_store_item(offset_ptr<sdict> sd, PyObject* key, PyObject* val, visited_map_t  visited) {
  if (!PyString_Check(key)) {
    PyErr_SetString(PyExc_TypeError,"only strings can index sdict");
    return 1;
  }
  const char* strkey = PyString_AsString(key);

  if (val == 0) { // delete item
    if (!sdict_has_item(sd, strkey)) {
      PyErr_SetString(PyExc_TypeError,strkey);
      return 1;
    } else {
      sdict_delete_item(sd, strkey);
      return 0;
    }
  } else if (val == Py_None) {
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
    //std::cout << "rec_store_item::dict() " << strkey << "\n";
    visited_map_t::iterator it = visited.find(val);
    if (it != visited.end()) { //recursive structure
      sdict_set_sdict_item(sd, strkey, it->second);
      return 0;
    } else {
      offset_ptr<sdict> entry = manager->create_sdict();
      //std::cout << "rec_store_item:: created sdict: " << entry << "|" << entry.get() << "\n";

      int ret;
      if ((ret = populate_sdict(entry, val, visited)) != 0) {
        return ret;
      }
      //std::cout << "rec_store_item:: populated it, storing...\n";
      sdict_set_sdict_item(sd, strkey, entry);
      return 0;
    }
  } else if (PyObject_TypeCheck(val, &SDictType)) {
    offset_ptr<sdict> other = ((SDict*)val)->sd;
    sdict_set_sdict_item(sd, strkey, other);
    return 0;
  } else if (PyObject_HasAttrString(val, "__class__")) {
    //std::cout << "SETTING OBJECT " << strkey << ": " << val << "\n";
    SDict* _dict_;
    if ((_dict_ = (SDict*)PyObject_GetAttrString(val, "__dict__")) == NULL) {
      return 1;
    }
    //std::cout << "  OBJ: the __dict__: " << _dict_ << "\n";

    PyObject* pyclass;
    if ((pyclass = PyObject_GetAttrString(val, "__class__")) == NULL) {
      return 1;
    }
    //std::cout << "  OBJ: the  __class__ is " << pyclass << "\n";
    Py_INCREF(val);
    if (PyObject_TypeCheck(_dict_, &SDictType)) { // blessed
      //std::cout << "  OBJ: is SHARED already. Just set sd on our dict: " << _dict_->sd << "\n";
      sdict_set_obj_item(sd, strkey, _dict_->sd, pyclass, val);
    } else { //not blessed
      //std::cout << "  OBJ: is NOT shared. Creating SDict\n";
      SDict* new_obj_dict;
      if ((new_obj_dict = (SDict*)PyObject_CallObject((PyObject *) &SDictType, PyTuple_New(0))) == NULL) {
        return 1;
      }
      int ret;
      //std::cout << "  OBJ: populating our blessed __dict__: " << new_obj_dict <<"\n";
      if ((ret = populate_sdict(new_obj_dict->sd, val)) != 0) {
        return ret;
      }
      //std::cout << "  OBJ: setting new sdict as __dict__\n";
      if (PyObject_SetAttrString(val, "__dict__", (PyObject*)new_obj_dict) == -1) {
        return 1;
      }
      //std::cout << "  OBJ: adding sd " << new_obj_dict->sd << " to our dict\n";
      sdict_set_obj_item(sd, strkey, new_obj_dict->sd, pyclass, val);
    }
    return PyObject_SetAttrString(val, "__dict__", PyDict_New());
  } else {
    PyErr_SetString(PyExc_TypeError,"value type not supported");
    return 1;
  }
  return 0;
}

/** END: dshared.Dict  **/

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
  // SListType.tp_base = &PyList_Type;
  // if (PyType_Ready(&SListType) < 0)
  //   return;

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
