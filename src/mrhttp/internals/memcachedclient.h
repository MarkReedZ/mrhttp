#pragma once

#include <Python.h>
#include <stdbool.h>
#include "memprotocol.h"

typedef struct {
  MemcachedProtocol **conns;
  int conns_sz;
  int next_conn;
  int num_conns;
} MemcachedServer;

typedef struct {
  PyObject_HEAD
  MemcachedServer **servers; 
  int num_servers;
} MemcachedClient;

PyObject *MemcachedClient_new    (PyTypeObject* self, PyObject *args, PyObject *kwargs);
int       MemcachedClient_init   (MemcachedClient* self,    PyObject *args, PyObject *kwargs);
void      MemcachedClient_dealloc(MemcachedClient* self);

PyObject *MemcachedClient_cinit(MemcachedClient* self);
void MemcachedClient_setupConnMap( MemcachedClient* self ) ;

PyObject *MemcachedClient_addConnection(MemcachedClient* self, MemcachedProtocol *conn, int server);
PyObject *MemcachedClient_get(MemcachedClient* self, char *key, void *fn, void *connection );
PyObject *MemcachedClient_set(MemcachedClient* self, PyObject *args);

int MemcachedServer_init( MemcachedServer *self );
int MemcachedServer_get( MemcachedServer *self, char *k, void *fn, void *connection);
int MemcachedServer_set( MemcachedServer *self, char *k, int ksz, char* d, int dsz );
int MemcachedServer_addConnection( MemcachedServer *self, MemcachedProtocol *conn) ;
void MemcachedServer_connection_lost( MemcachedServer* self, MemcachedProtocol* conn );
