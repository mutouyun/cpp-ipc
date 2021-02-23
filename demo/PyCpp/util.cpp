#include "util.h"
namespace ipc {
        
    PyObject *
    PyByteArray_FromString_WithoutCopy(char *bytes, Py_ssize_t size)
    {
        PyByteArrayObject *arrayObject;
        Py_ssize_t alloc;

        if (size < 0) {
            PyErr_SetString(PyExc_SystemError,
                "Negative size passed to PyByteArray_FromStringAndSize");
            return NULL;
        }

        /* Prevent buffer overflow when setting alloc to size+1. */
        if (size == PY_SSIZE_T_MAX) {
            return PyErr_NoMemory();
        }

        arrayObject = PyObject_New(PyByteArrayObject, &PyByteArray_Type);
        if (arrayObject == NULL)
            return NULL;

        if (size == 0) {
            arrayObject->ob_bytes = NULL;
            alloc = 0;
        }
        else {
            alloc = size + 1;
            // arrayObject->ob_bytes = PyObject_Malloc(alloc);
            if (arrayObject->ob_bytes == NULL) {
                Py_DECREF(arrayObject);
                return PyErr_NoMemory();
            }
            arrayObject->ob_bytes = bytes;
        }
        Py_SIZE(arrayObject) = size;
        arrayObject->ob_alloc = alloc;
        arrayObject->ob_start = arrayObject->ob_bytes;
        arrayObject->ob_exports = 0;

        return (PyObject *)arrayObject;
    }       

}
