
#include <Python.h>

PyObject* myrandint(PyObject* self, PyObject* args)
{
  int min, max;
  if (!PyArg_ParseTuple(args, "ii", &min, &max)) return NULL;
  return PyLong_FromLong(min + (rand() / (RAND_MAX / (max + 1) + 1)));
}

