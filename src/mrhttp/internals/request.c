
#include <stddef.h>
#include <sys/param.h>
#include <strings.h>
#include <string.h>

#include "common.h"
#include "utils.h"
#include "request.h"
#include "mrhttpparser.h"

#ifdef __AVX2__
#include <immintrin.h>
#elif defined __SSE4_2__
#ifdef _MSC_VER
#include <nmmintrin.h>
#else
#include <x86intrin.h>
#endif
#endif

#ifdef _MSC_VER
#define ALIGNED(n) _declspec(align(n))
#else
#define ALIGNED(n) __attribute__((aligned(n)))
#endif

#if __GNUC__ >= 3
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#define hex_to_dec(x) \
  ((x <= '9' ? 0 : 9) + (x & 0x0f))
#define is_hex(x) ((x >= '0' && x <= '9') || (x >= 'A' && x <= 'F'))

#define CHAR4_TO_INT(a, b, c, d)         \
   (unsigned int)((d << 24) | (c << 16) | (b << 8) | a)


//#include "cresponse.h"
//#include "capsule.h"

//static PyObject* partial;


//static PyObject* HTTP10;
//static PyObject* HTTP11;
//static PyObject* request;


static unsigned long TZCNT(unsigned long long in) {
  unsigned long res;
  asm("tzcnt %1, %0\n\t" : "=r"(res) : "r"(in));
  return res;
}


PyObject* Request_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  Request* self = NULL;
  self = (Request*)type->tp_alloc(type, 0);
  self->set_user = NULL;
  self->response = NULL;

  return (PyObject*)self;
}

void Request_dealloc(Request* self) {

  free(self->headers);

  Py_XDECREF(self->set_user);
  Py_XDECREF(self->py_headers);
  Py_XDECREF(self->py_body);
  Py_XDECREF(self->py_cookies);
  Py_XDECREF(self->py_query_string);
  Py_XDECREF(self->py_args);
  Py_XDECREF(self->py_path);
  Py_XDECREF(self->py_method);
  Py_XDECREF(self->py_json);
  Py_XDECREF(self->py_mrpack);
  Py_XDECREF(self->py_form);
  Py_XDECREF(self->py_file);
  Py_XDECREF(self->py_files);
  Py_XDECREF(self->py_mrq_servers_down);
  Py_XDECREF(self->response);
  Py_XDECREF(self->set_user);
  Py_TYPE(self)->tp_free((PyObject*)self);
}


int Request_init(Request* self, PyObject *args, PyObject* kw)
{
  //self->hreq.headers = malloc( sizeof(*(self->hreq.headers))*100 ); //TODO
  self->headers = malloc( sizeof(*(self->headers))*100 ); //TODO
  if(!(self->response = (Response*)PyObject_GetAttrString((PyObject*)self, "response"))) return -1;
  if(!(self->set_user = PyObject_GetAttrString((PyObject*)self, "set_user"))) return -1;
  Request_reset(self);
  self->return404 = false;
  
  return 0;
}

// This only resets stuff that needs to be for reuse
void Request_reset(Request *self) {
  //self->inprog = false;
  self->session_id = NULL;
  Py_XDECREF(self->py_headers); self->py_headers = NULL;
  Py_XDECREF(self->py_body);   self->py_body = NULL;
  Py_XDECREF(self->py_path);   self->py_path = NULL;
  Py_XDECREF(self->py_method); self->py_method = NULL;
  Py_XDECREF(self->py_cookies);self->py_cookies = NULL; 
  Py_XDECREF(self->py_json);   self->py_json = NULL;
  Py_XDECREF(self->py_mrpack); self->py_mrpack = NULL;
  Py_XDECREF(self->py_form);   self->py_form = NULL;
  Py_XDECREF(self->py_file);   self->py_file = NULL;
  Py_XDECREF(self->py_files);  self->py_files= NULL;
  Py_XDECREF(self->py_user);   self->py_user= NULL;
  self->hreq.ip = NULL;
  self->hreq.flags = 0;
  Py_XDECREF(self->py_mrq_servers_down);  self->py_mrq_servers_down= NULL;
  self->num_headers = 0;
  Response_reset(self->response);
}

