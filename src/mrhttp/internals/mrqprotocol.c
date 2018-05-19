
#include "mrqprotocol.h"
#include "Python.h"
#include "common.h"
#include <errno.h>
#include <string.h>


PyObject * MrqProtocol_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  MrqProtocol* self = NULL;
  DBG_MEMCAC printf("Mrq protocol new\n");

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
  Py_XDECREF(self->transport);
  Py_XDECREF(self->write);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

int MrqProtocol_init(MrqProtocol* self, PyObject *args, PyObject *kw)
{
  DBG_MEMCAC printf("Mrq protocol init\n");
  self->closed = true;

  if(!PyArg_ParseTuple(args, "O", &self->client)) return -1;
  Py_INCREF(self->client);


  //printf("Mrq protocol init end\n");
  return 0;
}

PyObject* MrqProtocol_connection_made(MrqProtocol* self, PyObject* transport)
{
  DBG_MEMCAC printf("MrqProtocol conn made\n");
  self->transport = transport;
  Py_INCREF(self->transport);

  self->closed = false;

  if(!(self->write      = PyObject_GetAttrString(transport, "write"))) return NULL;

  //printf("Mrq protocol made\n");
  //PyObject* connections = NULL;
  PyObject* setconn = NULL;
  if(!(setconn = PyObject_GetAttrString(self->client, "setConnection"))) return NULL;
  //printf("Mrq protocol made\n");
  PyObject* tmp = PyObject_CallFunctionObjArgs(setconn, (PyObject*)self, NULL);
  if(!tmp) return NULL;
  //printf("Mrq protocol made\n");
  Py_DECREF(tmp);
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
  DBG_MEMCAC printf("MrqProtocol eof received\n");
  Py_RETURN_NONE; // Closes the connection and conn lost will be called next
}

PyObject* MrqProtocol_connection_lost(MrqProtocol* self, PyObject* args)
{
  DBG_MEMCAC printf("MrqProtocol conn lost\n");
  self->closed = true;

  Py_RETURN_NONE;
}

PyObject* MrqProtocol_data_received(MrqProtocol* self, PyObject* data)
{

  DBG_MEMCAC printf("memcached protocol - data recvd\n");
  DBG_MEMCAC PyObject_Print( data, stdout, 0 ); 
  DBG_MEMCAC printf("\n");
  //tMrqCallback cb = self->queue[0].cb;
  //cb(self->queue[0].connection);
  Py_RETURN_NONE;
}

int MrqProtocol_asyncGet( MrqProtocol* self, void *fn, void *connection ) {
  DBG_MEMCAC printf("MrqProtocol - asyncGet\n");
  //PyObject *bytes = PyBytes_FromString("get session_key\r\n");
  //self->queue[0].cb = (tMrqCallback)fn;
  //self->queue[0].connection = connection;
  //if(!PyObject_CallFunctionObjArgs(self->write, bytes, NULL)) return 0;
  //DBG_MEMCAC printf("DELME memcached protocol - wrote data\n");
  return 1;
}


