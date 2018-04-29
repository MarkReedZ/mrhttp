#pragma once

#include <Python.h>
#include <stdbool.h>

#include "request.h"

typedef struct {
  PyObject *func;
  char *path;
  char **segs;
  int *segLens;
  int  numSegs;
  long len;
  bool iscoro;
  bool session;
} Route;

typedef struct {

  Route *routes;
  Route *staticRoutes;
  int numRoutes; 
  int numStaticRoutes;
  void *protocol;

} Router;

int router_init(Router* self, void* protocol);
void router_dealloc(Router* self);
Route* router_getRoute(Router* self, Request* request);
