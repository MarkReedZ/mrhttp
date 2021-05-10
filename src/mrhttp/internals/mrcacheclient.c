

#include <Python.h>
#include <stdbool.h>

#include "mrcacheclient.h"
#include "mrcacheprotocol.h"
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

PyObject *MrcacheClient_new(PyTypeObject* type, PyObject *args, PyObject *kwargs) {
  MrcacheClient* self = NULL;
  self = (MrcacheClient*)type->tp_alloc(type, 0);
  return (PyObject*)self;
}

void MrcacheClient_dealloc(MrcacheClient* self) {
  for (int i = 0; i < self->num_servers; i++ ) {
    free(self->servers[i]);
  }
  free(self->servers);
}

int MrcacheClient_init(MrcacheClient* self, PyObject *args, PyObject *kwargs) {

  if(!PyArg_ParseTuple(args, "i", &self->num_servers)) return 1;

  self->servers = malloc( sizeof(MrcacheServer*) * self->num_servers );
  for (int i = 0; i < self->num_servers; i++ ) {
    self->servers[i] = malloc( sizeof(MrcacheServer) );
    MrcacheServer_init( self->servers[i], i );
  }

  //MrcacheClient_setupConnMap(self); 
  return 0;
}

PyObject *MrcacheClient_cinit(MrcacheClient* self) {
  Py_RETURN_NONE;
}

void MrcacheClient_setupConnMap( MrcacheClient* self ) {

  int no_servers = 1;
  for (int i = 0; i < self->num_servers; i++ ) {
    if ( self->servers[i]->num_conns != 0 ) no_servers = 0;
  }

  if ( no_servers ) return;

  int srv = 0;
  for (int i = 0; i < 4096; i++ ) {
    while ( self->servers[srv]->num_conns == 0 ) srv = (srv+1)%self->num_servers;
    connmap[i] = srv;
    srv = (srv+1)%self->num_servers;
  }

}

PyObject *MrcacheClient_addConnection(MrcacheClient* self, MrcacheProtocol *conn, int server) { 

  int n = self->servers[server]->num_conns;
  MrcacheServer_addConnection( self->servers[server], conn );

  // This is the first connection for the server so add it
  if ( n == 0 ) {
    MrcacheClient_setupConnMap(self);
  }

  Py_RETURN_NONE;
}

// Args Session key bytes, user bytes
int MrcacheClient_get(MrcacheClient* self, char *key, void *fn, void *connection ) {

  DBG_MEMCAC printf(" client get >%.*s<\n", 32, key );
  // TODO just hash the key? We're assuming this is only used by 32B session key with embedded user id
  // If we're hashing the key why not send the hash v to mrcache?
  int ksz = 32;

  //int hash = (hexchar[(uint8_t)key[ksz-3]]<<8) | (hexchar[(uint8_t)key[ksz-2]]<<4) | hexchar[(uint8_t)key[ksz-1]];
  int hash = ((from_64[(uint8_t)key[ksz-3]]&0x3) << 10) | ( from_64[(uint8_t)key[ksz-2]]<<5 ) | from_64[(uint8_t)key[ksz-1]] ;
  int server = connmap[hash];
  if ( server == -1 ) return -1;

  int rc = MrcacheServer_get( self->servers[server], key, fn, connection );
  return rc;
}

// Args Session key bytes, user bytes
PyObject *MrcacheClient_set(MrcacheClient* self, PyObject *args) {

  PyObject *pykey, *pydata;
  if(!PyArg_ParseTuple(args, "OO", &pykey, &pydata)) return NULL;

  Py_ssize_t ksz;
  char *k = PyUnicode_AsUTF8AndSize( pykey, &ksz ); 
  Py_ssize_t dsz;
  char *d;
  if ( PyUnicode_Check(pydata) ) {
    d = PyUnicode_AsUTF8AndSize( pydata, &dsz ); 
  } else {
    if ( PyBytes_AsStringAndSize( pydata, &d, &dsz ) == -1 ) {
      return NULL;
    }
  }

  // TODO This assumes its a base64 encoded key - our session key.  It won't work properly as a general purpose set
  //int hash = (hexchar[(uint8_t)k[ksz-3]]<<8) | (hexchar[(uint8_t)k[ksz-2]]<<4) | hexchar[(uint8_t)k[ksz-1]];
  int hash = ((from_64[(uint8_t)k[ksz-3]]&0x3) << 10) | ( from_64[(uint8_t)k[ksz-2]]<<5 ) | from_64[(uint8_t)k[ksz-1]] ;
  int server = connmap[hash];
  if ( server == -1 ) return NULL;

  DBG_MEMCAC printf("  memcached set server %d\n",server); 
  int rc = MrcacheServer_set( self->servers[server], k, ksz, d, dsz );
  // TODO We have to set an exception here
  if ( rc != 0 ) {
    return NULL;
  }
  Py_RETURN_NONE;
}


void MrcacheClient_connection_lost( MrcacheClient* self, MrcacheProtocol* conn, int server ) {
  DBG_MEMCAC printf("conn %p lost server %d\n", conn, server);
  MrcacheServer_connection_lost( self->servers[server], conn );

  PyObject* func = PyObject_GetAttrString((PyObject*)self, "lost_connection");
  PyObject* tmp = PyObject_CallFunctionObjArgs(func, PyLong_FromLong(server), NULL);
  Py_XDECREF(func);
  Py_XDECREF(tmp);

  // If we have no more connections to this server remove it 
  if ( self->servers[server]->num_conns == 0 ) {
    MrcacheClient_setupConnMap(self);
  }
}

int MrcacheServer_dealloc( MrcacheServer *self ) {
  free(self->conns);
  return 0;
}

int MrcacheServer_init( MrcacheServer *self, int server_num ) {
  self->num_conns = 0;
  self->next_conn = 0;
  //self->client = client;
  self->num = server_num;
  self->conns = malloc( sizeof(MrcacheProtocol*) * 16 );
  return 0;
}
int MrcacheServer_addConnection( MrcacheServer *self, MrcacheProtocol *conn) {
  DBG_MEMCAC printf("  MrcacheServer add conn %p\n", conn);
  self->conns[ self->num_conns++ ] = conn;
  return 0;
}
void MrcacheServer_connection_lost( MrcacheServer* self, MrcacheProtocol* conn ) {
  DBG_MEMCAC printf("conn %p lost\n", conn);
  self->num_conns--;
  self->next_conn = 0;
  if ( self->num_conns == 0 ) {
    DBG_MEMCAC printf("  No more memcached connections\n");
    return;
  }

  // Remove the connection by shifting the rest left    
  MrcacheProtocol **p = self->conns;
  for (int i = 0; i < self->num_conns+1; i++) {
    p[i] = self->conns[i];
    if ( self->conns[i] != conn ) p++;
  }
  
}

int MrcacheServer_set( MrcacheServer *self, char *k, int ksz, char* d, int dsz ) {
  if ( self->num_conns == 0 ) return -1;
  int c = self->next_conn++;
  if ( self->next_conn == self->num_conns ) self->next_conn = 0;
  return MrcacheProtocol_asyncSet( self->conns[c], k, d, dsz );
}
int MrcacheServer_get( MrcacheServer *self, char *k, void *fn, void *connection ) {
  if ( self->num_conns == 0 ) return -1;
  int c = self->next_conn++;
  if ( self->next_conn == self->num_conns ) self->next_conn = 0;
  MrcacheProtocol_asyncGet( self->conns[c], k, fn, connection );
  return 0;
}










