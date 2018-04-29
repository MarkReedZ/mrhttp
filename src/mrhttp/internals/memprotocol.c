

#include "memprotocol.h"
#include "Python.h"
#include <errno.h>
#include <string.h>


PyObject * MemcachedProtocol_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  MemcachedProtocol* self = NULL;
  //printf("Memcached protocol new\n");

  self = (MemcachedProtocol*)type->tp_alloc(type, 0);
  if(!self) goto finally;

  self->transport = NULL;
  self->write = NULL;
  self->client = NULL;

  finally:
  return (PyObject*)self;
}

void MemcachedProtocol_dealloc(MemcachedProtocol* self)
{
  //printf("DELME DEALLOC\n");
  Py_XDECREF(self->transport);
  Py_XDECREF(self->write);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

int MemcachedProtocol_init(MemcachedProtocol* self, PyObject *args, PyObject *kw)
{
  //printf("Memcached protocol init\n");
  self->closed = true;

  if(!PyArg_ParseTuple(args, "O", &self->client)) return -1;
  Py_INCREF(self->client);

  //printf("Memcached protocol init end\n");
  return 0;
}

PyObject* MemcachedProtocol_connection_made(MemcachedProtocol* self, PyObject* transport)
{
  //printf("Memcached protocol made\n");
  self->transport = transport;
  Py_INCREF(self->transport);

  self->closed = false;

  if(!(self->write      = PyObject_GetAttrString(transport, "write"))) return NULL;

  //printf("Memcached protocol made\n");
  //PyObject* connections = NULL;
  PyObject* setconn = NULL;
  if(!(setconn = PyObject_GetAttrString(self->client, "setConnection"))) return NULL;
  //printf("Memcached protocol made\n");
  PyObject* tmp = PyObject_CallFunctionObjArgs(setconn, (PyObject*)self, NULL);
  if(!tmp) return NULL;
  //printf("Memcached protocol made\n");
  Py_DECREF(tmp);
  //if(!(connections = PyObject_GetAttrString(self->client, "_connections"))) return NULL;
  //if(PyList_Add(connections, (PyObject*)self) == -1) return NULL;
  //Py_XDECREF(connections);

  //return (PyObject*)self;
  Py_RETURN_NONE;
}

void* MemcachedProtocol_close(MemcachedProtocol* self)
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

PyObject* MemcachedProtocol_eof_received(MemcachedProtocol* self) {
  //printf("eof received\n");
  Py_RETURN_NONE; // Closes the connection and conn lost will be called next
}

PyObject* MemcachedProtocol_connection_lost(MemcachedProtocol* self, PyObject* args)
{
  //printf("conn lost\n");
  self->closed = true;

  Py_RETURN_NONE;
}

PyObject* MemcachedProtocol_data_received(MemcachedProtocol* self, PyObject* data)
{
  //printf("memcached protocol - data recvd\n");
  //PyObject_Print( data, stdout, 0 ); printf("\n");
  Py_RETURN_NONE;
}

int MemcachedProtocol_asyncGet( MemcachedProtocol* self, void *fn ) {
  PyObject *bytes = PyBytes_FromString("send this back to me baby");
  if(!PyObject_CallFunctionObjArgs(self->write, bytes, NULL)) return 0;
  //printf("memcached protocol - wrote data\n");
  return 1;
}


