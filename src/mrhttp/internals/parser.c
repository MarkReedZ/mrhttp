

#include <strings.h>
#include <sys/param.h>
#include <immintrin.h>

#include "parser.h"
#include "common.h"
#include "module.h"
#include "unpack.h"
//#include "cpu_features.h"

//#include "protocol.h"
#include "faststrcmp.h"
//int fast_compare(const unsigned char *s1, const unsigned char *s2, size_t len)

// DELME
static void print_buffer( char* b, int len ) {
  for ( int z = 0; z < len; z++ ) {
    printf( "%02x ",(int)b[z]);
  }
  printf("\n");
}

static inline bool _isdigit(char c)  { return  c >= '0'  && c <= '9'; }
static long my_strtol( char* s, int maxlen ) {
  long l = 0;
  int n = 0;
  while (_isdigit(*s)) {

    l = (l * 10) + (*s++ - '0');
    n += 1;
    if ( n >= maxlen ) return l;
  }
  return l;
}

static void _reset(Parser* self, bool reset_buffer) {
  self->body_length = 0;
  if ( reset_buffer ) {
    self->start = self->buf;
    self->end = self->buf;
  }
}


int parser_init(Parser *self, void *protocol) {
  DBG printf("parser_init\n");
  self->protocol = protocol;
  self->buf = malloc(8096); self->buf_size = 8096;
  if ( !self->buf ) return 0;
  _reset(self, true);

  return 1;
}

// TODO Check we don't exceed a max request size.  Nginx does this for us though
//      Sanic:  app.config.REQUEST_MAX_SIZE = 300_000_000
int parser_data_received(Parser *self, PyObject *py_data, Request *request ) {
  DBG printf("parser data\n");
  char* data;
  Py_ssize_t datalen;
  size_t i;

  if(PyBytes_AsStringAndSize(py_data, &data, &datalen) == -1) {
    DBG printf("WARNING py bytes as string failed\n");
    goto error;
  }
  DBG_PARSER printf("parser data\n%.*s\n",(int)datalen, data);

  // If we need more space increase the size of the buffer
  DBG_PARSER printf("parser datalen %zu buflen %ld buffer size %d\n", datalen, (self->end-self->start), self->buf_size);
  if ( unlikely( (datalen+(self->end-self->start)) > self->buf_size) ) {
    while ( (datalen+(self->end-self->start)) > self->buf_size )  self->buf_size *= 2;
    int l = (self->end - self->buf);
    self->buf = realloc( self->buf, self->buf_size );
    self->end = self->buf + l;
    self->start = self->buf;
    DBG printf("Increasing buffer size %d end %p st %p\n", self->buf_size, self->end, self->start);
  }
  DBG_PARSER printf("buf    data\n%.*s\n",(int)(self->end-self->buf), self->buf);

  DBG_PARSER printf("parser data startptr %p endptr %p dataptr %p\n",self->start, self->end, data);
  memcpy(self->end, data, (size_t)datalen);
  self->end += (size_t)datalen;

parse_headers:

  if (0) {}  

  char *method, *path;
  int rc, minor_version;
  size_t method_len, path_len;

  request->num_headers = 100; // Max allowed headers
  DBG_PARSER printf("before parser requests\n");
  //request->hreq.flags = 0; // TODO This isn't currently used in the parser
  rc = mr_parse_request(self->start, self->end-self->start, (const char**)&method, &method_len, (const char**)&path, &path_len, &minor_version, request->headers, &(request->num_headers), &(request->hreq));

  DBG_PARSER printf("parser requests rc %d\n",rc);
  if ( rc < 0 ) return rc; // -2 incomplete, -1 error otherwise byte len of headers

  self->start += rc; 
  
  DBG_PARSER printf("request is %d bytes long\n", rc);
  DBG_PARSER printf("method is %.*s\n", (int)method_len, method);
  DBG_PARSER printf("path is %.*s\n", (int)path_len, path);
  DBG_PARSER printf("HTTP version is 1.%d\n", minor_version);
  DBG_PARSER printf("headers:\n");
  DBG_PARSER for (i = 0; i != request->num_headers; ++i) {
    printf("%.*s: %.*s\n", (int)request->headers[i].name_len, request->headers[i].name, (int)request->headers[i].value_len, request->headers[i].value);
  }

  if(minor_version == 0) request->keep_alive = false;
  else                   request->keep_alive = true;
 
  //self->body_length = request->hreq.body_length;

#define name_compare(val) \
  header->name_len == strlen(val) && fast_compare(header->name, val, header->name_len) == 0
#define value_compare(val) \
  header->value_len == strlen(val) && fast_compare(header->value, val, header->value_len) == 0

 for(struct mr_header* header = request->headers;
      header < request->headers + request->num_headers;
      header++) {

    if(name_compare("Content-Type")) {
      if ( header->value[0] == 'a' && header->value_len == 20 ) { //"application/mrpacker"
        request->hreq.flags = 2;
      } 
    }
    if(name_compare("Content-Length")) {
      self->body_length = my_strtol(header->value, header->value_len);

      // TODO If the request is too large.  I think we already checked

      // Check for a bad 0 length - ie non digits for the length
      if ( self->body_length == 0 && !( header->value_len == 1 && *(header->value) == '0') ) { 
        goto error;
      }

    } else if(name_compare("Connection")) {
      if      (value_compare("close"))      request->keep_alive = false;
    }
  } 

  if(!Protocol_on_headers( self->protocol, method, method_len, path, path_len, minor_version, request->headers, request->num_headers)) goto error; 

//body:

  DBG_PARSER printf("body:\n%.*s\n", (int)(self->end-self->start),self->start);

  // Need more data
  if ( self->body_length > ( self->end - self->start ) ) {
    while ( (self->body_length+(self->end-self->start)) > self->buf_size )  self->buf_size *= 2;
    int l = (self->end - self->buf);
    self->buf = realloc( self->buf, self->buf_size );
    self->end = self->buf + l;
    self->start = self->buf;
    DBG printf("Increasing buffer size %d end %p st %p\n", self->buf_size, self->end, self->start);
    return -2;
  }

  if ( request->hreq.flags == 2 ) {
    // TODO how to handle errors.  Have unpackc not do a python error? Or clear py error if null...  Return negative? 
    request->py_mrpack = unpackc( self->start, self->body_length ); 
    if ( request->py_mrpack == NULL ) {
      printf("WARNING unpackc returned null in parser\n");
      printf("WARNING data len %ld\n",self->body_length);
      print_buffer(self->start, 16);
      return -1;
    }

  }

  if(!Protocol_on_body(self->protocol, self->start, self->body_length)) return -1;

  self->start += self->body_length; 

  // If we still have data start parsing the next request
  if ( self->start < self->end ) {
    _reset(self, false);
    goto parse_headers;
  }
  _reset(self, true);
  
// TODO CHUNKED body.  NGINX will never do chunked.
  



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
