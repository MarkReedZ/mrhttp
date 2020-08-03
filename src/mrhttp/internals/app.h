#pragma once

#include <Python.h>
#include <stdbool.h>

typedef struct Protocol Protocol;
typedef struct Request Request;

typedef struct {
  Protocol *protocol;
  Request  *request;
} SessionCallbackData;

typedef void (*tSessionCallback)(void*, char*, int);
typedef int (*tSessionClientGet)(void*, char*, void*, void*);

#include "memcachedclient.h"

typedef struct {
  PyObject_HEAD
  
  // Idle timeouts
  PyObject *connections;
  PyObject* call_later;
  PyObject* check_idle;
  PyObject* check_idle_handle;
  PyObject* check_interval;
  PyObject* loop;

  // Custom error pages
  PyObject* err404;

  // Request pool
  PyObject *func_expand_requests;
  PyObject *requests;
  //Request **requests;
  int numRequests, nextRequest,freeRequests; 
  //int numGets, numReleases;

  // Clients
  PyObject *py_mc;
  PyObject *py_mrq;
  PyObject *py_mrc;
  PyObject *py_redis;
  PyObject *py_session_backend_type; // int 1,2,3 ( memcached, mrworkserver, mrcache )
  PyObject *py_session; // points to mc, mrq, or mrcache

  tSessionClientGet session_get;

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

void MrhttpApp_setup_error_pages(MrhttpApp* self);
