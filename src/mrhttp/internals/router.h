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
  bool mrq;
  char mtype;
} Route;

typedef struct {
  PyObject_HEAD

  Route *routes;
  Route *staticRoutes;
  int numRoutes; 
  int numStaticRoutes;

} Router;

PyObject *Router_new    (PyTypeObject* self, PyObject *args, PyObject *kwargs);
int       Router_init   (Router* self, PyObject *args, PyObject *kwargs);
void      Router_dealloc(Router* self);
PyObject *Router_setupRoutes(Router* self);

Route* router_getRoute(Router* self, Request* request);
