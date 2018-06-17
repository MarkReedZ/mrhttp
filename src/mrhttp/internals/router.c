

#include <Python.h>
#include <stdbool.h>

#include "common.h"
#include "router.h"
#include "request.h"
#include "module.h"

PyObject* Router_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
  Router* self = NULL;
  self = (Router*)type->tp_alloc(type, 0);
  return (PyObject*)self;
}

void Router_dealloc(Router* self) {
  if ( self->staticRoutes ) {
    Route *r = self->staticRoutes;
    for (int i = 0; i<self->numStaticRoutes; i++,r++ ) {
      Py_DECREF(r->func);
    }
    free (self->staticRoutes);
  }
  if ( self->routes ) {
    Route *r = self->routes;
    for (int i = 0; i<self->numRoutes; i++,r++ ) {
      free(r->segs);
      free(r->segLens);
    }
    free (self->routes);
  }
  Py_TYPE(self)->tp_free((PyObject*)self);
}


int Router_init(Router* self, PyObject *args, PyObject *kwargs)
{
  self->staticRoutes = NULL;
  self->routes       = NULL;
  self->numStaticRoutes = 0;
  self->numRoutes = 0;
  return 0;
}

static int numInString( char c, char* s, int len ) {
  int ret = 0;
  for (int i=0; i<len; i++) {
    if ( s[i] == c ) ret += 1;
  }
  return ret;
}


PyObject *Router_setupRoutes (Router* self) {
  //PyObject *sroutes = self->pyStaticRoutes; //PyObject_GetAttrString((PyObject*)self, "static_routes");    
  //PyObject *routes  = self->pyRoutes; //PyObject_GetAttrString((PyObject*)self, "routes");    
  PyObject *sroutes = PyObject_GetAttrString((PyObject*)self, "static_routes");    
  PyObject *routes  = PyObject_GetAttrString((PyObject*)self, "routes");    
  int l = PyList_Size(sroutes);

  self->staticRoutes = malloc( l * sizeof(Route) );
  self->numStaticRoutes = l;

  PyObject *r, *o;

  Route *rte = self->staticRoutes;
  for (int i = 0; i<l; i++,rte++ ) {
    r = PyList_GetItem(sroutes, i);
    rte->iscoro  = false;
    rte->session = false;
    rte->mrq = false;

    PyObject *handler = PyLong_AsVoidPtr(PyDict_GetItemString( r, "handler" ));
    rte->func = handler;
    if ( !(o = PyDict_GetItemString( r, "path" )) ) goto error;
    rte->path = PyUnicode_AsUTF8AndSize( o, &(rte->len) );
    if ( Py_True == PyDict_GetItemString( r, "iscoro"  ) ) rte->iscoro  = true;
    if ( Py_True == PyDict_GetItemString( r, "session" ) ) rte->session = true;
    if ( Py_True == PyDict_GetItemString( r, "mrq" ) ) rte->mrq = true;
    o = PyDict_GetItemString( r, "type"  );
    if (o) rte->mtype = PyLong_AsLong(o);

    DBG printf(" path %.*s func ptr %p\n", (int)rte->len, rte->path, rte->func);
  }

  rte = self->staticRoutes;
  for (int i = 0; i<l; i++,rte++ ) {
    Py_INCREF(rte->func);
  }

  if ( routes != NULL ) l = PyList_Size(routes);
  else          l = 0;
  DBG printf(" len routes %d\n", l );

  self->routes = malloc( l * sizeof(Route) );
  self->numRoutes = l;

  rte = self->routes;
  for (int i = 0; i<l; i++,rte++ ) {
    r = PyList_GetItem(routes, i);
    o = PyDict_GetItemString( r, "handler" );
    if ( !o ) goto error;
    PyObject *handler = PyLong_AsVoidPtr( o );
    rte->func = handler;
    if ( !(o = PyDict_GetItemString( r, "path" )) ) goto error;
    rte->path = PyUnicode_AsUTF8AndSize( o, &(rte->len) );
    DBG printf( " path len %ld str %.*s\n", rte->len, (int)rte->len, rte->path );

    rte->iscoro  = false; rte->session = false; rte->mrq = false;
    if ( Py_True == PyDict_GetItemString( r, "iscoro"  ) ) rte->iscoro = true;
    if ( Py_True == PyDict_GetItemString( r, "session" ) ) rte->session = true;
    if ( Py_True == PyDict_GetItemString( r, "mrq" ) ) rte->mrq = true;
    o = PyDict_GetItemString( r, "type"  );
    if (o) rte->mtype = PyLong_AsLong(o);

    // Segs
    int numSegs = numInString( '/', rte->path, rte->len );
    if ( rte->path[rte->len-1] == '/' ) numSegs -= 1;
    rte->numSegs = numSegs;
    DBG printf( " num segs %d\n", numSegs );
    rte->segs    = malloc( numSegs * sizeof(*(rte->segs)) );
    rte->segLens = malloc( numSegs * sizeof(int)  );
    char* rest = rte->path+1;
    size_t rest_len = rte->len-1;
    DBG printf( " rest %.*s\n", (int)rest_len, rest );
    for ( int j = 0; j < numSegs; j++ ) {
      char* slash = memchr(rest, '/', rest_len);
      if ( slash ) {
        rte->segs[j] = rest;
        rte->segLens[j] = slash - rest;
        //DBG printf("rte seg len %d\n", rte->segLens[j] );
        DBG printf("rte seg >%.*s<\n", rte->segLens[j], rte->segs[j]);
        //DBG printf("seglen  %d\n", rte->segLens[j] );
        rest_len -= slash-rest+1;
        rest = slash+1;
        //DBG printf( " rest len %d str %.*s\n", rest_len, rest_len, rest );
      } else {
        rte->segs[j] = rest;
        rte->segLens[j] = rest_len;
        DBG printf("rte seg >%.*s<\n", rte->segLens[j], rte->segs[j]);
      }
    }
    DBG printf("rte segements:\n");
    for ( int j = 0; j < numSegs; j++ ) {
      DBG printf("rte seg >%.*s<\n", rte->segLens[j], rte->segs[j]);
    }
    
  }

  Py_RETURN_NONE;
error:
  printf("ERROR in router init\n");
  //r["handler"] = id(handler)
  //r["iscoro"]  = asyncio.iscoroutinefunction(handler)
  //r["path"]    = uri
  //r["methods"] = methods
  return NULL;
}

