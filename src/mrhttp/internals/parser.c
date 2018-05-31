

#include <strings.h>
#include <sys/param.h>
#include <immintrin.h>

#include "parser.h"
#include "common.h"
#include "module.h"
//#include "cpu_features.h"


//#include "protocol.h"
#include "faststrcmp.h"
//int fast_compare(const unsigned char *s1, const unsigned char *s2, size_t len)

static void _reset(Parser* self, bool reset_buffer) {
  self->parsed_headers = 0;
  self->body_length = 0;
  if ( reset_buffer ) {
    self->start = self->buf;
    self->end = self->buf;
  }
}


int parser_init(Parser *self, void *protocol) {
  DBG printf("parser init\n");
  self->protocol = protocol;
  self->buf = malloc(8096); self->buf_size = 8096;
  if ( !self->buf ) return 0;
  _reset(self, true);

  //PyObject *pObj = NULL;
  //pObj = PyBytes_FromString("Hello world\n");
  //printf("parser init hello world\n");
  //PyObject* func = NULL;
  //PyObject* tmp;
  //if(!(func = PyObject_GetAttrString(((Protocol*)protocol)->app, "wtf"))) return NULL;
  //if(!(tmp = PyObject_CallFunction(func, NULL))) return NULL;
  //if(!(func = PyObject_GetAttrString(((Protocol*)protocol)->app, "echo"))) return NULL;
  //if(!(tmp = PyObject_CallFunctionObjArgs(func, pObj, NULL))) return NULL;
  //if(!(tmp = PyObject_CallFunctionObjArgs(func, protocol->app, pObj))) return NULL;
  //if(!(tmp = PyObject_CallFunctionObjArgs(func, self->app, (PyObject*)&self->request))) return NULL;
  //Py_DECREF(pObj); 
  return 1;
}

