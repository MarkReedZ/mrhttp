#include "Python.h"
#include "string.h"
#include "response.h"
#include "common.h"

static char *resp_plain = "text/plain\r\n\r\n";
static char *resp_json  = "application/json\r\n\r\n";

static char *rbuf = NULL;//malloc(128*1024);
static char *errbuf = NULL;//malloc(16*1024);

PyObject* Response_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  Response* self = NULL;
  DBG printf("req new type %s\n", type->tp_name);
  self = (Response*)type->tp_alloc(type, 0);

  return (PyObject*)self;
}

void setupResponseBuffer(char* buf) {
//static void setupResponseBuffer(Response *self) {
  if ( rbuf ==NULL ) {
    rbuf = malloc(128*1024);
    errbuf = malloc(16*1024);
  }
  char *p = rbuf;

  char *s;
  // Update updateDate and write_response if this changes
  s = "HTTP/1.1 200 OK\r\n"; memcpy(p, s, strlen(s)); p += strlen(s);
  s = "Content-Length: 1        \r\n"; memcpy(p, s, strlen(s)); p += strlen(s);
  s = "Server: MrHTTP/0.1.1\r\n"; memcpy(p, s, strlen(s)); p += strlen(s);
  s = "Date: Thu, 05 Apr 2018 22:54:19 GMT\r\n"; memcpy(p, s, strlen(s)); p += strlen(s);
  s = "Content-Type: text/html; charset=utf-8\r\n\r\n"; memcpy(p, s, strlen(s)); p += strlen(s);
  //DBG_RESP printf("Init resp buffer:\n%.*s", (int)(p-buf), buf);
  DBG_RESP printf("Init resp buffer:\n%.*s", (int)(p-rbuf), rbuf);
}

void Response_dealloc(Response* self) {
  Py_TYPE(self)->tp_free((PyObject*)self);
  //free(self->rbuf);
  //free(self->errbuf);
}

char *getResponseBuffer() {
  return rbuf;
}


int Response_init(Response* self, PyObject *args, PyObject* kw)
{
  DBG_RESP printf("response init\n");
  //self->rbuf = malloc(128*1024);
  //self->rbuf_size = 128*1024;
  //self->errbuf = malloc(4*1024);
  //self->errbuf_size = 4*1024;
  self->mtype = 0;
  //if ( !self->rbuf ) {
    //PyErr_NoMemory();
    //return -1;
  //}
  //setupResponseBuffer(self);
  return 0;
}


PyObject *response_updateDate(Response *self, PyObject *date) {
  Py_RETURN_NONE;
  //Py_ssize_t l;
  //char *d = PyUnicode_AsUTF8AndSize( date, &l );
  //char *p = rbuf;
  //p += strlen("HTTP/1.1 200 OK\r\n") + strlen("Server: MrHTTP/0.1.1\r\n") + strlen("Content-Length: 17       \r\n");
  //p += 6;
  //memcpy(p, d, l);
  //Py_RETURN_NONE;
}

// Returns the header length
int response_updateHeaders(Response *self) {
  int ret = 145;

  if ( self->mtype ) {
    char *p = rbuf;
    if      ( self->mtype == 1 ) { memcpy( p+117, resp_plain, 14 ); ret = 131; }
    else if ( self->mtype == 2 ) { memcpy( p+117, resp_json,  20 ); ret = 137; }
  } 
/*
  Py_ssize_t num;
  if((num = PyDict_Size(self->headers)) < 0) return 0;

  if(!num) goto skip_headers;

  PyObject *name, *value;
  Py_ssize_t pos = 0;

  while (PyDict_Next(self->headers, &pos, &name, &value)) {
    const char* k, v;
    Py_ssize_t klen, vlen;

    if(!(k = PyUnicode_AsUTF8AndSize(name, &name_len))) return 0;

    memcpy(self->buffer + buffer_offset, cname, (size_t)name_len);
    buffer_offset += (size_t)name_len;

    *(self->buffer + buffer_offset) = ':';
    buffer_offset++;
    *(self->buffer + buffer_offset) = ' ';
    buffer_offset++;

    if(!(cvalue = PyUnicode_AsUTF8AndSize(value, &value_len)))
      goto error;

    memcpy(self->buffer + buffer_offset, cvalue, (size_t)value_len);
    buffer_offset += (size_t)value_len;

    CRLF
  }

skip_headers:
*/
  return ret;
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
  char *p = errbuf;
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
  char *p = errbuf;
  char *s;
  char body[512];
  sprintf( body, "<html><head><title>%d %s</title></head><body><h1>%s</h1><p>%s</p></body></html>", code, reason, reason, msg );
  int blen = strlen(body);   
  
  sprintf(p, "HTTP/1.1 %d %s\r\nServer: MrHTTP/0.1.1\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %d\r\n", code, reason, blen);
  p += strlen(p);
  memcpy(p, 
         rbuf + strlen("HTTP/1.1 200 OK\r\n") + strlen("Server: MrHTTP/0.1.1\r\n") + strlen("Content-Length: 17       \r\n"), 
         strlen("Date: Thu, 05 Apr 2018 22:54:19 GMT\r\n"));
  p += strlen("Date: Thu, 05 Apr 2018 22:54:19 GMT\r\n");

  s = "\r\n"; memcpy(p, s, strlen(s)); p += strlen(s);
  memcpy(p, body, blen);
 
  return PyBytes_FromStringAndSize( errbuf, p - errbuf + blen  ); 
  //DBG_RESP printf("Init resp buffer:\n%.*s", (int)(p-self->rbuf), self->rbuf);
}

  