void request_load(Request* self, char* method, size_t method_len, char* path, size_t path_len, int minor_version, struct mr_header* headers, size_t num_headers)
{
  DBG printf("request load\n");
  // fill
  self->method = method;
  self->method_len = method_len;
  self->path = path;
  self->path_decoded = false;
  self->path_len = path_len;
  self->qs_len = 0;
  self->qs_decoded = false;
  self->minor_version = minor_version;
  //self->headers = headers;
  //self->num_headers = num_headers;
  //self->keep_alive = KEEP_ALIVE_UNSET;


}
PyObject* Request_cleanup(Request* self) {
  Py_XDECREF(self->set_user); self->set_user = NULL;
  Py_RETURN_NONE;
}

PyObject* Request_add_done_callback(Request* self, PyObject* callback)
{
  Py_RETURN_NONE;
}

PyObject* Request_get_transport(Request* self, void* closure) {
  if (self->transport) {
    Py_INCREF(self->transport);
    return self->transport;
  }
  Py_RETURN_NONE;
}

static inline int path_decode(char* buf, int len, int *qs_len) {
  unsigned long msk;
  int i=0,tz; // 32B index
  int cnt = 0;
  unsigned int shifted;
  char *sbuf = buf;
  char *obuf = buf;
  char *buf_end = buf+len;
  char *wbuf;
  int found = 0;

  __m256i m37 = _mm256_set1_epi8(37);
  __m256i m63 = _mm256_set1_epi8(63);

  do {
    const char *block_start = obuf+64*i;
    __m256i b0 = _mm256_loadu_si256((const __m256i *) block_start);
    __m256i b1 = _mm256_loadu_si256((const __m256i *) (block_start+32));
    msk = (unsigned int) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b0, m37), _mm256_cmpeq_epi8(b0, m63) ) )  |
        ((unsigned long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b1, m37), _mm256_cmpeq_epi8(b1, m63) ) ) << 32);

    while (1) {

      //if ( buf >= buf_end ) { goto decdone; }
      shifted = buf-block_start;
      if ( shifted >= 64 ) break;
      tz = TZCNT((msk >> shifted));
      if ( tz < 64 ) {
        buf += tz;
        //printf( " fnd >%.*s<\n", (int)(buf-sbuf), sbuf );  
        if ( buf >= buf_end ) { goto decdone; }
        if ( *buf == '?' ) {
          len -= buf_end-buf;
          *qs_len = buf_end-buf;
          //printf("path_decode len %d path >%.*s<\n", (int)len, (int)len, obuf);
          goto decdone;
        }
        if ( *buf == '%' ) {
          if ( found ) {
            memcpy( wbuf, sbuf, buf-sbuf );
            wbuf += buf-sbuf;
            *wbuf = (hex_to_dec(buf[1]) << 4) + hex_to_dec(buf[2]);
            wbuf++;
          } else {
            found = 1;
            *buf = (hex_to_dec(buf[1]) << 4) + hex_to_dec(buf[2]);
            wbuf = buf+1;
          }
          len -= 2;
          buf += 3;
        }
        sbuf = buf;
      } else {
        buf += 64 - shifted;
        break;
      }

    }
    i+=1;
    if ( buf >= buf_end ) { goto decdone; }
  } while ( buf < buf_end ); // Why doesn't this work

decdone:
  if ( found ) {
    memcpy( wbuf, sbuf, buf_end-sbuf );
  }
  return len;
}