// TODO Check we don't exceed a max request size
int parser_data_received(Parser *self, PyObject *py_data, Request *request ) {
//#ifdef DEBUG_PRINT
  DBG printf("parser data\n");
//#endif
  char* data;
  Py_ssize_t datalen;
  int i;

  if(PyBytes_AsStringAndSize(py_data, &data, &datalen) == -1) {
    DBG printf("WARNING py bytes as string failed\n");
    goto error;
  }
  DBG printf("parser data\n%.*s\n",(int)datalen, data);

  //if ( unlikely( datalen > ( self->buf_size - (self->end-self->start) )) ) {
    //memmove(self->buf, self->start, self->end-self->start); 
    //self->end = self->buf + (self->end-self->start);
    //self->start = self->buf;
  //}
  // If we need more space increase the size of the buffer
  DBG printf("parser datalen %lu buflen %ld buffer size %d\n", datalen, (self->end-self->start), self->buf_size);
  if ( unlikely( (datalen+(self->end-self->start)) > self->buf_size) ) {
    while ( (datalen+(self->end-self->start)) > self->buf_size )  self->buf_size *= 2;
    self->buf = realloc( self->buf, self->buf_size );
    self->end = self->buf + (self->end-self->start);
    self->start = self->buf;
  }

  DBG printf("parser data endptr %p dataptr %p\n",self->end, data);
  memcpy(self->end, data, (size_t)datalen);
  self->end += (size_t)datalen;

parse_headers:

  if ( self->parsed_headers ) goto body;

  
  char *method, *path;
  int rc, minor_version;
  //struct phr_header headers[100];
  size_t prevbuflen = 0, method_len, path_len;//, num_headers;

  request->num_headers = 100; // Max allowed headers
  DBG_PARSER printf("before parser requests\n");

  rc = mr_parse_request(self->start, self->end-self->start, (const char**)&method, &method_len, (const char**)&path, &path_len, &minor_version, request->headers, &(request->num_headers), prevbuflen);
  DBG_PARSER printf("parser requests rc %d\n",rc);
  if ( rc < 0 ) return rc; // -2 incomplete, -1 error otherwise byte len of headers

  self->start += rc; 
  self->parsed_headers = 1;
  
  DBG_PARSER printf("request is %d bytes long\n", rc);
  DBG_PARSER printf("method is %.*s\n", (int)method_len, method);
  DBG_PARSER printf("path is %.*s\n", (int)path_len, path);
  DBG_PARSER printf("HTTP version is 1.%d\n", minor_version);
  DBG_PARSER printf("headers:\n");
  DBG_PARSER for (i = 0; i != request->num_headers; ++i) {
    printf("%.*s: %.*s\n", (int)request->headers[i].name_len, request->headers[i].name, (int)request->headers[i].value_len, request->headers[i].value);
  }

  if(minor_version == 0) self->conn_state = CONN_CLOSE;
  else                   self->conn_state = CONN_KEEP_ALIVE;
 
#define header_name_equal(val) \
  header->name_len == strlen(val) && fast_compare(header->name, val, header->name_len) == 0
#define header_value_equal(val) \
  header->value_len == strlen(val) && fast_compare(header->value, val, header->value_len) == 0

 for(struct mr_header* header = request->headers;
      header < request->headers + request->num_headers;
      header++) {

/*
Transfer-Encoding: chunked
Transfer-Encoding: compress
Transfer-Encoding: deflate
Transfer-Encoding: gzip
Transfer-Encoding: identity

// Several values can be listed, separated by a comma
Transfer-Encoding: gzip, chunked

with nginx proxy_request_buffering on which is the default we will never see chunked
    if(header_name_equal("Transfer-Encoding")) {
      if(header_value_equal("chunked"))
        self->transfer = PARSER_CHUNKED;
      else if(header_value_equal("identity"))
        self->transfer = PARSER_IDENTITY;
*/

    if(header_name_equal("Content-Length")) {
      //if(!header->value_len) {
        //error = invalid_headers;
        //goto on_error;
      //}
      char * endptr = (char *)header->value + header->value_len;
      self->body_length = strtol(header->value, &endptr, 10);
/* This would be faster than strol, but loses error checking
    unsigned int x = 0;
    while (*p != '\0') {
        x = (x*10) + (*p - '0');
        ++p;
    }
    return x;
*/
      // 0 means error from strtol, but it is also a valid value
      if ( self->body_length == 0 && !( header->value_len == 1 && *(header->value) == '0') ) { 
        //TODO ERROR
        //error = invalid_headers;
        goto error;
      }
      // If the value was not all digits we'll error here
      if(endptr != (char*)header->value + header->value_len) {
        //TODO ERROR
        //error = invalid_headers;
        goto error;
      }

    } else if(header_name_equal("Connection")) {
      if      (header_value_equal("close"))      self->conn_state = CONN_CLOSE;
      else if (header_value_equal("keep-alive")) self->conn_state = CONN_KEEP_ALIVE;
      else goto error;
      //TODO ERROR
    }
  } 

  if(!Protocol_on_headers( self->protocol, method, method_len, path, path_len, minor_version, request->headers, request->num_headers)) goto error; 

body:

  DBG_PARSER printf("body:\n%.*s", (int)(self->end-self->start),self->start);

  // No body
  //if ( self->body_length == 0 ) { }

  // Need more data
  if ( self->body_length > ( self->end - self->start ) ) return -2;

  if(!Protocol_on_body(self->protocol, self->start, self->body_length)) return -1;

  self->start += self->body_length; 

  // If we still have data start parsing the next request
  if ( self->start < self->end ) {
    _reset(self, false);
    goto parse_headers;
  }
  _reset(self, true);
  
// TODO CHUNKED body.  NGINX will never do chunked so I haven't bothered
  



  return 1;

error:
  DBG printf("ERROR in data recvd\n");
  //if(!Protocol_on_error(self->protocol, error)) goto error;
  return 0;
}

void parser_dealloc(Parser* self)
{
  //if(self->buffer != self->inline_buffer) free(self->buffer);
  free(self->buf);
}
