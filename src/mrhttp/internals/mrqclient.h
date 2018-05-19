#pragma once

#include <Python.h>
#include <stdbool.h>

typedef struct {
  PyObject_HEAD

} MrqClient;

PyObject *MrqClient_new    (PyTypeObject* self, PyObject *args, PyObject *kwargs);
int       MrqClient_init   (MrqClient* self,    PyObject *args, PyObject *kwargs);
void      MrqClient_dealloc(MrqClient* self);

PyObject *MrqClient_cinit(MrqClient* self);
void MrqClient_push(MrqClient* self);