static inline PyObject* parse_query_args( char *buf, size_t len ) {
  unsigned long long msk;
  int i=0,tz; // 32B index
  unsigned int shifted;
  char *sbuf = buf;
  char *obuf = buf;
  int state = 0;
  int ignore_me = 0;

  PyObject* args = PyDict_New();
  PyObject* key = NULL; PyObject* value = NULL;

  if ( len == 0 ) return;
  //len = path_decode( buf, len, &ignore_me );
  char *buf_end = buf+len;

  __m256i m38 = _mm256_set1_epi8(38); // &
  __m256i m61 = _mm256_set1_epi8(61); // =
  do {
    const char *block_start = obuf+64*i;
    __m256i b0 = _mm256_loadu_si256((const __m256i *) block_start);
    __m256i b1 = _mm256_loadu_si256((const __m256i *) (block_start+32));
    msk = (unsigned int) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b0, m38), _mm256_cmpeq_epi8(b0, m61) ) )  |
        ((unsigned long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b1, m38), _mm256_cmpeq_epi8(b1, m61) ) ) << 32);
    while (1) {

      //if ( buf >= buf_end ) { goto decdone; }
      shifted = buf-block_start;
      if ( shifted >= 64 ) break;
      tz = TZCNT((msk >> shifted));
      if ( tz < 64 ) {
        buf += tz;
        if ( buf >= buf_end ) { goto pdone; }
        if ( *buf == '=' ) {
          if ( state == 1 ) { buf+=1; continue; }
          //printf( " key >%.*s<\n", (int)(buf-sbuf), sbuf );  
          //key = PyUnicode_FromStringAndSize(sbuf, buf-sbuf); 
          len = path_decode( sbuf, buf-sbuf, &ignore_me );
          key = PyUnicode_FromStringAndSize(sbuf, len);

          state = 1;
          buf += 1;
        }
        else if ( *buf == '&' ) {
          if ( state == 0 ) { buf+=1; continue; }
          //printf( " val >%.*s<\n", (int)(buf-sbuf), sbuf);
          //value = PyUnicode_FromStringAndSize(sbuf, buf-sbuf); 
          len = path_decode( sbuf, buf-sbuf, &ignore_me );
          value = PyUnicode_FromStringAndSize(sbuf, len);
          PyDict_SetItem(args, key, value);  
          Py_XDECREF(key);
          Py_XDECREF(value);

          state = 0;
          buf+=1;
        }
        sbuf = buf;
      } else {
        buf += 64 - shifted;
        break;
      }

    }
    i+=1;
    if ( buf >= buf_end ) { goto pdone; }
  } while ( buf < buf_end ); // Why doesn't this work

pdone:
  //printf( " done >%.*s<\n", (int)(buf_end-sbuf), sbuf );  
  //value = PyUnicode_FromStringAndSize(sbuf, buf_end-sbuf); 
  len = path_decode( sbuf, buf_end-sbuf, &ignore_me );
  value = PyUnicode_FromStringAndSize(sbuf, len);
  PyDict_SetItem(args, key, value);  
  Py_XDECREF(key);
  Py_XDECREF(value);
  return args;
}


void request_decodePath(Request* self) {
  if(!self->path_decoded) {
    self->path_len = path_decode( self->path, self->path_len, &(self->qs_len) );
    self->path_decoded = true;
  }
}

PyObject* Request_getattro(Request* self, PyObject* name)
{
  PyObject* result;

  if((result = PyObject_GenericGetAttr((PyObject*)self, name))) return result;

  Py_RETURN_NONE;
/*
  PyObject* extensions = NULL;
  if(!(extensions = PyObject_GetAttrString(self->app, "_request_extensions")))
    goto error;

  PyObject* entry;
  if(!(entry = PyDict_GetItem(extensions, name)))
    goto error;
  else
    PyErr_Clear();

  PyObject* handler;
  PyObject* property;
  if(!(handler = PyTuple_GetItem(entry, 0)))
    goto error;

  if(!(property = PyTuple_GetItem(entry, 1)))
    goto error;

  if(property == Py_True) {
    if(!(result = PyObject_CallFunctionObjArgs(handler, self, NULL)))
      goto error;
  } else {
    if(!(result = PyObject_CallFunctionObjArgs(partial, handler, self, NULL)))
      goto error;
  }

  error:
  Py_XDECREF(extensions);

  return result;
*/
}

