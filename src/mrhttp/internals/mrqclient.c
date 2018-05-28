


#include <Python.h>
#include <stdbool.h>

#include "request.h"
#include "common.h"
#include "module.h"

//static char *resp_buf;

PyObject *MrqClient_new(PyTypeObject* type, PyObject *args, PyObject *kwargs) {
  MrqClient* self = NULL;
  self = (MrqClient*)type->tp_alloc(type, 0);
  return (PyObject*)self;
}


void MrqClient_dealloc(MrqClient* self) {
  Py_TYPE(self)->tp_free((PyObject*)self);
}

int MrqClient_init(MrqClient* self, PyObject *args, PyObject *kwargs) {
  printf("DELME capp init\n");
  return 0;
}

PyObject *MrqClient_cinit(MrqClient* self) {

  //PyObject *requests  = PyObject_GetAttrString((PyObject*)self, "requests");
  //int l = PyList_Size(requests);

  //self->numRequests = l;
  //self->nextRequest = 0;

  //resp_buf = malloc(128*1024);
  //if ( !resp_buf ) {
    //PyErr_NoMemory();
    //return -1;
  //}

  //setupResponseBuffer(resp_buf);

  Py_RETURN_NONE;
}

PyObject *MrqClient_get_request(MrqClient* self) {
  //PyObject *requests  = PyObject_GetAttrString((PyObject*)self, "requests");
  //PyObject *ret = PyList_GET_ITEM( requests, self->nextRequest );

  //Request *r = (Request*)ret;
  //r->inprog = true;
  //printf("capp get request %d\n",self->nextRequest);
  //self->nextRequest = (self->nextRequest+1)%self->numRequests;
  //return ret;

  Py_RETURN_NONE;
}


