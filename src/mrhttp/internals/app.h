#pragma once

#include <Python.h>
#include <stdbool.h>

#include "app.h"

typedef struct {
  PyObject_HEAD

  Request **requests;
  int numRequests, nextRequest; 

} MrhttpApp;

PyObject *MrhttpApp_new    (PyTypeObject* self, PyObject *args, PyObject *kwargs);
int       MrhttpApp_init   (MrhttpApp* self,    PyObject *args, PyObject *kwargs);
void      MrhttpApp_dealloc(MrhttpApp* self);

PyObject *MrhttpApp_cinit(MrhttpApp* self);
PyObject *MrhttpApp_get_request(MrhttpApp* self);
