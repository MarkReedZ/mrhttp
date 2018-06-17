
#include "Python.h"
#include "string.h"
#include "response.h"
#include "common.h"

static char *resp_plain = "text/plain\r\n\r\n";
static char *resp_json  = "application/json\r\n\r\n";

static char *rbuf   = NULL;
static char *errbuf = NULL;
static int rbuf_sz = 0;

PyObject* Response_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  Response* self = NULL;
  self = (Response*)type->tp_alloc(type, 0);

  return (PyObject*)self;
}

void response_setupResponseBuffer(void) {
  if ( rbuf == NULL ) {
    rbuf_sz = 128*1024; rbuf = malloc(rbuf_sz);
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
  DBG_RESP printf("Init resp buffer:\n%.*s", (int)(p-rbuf), rbuf);
}

void Response_dealloc(Response* self) {
  Py_TYPE(self)->tp_free((PyObject*)self);
  //free(self->rbuf);
  //free(self->errbuf);
  Py_XDECREF(self->cookies);
  Py_XDECREF(self->headers);
}

void Response_reset(Response *self) {
  self->headers = NULL;
  self->cookies = NULL;
}


char *getResponseBuffer(int sz) {
  if ( sz > rbuf_sz ) {
    while ( sz > rbuf_sz ) rbuf_sz *= 2;
    rbuf = realloc( rbuf, rbuf_sz );
  }
  return rbuf;
}

int Response_init(Response* self, PyObject *args, PyObject* kw)
{
  self->mtype = 0;
  self->headers = NULL;
  self->cookies = NULL;
  return 0;
}

PyObject *response_updateDate(PyObject *date) {
  Py_ssize_t l;
  char *d = PyUnicode_AsUTF8AndSize( date, &l );
  char *p = rbuf;
  p += strlen("HTTP/1.1 200 OK\r\n") + strlen("Server: MrHTTP/0.1.1\r\n") + strlen("Content-Length: 1        \r\n");
  p += 6;
  memcpy(p, d, l);
  Py_RETURN_NONE;
}

// Returns the header length
int response_updateHeaders(Response *self) {
  int ret = 145;

  // Set Content-Type:  1 plain, 2 json, default is html
  if ( self->mtype ) {
    char *p = rbuf;
    if      ( self->mtype == 1 ) { memcpy( p+117, resp_plain, 14 ); ret = 131; }
    else if ( self->mtype == 2 ) { memcpy( p+117, resp_json,  20 ); ret = 137; }
  } 

  if ( self->headers != NULL ) {
    int hlen = response_add_headers( self, rbuf+ret-2 );
    if ( hlen ) ret += hlen-2;
    // else TODO raise an error or just ignore 
  } else {
  
    // As we reuse a single buffer for the response if we aren't writing headers we need to make sure that
    // the headers ends in \r\n
    char *p = rbuf+ret-2;
    *p++ = '\r';
    *p++ = '\n';
  }
  if ( self->cookies != NULL ) {
    int l = response_add_cookies( self, rbuf+ret-2 );
    if ( l ) ret += l-2;
    // else TODO raise an error or just ignore 
  }
  Py_XDECREF(self->cookies);
  Py_XDECREF(self->headers);
  self->headers = NULL;
  self->cookies = NULL;
  return ret;
}

int response_add_headers( Response *self, char *p ) {
  int ret = 0;
  Py_ssize_t num;
  num = PyDict_Size(self->headers);
  if(!num) return 0;
  char *s = p;

  PyObject *name, *value;
  Py_ssize_t pos = 0;

  while (PyDict_Next(self->headers, &pos, &name, &value)) {
    char* k;
    char* v;
    Py_ssize_t klen, vlen;

    //PyObject_Print( name, stdout, 0 ); printf("\n");
    //PyObject_Print( value, stdout, 0 ); printf("\n");

    // TODO This returns a bad v.  No matter how I do AsUTF8 its bad so convert to bytes first 
    //v = PyUnicode_AsUTF8AndSize(value, &vlen);

    PyBytes_AsStringAndSize( PyUnicode_AsUTF8String(value), &v, &vlen);
    if(!(k = PyUnicode_AsUTF8AndSize(name, &klen))) return 0;
    if ( v == NULL ) return 0;

    // If Content-Type modify that header
    //if ( k[0] == 'C' && klen == 12 && k[11] == 'e' ) {
      //memcpy( rbuf+117, v, (size_t)vlen );
    //} else {
    memcpy(p, k, (size_t)klen); p += (size_t)klen;

    *p++ = ':';
    *p++ = ' ';

    memcpy(p, v, (size_t)vlen); p += (size_t)vlen;

    *p++ = '\r';
    *p++ = '\n';
    //}

  }
  *p++ = '\r';
  *p++ = '\n';

  return p-s;
}

int response_add_cookies( Response *self, char *p ) {
  Py_ssize_t clen;
  if((clen = PyObject_Size(self->cookies)) < 0) return 0;

  if(!clen) return 0;

  PyObject* c_str = NULL;
  PyObject* c_bytes = NULL;

  if(!(c_str = PyObject_Str(self->cookies))) return 0;

  if(!(c_bytes = PyUnicode_AsASCIIString(c_str))) return 0;

  char* cptr;
  if(PyBytes_AsStringAndSize(c_bytes, &cptr, &clen) == -1) return 0;

  memcpy(p, cptr, (size_t)clen); p += (size_t)clen;
  *p++ = '\r';
  *p++ = '\n';
  *p++ = '\r';
  *p++ = '\n';

  return clen+4;
}

PyObject* Response_get_headers(Response* self, void* closure)
{
  if(!self->headers) {
    self->headers = PyDict_New();
  }
  Py_XINCREF(self->headers);
  return self->headers;
}

int Response_set_cookies(Response *self, PyObject *value, void *closure) {
  self->cookies = value;
  Py_XINCREF(self->cookies);
  return 0;
}

PyObject* Response_get_cookies(Response* self, void* closure)
{
  if(!self->cookies) {
    self->cookies = PyDict_New();
  }
  Py_XINCREF(self->cookies);
  return self->cookies;
}

PyObject *response_getRedirectResponse(int code, char *url ) {
  char *p = errbuf;
  char body[1024];
  sprintf( body, "This resource can be found at %s", url );
  int blen = strlen(body);   
  
  sprintf(p, "HTTP/1.1 %d Moved\r\nLocation: %s\r\nContent-Length: %d\r\n\r\n", code, url, blen);
  p += strlen(p);
  memcpy(p, body, blen);
  return PyBytes_FromStringAndSize( errbuf, p - errbuf + blen  ); 
}

PyObject *response_getErrorResponse(int code, char *reason, char *msg) {
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
}

  
