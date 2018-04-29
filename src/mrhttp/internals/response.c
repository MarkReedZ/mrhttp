#include "Python.h"
#include "string.h"
#include "response.h"
#include "common.h"

PyObject* Response_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  Response* self = NULL;
  DBG printf("req new type %s\n", type->tp_name);
  self = (Response*)type->tp_alloc(type, 0);

  return (PyObject*)self;
}

static void setupResponseBuffer(Response *self) {
  char *p = self->rbuf;
  char *s;
  // Update updateDate and write_response if this changes
  s = "HTTP/1.1 200 OK\r\n"; memcpy(p, s, strlen(s)); p += strlen(s);
  s = "Content-Length: 1        \r\n"; memcpy(p, s, strlen(s)); p += strlen(s);
  s = "Server: MrHTTP/0.1.1\r\n"; memcpy(p, s, strlen(s)); p += strlen(s);
  s = "Date: Thu, 05 Apr 2018 22:54:19 GMT\r\n"; memcpy(p, s, strlen(s)); p += strlen(s);
  s = "Content-Type: text/html; charset=utf-8\r\n\r\n"; memcpy(p, s, strlen(s)); p += strlen(s);
  DBG_RESP printf("Init resp buffer:\n%.*s", (int)(p-self->rbuf), self->rbuf);
}

void Response_dealloc(Response* self) {
  Py_TYPE(self)->tp_free((PyObject*)self);
  free(self->rbuf);
  free(self->errbuf);
}


int Response_init(Response* self, PyObject *args, PyObject* kw)
{
  DBG printf( "Response init\n");
  self->rbuf = malloc(128*1024);
  self->rbuf_size = 128*1024;
  self->errbuf = malloc(4*1024);
  self->errbuf_size = 4*1024;
  if ( !self->rbuf ) {
    PyErr_NoMemory();
    return -1;
  }
  setupResponseBuffer(self);
  return 0;
}


PyObject *response_updateDate(Response *self, PyObject *date) {
  Py_ssize_t l;
  char *d = PyUnicode_AsUTF8AndSize( date, &l );
  char *p = self->rbuf;
  p += strlen("HTTP/1.1 200 OK\r\n") + strlen("Server: MrHTTP/0.1.1\r\n") + strlen("Content-Length: 17       \r\n");
  p += 6;
  memcpy(p, d, l);
  Py_RETURN_NONE;
}

PyObject* Response_get_headers(Response* self, void* closure)
{
  if(!self->headers) {
    self->headers = PyDict_New();
  }
  Py_XINCREF(self->headers);
  return self->headers;
}

PyObject* Response_get_cookies(Response* self, void* closure)
{
  if(!self->cookies) {
    self->cookies = PyDict_New();
  }
  Py_XINCREF(self->cookies);
  return self->cookies;
}

PyObject *response_getRedirectResponse(Response *self, int code, char *url ) {
  char *p = self->errbuf;
  char body[1024];
  sprintf( body, "This resource can be found at %s", url );
  int blen = strlen(body);   
  
  sprintf(p, "HTTP/1.1 %d Moved\r\nLocation: %s\r\nContent-Length: %d\r\n\r\n", code, url, blen);
  p += strlen(p);
  memcpy(p, body, blen);
  return PyBytes_FromStringAndSize( self->errbuf, p - self->errbuf + blen  ); 
}

PyObject *response_getErrorResponse(Response *self, int code, char *reason, char *msg) {
  DBG_RESP printf("response getErrorResponse code %d reason %s msg %s\n", code, reason, msg);
  char *p = self->errbuf;
  char *s;
  char body[512];
  sprintf( body, "<html><head><title>%d %s</title></head><body><h1>%s</h1><p>%s</p></body></html>", code, reason, reason, msg );
  int blen = strlen(body);   
  
  sprintf(p, "HTTP/1.1 %d %s\r\nServer: MrHTTP/0.1.1\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %d\r\n", code, reason, blen);
  p += strlen(p);
  memcpy(p, 
         self->rbuf + strlen("HTTP/1.1 200 OK\r\n") + strlen("Server: MrHTTP/0.1.1\r\n") + strlen("Content-Length: 17       \r\n"), 
         strlen("Date: Thu, 05 Apr 2018 22:54:19 GMT\r\n"));
  p += strlen("Date: Thu, 05 Apr 2018 22:54:19 GMT\r\n");

  s = "\r\n"; memcpy(p, s, strlen(s)); p += strlen(s);
  memcpy(p, body, blen);
 
  return PyBytes_FromStringAndSize( self->errbuf, p - self->errbuf + blen  ); 
  //DBG_RESP printf("Init resp buffer:\n%.*s", (int)(p-self->rbuf), self->rbuf);
}

  