static inline PyObject* Request_decode_headers(Request* self)
{
  DBG printf("request decode headers num heads %ld\n",self->num_headers);
  PyObject* result = NULL;
  PyObject* headers = PyDict_New();
  if(!headers) goto error;
  result = headers;

  for(struct mr_header* header = self->headers; header < self->headers + self->num_headers; header++) {
      PyObject* name = NULL; PyObject* value = NULL;
      name  = PyUnicode_FromStringAndSize(header->name,  header->name_len);        if(!name)  goto loop_error;
      value = PyUnicode_DecodeLatin1(     header->value, header->value_len, NULL); if(!value) goto loop_error;
      if(PyDict_SetItem(headers, name, value) == -1) goto loop_error;
      //DBG printf("Found header: %.*s %.*s\n",header->name_len,  header->name, header->value_len, header->value);

      goto loop_finally;
loop_error:
      result = NULL;
      DBG printf("loop error in decode headers\n");
      PyObject *type, *traceback;
      PyErr_Fetch(&type, &value, &traceback);
      printf("exception :\n");
      PyObject_Print( type, stdout, 0 ); printf("\n");
      PyObject_Print( value, stdout, 0 ); printf("\n");

loop_finally:
      Py_XDECREF(name);
      Py_XDECREF(value);
      if(!result) goto error;
  }

  goto finally;
  error:
  Py_XDECREF(headers);
  result = NULL;
  finally:
  return result;
}

PyObject* Request_get_headers(Request* self, void* closure) {
  if(!self->py_headers) self->py_headers = Request_decode_headers(self);
  Py_XINCREF(self->py_headers);
  return self->py_headers;
}

static inline PyObject* parseCookies( Request* r, char *buf, size_t buflen ) {
  unsigned int msk;
  int i=0,tz; // 32B index
  int cnt = 0;
  unsigned int shifted;
  const char *sbuf = buf;
  const char *obuf = buf;
  const char *buf_end = buf+buflen;
  int name_or_value = 0;
  int found = 0;

  __m256i m59 = _mm256_set1_epi8(59);
  __m256i m61 = _mm256_set1_epi8(61);

  PyObject* cookies = PyDict_New();
  PyObject* key = NULL; PyObject* value = NULL;

  DBG printf("parse cookies: %.*s\n",(int)buflen, buf);

  do {
    const char *block_start = obuf+32*i;
    __m256i b0 = _mm256_loadu_si256((const __m256i *) block_start);
    msk = _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b0, m59), _mm256_cmpeq_epi8(b0, m61) ) );
    while (1) {
      //if ( buf >= buf_end ) { goto sesdone; }
      shifted = buf-block_start;
      if ( shifted >= 32 ) break;
      tz = TZCNT((msk >> shifted));
      if ( tz < 32 ) {
        buf += tz;
        DBG printf( " fnd >%.*s<\n", buf-sbuf, sbuf );  
        if ( buf >= buf_end ) { goto sesdone; }
        if ( name_or_value == 1 ) {
          if ( *buf == '=' ) { buf += 1; continue; } // = in value field
          if ( found ) {
            DBG printf("session key %.*s\n", (int)(buf-sbuf), sbuf);
            r->session_id = sbuf;
            r->session_id_sz = buf-sbuf;
          }
          value = PyUnicode_FromStringAndSize(sbuf, buf-sbuf); //TODO error
          PyDict_SetItem(cookies, key, value);  //  == -1) goto loop_error;
          Py_XDECREF(key);
          Py_XDECREF(value);
          buf+=1;
          name_or_value = 0;
        } else {
          key = PyUnicode_FromStringAndSize(sbuf, buf-sbuf); //TODO error
          if ( buf-sbuf == 9 && ( *((unsigned int *)(sbuf)) == CHAR4_TO_INT('m', 'r', 's','e') ) ) {
            found = 1;
          }
          name_or_value = 1;
        }
        buf += 1;
        sbuf = buf;
      } else {
        buf += 32 - shifted;
        break;
      }

    }
    i+=1;
    if ( buf >= buf_end ) { goto sesdone; }
  } while ( buf-obuf < buf_end-obuf );

sesdone:
  if ( found ) {
    r->session_id = sbuf;
    r->session_id_sz = buf_end-sbuf;
  }
  if ( name_or_value ) {
    value = PyUnicode_FromStringAndSize(sbuf, buf_end-sbuf); //TODO error
    PyDict_SetItem(cookies, key, value); 
    Py_XDECREF(key);
    Py_XDECREF(value);
  }
  return cookies;
}

