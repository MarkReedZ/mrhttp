#pragma once

#include <Python.h>
#include <stdbool.h>
#include "response.h"
#include "mrhttpparser.h"

//#include "module.h"

typedef struct Request Request;

struct Request {
  PyObject_HEAD

  struct mr_request hreq;

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
  char argTypes[16];
  int numArgs;
  bool inprog;
  bool return404;

  char* session_id;
  int session_id_sz;
  PyObject *set_user;
  //char* session_value;
  void *route;

  PyObject* py_path;
  PyObject* py_method;
  PyObject* transport;
  PyObject* app;
  PyObject* py_ip;
  PyObject* py_headers;
  PyObject* py_cookies;
  PyObject* py_body;
  PyObject* py_query_string;
  PyObject* py_args;
  PyObject* py_json;
  PyObject* py_mrpack;
  PyObject* py_form;
  PyObject* py_file;
  PyObject* py_files;
  PyObject* py_mrq_servers_down;
  PyObject* py_user;

  Response* response;
}; // Request;


PyObject* Request_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
int Request_init(Request* self, PyObject *args, PyObject *kwds);
void Request_dealloc(Request* self);

void Request_reset(Request *self);
PyObject* Request_cleanup(Request* self);

PyObject* Request_notfound(Request* self);

void request_load(Request* self, char* method, size_t method_len, char* path, size_t path_len, int minor_version, struct mr_header* headers, size_t num_headers);
//void Request_set_body(Request* self, char* body, size_t body_len);
PyObject* Request_add_done_callback(Request* self, PyObject* callback);

PyObject* Request_getattro(Request* self, PyObject* name);
PyObject* Request_get_path(Request* self, void* closure);
PyObject* Request_get_method(Request* self, void* closure);
PyObject* Request_get_transport(Request* self, void* closure);
PyObject* Request_get_headers(Request* self, void* closure);
PyObject* Request_get_ip(Request* self, void* closure);
PyObject* Request_get_cookies(Request* self, void* closure);
PyObject* Request_get_body(Request* self, void* closure);
PyObject* Request_get_query_string(Request* self, void* closure);
PyObject* Request_get_query_args(Request* self, void* closure);

char* request_getDecodedPath(Request* self);
void request_decodePath(Request* self);

PyObject *request_updateDate(Request *self, PyObject *date);
void Request_load_cookies(Request* self);
void Request_load_session(Request* self);

PyObject* Request_parse_mp_form(Request* self);
PyObject* Request_parse_urlencoded_form(Request* self);

