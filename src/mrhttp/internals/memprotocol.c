

#include "memprotocol.h"
#include "memcachedclient.h"
#include "Python.h"
#include "common.h"
#include <errno.h>
#include <string.h>

// We only support 32 character session IDs and memcache key must be mrsession[32chars]

static char *memp_noreply = " noreply\r\n";

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

  self->set_cmd = malloc( 2048 );
  self->set_cmd_sz = 2048;
  // set <key> <flags> <exptime> <bytes> [noreply]\r\n[data]\r\n
  // 0 == never expire

  finally:
  return (PyObject*)self;
}

void MemcachedProtocol_dealloc(MemcachedProtocol* self)
{
  free(self->set_cmd);
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
  memcpy(self->set_cmd, "set mrsessionZZZZ9dd361cc443e976b05714581a7fb 0 0 ",strlen("set mrsessionZZZZ9dd361cc443e976b05714581a7fb 0 25920000 "));

  if(!PyArg_ParseTuple(args, "Oi", &self->client, &self->server_num)) return -1;
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

  MemcachedClient_addConnection( (MemcachedClient*)(self->client), self, self->server_num );

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
      free(buf);
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

static inline void reverse(char* begin, char* end)
{
  char t;
  while (end > begin) {
    t = *end;
    *end-- = *begin;
    *begin++ = t;
  }
}


int MemcachedProtocol_asyncSet( MemcachedProtocol* self, char *key, char *val, int val_sz ) {
// <command name> <key> <flags> <exptime> <bytes> [noreply]\r\n<data>\r\n
  DBG_MEMCAC printf("MemcachedProtocol - asyncSet\n");

  if ( 128 + val_sz > self->set_cmd_sz ) {
    while ( 128 + val_sz > self->set_cmd_sz ) self->set_cmd_sz *= 2;
    self->set_cmd = realloc(self->set_cmd, self->set_cmd_sz);
  }

  char *p = self->set_cmd+13;
  memcpy(p, key, 32);

  p += 37;
  // write val_sz
  int i = val_sz;
  do *p++ = (char)(48 + (i % 10ULL)); while(i /= 10ULL);
  reverse( self->set_cmd+50, p-1 );
 
  memcpy(p, memp_noreply, 10); 
  p+=10;
  memcpy(p, val,val_sz);
  p += val_sz;
  *p++ = '\r';
  *p++ = '\n';
  PyObject *bytes = PyBytes_FromStringAndSize(self->set_cmd,p-self->set_cmd);
  //DBG_MEMCAC PyObject_Print(bytes, stdout,0); 
  //DBG_MEMCAC printf("\n");
  if(!PyObject_CallFunctionObjArgs(self->write, bytes, NULL)) return 1;
  return 0;  
}

