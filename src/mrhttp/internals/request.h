#pragma once

#include <Python.h>
#include <stdbool.h>
#include "response.h"
//#include "module.h"

typedef struct {
  PyObject_HEAD

  char* method;
  size_t method_len;
  char* path;
  bool path_decoded;
  size_t path_len;
  bool qs_decoded;
  size_t qs_len;
  int minor_version;
  struct mr_header* headers;
  size_t num_headers;
  char* body;
  size_t body_len;

  char *args[16];
  int argLens[16];
  int numArgs;
  bool inprog;

  char* session_id;
  PyObject *set_user;
  //char* session_value;
  void *route;

  PyObject* py_path;
  PyObject* py_method;
  PyObject* transport;
  PyObject* app;
  PyObject* py_headers;
  PyObject* py_cookies;
  PyObject* py_body;
  PyObject* py_query_string;
  PyObject* py_args;

  Response* response;
} Request;


PyObject* Request_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
int Request_init(Request* self, PyObject *args, PyObject *kwds);
void Request_dealloc(Request* self);

void Request_reset(Request *self);
PyObject* Request_cleanup(Request* self);


void request_load(Request* self, char* method, size_t method_len, char* path, size_t path_len, int minor_version, struct mr_header* headers, size_t num_headers);
//void Request_set_body(Request* self, char* body, size_t body_len);
PyObject* Request_add_done_callback(Request* self, PyObject* callback);

PyObject* Request_getattro(Request* self, PyObject* name);
PyObject* Request_get_path(Request* self, void* closure);
PyObject* Request_get_method(Request* self, void* closure);
PyObject* Request_get_transport(Request* self, void* closure);
PyObject* Request_get_headers(Request* self, void* closure);
PyObject* Request_get_cookies(Request* self, void* closure);
PyObject* Request_get_body(Request* self, void* closure);
PyObject* Request_get_query_string(Request* self, void* closure);
PyObject* Request_get_query_args(Request* self, void* closure);

char* request_getDecodedPath(Request* self);
void request_decodePath(Request* self);

PyObject *request_updateDate(Request *self, PyObject *date);
void Request_load_cookies(Request* self);
