
#include <Python.h>
#include <stdbool.h>
#include <math.h>

#include "request.h"
#include "common.h"
#include "module.h"
#include "time.h"

PyObject *MrhttpApp_new(PyTypeObject* type, PyObject *args, PyObject *kwargs) {
  MrhttpApp* self = NULL;
  self = (MrhttpApp*)type->tp_alloc(type, 0);
  return (PyObject*)self;
}


void MrhttpApp_dealloc(MrhttpApp* self) {
  Py_XDECREF( self->check_interval ); 
  Py_TYPE(self)->tp_free((PyObject*)self);
}

int MrhttpApp_init(MrhttpApp* self, PyObject *args, PyObject *kwargs) {
  return 0;
}

PyObject *MrhttpApp_cinit(MrhttpApp* self) {
  srand(time(0)); // TODO seed utils.randint
  //initmrpacker();
  self->requests  = PyObject_GetAttrString((PyObject*)self, "requests");
  int l = PyList_Size(self->requests);

  self->numRequests = l;
  self->nextRequest = 0;
  self->freeRequests = l;

  self->numGets = self->numReleases = 0;

  // Idle timeouts (conn and request)
  PyObject* loop = NULL;

  if(!(self->connections = PyObject_GetAttrString((PyObject*)self, "_connections"))) goto error;
  if(!(self->loop = PyObject_GetAttrString((PyObject*)self, "_loop"))) goto error;
  if(!(self->call_later = PyObject_GetAttrString(self->loop, "call_later"))) goto error;

  if(!(self->check_idle = PyObject_GetAttrString((PyObject*)self, "check_idle"))) goto error;
  self->check_interval = PyLong_FromLong(5);
  self->check_idle_handle = PyObject_CallFunctionObjArgs( self->call_later, self->check_interval, self->check_idle, NULL);

  self->func_expand_requests = PyObject_GetAttrString((PyObject*)self, "expand_requests");

  response_setupResponseBuffer();

  MrhttpApp_setup_error_pages(self);

  Py_RETURN_NONE;
error:
  return NULL;
}

void MrhttpApp_release_request(MrhttpApp* self, Request *r) {
  //r->inprog = false;
  Request_reset(r);
  self->freeRequests++;
  self->numReleases++;
  //printf("DELME release gets %d rels %d free %d\n", self->numGets, self->numReleases, self->freeRequests); 
  int cnt = 0;
  int numinprog = 0;
  while (cnt < self->numRequests) {
    r = (Request*)PyList_GET_ITEM( self->requests, cnt );
    if ( r->inprog ) numinprog++;
    cnt++;
  }
}

void MrhttpApp_double_requests(MrhttpApp* self) {
  self->nextRequest = self->numRequests-1;
  PyObject *ret = PyObject_CallFunctionObjArgs(self->func_expand_requests, NULL);
    
  if ( ret == NULL ) {
    printf("ret null\n");
    exit(1);
  }
  self->freeRequests += self->numRequests;
  self->numRequests *= 2;
  //printf("DELME dbl req list sz %d num %d\n", PyList_GET_SIZE(self->requests), self->numRequests);
}

PyObject *MrhttpApp_get_request(MrhttpApp* self) {
  PyObject *ret = PyList_GET_ITEM( self->requests, self->nextRequest );
  Request *r = (Request*)ret;
  self->freeRequests--;
  self->numGets++;
  DBG printf(" get request index %d\n", self->nextRequest ); 

  // If we wrap and hit an in progress request double the number of requests and 
  // start at the new ones. 
  if ( r->inprog ) {
    //printf("DELME get req free %d num %d\n", self->freeRequests, self->numRequests); 
    // Double the number of requests if necessary
    if ( self->freeRequests < 10 ) {
      MrhttpApp_double_requests(self);
    }
    // Loop until found a free request
    int cnt;
redo:
    cnt = 0;
    while (r->inprog) {
      cnt++; 
      if ( cnt > self->numRequests ) { 
        printf("DELME ARGH %d > %d free %d!\n", cnt, self->numRequests,self->freeRequests); 
        break; 
      }
      self->nextRequest = (self->nextRequest+1)%self->numRequests;
      r = (Request*)PyList_GET_ITEM( self->requests, self->nextRequest );
    }
    if ( cnt > self->numRequests ) {
      MrhttpApp_double_requests(self);
      goto redo;
    }
  }
  r->inprog = true;
  self->nextRequest = (self->nextRequest+1)%self->numRequests;
  return ret;
}

PyObject *MrhttpApp_updateDate(MrhttpApp *self, PyObject *date) {
  return response_updateDate(date);
}

//DELME
//PyObject *MrhttpApp_test_fut(MrhttpApp *self) {
  //PyObject *cf = PyObject_GetAttrString(self->loop, "create_future");
  //self->fut =     PyObject_CallFunctionObjArgs(cf, NULL);
  //return self->fut;
//}

PyObject *MrhttpApp_check_idle(MrhttpApp *self) {

  PyObject* iterator = NULL;
  Protocol* c = NULL;

  // DELME
  //if ( self->fut != NULL ) {
    //printf("Setting fut result!\n");
    //PyObject *cf = PyObject_GetAttrString(self->fut, "set_result");
    //PyObject *ret = PyObject_CallFunctionObjArgs(cf, Py_None, NULL);
    //self->fut = NULL;
    //Py_RETURN_NONE;
  //}

  
  if(!(iterator = PyObject_GetIter(self->connections))) goto error;
  
  unsigned long check_interval = PyLong_AsLong(self->check_interval);
  while((c = (Protocol*)PyIter_Next(iterator))) {

    // Connection hung
    if ( c->num_data_received == 0 ) {
      c->conn_idle_time += check_interval;
      if ( c->conn_idle_time > 20 ) {
        if( !Protocol_close(c) ) goto error;
      }
    } else {
      c->conn_idle_time = 0;
      c->num_data_received = 0;
    }

    // Request handler hung
    if ( c->num_requests_popped == 0 ) {
      c->request_idle_time += check_interval;
      if ( c->request_idle_time >  4 ) {
        Protocol_timeout_request(c);
      }
    } else {
      c->request_idle_time = 0;
      c->num_requests_popped = 0;
    }


    Py_DECREF(c);
  }
  
  goto finally;
  
  error: 
  Py_XDECREF(iterator);
  return NULL;
  
  finally:
  Py_XDECREF(iterator);

  Py_XDECREF(self->check_idle_handle);
  self->check_idle_handle = PyObject_CallFunctionObjArgs( self->call_later, self->check_interval, self->check_idle, NULL);
  Py_RETURN_NONE;
}

void MrhttpApp_setup_error_pages(MrhttpApp* self) {
  PyObject *u = PyObject_GetAttrString((PyObject*)self, "err404");
  if ( !u ) return;

  Py_ssize_t l;
  char *body = PyUnicode_AsUTF8AndSize( u, &l );

  char *resp = malloc( l + 1024 );
  sprintf(resp, "HTTP/1.1 404 Not Found\r\nServer: MrHTTP/0.1.1\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %ld\r\n\r\n", l);
  char *p = resp + strlen(resp);
  memcpy(p, body, l);

  self->err404 = PyBytes_FromStringAndSize( resp, (p-resp) + l );
 
}

