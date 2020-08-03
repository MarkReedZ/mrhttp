#pragma once

#include "Python.h"
#include <stdbool.h>

typedef struct {
  void *connection;
  void *cb;
} MrcacheRequest;

typedef struct {
  PyObject_HEAD
  PyObject* app;
  bool closed;

  PyObject* transport;
  PyObject* write;
  PyObject* client;
  int server_num;

  MrcacheRequest queue[1024];
  int queue_sz;
  int queue_start;
  int queue_end;

  char get_cmd[64];
  char *set_cmd;
  int set_cmd_sz;

} MrcacheProtocol;

PyObject *MrcacheProtocol_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
int       MrcacheProtocol_init(   MrcacheProtocol* self, PyObject *args, PyObject *kw);
void      MrcacheProtocol_dealloc(MrcacheProtocol* self);


PyObject* MrcacheProtocol_connection_made (MrcacheProtocol* self, PyObject* transport);
void*     MrcacheProtocol_close           (MrcacheProtocol* self);
PyObject* MrcacheProtocol_connection_lost (MrcacheProtocol* self, PyObject* args);
PyObject* MrcacheProtocol_data_received   (MrcacheProtocol* self, PyObject* data);
PyObject* MrcacheProtocol_eof_received    (MrcacheProtocol* self);

int MrcacheProtocol_asyncGet( MrcacheProtocol* self, char *key, void *fn, void *connection );
int MrcacheProtocol_asyncSet( MrcacheProtocol* self, char *key, char *val, int val_sz );

