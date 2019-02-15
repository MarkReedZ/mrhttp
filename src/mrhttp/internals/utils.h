
#pragma once

#include <Python.h>

PyObject* myrandint(PyObject* self, PyObject* args);
PyObject* escape_html(PyObject* self, PyObject* s);

PyObject *to64     (PyObject *self, PyObject *num);
PyObject *from64   (PyObject *self, PyObject *str);
PyObject *timesince(PyObject *self, PyObject *ts);
PyObject *hot      (PyObject *self, PyObject *args);

char *findchar_fast(char *buf, char *buf_end, char *ranges, size_t ranges_size, int *found);

