#include <Python.h>

static PyObject *
dshared_init(PyObject *self, PyObject *args)
{
    unsigned long size;
    int sts;

    if (!PyArg_ParseTuple(args, "k", &size))
        return NULL;
    return Py_BuildValue("i", size);
}



static PyMethodDef DSharedMethods[] = {
    {"init",  dshared_init, METH_VARARGS,
     "Initialize shared memory."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};



PyMODINIT_FUNC
initdshared(void)
{
    (void) Py_InitModule("dshared", DSharedMethods);
}
