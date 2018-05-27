#pragma once

#include <Python.h>
#include <stdbool.h>

#include "app.h"

typedef struct {
  PyObject_HEAD

  PyObject *func_expand_requests;
  PyObject *requests;
  //Request **requests;
  int numRequests, nextRequest,freeRequests; 

} MrhttpApp;

PyObject *MrhttpApp_new    (PyTypeObject* self, PyObject *args, PyObject *kwargs);
int       MrhttpApp_init   (MrhttpApp* self,    PyObject *args, PyObject *kwargs);
void      MrhttpApp_dealloc(MrhttpApp* self);

PyObject *MrhttpApp_cinit(MrhttpApp* self);

void MrhttpApp_release_request(MrhttpApp* self, Request *r);
PyObject *MrhttpApp_get_request(MrhttpApp* self);
