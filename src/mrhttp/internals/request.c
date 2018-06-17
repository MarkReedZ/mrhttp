

#include <stddef.h>
#include <sys/param.h>
#include <strings.h>
#include <string.h>

#include "common.h"
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

#define CHAR4_INT(a, b, c, d)         \
   (unsigned int)((d << 24) | (c << 16) | (b << 8) | a)


//#include "cresponse.h"
//#include "capsule.h"

//static PyObject* partial;


//static PyObject* HTTP10;
//static PyObject* HTTP11;
//static PyObject* request;

PyObject* Request_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  Request* self = NULL;
  self = (Request*)type->tp_alloc(type, 0);
  self->set_user = NULL;
  self->response = NULL;

  return (PyObject*)self;
}

void Request_dealloc(Request* self) {

  printf("DELME request dealloc\n");

  free(self->headers);

  Py_XDECREF(self->set_user);
  Py_XDECREF(self->py_headers);
  Py_XDECREF(self->py_body);
  Py_XDECREF(self->py_cookies);
  Py_XDECREF(self->py_query_string);
  Py_XDECREF(self->py_args);
  Py_XDECREF(self->py_path);
  Py_XDECREF(self->py_method);
  Py_TYPE(self)->tp_free((PyObject*)self);
}


int Request_init(Request* self, PyObject *args, PyObject* kw)
{
  
  self->headers = malloc( sizeof(*(self->headers))*100 ); //TODO
  if(!(self->response = (Response*)PyObject_GetAttrString((PyObject*)self, "response"))) return -1;
  if(!(self->set_user = PyObject_GetAttrString((PyObject*)self, "set_user"))) return -1;
  Request_reset(self);
  
  return 0;
}

// This only resets stuff that needs to be for reuse
void Request_reset(Request *self) {
  self->inprog = false;
  self->session_id = NULL;
  Py_XDECREF(self->py_headers);
  Py_XDECREF(self->py_body);
  Py_XDECREF(self->py_path);
  Py_XDECREF(self->py_method);
  self->py_headers = NULL;
  self->py_body = NULL;
  self->py_path = NULL;
  self->py_method = NULL;
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
  DBG printf("request transport ptr %p\n",self->transport);
  if (self->transport) {
    Py_INCREF(self->transport);
    return self->transport;
  }
  Py_RETURN_NONE;
}

// TODO query parse
//?fart=on+me+now&fart=no%20way&blank=not
//{'fart': ['on me now', 'no way'], 'blank': ['not']}

//#ifdef __SSE4_2__

static char *findchar_fast(char *buf, char *buf_end, char *ranges, size_t ranges_size, int *found)
{   
    *found = 0;
    if (likely(buf_end - buf >= 16)) {
        __m128i ranges16 = _mm_loadu_si128((const __m128i *)ranges);
        
        size_t left = (buf_end - buf) & ~15;
        do {
            __m128i b16 = _mm_loadu_si128((const __m128i *)buf); 
            int r = _mm_cmpestri(ranges16, ranges_size, b16, 16, _SIDD_LEAST_SIGNIFICANT | _SIDD_CMP_RANGES | _SIDD_UBYTE_OPS);
            if (unlikely(r != 16)) {
                buf += r;
                *found = 1;
                return buf;
            }
            buf += 16;
            left -= 16;
        } while (likely(left != 0));
    }
   
    *found = 0; 
    return buf;
}

// /spanish/objetos%20voladores%20no%20identificados?foo=bar
static inline size_t sse_decode(char* path, ssize_t length, size_t *qs_len) {
  //DBG printf("sse_decode >%.*s<\n", (int)length, path);
  if (length == 0) return length;
  char *pat = path;
  static char ranges1[] = "%%" "??";
  char *end = path + length;
  int found;
  do {
    //DBG printf("sse_decode >%.*s<\n", (int)length, path);
    pat = findchar_fast(pat, end, ranges1, sizeof(ranges1) - 1, &found);
    if ( found ) {
      if(*pat == '%' && is_hex(*(pat + 1)) && is_hex(*(pat + 2))) {
        *pat = (hex_to_dec(*(pat + 1)) << 4) + hex_to_dec(*(pat + 2));
        pat+=3;
        length -= 2;
      } else {
        *qs_len = end-pat;
        length -= end-pat;
        break;
      }
    }
  } while (0);

  // If we hit the query string separator we're done. Otherwise either there are <16 chars left or we hit a %.
  // Chinese(back to back encodes) is slow if we keep trying to use cmpestri so we stop. 
  //  TODO benches/path_decode and review Spanish/etc. Maybe offer a choice.
  if(*pat == '?') return length;

  char *write = pat;
  if ( found ) write -= 2;
  char *read = pat;
  for (;read < end;) {
    if (read[0] == '?') {
      length -= end-read;
      *qs_len  = end-pat;
      break;
    }
    if ( read[0] == '%' ) {
      if( is_hex(read[1]) && is_hex(read[2]) ) {
        *write = (hex_to_dec(read[1]) << 4) + hex_to_dec(read[2]);
        write+=1;
        read += 3;
        length-=2;
      } else {
        if (read > write) {
          write[0] = read[0];
          write[1] = read[1];
        }
        read += 2;
        write += 2;
      }

    } else {
      if (read > write) {
        write[0] = read[0];
      }
      read++;
      write++;
    }
  }
  DBG printf("sse_decode len %d path >%.*s<\n", (int)length, (int)length, path);

  return length;
}
//#endif

