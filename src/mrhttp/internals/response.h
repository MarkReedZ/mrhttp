#pragma once

#include <Python.h>
#include <stdbool.h>


#define RESPONSE_INITIAL_BUFFER_LEN 1024

typedef struct {
  PyObject_HEAD

  char* rbuf;
  unsigned long rbuf_size;
  char *errbuf;
  unsigned long errbuf_size;

  //bool opaque;
  //int minor_version;
  //KEEP_ALIVE keep_alive;

  //PyObject* code;
  PyObject* mime_type;
  //PyObject* body;
  //PyObject* encoding;
  PyObject* headers;
  PyObject* cookies;

  int mtype; // mime type 0 html 1 plain 2 json

  char* buffer;
  size_t buffer_len;
} Response;

PyObject* Response_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
int Response_init(Response* self, PyObject *args, PyObject *kwds);
void Response_dealloc(Response* self);

void Response_reset(Response *self);


PyObject *response_updateDate(PyObject *date);
int       response_updateHeaders(Response *self);

PyObject* Response_get_headers(Response* self, void* closure);
PyObject* Response_get_cookies(Response* self, void* closure);
int Response_set_cookies(Response *self, PyObject *value, void *closure);
int response_add_headers( Response *self, char *p );
int response_add_cookies( Response *self, char *p );

PyObject* response_getRedirectResponse( int code, char *url );
PyObject* response_getErrorResponse(    int code, char *reason, char *msg );

void response_setupResponseBuffer(void);
char *getResponseBuffer(int sz);