// /foo/bar/ - staticRoutes
// /{}/foo/{}/ - routes
Route* router_getRoute(Router* self, Request* request) {
  DBG printf("router getRoute\n");
  request_decodePath( request );
  int plen = request->path_len;
  DBG printf("DELME path >%.*s<\n", plen, request->path );
 
  Route *r = self->staticRoutes;
  for (int i = 0; i<self->numStaticRoutes; i++,r++ ) {
    DBG printf("request path %.*s\n", (int)request->path_len, request->path);
    if ( plen == r->len && !memcmp(r->path, request->path, plen) ) {
      DBG printf("router found path %.*s == %.*s\n", (int)r->len, r->path, (int)request->path_len, request->path);
      return r;
    }
  }

  request->numArgs = 0;

  int numSegs = numInString( '/', request->path, plen );
  if ( request->path[plen-1] == '/' ) numSegs -= 1;

  r = self->routes;
  for (int i = 0; i<self->numRoutes; i++,r++ ) {
    if ( numSegs == r->numSegs ) { 
      DBG printf("Checking route path >%.*s<\n", (int)r->len, r->path );

      // Nip off the first /
      char *p = request->path+1;
      plen = request->path_len-1;

      for (int j = 0; j<numSegs; j++ ) {
        DBG printf(" remaining path %.*s\n", plen, p);
        char* slash = memchr(p, '/', plen);
        int seglen;
        if ( !slash ) seglen = plen;
        else          seglen = slash - p;
        if ( r->segs[j][0] == '{' ) {
          request->args[   request->numArgs] = p;
          request->argLens[request->numArgs] = seglen;
          request->numArgs += 1;
        } else {
          if ( seglen != r->segLens[j] ) goto no_match;
          DBG printf(" %.*s == %.*s\n", seglen, p, seglen, r->segs[j] );
          if ( memcmp(p, r->segs[j], r->segLens[j]) != 0) goto no_match;
        }
        p = slash+1;
        plen -= seglen+1;
      }

      DBG printf("router found path %.*s == %.*s\n", (int)r->len, r->path, (int)request->path_len, request->path);
      return r;
    }
no_match:
    request->numArgs = 0;
  }
  return NULL;
}





