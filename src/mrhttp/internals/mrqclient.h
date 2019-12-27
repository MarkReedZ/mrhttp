#pragma once

#include <Python.h>
#include <stdbool.h>
#include "mrqprotocol.h"

typedef struct {
  MrqProtocol **conns;
  int conns_sz;
  int next_conn;
  int num_conns;
  int num;
  void *client;
} MrqServer;

typedef struct MrqClient {
  PyObject_HEAD
  MrqServer **servers;
  int num_servers;
} MrqClient;


PyObject *MrqClient_new    (PyTypeObject* self, PyObject *args, PyObject *kwargs);
int       MrqClient_init   (MrqClient* self,    PyObject *args, PyObject *kwargs);
void      MrqClient_dealloc(MrqClient* self);

PyObject *MrqClient_cinit(MrqClient* self);

//void MrqClient_setupConnMap( MrqClient* self ) ;

PyObject *MrqClient_addConnection(MrqClient* self, MrqProtocol *conn, int server);
void MrqClient_connection_lost( MrqClient* self, MrqProtocol *conn );
PyObject *MrqClient_get(MrqClient* self, PyObject *args);
int MrqClient_push (MrqClient* self, int slot, char *d, int dsz);
int MrqClient_pushj(MrqClient* self, int slot, char *d, int dsz);

int MrqClient_getSession(MrqClient* self, char *key, void *fn, void *connection );
//PyObject *MrqClient_get(MrqClient* self, char *key, void *fn, void *connection );
PyObject *MrqClient_set(MrqClient* self, PyObject *args);


int MrqServer_init( MrqServer *self, MrqClient *client, int server_num );
int MrqServer_addConnection( MrqServer *self, MrqProtocol *conn) ;
void MrqServer_connection_lost( MrqServer* self, MrqProtocol* conn );

int MrqServer_dealloc (MrqServer* self );
int MrqServer_getSession(MrqServer* self, char *key, void *fn, void *connection );
int MrqServer_get  (MrqServer* self, PyObject *args);
int MrqServer_push (MrqServer* self, char *d, int dsz);
int MrqServer_pushj(MrqServer* self, char *d, int dsz);
int MrqServer_set  (MrqServer* self, PyObject *d);
