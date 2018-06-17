
#include "mrqprotocol.h"
#include "mrqclient.h"
#include "Python.h"
#include "common.h"
#include <errno.h>
#include <string.h>


PyObject * MrqProtocol_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  MrqProtocol* self = NULL;
  DBG_MRQ printf("Mrq protocol new\n");

  self = (MrqProtocol*)type->tp_alloc(type, 0);
  if(!self) goto finally;

  self->transport = NULL;
  self->write = NULL;
  self->client = NULL;

  finally:
  return (PyObject*)self;
}

void MrqProtocol_dealloc(MrqProtocol* self)
{
  //printf("DELME DEALLOC\n");
  free(self->b);
  Py_XDECREF(self->transport);
  Py_XDECREF(self->write);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

int MrqProtocol_init(MrqProtocol* self, PyObject *args, PyObject *kw)
{
  DBG_MRQ printf("Mrq protocol init\n");
  self->closed = true;

  if(!PyArg_ParseTuple(args, "Oi", &self->client, &self->server_num)) return -1;
  Py_INCREF(self->client);

  self->b = malloc(1024);
  self->bsz = 1024;
  self->b[0] = 0;
  self->b[1] = 1;
  self->b[2] = 0;
  self->b[3] = 0;
  self->bb = self->b+8;
  self->bp4 = (int*)(self->b+4);


  //printf("Mrq protocol init end\n");
  return 0;
}

PyObject* MrqProtocol_connection_made(MrqProtocol* self, PyObject* transport)
{
  DBG_MRQ printf("MrqProtocol conn made\n");
  self->transport = transport;
  Py_INCREF(self->transport);

  self->closed = false;

  if(!(self->write      = PyObject_GetAttrString(transport, "write"))) return NULL;

  MrqClient_addConnection( (MrqClient*)(self->client), self, self->server_num );


  //printf("Mrq protocol made\n");
  //PyObject* connections = NULL;
  //PyObject* setconn = NULL;
  //if(!(setconn = PyObject_GetAttrString(self->client, "setConnection"))) return NULL;
  //printf("Mrq protocol made\n");
  //PyObject* tmp = PyObject_CallFunctionObjArgs(setconn, (PyObject*)self, NULL);
  //if(!tmp) return NULL;
  //printf("Mrq protocol made\n");
  //Py_DECREF(tmp);
  //if(!(connections = PyObject_GetAttrString(self->client, "_connections"))) return NULL;
  //if(PyList_Add(connections, (PyObject*)self) == -1) return NULL;
  //Py_XDECREF(connections);

  //return (PyObject*)self;
  Py_RETURN_NONE;
}

void* MrqProtocol_close(MrqProtocol* self)
{
  void* result = self;

  PyObject* close = PyObject_GetAttrString(self->transport, "close");
  if(!close) return NULL;
  PyObject* tmp = PyObject_CallFunctionObjArgs(close, NULL);
  Py_XDECREF(close);
  if(!tmp) return NULL;
  Py_DECREF(tmp);
  self->closed = true;

  return result;

}

PyObject* MrqProtocol_eof_received(MrqProtocol* self) {
  DBG_MRQ printf("MrqProtocol eof received\n");
  Py_RETURN_NONE; // Closes the connection and conn lost will be called next
}

PyObject* MrqProtocol_connection_lost(MrqProtocol* self, PyObject* args)
{
  DBG_MRQ printf("MrqProtocol conn lost\n");
  self->closed = true;

  Py_RETURN_NONE;
}

PyObject* MrqProtocol_data_received(MrqProtocol* self, PyObject* data)
{
  // TODO Handle the pause/resume msg
  DBG_MRQ printf("mrq protocol - data recvd\n");
  DBG_MRQ PyObject_Print( data, stdout, 0 ); 
  DBG_MRQ printf("\n");
  //tMrqCallback cb = self->queue[0].cb;
  //cb(self->queue[0].connection);
  Py_RETURN_NONE;
}

int MrqProtocol_push(MrqProtocol* self, int slot, char *d, int dsz) {

  if ( dsz > 10*1024 ) return -1;

  if ( dsz > self->bsz ) {
    self->bsz = 12*1024;
    self->b = realloc( self->b, self->bsz ); 
    self->bb = self->b+8;
    self->bp4 = (int*)(self->b+4);
  }

  //int *p_len = (int*)(self->bp4);
  *self->bp4 = dsz;
  memcpy(self->bb, d, dsz);

  PyObject *bytes = PyBytes_FromStringAndSize(self->b, dsz + 8);
  //DBG_MRQ PyObject_Print(bytes, stdout,0); 
  //DBG_MRQ printf("\n");
  if(!PyObject_CallFunctionObjArgs(self->write, bytes, NULL)) return 1;

  return 0;
}
