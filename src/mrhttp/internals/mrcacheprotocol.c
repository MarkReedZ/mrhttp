

#include "mrcacheprotocol.h"
#include "mrcacheclient.h"
#include "Python.h"
#include "common.h"
#include <errno.h>
#include <string.h>

#include "app.h"

static void print_buffer( char* b, int len ) {
  for ( int z = 0; z < len; z++ ) {
    printf( "%02x ",(int)b[z]);
  }
  printf("\n");
}


// We only support 32 character session IDs and memcache key must be mrsession[32chars]

PyObject * MrcacheProtocol_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  MrcacheProtocol* self = NULL;
  DBG_MEMCAC printf("Mrcache protocol new\n");

  self = (MrcacheProtocol*)type->tp_alloc(type, 0);
  if(!self) goto finally;

  self->transport = NULL;
  self->write = NULL;
  self->client = NULL;

  self->get_cmd[0] = 0;
  self->get_cmd[1] = 1;
  uint16_t *klenp = (uint16_t*)(self->get_cmd+2);
  *klenp = 32;

  self->set_cmd = malloc( 2048 );
  self->set_cmd_sz = 2048;

  finally:
  return (PyObject*)self;
}

void MrcacheProtocol_dealloc(MrcacheProtocol* self)
{
  free(self->set_cmd);
  Py_XDECREF(self->transport);
  Py_XDECREF(self->write);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

int MrcacheProtocol_init(MrcacheProtocol* self, PyObject *args, PyObject *kw)
{
  DBG_MEMCAC printf("Mrcache protocol init\n");
  self->closed = true;
  self->queue_sz = 1024;
  self->queue_start = 0;
  self->queue_end = 0;

  self->set_cmd[0] = 0;
  self->set_cmd[1] = 2;
  uint16_t *klenp = (uint16_t*)(self->set_cmd+2);
  *klenp = 32;
  strcpy( self->set_cmd+4, "mrsession");

  if(!PyArg_ParseTuple(args, "Oi", &self->client, &self->server_num)) return -1;
  Py_INCREF(self->client);

  //printf("Mrcache protocol init end\n");
  return 0;
}

PyObject* MrcacheProtocol_connection_made(MrcacheProtocol* self, PyObject* transport)
{
  DBG_MEMCAC printf("MrcacheProtocol conn made\n");
  self->transport = transport;
  Py_INCREF(self->transport);

  self->closed = false;

  if(!(self->write      = PyObject_GetAttrString(transport, "write"))) return NULL;

  MrcacheClient_addConnection( (MrcacheClient*)(self->client), self, self->server_num );

  Py_RETURN_NONE;
}

void* MrcacheProtocol_close(MrcacheProtocol* self)
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

PyObject* MrcacheProtocol_eof_received(MrcacheProtocol* self) {
  DBG_MEMCAC printf("MrcacheProtocol eof received\n");
  Py_RETURN_NONE; // Closes the connection and conn lost will be called next
}

PyObject* MrcacheProtocol_connection_lost(MrcacheProtocol* self, PyObject* args)
{
  DBG_MEMCAC printf("MrcacheProtocol conn lost\n");
  self->closed = true;
  MrcacheClient_connection_lost((MrcacheClient*)self->client, self, self->server_num );
  Py_RETURN_NONE;
}

PyObject* MrcacheProtocol_data_received(MrcacheProtocol* self, PyObject* data)
{
  DBG_MEMCAC printf("mrcache protocol - data recvd\n");
  DBG_MEMCAC PyObject_Print( data, stdout, 0 ); 
  DBG_MEMCAC printf("\n");
//                                                    50  
//b"VALUE mrsession43709dd361cc443e976b05714581a7fb 0 19\r\n{'username':'Mark'}\r\nEND\r\nVALUE mrqsession43709dd361cc443e976b05714581a7fb 0 19\r\n{'username':'Mark'}\r\nEND\r\nVALUE mrqsession43709dd361cc443e976b05714581a7fb 0 19\r\n{'username':'Mark'}\r\nEND\r\nVALUE mrqsession43709dd361cc443e976b05714581a7fb 0 19\r\n{'username':'Mark'}\r\nEND\r\n"
  char *p, *start;
  Py_ssize_t l;

  if(PyBytes_AsStringAndSize(data, &start, &l) == -1) Py_RETURN_NONE;

  p = start;
  char *end = start+l;
  do {

    if ( l < 6 ) {
      printf("TODO Partial memc response! l %zu\n",l); 
      //PyObject_Print( data, stdout, 0 ); printf("\n");
      exit(1);
    }
    bool get = ((p[0]==0)&&(p[1] == 1));
    uint32_t sz  = *((uint32_t*)(p+2));

    if ( get ) {
      // No session found
      if ( sz == 0 ) {
        p += 6;
        tSessionCallback cb = self->queue[self->queue_start].cb;
        cb(self->queue[self->queue_start].connection, NULL, 0);
        self->queue_start = (self->queue_start+1)%self->queue_sz;
      }
      // Session found
      // TODO The session key length must be 32 , allow variable and check performance
      else {
        p += 6; l -= 6;
  
        if ( (uint32_t)l < sz ) {
          printf("TODO Partial memc response! sz %d l %zu\n",sz,l);
          //PyObject_Print( data, stdout, 0 ); printf("\n");
          exit(1);
        }
  
        char *buf = malloc( sz ); // TODO Use a preallocated buffer
        memcpy(buf, p, sz);
        tSessionCallback cb = self->queue[self->queue_start].cb;
        cb(self->queue[self->queue_start].connection, buf, sz);
        free(buf);
        self->queue_start = (self->queue_start+1)%self->queue_sz;
  
        p += sz; l -= sz;
          
      } 
  	}
    else {
      printf("TODO Bad memc response data len %ld\n", l);
      PyObject_Print( data, stdout, 0 ); printf("\n"); // Print on error
      exit(1);
    }
  }
  while ( p < end );

  Py_RETURN_NONE;
}

int MrcacheProtocol_asyncGet( MrcacheProtocol* self, char *key, void *fn, void *connection ) {
  DBG_MEMCAC printf("MrcacheProtocol - asyncGet\n");
  char *kp = self->get_cmd+4;
  memcpy(kp, key, 32);
  PyObject *bytes = PyBytes_FromStringAndSize(self->get_cmd,36);

  // Queue up a callback for the response
  self->queue[self->queue_end].cb = (tSessionCallback*)fn;
  self->queue[self->queue_end].connection = connection;
  self->queue_end = (self->queue_end+1)%self->queue_sz;

  if(!PyObject_CallFunctionObjArgs(self->write, bytes, NULL)) return 0;
  return 1;
}

int MrcacheProtocol_asyncSet( MrcacheProtocol* self, char *key, char *val, int val_sz ) {
// get:  [version] [cmd=1] [keylen] [key]
// set:  [version] [cmd=2] [keylen] [valuelen] [key] [value]

  DBG_MEMCAC printf("MrcacheProtocol - asyncSet\n");

  if ( 128 + val_sz > self->set_cmd_sz ) {
    while ( 128 + val_sz > self->set_cmd_sz ) self->set_cmd_sz *= 2;
    self->set_cmd = realloc(self->set_cmd, self->set_cmd_sz);
  }

  uint16_t *klenp = (uint16_t*)(self->set_cmd+2);
  uint32_t *vlenp = (uint32_t*)(self->set_cmd+4);
  *klenp = 32;
  *vlenp = val_sz;

  //print_buffer( val, val_sz );


  char *p = self->set_cmd+8;
  memcpy(p, key, 32);
  DBG_MEMCAC printf("MrcacheProtocol - asyncSet3.1\n");

  p += 32;
  // write val_sz
  memcpy(p, val,val_sz);
  PyObject *bytes = PyBytes_FromStringAndSize(self->set_cmd, 8+32+val_sz);
  //PyObject_Print(bytes,stdout,0); 
  //print_buffer( self->set_cmd, 40+val_sz ); 
  if(!PyObject_CallFunctionObjArgs(self->write, bytes, NULL)) return 1;
  return 0;  
}

