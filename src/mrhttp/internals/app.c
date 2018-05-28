


#include <Python.h>
#include <stdbool.h>

#include "request.h"
#include "common.h"
#include "module.h"

PyObject *MrhttpApp_new(PyTypeObject* type, PyObject *args, PyObject *kwargs) {
  MrhttpApp* self = NULL;
  self = (MrhttpApp*)type->tp_alloc(type, 0);
  return (PyObject*)self;
}


void MrhttpApp_dealloc(MrhttpApp* self) {
  Py_TYPE(self)->tp_free((PyObject*)self);
}

int MrhttpApp_init(MrhttpApp* self, PyObject *args, PyObject *kwargs) {
  return 0;
}

PyObject *MrhttpApp_cinit(MrhttpApp* self) {

  self->requests  = PyObject_GetAttrString((PyObject*)self, "requests");
  int l = PyList_Size(self->requests);

  self->numRequests = l;
  self->nextRequest = 0;
  self->freeRequests = l;

  self->func_expand_requests = PyObject_GetAttrString(self, "expand_requests");

  response_setupResponseBuffer();

  Py_RETURN_NONE;
}

void MrhttpApp_release_request(MrhttpApp* self, Request *r) {
  //r->inprog = false;
  Request_reset(r);
  self->freeRequests++;
}

PyObject *MrhttpApp_get_request(MrhttpApp* self) {
  PyObject *ret = PyList_GET_ITEM( self->requests, self->nextRequest );
  Request *r = (Request*)ret;
  self->freeRequests--;

  // If we wrap and hit an in progress request double the number of requests and 
  // start at the new ones. 
  if ( r->inprog ) {
    // Double the number of requests if necessary
    if ( self->freeRequests < 10 ) {
      self->nextRequest = self->numRequests-1;
      PyObject *ret = PyObject_CallFunctionObjArgs(self->func_expand_requests, NULL);
      if ( ret == NULL ) {
        printf("ret null\n");
      }
      self->numRequests *= 2;
    }
    // Loop until found a free request
    while (r->inprog) {
      self->nextRequest = (self->nextRequest+1)%self->numRequests;
      r = (Request*)PyList_GET_ITEM( self->requests, self->nextRequest );
    }
  }
  r->inprog = true;
  self->nextRequest = (self->nextRequest+1)%self->numRequests;
  return ret;
}

PyObject *MrhttpApp_updateDate(MrhttpApp *self, PyObject *date) {
  return response_updateDate(date);
}

