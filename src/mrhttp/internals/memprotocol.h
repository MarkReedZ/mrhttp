#pragma once

#include "Python.h"
#include <stdbool.h>

typedef void (*tMemcachedCallback)(void*, char*);

typedef struct {
  void *connection;
  tMemcachedCallback *cb;
} MemcachedRequest;

typedef struct {
  PyObject_HEAD
  PyObject* app;
  bool closed;

  PyObject* transport;
  PyObject* write;
  PyObject* client;

  MemcachedRequest queue[1024];
  int queue_sz;
  int queue_start;
  int queue_end;

  char get_cmd[64];

} MemcachedProtocol;

PyObject *MemcachedProtocol_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
int       MemcachedProtocol_init(   MemcachedProtocol* self, PyObject *args, PyObject *kw);
void      MemcachedProtocol_dealloc(MemcachedProtocol* self);


PyObject* MemcachedProtocol_connection_made (MemcachedProtocol* self, PyObject* transport);
void*     MemcachedProtocol_close           (MemcachedProtocol* self);
PyObject* MemcachedProtocol_connection_lost (MemcachedProtocol* self, PyObject* args);
PyObject* MemcachedProtocol_data_received   (MemcachedProtocol* self, PyObject* data);
PyObject* MemcachedProtocol_eof_received    (MemcachedProtocol* self);

//int MemcachedProtocol_asyncGet    (MemcachedProtocol* self, void* fn);
//typedef struct {
  //int (*MemcachedProtocol_asyncGet) (MemcachedProtocol* self, void* fn);
//} MemcachedProtocol_CAPI;
