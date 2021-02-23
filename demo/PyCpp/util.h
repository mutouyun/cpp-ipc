#include<python3.6/Python.h>

namespace ipc {
	PyObject * PyByteArray_FromString_WithoutCopy(char *bytes, Py_ssize_t size);
}
