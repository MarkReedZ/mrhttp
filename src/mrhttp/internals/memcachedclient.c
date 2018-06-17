
#include <Python.h>
#include <stdbool.h>

#include "memprotocol.h"
#include "common.h"
#include "module.h"

static int connmap[4096];

static char hexchar[256] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                             0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                             0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

PyObject *MemcachedClient_new(PyTypeObject* type, PyObject *args, PyObject *kwargs) {
  MemcachedClient* self = NULL;
  self = (MemcachedClient*)type->tp_alloc(type, 0);
  return (PyObject*)self;
}


void MemcachedClient_dealloc(MemcachedClient* self) {
  for (int i = 0; i < self->num_servers; i++ ) {
    free(self->servers[i]);
  }
  free(self->servers);
}

int MemcachedClient_init(MemcachedClient* self, PyObject *args, PyObject *kwargs) {

  if(!PyArg_ParseTuple(args, "i", &self->num_servers)) return 1;

  self->servers = malloc( sizeof(MemcachedServer*) * self->num_servers );
  for (int i = 0; i < self->num_servers; i++ ) {
    self->servers[i] = malloc( sizeof(MemcachedServer) );
    MemcachedServer_init( self->servers[i] );
  }

  MemcachedClient_setupConnMap(self); 
  return 0;
}

PyObject *MemcachedClient_cinit(MemcachedClient* self) {
  Py_RETURN_NONE;
}

PyObject *MemcachedClient_addConnection(MemcachedClient* self, MemcachedProtocol *conn, int server) { 

  MemcachedServer_addConnection( self->servers[server], conn );

  Py_RETURN_NONE;
}

// Args Session key bytes, user bytes
PyObject *MemcachedClient_get(MemcachedClient* self, char *key, void *fn, void *connection ) {

  int ksz = 32;
  int hash = (hexchar[(uint8_t)key[ksz-3]]<<8) | (hexchar[(uint8_t)key[ksz-2]]<<4) | hexchar[(uint8_t)key[ksz-1]];
  int server = connmap[hash];
  int rc = MemcachedServer_get( self->servers[server], key, fn, connection );
  //TODO no connections if ( rc == 1) 
  Py_RETURN_NONE;
}

// Args Session key bytes, user bytes
PyObject *MemcachedClient_set(MemcachedClient* self, PyObject *args) {

  PyObject *pykey, *pydata;
  if(!PyArg_ParseTuple(args, "OO", &pykey, &pydata)) return NULL;

  Py_ssize_t ksz;
  char *k = PyUnicode_AsUTF8AndSize( pykey, &ksz ); 
  Py_ssize_t dsz;
  char *d = PyUnicode_AsUTF8AndSize( pydata, &dsz ); 

  int hash = (hexchar[(uint8_t)k[ksz-3]]<<8) | (hexchar[(uint8_t)k[ksz-2]]<<4) | hexchar[(uint8_t)k[ksz-1]];
  int server = connmap[hash];
 
  int rc = MemcachedServer_set( self->servers[server], k, ksz, d, dsz );
  //TODO no connections if ( rc == 1) 

  Py_RETURN_NONE;
}

void MemcachedClient_setupConnMap( MemcachedClient* self ) {
  if ( self->num_servers == 0 ) return;

  int seg = 4096 / self->num_servers;
  int i = 0;
  int off = 0;
  for ( ; i < self->num_servers-1; i++ ) {
    for (int j = 0; j < seg; j++ ) {
      connmap[off++] = i; 
    }
  }
  while ( off < 4096 ) connmap[off++] = i;

}

void MemcachedClient_connection_lost( MemcachedClient* self, MemcachedProtocol* conn, int server ) {
  DBG_MEMCAC printf("conn %p lost server %d\n", conn, server);
  MemcachedServer_connection_lost( self->servers[server], conn );
  if ( self->servers[server]->num_conns == 0 ) {
    // TODO We need to poll until it comes back online and then redo connections
    // TODO setupConnMap without this server until we re-connect
  }
}

int MemcachedServer_dealloc( MemcachedServer *self ) {
  free(self->conns);
  return 0;
}

int MemcachedServer_init( MemcachedServer *self ) {
  self->num_conns = 0;
  self->next_conn = 0;
  self->conns = malloc( sizeof(MemcachedProtocol*) * 16 );
  return 0;
}
int MemcachedServer_addConnection( MemcachedServer *self, MemcachedProtocol *conn) {
  self->conns[ self->num_conns++ ] = conn;
  return 0;
}
void MemcachedServer_connection_lost( MemcachedServer* self, MemcachedProtocol* conn ) {
  DBG_MEMCAC printf("conn %p lost\n", conn);
  self->num_conns--;
  if ( self->num_conns == 0 ) return;

  // Remove the connection by shifting the rest left    
  MemcachedProtocol **p = self->conns;
  for (int i = 0; i < self->num_conns+1; i++) {
    p[i] = self->conns[i];
    if ( self->conns[i] != conn ) p++;
  }
  
}

// TODO Check ifclosed and skip 
int MemcachedServer_set( MemcachedServer *self, char *k, int ksz, char* d, int dsz ) {
  if ( self->num_conns == 0 ) return 1;
  int c = self->next_conn++;
  if ( self->next_conn == self->num_conns ) self->next_conn = 0;
  MemcachedProtocol_asyncSet( self->conns[c], k, d, dsz );
  return 0;
}
int MemcachedServer_get( MemcachedServer *self, char *k, void *fn, void *connection ) {
  if ( self->num_conns == 0 ) return 1;
  int c = self->next_conn++;
  if ( self->next_conn == self->num_conns ) self->next_conn = 0;
  MemcachedProtocol_asyncGet( self->conns[c], k, fn, connection );
  return 0;
}










