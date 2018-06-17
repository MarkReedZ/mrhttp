
#include <Python.h>
#include <stdbool.h>

#include "mrqprotocol.h"
#include "common.h"
#include "module.h"

//static char *resp_buf;

PyObject *MrqClient_new(PyTypeObject* type, PyObject *args, PyObject *kwargs) {
  MrqClient* self = NULL;
  self = (MrqClient*)type->tp_alloc(type, 0);
  return (PyObject*)self;
}


void MrqClient_dealloc(MrqClient* self) {
  Py_TYPE(self)->tp_free((PyObject*)self);
}

int MrqClient_init(MrqClient* self, PyObject *args, PyObject *kwargs) {
  if(!PyArg_ParseTuple(args, "i", &self->num_servers)) return 1;

  self->servers = malloc( sizeof(MrqServer*) * self->num_servers );
  for (int i = 0; i < self->num_servers; i++ ) {
    self->servers[i] = malloc( sizeof(MrqServer) );
    MrqServer_init( self->servers[i] );
  }

  //TODO which slot goes to which server
  //MrqClient_setupConnMap(self);

  return 0;
}

PyObject *MrqClient_cinit(MrqClient* self) {
  Py_RETURN_NONE;
}

PyObject *MrqClient_addConnection(MrqClient* self, MrqProtocol *conn, int server) {

  MrqServer_addConnection( self->servers[server], conn );

  Py_RETURN_NONE;
}

void MrqClient_connection_lost( MrqClient* self, MrqProtocol* conn, int server ) {
  DBG_MRQ printf("conn %p lost server %d\n", conn, server);
  MrqServer_connection_lost( self->servers[server], conn );
  if ( self->servers[server]->num_conns == 0 ) {
    // TODO We need to poll until it comes back online and then redo connections
    // TODO setupConnMap without this server until we re-connect
  }
}



PyObject *MrqClient_push(MrqClient* self, int slot, char *d, int dsz) {
  //TODO pick right server based on slot
  int server = 0;
  int rc = MrqServer_push( self->servers[server], slot, d, dsz );
  if ( rc ) return NULL;//TODO?
  Py_RETURN_NONE;
}

int MrqServer_dealloc( MrqServer *self ) {
  free(self->conns);
  return 0;
}

int MrqServer_init( MrqServer *self ) {
  self->num_conns = 0;
  self->next_conn = 0;
  self->conns = malloc( sizeof(MrqProtocol*) * 16 );
  return 0;
}
int MrqServer_addConnection( MrqServer *self, MrqProtocol *conn) {
  self->conns[ self->num_conns++ ] = conn;
  return 0;
}
void MrqServer_connection_lost( MrqServer* self, MrqProtocol* conn ) {
  DBG_MRQ printf("server conn %p lost\n", conn);
  self->num_conns--;
  if ( self->num_conns == 0 ) return;

  // Remove the connection by shifting the rest left    
  MrqProtocol **p = self->conns;
  for (int i = 0; i < self->num_conns+1; i++) {
    p[i] = self->conns[i];
    if ( self->conns[i] != conn ) p++;
  }

}

int MrqServer_push(MrqServer* self, int slot, char *d, int dsz) {
  if ( self->num_conns == 0 ) return 1;
  int c = self->next_conn++;
  if ( self->next_conn == self->num_conns ) self->next_conn = 0;

  //printf("mrq server push: %.*s\n",dsz, d);
  MrqProtocol_push( self->conns[c], slot, d, dsz );
  return 0;
}


