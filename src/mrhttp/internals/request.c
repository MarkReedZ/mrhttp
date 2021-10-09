


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

  free(self->headers);

  Py_XDECREF(self->set_user);
  Py_XDECREF(self->py_headers);
  Py_XDECREF(self->py_body);
  Py_XDECREF(self->py_cookies);
  Py_XDECREF(self->py_query_string);
  Py_XDECREF(self->py_args);
  Py_XDECREF(self->py_path);
  Py_XDECREF(self->py_method);
  //Py_XDECREF(self->py_ip);
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
  self->py_ip   = NULL;
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

//#ifdef __SSE4_2__

// /spanish/objetos%20voladores%20no%20identificados?foo=bar
static inline size_t sse_decode(char* path, ssize_t length, size_t *qs_len) {
  //DBG printf("sse_decode >%.*s<\n", (int)length, path);
  if (length == 0) return length;
  char *pat = path;
  static char ranges1[] = "%%" "??";
  char *end = path + length;
  int found;

  // We only findchar once - benchmark one or more % encodings with continuing to use findchar ( Spanish / Chinese )
  do {
    //DBG printf("sse_decode >%.*s<\n", (int)length, path);
    pat = findchar(pat, end, ranges1, sizeof(ranges1) - 1, &found);
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

  if( !found || *pat == '?') return length;

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
PyObject* Request_get_ip(Request* self, void* closure) {
  if(!self->py_ip) {
    if ( self->hreq.ip_len ) {
      self->py_ip = PyUnicode_FromStringAndSize(self->hreq.ip, self->hreq.ip_len);
    } else {
      self->py_ip = Py_None;
    }
  }
  Py_INCREF(self->py_ip);
  return self->py_ip;
}

static inline PyObject* parseCookies( Request* r, char *buf, size_t buflen ) {
  char *end = buf + buflen;
  char *last = buf;
  PyObject* cookies = PyDict_New();
  PyObject* key = NULL; PyObject* value = NULL;

  DBG printf("parse cookies: %.*s\n",(int)buflen, buf);

  static char ALIGNED(16) ranges1[] = "==" ";;" "\x00 "; // Control chars up to space illegal
  int found;
  int state = 0;
  int grab_session = 0;
//Cookie: key=session_key; bar=2; nosemi=foo
  do { 
    last = buf;
    buf = findchar(buf, end, ranges1, sizeof(ranges1) - 1, &found);
    if ( found ) {
      if ( *buf == '=' ) {
        if ( state == 0 ) {
          // Save out the mrsession id 
          if ( buf-last == 9 && ( *((unsigned int *)(last)) == CHAR4_INT('m', 'r', 's','e') ) ) {
            DBG printf("Grab session\n");
            grab_session = 1;
          }
          key = PyUnicode_FromStringAndSize(last, buf-last); //TODO error
          DBG printf("session key %.*s\n", (int)(buf-last), last);
          state = 1;
          buf+=1;
        } else {
          // If we're in the value ignore the = so cookie name/value splits on the first =
          while(found && *buf == '=') buf = findchar(++buf, end, ranges1, sizeof(ranges1) - 1, &found);
        }
      } 
      else if ( *buf == ';' ) {
        if ( state == 0 ) key  = PyUnicode_FromString("");
        if (grab_session) {
          grab_session = 0;
          r->session_id = last;
          r->session_id_sz = buf-last;
        }
        value = PyUnicode_FromStringAndSize(last, buf-last); //TODO error
        DBG printf(" value %.*s\n", (int)(buf-last), last);
        state = 0;
        PyDict_SetItem(cookies, key, value);  //  == -1) goto loop_error;
        Py_XDECREF(key);
        Py_XDECREF(value);
        buf+=1;
        while ( *buf == 32 ) buf++;
      }
      else {
        // Bad character found so skip
        state = 0;
        while(found && *buf != ';') buf = findchar(++buf, end, ranges1, sizeof(ranges1) - 1, &found);
        if ( buf != end ) buf += 1;
        while ( *buf == 32 ) buf++;
      }
      //else if(*buf == '%' && is_hex(*(buf + 1)) && is_hex(*(buf + 2))) {
        //*write = (hex_to_dec(*(buf + 1)) << 4) + hex_to_dec(*(buf + 2));
        //write+=1;
        //length -= 2;
      //}
    }
  } while( found );

  // If the trailing ; is left off we need to finish up
  if (state) {
    if (grab_session) {
      grab_session = 0;
      r->session_id = last;
      r->session_id_sz = buf-last;
      DBG printf("session2 %.*s\n", r->session_id_sz, r->session_id);
    }
    value = PyUnicode_FromStringAndSize(last, buf-last); //TODO error
    PyDict_SetItem(cookies, key, value);  //  == -1) goto loop_error;
    Py_XDECREF(key);
    Py_XDECREF(value);
  }

  return cookies;
}
static inline void getSession( Request* r, char *buf, size_t buflen ) {
  char *end = buf + buflen;
  char *last = buf;

  static char ALIGNED(16) ranges1[] = "==" ";;";
  int found;
  int state = 0;
  do { 
    last = buf;
    buf = findchar(buf, end, ranges1, sizeof(ranges1) - 1, &found);
    if ( found ) {
      if ( *buf == '=' ) {
        if ( state == 0 ) {
          // Save out the mrsession id 
          if ( buf-last == 9 && ( *((unsigned int *)(last)) == CHAR4_INT('m', 'r', 's','e') ) ) {
            DBG printf("Grab session\n");
            state = 1;
          }
          buf+=1;
        } 
      } 
      else if ( *buf == ';' ) {
        if (state == 1 ) {
          r->session_id = last;
          r->session_id_sz = buf-last;
          return;
        }
        state = 0;
        buf+=1;
        while ( *buf == 32 ) buf++;
      }
    }
  } while( found );
  if (state) {
    r->session_id = last;
    r->session_id_sz = buf-last;
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
  size_t len;
  // foo=bar&key=23%28
  do { 
    buf = findchar(buf, end, ranges1, sizeof(ranges1) - 1, &found);
    if ( found ) {
      if ( *buf == '=' ) {
        len = sse_decode( last, buf-last, NULL );
        key = PyUnicode_FromStringAndSize(last, len); //TODO error
        state = 1;
        buf+=1;
        last = buf;
      } 
      else if ( *buf == '&' ) {
        if ( state == 0 ) key  = PyUnicode_FromString("");

        len = sse_decode( last, buf-last, NULL );
        value = PyUnicode_FromStringAndSize(last, len);
        state = 0;
        PyDict_SetItem(args, key, value);  //  == -1) goto loop_error;
        Py_XDECREF(key);
        Py_XDECREF(value);
        buf+=1;
        while ( *buf == 32 ) buf++;
        last = buf;
      }
      else {
        printf(" ERR found not = or ; %.*s\n", 5, buf );
      }
    }
  } while( found );

  if ( buf == end ) {
    if ( state == 0 ) key  = PyUnicode_FromString("");
    if ( buf == end && *(buf-1) == ' ' ) {
      len = sse_decode( last, buf-last-1, NULL );
      value = PyUnicode_FromStringAndSize(last, len); //TODO error
    } else {
      len = sse_decode( last, buf-last, NULL );
      value = PyUnicode_FromStringAndSize(last, len); //TODO error
    }
    state = 0;
    PyDict_SetItem(args, key, value);  //  == -1) goto loop_error;
    Py_XDECREF(key);
    Py_XDECREF(value);
  }

  return args;
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
        if ( p[2+bndlen] == '-' ) break; // Last boundary
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

    p = findchar(p, pend, crlf, sizeof(crlf) - 1, &found);
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

