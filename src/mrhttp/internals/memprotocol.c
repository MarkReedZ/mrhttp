

#include "memprotocol.h"
#include "Python.h"
#include "common.h"
#include <errno.h>
#include <string.h>

// We only support 32 character session IDs and memcache key must be mrsession[32chars]

PyObject * MemcachedProtocol_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  MemcachedProtocol* self = NULL;
  DBG_MEMCAC printf("Memcached protocol new\n");

  self = (MemcachedProtocol*)type->tp_alloc(type, 0);
  if(!self) goto finally;

  self->transport = NULL;
  self->write = NULL;
  self->client = NULL;

  memcpy(self->get_cmd, "get mrsessionZZZZ9dd361cc443e976b05714581a7fb\r\n",strlen("get mrsession43709dd361cc443e976b05714581a7fb\r\n"));

  finally:
  return (PyObject*)self;
}

void MemcachedProtocol_dealloc(MemcachedProtocol* self)
{
  Py_XDECREF(self->transport);
  Py_XDECREF(self->write);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

int MemcachedProtocol_init(MemcachedProtocol* self, PyObject *args, PyObject *kw)
{
  DBG_MEMCAC printf("Memcached protocol init\n");
  self->closed = true;
  self->queue_sz = 1024;
  self->queue_start = 0;
  self->queue_end = 0;

  if(!PyArg_ParseTuple(args, "O", &self->client)) return -1;
  Py_INCREF(self->client);


  //printf("Memcached protocol init end\n");
  return 0;
}

PyObject* MemcachedProtocol_connection_made(MemcachedProtocol* self, PyObject* transport)
{
  DBG_MEMCAC printf("MemcachedProtocol conn made\n");
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
  DBG_MEMCAC printf("MemcachedProtocol eof received\n");
  Py_RETURN_NONE; // Closes the connection and conn lost will be called next
}

PyObject* MemcachedProtocol_connection_lost(MemcachedProtocol* self, PyObject* args)
{
  DBG_MEMCAC printf("MemcachedProtocol conn lost\n");
  self->closed = true;

  Py_RETURN_NONE;
}

PyObject* MemcachedProtocol_data_received(MemcachedProtocol* self, PyObject* data)
{
  DBG_MEMCAC printf("memcached protocol - data recvd\n");
  DBG_MEMCAC PyObject_Print( data, stdout, 0 ); 
  DBG_MEMCAC printf("\n");
//                                                    50  
//b"VALUE mrsession43709dd361cc443e976b05714581a7fb 0 19\r\n{'username':'Mark'}\r\nEND\r\nVALUE mrqsession43709dd361cc443e976b05714581a7fb 0 19\r\n{'username':'Mark'}\r\nEND\r\nVALUE mrqsession43709dd361cc443e976b05714581a7fb 0 19\r\n{'username':'Mark'}\r\nEND\r\nVALUE mrqsession43709dd361cc443e976b05714581a7fb 0 19\r\n{'username':'Mark'}\r\nEND\r\n"
  char *p, *start;
  Py_ssize_t l;

  if(PyBytes_AsStringAndSize(data, &start, &l) == -1) Py_RETURN_NONE;

  p = start;
  do {
    // No session found
    if ( p[0] == 'E' ) {
      p += 5;
      tMemcachedCallback cb = self->queue[self->queue_start].cb;
      cb(self->queue[self->queue_start].connection, NULL, 0);
      self->queue_start = (self->queue_start+1)%self->queue_sz;
    }
    // Session found
    else if ( p[0] == 'V' ) {
      p += 50;
      int vlen = 0;
      while( *p != '\r' ) {
        vlen = (*p-'0') + 10*vlen;
        p += 1;
      } 
      p += 2;
  
      if ( l < (60+vlen) ) {
        printf("Partial memc response! vlen %d l %ld\n",vlen,l);
        PyObject_Print( data, stdout, 0 ); 
        printf("\n");
        exit(1);
      }

      char *buf = malloc( vlen );
      memcpy(buf, p, vlen);
      tMemcachedCallback cb = self->queue[self->queue_start].cb;
      cb(self->queue[self->queue_start].connection, buf, vlen);
      self->queue_start = (self->queue_start+1)%self->queue_sz;

      p += vlen + 7;  
        
    } else {
      printf("Bad memc response data len %ld\n", strlen(p));
      PyObject_Print( data, stdout, 0 ); 
      printf("\n");
      exit(1);
    }
  }
  while ( p < (start+l) );

  Py_RETURN_NONE;
}

int MemcachedProtocol_asyncGet( MemcachedProtocol* self, char *key, void *fn, void *connection ) {
  DBG_MEMCAC printf("MemcachedProtocol - asyncGet\n");
  //printf("key %.*s\n", 32, key);
  char *kp = self->get_cmd+13;
  memcpy(kp, key, 32);
  PyObject *bytes = PyBytes_FromString(self->get_cmd);
  self->queue[self->queue_end].cb = (tMemcachedCallback*)fn;
  self->queue[self->queue_end].connection = connection;
  self->queue_end = (self->queue_end+1)%self->queue_sz;
  if(!PyObject_CallFunctionObjArgs(self->write, bytes, NULL)) return 0;
  return 1;
}


