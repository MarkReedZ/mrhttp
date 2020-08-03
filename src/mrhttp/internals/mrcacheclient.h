#pragma once

#include <Python.h>
#include <stdbool.h>
#include "mrcacheprotocol.h"

typedef struct {
  MrcacheProtocol **conns;
  int conns_sz;
  int next_conn;
  int num_conns;
  int num;
  void *client;
} MrcacheServer;

typedef struct {
  PyObject_HEAD
  MrcacheServer **servers; 
  int num_servers;
} MrcacheClient;

PyObject *MrcacheClient_new    (PyTypeObject* self, PyObject *args, PyObject *kwargs);
int       MrcacheClient_init   (MrcacheClient* self,    PyObject *args, PyObject *kwargs);
void      MrcacheClient_dealloc(MrcacheClient* self);

PyObject *MrcacheClient_cinit(MrcacheClient* self);
void MrcacheClient_setupConnMap( MrcacheClient* self ) ;

void MrcacheClient_connection_lost( MrcacheClient* self, MrcacheProtocol* conn, int server_num );
PyObject *MrcacheClient_addConnection(MrcacheClient* self, MrcacheProtocol *conn, int server);

int MrcacheClient_get(MrcacheClient* self, char *key, void *fn, void *connection );
PyObject *MrcacheClient_set(MrcacheClient* self, PyObject *args);

int MrcacheServer_init( MrcacheServer *self, int server_num );
int MrcacheServer_get( MrcacheServer *self, char *k, void *fn, void *connection);
int MrcacheServer_set( MrcacheServer *self, char *k, int ksz, char* d, int dsz );
int MrcacheServer_addConnection( MrcacheServer *self, MrcacheProtocol *conn) ;
void MrcacheServer_connection_lost( MrcacheServer* self, MrcacheProtocol* conn );
