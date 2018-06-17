#pragma once

#include <Python.h>
#include <stdbool.h>

#include "app.h"

typedef struct {
  PyObject_HEAD
  
  // Idle timeouts
  PyObject *connections;
  PyObject* call_later;
  PyObject* check_idle;
  PyObject* check_idle_handle;
  PyObject* check_interval;
  PyObject* loop;


  // Request pool
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

PyObject *MrhttpApp_updateDate(MrhttpApp *self, PyObject *date);
PyObject *MrhttpApp_check_idle(MrhttpApp *self);
PyObject *MrhttpApp_test_fut(MrhttpApp *self);


