


#include <Python.h>
#include <stdbool.h>

#include "request.h"
#include "common.h"
#include "module.h"

static char *resp_buf;

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


  //resp_buf = malloc(128*1024);
  //if ( !resp_buf ) {
    //PyErr_NoMemory();
    //return NULL;
  //}

  setupResponseBuffer(resp_buf);

  Py_RETURN_NONE;
}

void MrhttpApp_release_request(MrhttpApp* self, Request *r) {
  r->inprog = false;
  self->freeRequests++;
  //printf("DELME release request free %d\n", self->freeRequests);
}

PyObject *MrhttpApp_get_request(MrhttpApp* self) {
  //printf("DELME get request number %d free %d\n", self->nextRequest, self->freeRequests);
  PyObject *ret = PyList_GET_ITEM( self->requests, self->nextRequest );
  Request *r = (Request*)ret;
  self->freeRequests--;

  // If we wrap and hit an in progress request double the number of requests and 
  // start at the new ones. 
  if ( r->inprog ) {
    //printf("DELME hit inprog %d free %d\n", self->nextRequest, self->freeRequests);
    // Double the number of requests if necessary
    if ( self->freeRequests < 10 ) {
      self->nextRequest = self->numRequests-1;
      PyObject *ret = PyObject_CallFunctionObjArgs(self->func_expand_requests, NULL);
      if ( ret == NULL ) {
        printf("ret null\n");
  PyObject *type, *value, *traceback;
  PyErr_Fetch(&type, &value, &traceback);
  printf("Unhandled exception :\n");
  PyObject_Print( type, stdout, 0 ); printf("\n");
  PyObject_Print( value, stdout, 0 ); printf("\n");
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
  //printf("capp get request %d\n",self->nextRequest);
  self->nextRequest = (self->nextRequest+1)%self->numRequests;
  return ret;
}