// We can probably create a separate 1 and 2 reg avx2 find func to share
static inline void getSession( Request* r, char *buf, size_t buflen ) {
  unsigned int msk;
  int i=0,tz; // 32B index
  int cnt = 0;
  unsigned int shifted;
  const char *sbuf = buf;
  const char *obuf = buf;
  const char *buf_end = buf+buflen;
  int name_or_value = 0;
  int found = 0;

  __m256i m59 = _mm256_set1_epi8(59);
  __m256i m61 = _mm256_set1_epi8(61);

  do {
    const char *block_start = obuf+32*i;
    __m256i b0 = _mm256_loadu_si256((const __m256i *) block_start);
    msk = _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b0, m59), _mm256_cmpeq_epi8(b0, m61) ) );
    while (1) {
      //if ( buf >= buf_end ) { goto sesdone; }
      shifted = buf-block_start;
      if ( shifted >= 32 ) break;
      tz = TZCNT((msk >> shifted));
      if ( tz < 32 ) {
        buf += tz;
        //printf( " fnd >%.*s<\n", buf-sbuf, sbuf );  
        if ( buf >= buf_end ) { goto sesdone; }
        if ( name_or_value == 1 ) {
          if ( *buf == '=' ) { buf += 1; continue; } // = in value field
          if ( found ) {
            //printf( " done >%.*s<\n", buf-sbuf, sbuf );  
            r->session_id = sbuf;
            r->session_id_sz = buf-sbuf;
            return;
          }
          buf+=1;
          name_or_value = 0;
        } else {
          if ( buf-sbuf == 9 && ( *((unsigned int *)(sbuf)) == CHAR4_TO_INT('m', 'r', 's','e') ) ) {
            found = 1;
          }
          name_or_value = 1;
        }
        buf += 1;
        sbuf = buf;
      } else {
        buf += 32 - shifted;
        break;
      }

    }
    i+=1;
    if ( buf >= buf_end ) { goto sesdone; }
  } while ( buf-obuf < buf_end-obuf );

sesdone:
  if ( found ) {
    r->session_id = sbuf;
    r->session_id_sz = buf-sbuf;
    //printf( " sesdone >%.*s<\n", buf-sbuf, sbuf );  
  }
}


static inline PyObject* Request_decode_cookies(Request* self)
{
  for(struct mr_header* header = self->headers; header < self->headers + self->num_headers; header++) {
    if ( header->name_len == 6 && header->name[1] == 'o' && header->name[3] == 'k' ) {
      return parseCookies( self, header->value, header->value_len );
    }
  }
  return NULL;
}

void Request_load_cookies(Request* self) {
  if(!self->py_headers) self->py_headers = Request_decode_headers(self);
  if(!self->py_cookies) self->py_cookies = Request_decode_cookies(self);
}

// Instead of loading all headers and cookies just go directly for the session value
void Request_load_session(Request* self) {
  for(struct mr_header* header = self->headers; header < self->headers + self->num_headers; header++) {
    if ( header->name_len == 6 && header->name[1] == 'o' && header->name[3] == 'k' ) {
      getSession( self, header->value, header->value_len );
      return;
    }
  }
}

PyObject* Request_get_cookies(Request* self, void* closure) {
  if(!self->py_headers) self->py_headers = Request_decode_headers(self);
  if(!self->py_cookies) self->py_cookies = Request_decode_cookies(self);
  return self->py_cookies;
}

PyObject* Request_get_body(Request* self, void* closure)
{
  if(!self->body) Py_RETURN_NONE;
  if(!self->py_body) self->py_body = PyBytes_FromStringAndSize(self->body, self->body_len);
  Py_XINCREF(self->py_body);
  return self->py_body;
}



PyObject* Request_get_path(Request* self, void* closure)
{
  request_decodePath(self);
  if(!self->py_path) {
    if ( self->path_len == 0 ) self->py_path = Py_None;
    else self->py_path = PyUnicode_FromStringAndSize(self->path, self->path_len);
    Py_XINCREF(self->py_path);
  }
  return self->py_path;
}
PyObject* Request_get_method(Request* self, void* closure)
{
  if(!self->py_method) {
    if ( self->method_len == 0 ) self->py_method = Py_None;
    else self->py_method = PyUnicode_FromStringAndSize(self->method, self->method_len);
    //TODO Decode latin? self->py_method = PyUnicode_DecodeLatin1( REQUEST_METHOD(self), self->method_len, NULL);

    Py_XINCREF(self->py_method);
  }
  return self->py_method;
}
PyObject* Request_get_query_string(Request* self, void* closure)
{
  if(!self->py_query_string) {
    if ( self->qs_len == 0 ) self->py_query_string = Py_None;
    else self->py_query_string = PyUnicode_FromStringAndSize(self->path + self->path_len + 1, self->qs_len - 1);
    Py_XINCREF(self->py_query_string);
  }
  return self->py_query_string;
}

