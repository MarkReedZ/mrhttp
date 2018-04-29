#include <Python.h>

#include "cdict.h"

PyObject* cdict_to_dict(CDictEntry* entries, size_t length)
{
  PyObject* d = NULL;
  if(!(d = PyDict_New())) Return NULL;

  for(CDictEntry* entry = entries; entry < entries + length; entry++) {
    PyObject* key = NULL;
    PyObject* value = NULL;

    if(!(key   = PyUnicode_FromStringAndSize(entry->key,   entry->key_length  ))) goto loop_error;
    if(!(value = PyUnicode_FromStringAndSize(entry->value, entry->value_length))) goto loop_error;

    if(PyDict_SetItem(d, key, value) == -1) goto loop_error;

    goto loop_finally;
    loop_error:
    Py_XDECREF(d); d = NULL;

    loop_finally:
    Py_XDECREF(key);
    Py_XDECREF(value);
    if(!d) goto error;
  }

  goto finally;

  error:
  Py_XDECREF(d); d = NULL;

  finally:
  return d;
}