void request_decodePath(Request* self) {
  if(!self->path_decoded) {
    self->path_len = sse_decode( self->path, self->path_len, &(self->qs_len) );
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
      //title_case((char*)header->name, header->name_len);
      name  = PyUnicode_FromStringAndSize(header->name,  header->name_len);        if(!name)  goto loop_error;
      value = PyUnicode_DecodeLatin1(     header->value, header->value_len, NULL); if(!value) goto loop_error;
      if(PyDict_SetItem(headers, name, value) == -1) goto loop_error;

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
  char *end = buf + buflen;
  char *last = buf;
  PyObject* cookies = PyDict_New();
  PyObject* key = NULL; PyObject* value = NULL;

  static char ALIGNED(16) ranges1[] = "==" ";;";
  int found;
  int state = 0;
  int grab_session = 0;
//Cookie: key=session_key; bar=2;
  do { 
    last = buf;
    buf = findchar_fast(buf, end, ranges1, sizeof(ranges1) - 1, &found);
    if ( found ) {
      if ( *buf == '=' ) {
        // Save out the mrsession id TODO use a separate function since we dont need this all the time
        if ( buf-last == 9 && ( *((unsigned int *)(last)) == CHAR4_INT('m', 'r', 's','e') ) ) {
          grab_session = 1;
        }
        key = PyUnicode_FromStringAndSize(last, buf-last); //TODO error
        state = 1;
        buf+=1;
        //if ( state == 1 )  TODO ERROR double =s 
      } 
      else if ( *buf == ';' ) {
        if ( state == 0 ) key  = PyUnicode_FromString("");
        if (grab_session) {
          grab_session = 0;
          r->session_id = last;
        }
        value = PyUnicode_FromStringAndSize(last, buf-last); //TODO error
        state = 0;
        PyDict_SetItem(cookies, key, value);  //  == -1) goto loop_error;
        Py_XDECREF(key);
        Py_XDECREF(value);
        buf+=1;
        while ( *buf == 32 ) buf++;
      }
      else {
        printf(" ERR found not = or ; %.*s\n", 5, buf );
      }
      //else if(*buf == '%' && is_hex(*(buf + 1)) && is_hex(*(buf + 2))) {
        //*write = (hex_to_dec(*(buf + 1)) << 4) + hex_to_dec(*(buf + 2));
        //write+=1;
        //length -= 2;
      //}
    }
  } while( found );
  // May have 15 extra bytes
  for (;buf <= end;) {
    
    if ( buf == end || *buf == ';' ) {
      if ( state == 0 ) key  = PyUnicode_FromString("");
      if (grab_session) {
        grab_session = 0;
        r->session_id = last;
      }
      value = PyUnicode_FromStringAndSize(last, buf-last); //TODO error
      state = 0;
      PyDict_SetItem(cookies, key, value);  //  == -1) goto loop_error;
      Py_XDECREF(key);
      Py_XDECREF(value);
      buf+=1;
      while ( *buf == 32 ) buf++;
      last = buf;
    }
    else if ( *buf == '=' ) {
      key = PyUnicode_FromStringAndSize(last, buf-last); //TODO error
      if (!key) printf("!key\n");
      state = 1;
      last = buf+1;
    }
    buf++;
  }

  return cookies;
}

static inline PyObject* Request_decode_cookies(Request* self)
{
  for(struct mr_header* header = self->headers; header < self->headers + self->num_headers; header++) {
    if ( header->name_len == 6 && header->name[0] == 'C' ) {
      return parseCookies( self, header->value, header->value_len );
    }
  }
  return NULL;
}

void Request_load_cookies(Request* self) {
  if(!self->py_headers) self->py_headers = Request_decode_headers(self);
  if(!self->py_cookies) self->py_cookies = Request_decode_cookies(self);
}

PyObject* Request_get_cookies(Request* self, void* closure) {
  if(!self->py_headers) self->py_headers = Request_decode_headers(self);
  if(!self->py_cookies) self->py_cookies = Request_decode_cookies(self);
  Py_XINCREF(self->py_cookies);
  return self->py_cookies;
}

PyObject* Request_get_body(Request* self, void* closure)
{
  if(!self->body) Py_RETURN_NONE;
  if(!self->py_body) self->py_body = PyBytes_FromStringAndSize(self->body, self->body_len);
  Py_XINCREF(self->py_body);
  return self->py_body;
}

static inline PyObject* parse_query_args( char *buf, size_t buflen ) {
  char *end = buf + buflen;
  char *last = buf;
  PyObject* args = PyDict_New();

  if ( buflen == 0 ) return args;

  PyObject* key = NULL; PyObject* value = NULL;

  static char ALIGNED(16) ranges1[] = "==" "&&";
  int found;
  int state = 0;
  int grab_session = 0;
// foo=bar&key=23
  do { 
    last = buf;
    buf = findchar_fast(buf, end, ranges1, sizeof(ranges1) - 1, &found);
    if ( found ) {
      if ( *buf == '=' ) {
        key = PyUnicode_FromStringAndSize(last, buf-last); //TODO error
        state = 1;
        buf+=1;
        //if ( state == 1 )  TODO ERROR double =s 
      } 
      else if ( *buf == '&' ) {
        if ( state == 0 ) key  = PyUnicode_FromString("");
        value = PyUnicode_FromStringAndSize(last, buf-last); //TODO error
        state = 0;
        PyDict_SetItem(args, key, value);  //  == -1) goto loop_error;
        Py_XDECREF(key);
        Py_XDECREF(value);
        buf+=1;
        while ( *buf == 32 ) buf++;
      }
      else {
        printf(" ERR found not = or ; %.*s\n", 5, buf );
      }
      //else if(*buf == '%' && is_hex(*(buf + 1)) && is_hex(*(buf + 2))) {
        //*write = (hex_to_dec(*(buf + 1)) << 4) + hex_to_dec(*(buf + 2));
        //write+=1;
        //length -= 2;
      //}
    }
  } while( found );
  // May have 15 extra bytes
  for (;buf <= end;) {
    
    if ( buf == end || *buf == '&' ) {
      if ( state == 0 ) key  = PyUnicode_FromString("");
      if ( buf == end && *(buf-1) == ' ' ) {
        value = PyUnicode_FromStringAndSize(last, buf-last-1); //TODO error
      } else {
        value = PyUnicode_FromStringAndSize(last, buf-last); //TODO error
      }
      state = 0;
      PyDict_SetItem(args, key, value);  //  == -1) goto loop_error;
      Py_XDECREF(key);
      Py_XDECREF(value);
      buf+=1;
      while ( *buf == 32 ) buf++;
      last = buf;
    }
    else if ( *buf == '=' ) {
      key = PyUnicode_FromStringAndSize(last, buf-last); //TODO error
      if (!key) printf("!key\n");
      state = 1;
      last = buf+1;
    }
    buf++;
  }

  return args;
}


PyObject* Request_get_path(Request* self, void* closure)
{
  request_decodePath(self);
  if(!self->py_path) {
    if ( self->path_len == 0 ) self->py_path = Py_None;
    else self->py_path = PyUnicode_FromStringAndSize(self->path, self->path_len);
  }
  Py_XINCREF(self->py_path);
  return self->py_path;
}
PyObject* Request_get_method(Request* self, void* closure)
{
  if(!self->py_method) {
    if ( self->method_len == 0 ) self->py_method = Py_None;
    else self->py_method = PyUnicode_FromStringAndSize(self->method, self->method_len);
    //TODO Decode latin? self->py_method = PyUnicode_DecodeLatin1( REQUEST_METHOD(self), self->method_len, NULL);

  }
  Py_XINCREF(self->py_method);
  return self->py_method;
}
PyObject* Request_get_query_string(Request* self, void* closure)
{
  if(!self->py_query_string) {
    if ( self->qs_len == 0 ) self->py_query_string = Py_None;
    else self->py_query_string = PyUnicode_FromStringAndSize(self->path + self->path_len + 1, self->qs_len - 1);
  }
  Py_XINCREF(self->py_query_string);
  return self->py_query_string;
}

PyObject* Request_get_query_args(Request* self, void* closure)
{
  if(!self->py_args) {
    self->py_args = parse_query_args(self->path + self->path_len + 1, self->qs_len - 1);
  }
  Py_XINCREF(self->py_args);
  return self->py_args;
}