PyObject* Request_get_query_args(Request* self, void* closure)
{
  if(!self->py_args) {
    if ( self->qs_len ) {
      self->py_args = parse_query_args(self->path + self->path_len + 1, self->qs_len - 1);
      Py_XINCREF(self->py_args);
    } else {
      self->py_args = PyDict_New();
      return self->py_args;
    }
  }
  return self->py_args;
}

PyObject* Request_notfound(Request* self)
{
  self->return404 = true;
  //PyErr_SetString(PyExc_ValueError, "System command failed");
  //return NULL;
  Py_RETURN_NONE;
}


PyObject* Request_parse_mp_form(Request* self) {


  Py_ssize_t ctlen;
  char *ct = NULL;
  for(struct mr_header* header = self->headers; header < self->headers + self->num_headers; header++) {
    if ( header->name_len == 12 && header->name[0] == 'C' ) {
      ct = header->value;
      ctlen = header->value_len;
      break;
    }
  }

  if (ct == NULL ) {
    PyObject *ret = PyTuple_New(2);
    Py_INCREF(Py_None);
    Py_INCREF(Py_None);
    PyTuple_SetItem(ret, 0, Py_None);
    PyTuple_SetItem(ret, 1, Py_None);
    return ret;
  }

  //ct = PyUnicode_AsUTF8AndSize( pyct, &ctlen);

  int found;
  static char crlf[] = "\015\015";
  static char colon[] = "::" ";;" "==" "\015\015";
  static char semi[] = ";;" "=="; 
    
  //printf("ct >%.*s<\n", ctlen, ct);

/*
  char *p = ct;
  char *pend = ct+ctlen;
  char *bnd = NULL;
  int bndlen;
  p = findchar(p, pend, semi, sizeof(semi) - 1, &found);
  //printf("find >%.*s<\n", 5, p);
  if ( p[2] == 'b' ) {
    bnd = p + 11;
    bndlen = pend-bnd;
  }
*/
  // Boundary is always a fixed offset "Content-Type: multipart/form-data; boundary=foo" and ct is pointing to multipart
  //   If that changes then the above code may need to parse the entire line for the boundary instead of stopping at the first ;
  char *p, *pend, *bnd=NULL;
  int bndlen;
  if ( ct[21] == 'b' ) {
    bnd = ct+30;
    bndlen = ctlen-30;
  }
  if ( bnd == NULL ) {
    PyObject *ret = PyTuple_New(2);
    Py_INCREF(Py_None);
    Py_INCREF(Py_None);
    PyTuple_SetItem(ret, 0, Py_None);
    PyTuple_SetItem(ret, 1, Py_None);
    return ret;
  }

  int i = 0;
  p = self->body;
  pend = p + self->body_len;

  int state = 0;

  char *name = NULL;
  char *filename = NULL;
  char *content_type = NULL;
  int namesz=0, filenamesz=0, content_typesz=0;
  char *body = NULL;

//Content-Disposition: form-data
//Content-Disposition: form-data; name="fieldName"
//Content-Disposition: form-data; name="fieldName"; filename="filename.jpg"

  // We're looping line by line until end of body
  // TODO add a findBoundary function and just search on '-' instead of line by line
  //      then add a get headers for parsing the lines after the boundary
  while ( p < pend ) {

    // Consume boundary
    if ( state == 0 ) {
      if ( p[0] == '-' && p[1] == '-' && !strncmp(p+2, bnd, bndlen) ) {

        if ( body ) {
          //printf("body len %d\n", p-body-2);
          //if ( name         ) printf("name >%.*s<\n", 4, name);
          //if ( filename     ) printf("filename >%.*s<\n", 4, filename);
          //if ( content_type ) printf("content_type >%.*s<\n", 4, content_type);

          PyObject *tmp; 
          if ( filename ) {
            PyObject *f = PyDict_New();
            if ( content_type ) {
              tmp = PyUnicode_FromStringAndSize( content_type, content_typesz);
              PyDict_SetItemString( f, "type", tmp );
              Py_DECREF(tmp);
            } else {
              tmp = PyUnicode_FromStringAndSize( "text/plain", 10 );
              PyDict_SetItemString( f, "type", tmp );
              Py_DECREF(tmp);
            }
            tmp = PyUnicode_FromStringAndSize( filename, filenamesz );
            PyDict_SetItemString( f, "name", tmp );
            Py_DECREF(tmp);
            tmp = PyUnicode_FromStringAndSize( body, p-body-2 );
            PyDict_SetItemString( f, "body", tmp );
            Py_DECREF(tmp);

            if ( self->py_file == NULL ) {
              self->py_file = f;
            }
            if ( self->py_files == NULL ) {
              self->py_files = PyList_New(0);
            }
            PyList_Append( self->py_files, f );

          } else {

            if ( self->py_form == NULL ) self->py_form = PyDict_New();

            PyObject *val = PyUnicode_FromStringAndSize( body, p-body-2 );
            PyObject *key = PyUnicode_FromStringAndSize( name, namesz   );

            if ( key && val ) PyDict_SetItem( self->py_form, key, val );

            Py_XDECREF(key);
            Py_XDECREF(val);

          }

          body = name = filename = content_type = NULL;

        }
        if ( p[2+bndlen] == '-' ) break; // Last boundary has -- appended
        state = 1;
      }
    }
    // Consume header fields ending in blank line
    //Content-Disposition: form-data; name="fieldName"; filename="filename.jpg"
    //Content-Type: text/plain; charset=utf-8
    else if ( state == 1 ) {
      if ( p[0] == '\r' ) {
        //printf("end of state 1\n");
        state = 0;
        body = p+2;
      }
     else {
        char *last = p;
        p = findchar(p, pend, colon, sizeof(colon) - 1, &found);
        if ( found && p[0] == ':' ) {
          p += 1;
          if ( last[0] == 'C' || last[0] =='c' ) { // TODO need the little c?
            //printf(" >%.*s<\n", p-last,last);
            //printf(" p-last %d\n", (int)(p-last) );

            if ( p-last == 20 ) { // Content-Disposition
              while ( p[0] != '\r' ) {
                last = p;
                p = findchar(p, pend, colon, sizeof(colon) - 1, &found);
                //printf(" >%.*s<\n", 16,p); 


 // >; name="file"; f<
 // >="file"; filenam<
 // >="test.txt"

                if ( p[0] == ';' ) { p += 1; while ( p[0] == ' ' ) p++;  }
                else if ( p[0] == '=' ) { 
                  //printf("last >%.*s<\n", 4,last);
                  
                  if ( last[0] == 'n' ) {
                    name = p+2;
                    p = findchar(p+1, pend, colon, sizeof(colon) - 1, &found);
                    namesz = p-1-name;
                  }
                  else if ( last[0] == 'f' ) {
                    filename = p+2; 
                    p = findchar(p+1, pend, colon, sizeof(colon) - 1, &found);
                    filenamesz = p-1-filename;
                  }
                  else p += 1;
                }
              }
            } else if ( p-last == 13 ) {  // Content-Type
              content_type = p+1; 
              p = findchar(p, pend, colon, sizeof(colon) - 1, &found);
              content_typesz = p-content_type;
            }
          }
        }  
      }    
    }
    
    //if ( state == 2 ) {
    //}
    //p = findchar(p, pend, crlf, sizeof(crlf) - 1, &found);
    p = my_get_eol(p, pend);
    p += 2;
  }

  PyObject *ret = PyTuple_New(2);
  Py_INCREF(Py_None);
  Py_INCREF(Py_None);
  PyTuple_SetItem(ret, 0, Py_None);
  PyTuple_SetItem(ret, 1, Py_None);
  return ret;
}

PyObject* Request_parse_urlencoded_form(Request* self) {
  self->py_form = parse_query_args(self->body, self->body_len);
  Py_RETURN_NONE;
}

