
// PyObject_Print( req->py_user, stdout, 0 ); printf("\n");

#include "protocol.h"
#include "common.h"
#include "module.h"
#include "request.h"
#include "response.h"
#include "router.h"
#include "unpack.h"
#include "utils.h"

#include "Python.h"
#include <errno.h>
#include <string.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

// DELME
static void print_buffer( char* b, int len ) {
  for ( int z = 0; z < len; z++ ) {
    printf( "%02x ",(int)b[z]);
  }
  printf("\n");
}

void printErr(void) {
  PyObject *type, *value, *traceback;
  PyErr_Fetch(&type, &value, &traceback);
  printf("Unhandled exception :\n");
  PyObject_Print( type, stdout, 0 ); printf("\n");
  PyObject_Print( value, stdout, 0 ); printf("\n");
}

PyObject * Protocol_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  Protocol* self = NULL;

  self = (Protocol*)type->tp_alloc(type, 0);
  if(!self) goto finally;

  self->app = NULL;
  self->transport = NULL;
  self->write = NULL;
  self->writelines = NULL;

  self->create_task = NULL;
  self->task_done = NULL;

  self->conn_idle_time = 0;
  self->num_data_received = 0;
  self->request_idle_time = 0;
  self->num_requests_popped = 0;


  DBG printf("proto new self = %p\n",(void*)self);

  finally:
  return (PyObject*)self;
}

void Protocol_dealloc(Protocol* self)
{
  DBG printf("procotol dealloc\n");
  parser_dealloc(&self->parser);
  //Py_XDECREF(self->request);
  //Py_XDECREF(self->response);
  Py_XDECREF(self->router);
  Py_XDECREF(self->app);
  //Py_XDECREF(self->transport);
  Py_XDECREF(self->write);
  Py_XDECREF(self->writelines);
  Py_XDECREF(self->create_task);
  Py_XDECREF(self->task_done);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

int Protocol_init(Protocol* self, PyObject *args, PyObject *kw)
{
  DBG printf("protocol init %p\n",self);
  self->closed = true;

  if(!PyArg_ParseTuple(args, "O", &self->app)) return -1;
  Py_INCREF(self->app);

  self->request = (Request*)MrhttpApp_get_request( self->app );
  //if(!(self->request  = (Request*) PyObject_GetAttrString((PyObject*)self->app, "request" ))) return -1;
  //if(!(self->response = (Response*)PyObject_GetAttrString((PyObject*)self->app, "response"))) return -1;
  if(!(self->router   = (Router*)  PyObject_GetAttrString((PyObject*)self->app, "router"  ))) return -1;

  if ( !parser_init(&self->parser, self) ) return -1;

  PyObject* loop = NULL;
  if(!(loop = PyObject_GetAttrString((PyObject*)self->app, "_loop"  ))) return -1;
  
  self->queue_start = 0;
  self->queue_end = 0;
  if(!(self->task_done   = PyObject_GetAttrString((PyObject*)self, "task_done"  ))) return -1;
  if(!(self->create_task = PyObject_GetAttrString(loop, "create_task"))) return -1;
  DBG printf("protocol init task done %p\n",self->task_done);
  Py_XDECREF(loop);
  return 0;
}

// copied from Modules/socketmodule.h
// TODO uvloop 0.9+ returns their own socket here
typedef int SOCKET_T;
typedef struct {
    PyObject_HEAD
    int sock_family;            /* Address family, e.g., AF_INET */
    int sock_type;              /* Socket type, e.g., SOCK_STREAM */
    int sock_proto;             /* Protocol type, usually 0 */
    SOCKET_T sock_fd;           /* Socket file descriptor */
    PyObject *(*errorhandler)(void); /* Error handler; checks
                                        errno, returns NULL and
                                        sets a Python exception */
    _PyTime_t sock_timeout;     /* Operation timeout in seconds;
                                        0.0 means non-blocking */
} PySocketSockObject;



PyObject* Protocol_connection_made(Protocol* self, PyObject* transport)
{

  DBG printf("conn made %p\n",self);
  self->transport = transport;
  self->closed = false;

  if(!(self->write      = PyObject_GetAttrString(transport, "write"))) return NULL;
  if(!(self->writelines = PyObject_GetAttrString(transport, "writelines"))) return NULL;

  Py_RETURN_NONE;
}

void* Protocol_close(Protocol* self)
{
  DBG printf("protocol close\n");
  void* result = self;
  self->closed = true;

  PyObject* close = PyObject_GetAttrString(self->transport, "close");
  if(!close) return NULL;
  PyObject* tmp = PyObject_CallFunctionObjArgs(close, NULL);
  Py_XDECREF(close);
  if(!tmp) return NULL;
  Py_DECREF(tmp);

  return result;

}

PyObject* Protocol_eof_received(Protocol* self) {
  DBG printf("eof received\n");
  Py_RETURN_NONE; // Closes the connection and conn lost will be called next
}

PyObject* Protocol_connection_lost(Protocol* self, PyObject* args)
{
  DBG printf("conn lost %p\n",self);
  self->closed = true;
  MrhttpApp_release_request( self->app, self->request );

  PyObject* connections = NULL;

  if(!Protocol_pipeline_cancel(self)) return NULL;

  Py_RETURN_NONE;
}

PyObject* Protocol_data_received(Protocol* self, PyObject* data)
{
  self->num_data_received++;
  DBG printf("protocol data recvd %zu\n", Py_SIZE(data));

  // If -1 it was an error, but we should have raised it already
  if(parser_data_received(&self->parser, data, self->request) == -1) {
    Py_RETURN_NONE;
  }

  Py_RETURN_NONE;
}

PyObject* pipeline_queue(Protocol* self, PipelineRequest r)
{ 
  DBG printf("Queueing proto %p\n",self);
  PyObject* result = Py_None; 
  PyObject* add_done_callback = NULL;

  if(PIPELINE_EMPTY(self)) {
    self->queue_start = self->queue_end = 0;
  }
  
  assert(self->queue_end < sizeof(self->queue) / sizeof(self->queue[0]));
  
  PipelineRequest* end = self->queue + self->queue_end;
  *end = r;
  pipeline_INCREF(r);
  
  self->queue_end++;
  DBG printf("Queueing in position %zu task done %p\n", self->queue_end,self->task_done);

  // If this is a task then have it call Protocol_task_done when it is done
  if(pipeline_is_task(r)) {
    DBG printf(" is task\n");
    //PyObject* task = pipeline_get_task(r);
    PyObject* task = r.task;
    if(!(add_done_callback = PyObject_GetAttrString(task, "add_done_callback")))
      goto error;
    
    PyObject* tmp;
    if(!(tmp = PyObject_CallFunctionObjArgs(add_done_callback, self->task_done, NULL)))
      goto error;
    Py_DECREF(tmp);
  }
  
  goto finally;
  
error: 
  result = NULL;
  
finally:
  Py_XDECREF(add_done_callback);
  return result;
} 

Protocol* Protocol_on_headers(Protocol* self, char* method, size_t method_len,
                    char* path, size_t path_len, int minor_version,
                    void* headers, size_t num_headers)
{
  DBG printf("on headers\n");
  Protocol* result = self;
  DBG printf("path >%.*s<\n", (int)path_len, path );
  request_load( self->request, method, method_len, path, path_len, minor_version, headers, num_headers);
  return result;
}

static inline bool _isdigit(char c)  { return  c >= '0'  && c <= '9'; }

PyObject *protocol_callPageHandler( Protocol* self, PyObject *func, Request *request ) {
  PyObject* ret = NULL;
  PyObject *args[10];

  int numArgs = request->numArgs;
  DBG printf("num args %d\n",numArgs);
  if ( numArgs ) {
    //PyObject *args = PyTuple_New( numArgs );
    //PyObject **args = malloc( numArgs * sizeof(PyObject*) );
    
    for (int i=0; i<numArgs; i++) { 

      if ( request->argTypes[i] == 1 ) {
        char *p = request->args[i];

        int cnt = request->argLens[i];

        if ( cnt < 20 ) {
          long l = 0;
          while (_isdigit(*p) && (cnt-- != 0)) l = (l * 10) + (*p++ - '0');
          if ( !cnt ) {
            args[i] = PyLong_FromLong(l);
          } else {
            args[i] = PyUnicode_FromStringAndSize( request->args[i],  request->argLens[i] );
          }
        } else {
          args[i] = PyUnicode_FromStringAndSize( request->args[i],  request->argLens[i] );
          args[i] = PyLong_FromUnicodeObject( args[i], 10 );
        }

      } else {
        args[i] = PyUnicode_FromStringAndSize( request->args[i],  request->argLens[i] );
      }
    }
    switch (numArgs) {
      case 1: ret = PyObject_CallFunctionObjArgs(func, request, args[0], NULL); break;
      case 2: ret = PyObject_CallFunctionObjArgs(func, request, args[0], args[1], NULL); break;
      case 3: ret = PyObject_CallFunctionObjArgs(func, request, args[0], args[1], args[2], NULL); break;
      case 4: ret = PyObject_CallFunctionObjArgs(func, request, args[0], args[1], args[2], args[3], NULL); break;
      case 5: ret = PyObject_CallFunctionObjArgs(func, request, args[0], args[1], args[2], args[3], args[4], NULL); break;
      case 6: ret = PyObject_CallFunctionObjArgs(func, request, args[0], args[1], args[2], args[3], args[4], args[5], NULL); break;
    }
    for (int i=0; i<numArgs; i++) { 
      Py_DECREF(args[i]);
    }
  } else {
    return PyObject_CallFunctionObjArgs(func, request, NULL);
  }
  //TODO num args > 6 fail at start 
  return ret;
}

void Protocol_on_memcached_reply( SessionCallbackData *scd, char *data, int data_sz ) {
  Protocol *self = (Protocol*)scd->protocol;
  Request  *req  = (Request*)scd->request;


  DBG_MEMCAC printf(" memcached reply data len %d data %.*s\n",data_sz,data_sz,data);

  bool json = false;
  // If append_user then the stored session's format must match the incoming.  json or mrpacker

  // TODO Support json?  
  if ( data_sz ) {
    //if ( data[0] == '{' ) {
      //json = true;
      //PyObject *o = PyBytes_FromStringAndSize( data, data_sz );
      //PyObject_CallFunctionObjArgs(req->set_user, o, NULL);
      //Py_XDECREF(o); 
    //} else {
    // TODO error clear py err or just not raise
    //DELME PyObject *z = PyObject_GetAttrString((PyObject*)self->app, "printrefs"); PyObject* tmp = PyObject_CallFunctionObjArgs(z, NULL); Py_XDECREF(z); Py_XDECREF(tmp);

    req->py_user = unpackc( data, data_sz );

    //DELME z = PyObject_GetAttrString((PyObject*)self->app, "printrefs"); tmp = PyObject_CallFunctionObjArgs(z, NULL); Py_XDECREF(z); Py_XDECREF(tmp);
    //if ( req->py_user == NULL ) {
      //printf("DELME unpackc returned null on memcached reply\n");
      //printf("DELME data sz %d\n",data_sz);
      //print_buffer(data, 16);
      //exit(1);
    //}
    //}
  } 
  
  free(scd);
  if ( !self->closed ) {

    Route *r = req->route;
    if ( r->mrq ) { 
      int slot = 0;

      // Pull slot from the first arg. Must be a number though a string won't break
      if ( req->numArgs > 0 ) {
        int len = req->argLens[0];
        char *p = req->args[0];
        while (len--) slot = (slot * 10) + (*p++ - '0');
        slot &= 0xff;
      } else {
        // The slot is the user id embedded in the session key
        unsigned char *p = (unsigned char*)req->session_id;
        uint64_t uid = 0;
        int x = from_64[p[0]];
        int num = 0;
        while( (x & 0x20) == 0 && num++ < req->session_id_sz ) {
          uid <<= 5;
          uid |= x;
          p += 1; num += 1;
          x = from_64[p[0]];
        }
        slot = uid;
      }

      // Only push to mrq if we found a session for the user's key
      if ( data_sz ) {

        // push [ user, json ] if append_user is set
        if ( r->append_user ) {
          // TODO Test using a static buffer

          //int rc = MrqClient_push( (MrqClient*)self->app->py_mrq, slot, tmp, (int)(p-tmp) );

          int rc;
          // TODO Accept json and mrpacker?

          if ( json ) {
            char *tmp = malloc( req->body_len + data_sz + 16 );
            tmp[0] = '[';
            char *p = tmp+1;
            memcpy(p, req->body, req->body_len);
            p += req->body_len;
            *p++ = ',';
            memcpy(p, data, data_sz);
            p += data_sz;
            *p++ = ']';
            rc = MrqClient_pushj( (MrqClient*)self->app->py_mrq, slot, tmp, (int)(p-tmp) );
            free(tmp);
          } else {

            char *tmp = malloc( req->body_len + data_sz + 16 );
            tmp[0] = 0x42;
            char *p = tmp+1;
            memcpy(p, req->body, req->body_len);
            p += req->body_len;
            memcpy(p, data, data_sz);
            p += data_sz;
            rc = MrqClient_push ( (MrqClient*)self->app->py_mrq, slot, tmp, (int)(p-tmp) );
            free(tmp);
          }

          if ( rc == -1 ) {
            req->py_mrq_servers_down = Py_True; Py_INCREF(Py_True);
            //rc = PyObject_SetAttrString((PyObject*)req, "servers_down", Py_True );
            //Py_DECREF(Py_True);
          }

        }

  // TODO Delete the below and just keep append user? This pulls a user defined key from the json and sends that object
        /*
        else if ( r->user_key ) { 
          //request.user has a dict and we need r->user_key key
          // TODO or we search for "key": in the char* 
          PyObject *user = PyObject_GetAttrString((PyObject*)req, "user"  );
          if ( user ) {
            PyObject *tmp = PyDict_GetItem( user, r->user_key );
            // TODO This object could be a long or unicode
            if ( PyLong_Check(tmp) ) {
              long v = PyLong_AsLong(tmp);
              //We want [ 12345, {msg} ]
              char *tmp = malloc( req->body_len + 32 );
              tmp[0] = '[';
              char *s = tmp + 1;
              do *s++ = (char)(48 + (v % 10ULL)); while(v /= 10ULL);
              reverse( tmp+1, s-1 );
              *s = ',';
              strncpy(s, req->body, req->body_len);
              s += req->body_len;
              *s++ = ']';
              MrqClient_push( (MrqClient*)self->app->py_mrq, slot, tmp, (int)(s-tmp) );
              free(tmp);
            }
  
          } else {
            MrqClient_push( (MrqClient*)self->app->py_mrq, slot, req->body, req->body_len );
          }
        */  
        else {
          // Send body to mrq
          if ( req->py_mrpack == NULL ) {
            MrqClient_pushj( (MrqClient*)self->app->py_mrq, slot, req->body, req->body_len );
          } else {
            MrqClient_push ( (MrqClient*)self->app->py_mrq, slot, req->body, req->body_len );
          }
        }
      }
      // TODO else tell user to login?
  
      // TODO set a client member to say success/fail? Have to start failing if slow consumer / connection gone.
    } 

    Protocol_handle_request( self, req, req->route );
  } else {
    // TODO Need to do anything if they dropped connection before the memcached reply?
  }
  Py_DECREF(self); 
}

Protocol* Protocol_on_body(Protocol* self, char* body, size_t body_len) {
  DBG printf("protocol - on body\n");

  Request* request = self->request;

  request->body     = body;
  request->body_len = body_len;

  request->transport = self->transport;
  request->app = (PyObject*)self->app; //TODO need this?


  // URL:  /  /json 
  Route *r = router_getRoute( self->router, self->request );
  if ( r == NULL ) {
    // TODO If the pipeline isn't empty...
    if ( self->app->err404 ) {
      return protocol_write_error_response_bytes(self, self->app->err404);
    }
    return self;
  }

  // If the route requires a session we need to check for a cookie session id, and fetch the session
  if ( r->session ) {
    DBG printf("Route requires a session\n"); 

    self->request->route = r;
    Request_load_session(self->request);

    // If we found a session id in the cookies lets fetch it
    if ( self->request->session_id != NULL ) {
      DBG printf("found session in cookies\n");

      SessionCallbackData *scd = malloc( sizeof(SessionCallbackData));
      scd->protocol = self;
      Py_INCREF(self); // Incref so we don't get GC'd before the response comes back
      scd->request  = self->request;
      //int rc = (tSessionClientGet*)(self->app->session_get)( (MemcachedClient*)self->app->py_mc, self->request->session_id, (tSessionCallback)&Protocol_on_memcached_reply, scd );
      int rc = self->app->session_get( self->app->py_session, self->request->session_id, (tSessionCallback)&Protocol_on_memcached_reply, scd );
      //int rc = MemcachedClient_get( (MemcachedClient*)self->app->py_mc, self->request->session_id, (tSessionCallback)&Protocol_on_memcached_reply, scd );
      // TODO allow keys of any size
      //int rc = MemcachedClient_get2( (MemcachedClient*)self->app->py_mc, self->request->session_id, self->request->session_id_sz, (tSessionCallback)&Protocol_on_memcached_reply, scd );

      // If the get failed (no servers) then we're done
      if ( rc != 0 ) {
        Protocol_handle_request( self, self->request, r );
        return self;
      }
      // Get a new request object so we don't overwrite this one while waiting for the reply
      self->request = (Request*)MrhttpApp_get_request( self->app );
      return self;
    }

    // if mrq return now as the user is not logged in
    if ( r->mrq ) {
      return Protocol_handle_request( self, self->request, r );
    }

    //?  PyObject *ret = pipeline_queue(self, (PipelineRequest){true, self->request, task});
  }
  if ( r->mrq ) { //TODO
    DBG printf("Route uses mrq\n"); 
    int slot = 0;
    // Pull slot from the first arg. Must be a number
    if ( self->request->numArgs > 0 ) {
      int len = self->request->argLens[0];
      char *p = self->request->args[0];
      while (len--) slot = (slot * 10) + (*p++ - '0');
    }

    // Send body to mrq
    if ( self->request->py_mrpack == NULL ) {
      MrqClient_pushj( (MrqClient*)self->app->py_mrq, slot, self->request->body, self->request->body_len );
    } else {
      MrqClient_push ( (MrqClient*)self->app->py_mrq, slot, self->request->body, self->request->body_len );
    }

    // TODO set a client member to say success/fail? Have to start failing if slow consumer / connection gone.

  }

  return Protocol_handle_request( self, self->request, r );
}

Protocol* Protocol_handle_request(Protocol* self, Request* request, Route* r) {
  PyObject* result = NULL;
  DBG printf("protocol handle request\n");

  // if the page is an async def or there are already requests in the Q get a free request object
  // TODO look at the iscoro
  if ( r->iscoro || !PIPELINE_EMPTY(self)) {
    //self->gather.enabled = false;
    // If we can't finish now save this request by grabbing a new free request if we haven't already done so
    // TODO what if all request objects are busy
    if ( self->request == request ) self->request = (Request*)MrhttpApp_get_request( self->app );
  }

  
  if(!(result = protocol_callPageHandler(self, r->func, request)) ) {

    if ( request->return404 ) {
      request->return404 = false;
      PyErr_Clear();
      if ( self->app->err404 ) {
        return protocol_write_error_response_bytes(self, self->app->err404);
      }
      return self;
    }

  //if(!(result = PyObject_CallFunctionObjArgs(r->func, NULL))) {
    DBG printf("Page handler call failed with an exception\n");
    PyObject *type, *value, *traceback;
    PyErr_Fetch(&type, &value, &traceback);
    //PyErr_NormalizeException(&type, &value, &traceback);
    if (value) {
      PyObject *msg = PyObject_GetAttrString(value, "_message");

      // Check for HTTPError
      if ( msg ) {
        int code = PyLong_AsLong(PyObject_GetAttrString(value, "code"));
        PyObject *reason = PyObject_GetAttrString(value, "reason");
        Py_DECREF(value);
        PyErr_Clear();
        if(!protocol_write_error_response(self, code, PyUnicode_AsUTF8(reason), PyUnicode_AsUTF8(msg))) return NULL;
        Py_XDECREF(msg);
        Py_XDECREF(reason);
        Py_XDECREF(traceback);
        Py_XDECREF(type);
        return self;
      }
      // Check for HTTPRedirect
      msg = PyObject_GetAttrString(value, "url");
      if ( msg ) {
        int code = PyLong_AsLong(PyObject_GetAttrString(value, "code"));
        Py_DECREF(value);
        PyErr_Clear();
        if(!protocol_write_redirect_response(self, code, PyUnicode_AsUTF8(msg))) return NULL;
        Py_XDECREF(msg);
        Py_XDECREF(traceback);
        Py_XDECREF(type);
        return self;
      }
    }
    printf("Unhandled exception :\n");
    PyObject_Print( type, stdout, 0 ); printf("\n");
    if ( value ) { PyObject_Print( value, stdout, 0 ); printf("\n"); }
    PyErr_Clear();
    //PyObject_Print( traceback, stdout, 0 ); printf("\n");

    Py_XDECREF(traceback);
    Py_XDECREF(type);
    Py_XDECREF(value);
    protocol_write_error_response(self, 500,"Internal Server Error","The server encountered an unexpected condition which prevented it from fulfilling the request.");
    return self;
    //PyErr_Restore(type, value, traceback);
  }


  if ( r->iscoro ) {
    DBG printf("protocol - Request is a coroutine\n");
    PyObject *task;
    if(!(task = PyObject_CallFunctionObjArgs(self->create_task, result, NULL))) return NULL;
    PyObject *ret = pipeline_queue(self, (PipelineRequest){true, request, task});
    Py_XDECREF(task);
    Py_DECREF(result);
    if ( !ret ) return NULL;
    return self;
  }

  if(!PIPELINE_EMPTY(self))
  {
    if(!pipeline_queue(self, (PipelineRequest){false, request, result})) goto error;
    Py_DECREF(result);
    return self;
  }

  //if ( PyBytes_Check( result ) ) {
    //result = PyUnicode_FromEncodedObject( result, "utf-8", "strict" );
    //printf("WARNING: Page handler should return a string. Bytes object returned from the page handler is being converted to unicode using utf-8\n");
  //}

  // Make sure the result is a string
  if ( !( PyUnicode_Check( result ) || PyBytes_Check( result ) ) ) {
    protocol_write_error_response(self, 500,"Internal Server Error","The server encountered an unexpected condition which prevented it from fulfilling the request.");
    if ( PyCoro_CheckExact( result ) ) {
      PyErr_SetString(PyExc_ValueError, "Page handler must return a string, did you forget to await an async function?");
    } else {
      PyErr_SetString(PyExc_ValueError, "Page handler must return a string");
    }
    return NULL;
  }

  request->response->mtype = r->mtype;

  if(!protocol_write_response(self, request, result)) goto error;

  if ( !request->keep_alive ) Protocol_close(self);

  Py_DECREF(result);
  return self;

error:
  Py_XDECREF(result);
  return NULL;

}

// Initial response buffer contents RN is \r\n

// HTTP/1.1 200 OKRN  17
// Server: MrHTTP/0.2.0RN 22
// Content-Length: 1        RN 25
// Date: Thu, 05 Apr 2018 22:54:19 GMTRN 37
// Content-Type: text/html; charset=utf-8RNRN 44

// total length = 145

static inline Protocol* protocol_write_response(Protocol* self, Request *req, PyObject *resp) { //, Response* response) {

  Py_ssize_t rlen, t;
  DBG_RESP printf("protocol write response\n");

  int headerLen = response_updateHeaders(req->response); // Add user headers 
  if (!headerLen) return NULL;

  // TODO Would unicode to bytes (PyUnicode_AsEncodedString) then concatenate body and header be faster? Could check those functions
  char *r;
  if ( PyBytes_Check(resp) ) {
    r = PyBytes_AsString(resp);
    rlen = PyBytes_GET_SIZE(resp);
  } else {
    r = PyUnicode_AsUTF8AndSize( resp, &rlen ); 
  }
  t = rlen;
  DBG_RESP printf("Response body >%.*s<\n", (int)rlen, r);

  char *rbuf = getResponseBuffer(headerLen + rlen);

  memcpy( rbuf + headerLen, r, rlen );

  // Faster than sprintf
    //int result = sprintf( rbuf + 176, "%ld", rlen);
  int off = 33;
  // clear the response length then write in the new length
  int *ip = (int*)(rbuf + off);  
  ip[0] = 0x20202020;
  ip[1] = 0x20202020;
  char *s = rbuf + off;  
  do *s++ = (char)(48 + (t % 10ULL)); while(t /= 10ULL);
  reverse( rbuf+off, s-1 );  

  if ( unlikely(req->minor_version == 0) ) {
    rbuf[7] = '0';
  }

  if ( (0) ) {
    rbuf[88] = 'c'; rbuf[89] = 'l'; rbuf[90] = 'o'; rbuf[91] = 's'; rbuf[92] = 'e'; rbuf[93] = ' '; rbuf[94] = ' '; rbuf[95] = ' '; rbuf[96] = ' '; rbuf[97] = ' ';
  }

  DBG_RESP printf( "rlen headerlen %ld %d\n", rlen, headerLen);
  DBG_RESP printf( "Sending response:\n\n%.*s\n", (int)rlen + headerLen, rbuf );
  PyObject *bytes;
  bytes = PyBytes_FromStringAndSize( rbuf, rlen + headerLen );
  PyObject *o;
  if(!(o = PyObject_CallFunctionObjArgs(self->write, bytes, NULL))) return NULL;
  Py_DECREF(o);
  Py_DECREF(bytes);

  // If we modified the header in the response buffer return it to default TODO
  if ( req->response->mtype ) response_setHtmlHeader();

  if ( req != self->request ) MrhttpApp_release_request( self->app, req );
  else                        Request_reset(req);
  return self;
}

static inline Protocol* protocol_write_redirect_response(Protocol* self, int code, char *url ) {
  PyObject *bytes = response_getRedirectResponse( code, url );
  if ( !bytes ) return NULL;
  PyObject *o;
  if(!(o = PyObject_CallFunctionObjArgs(self->write, bytes, NULL))) return NULL;
  Py_DECREF(o);
  Py_DECREF(bytes);
  return self;
}

// TODO rename write response bytes doesn't have to be error
static inline Protocol* protocol_write_error_response_bytes(Protocol* self, PyObject *bytes) { 
  if ( !bytes ) return NULL;
  PyObject *o;
  if(!(o = PyObject_CallFunctionObjArgs(self->write, bytes, NULL))) return NULL;
  Py_DECREF(o);
  return self;
}
// TODO rename Why? 
static inline Protocol* protocol_write_error_response(Protocol* self, int code, char *reason, char *msg) { 
  PyObject *bytes = response_getErrorResponse( code, reason, msg );
  if ( !bytes ) return NULL;
  PyObject *o;
  if(!(o = PyObject_CallFunctionObjArgs(self->write, bytes, NULL))) return NULL;
  Py_DECREF(o);
  Py_DECREF(bytes);
  return self;
}

// TODO rename
static void* protocol_pipeline_ready(Protocol* self, PipelineRequest r)
{
  DBG printf("  pipeline ready\n");
  PyObject* get_result = NULL;
  PyObject* response = NULL;
  Request* request = r.request;
  PyObject* task = r.task;

  if(pipeline_is_task(r)) {
    if(!(get_result = PyObject_GetAttrString(task, "result")))
      goto error;

    if(!(response = PyObject_CallFunctionObjArgs(get_result, NULL))) {
      Py_XDECREF(get_result);

      if ( self->closed ) { // Probably CancelledError exception, but don't bother to check as we're done
        DBG printf("    connection closed so dropping exception\n");
        PyErr_Clear();
        self->num_requests_popped++; 
        if ( request != self->request ) MrhttpApp_release_request( self->app, request );
        return self;
      }

      DBG printf("  exception in coro page handler\n");
      // TODO exception
      //Protocol_catch_exception(request);
      PyObject *type, *value, *traceback;
      PyErr_Fetch(&type, &value, &traceback);
      Py_XDECREF(traceback);
      Py_XDECREF(type);
      if (value) {

        // Check for HTTPError ( if it has _message it is HTTPError )
        PyObject *msg = PyObject_GetAttrString(value, "_message");
        if ( msg ) {
          int code = PyLong_AsLong(PyObject_GetAttrString(value, "code"));
          PyObject *reason = PyObject_GetAttrString(value, "reason");
          Py_DECREF(value);
          PyErr_Clear();
          if ( request != self->request ) MrhttpApp_release_request( self->app, request );
          if(!protocol_write_error_response(self, code, PyUnicode_AsUTF8(reason), PyUnicode_AsUTF8(msg))) return NULL;
          return self;
        }
        // Check for HTTPRedirect
        msg = PyObject_GetAttrString(value, "url");
        if ( msg ) {
          int code = PyLong_AsLong(PyObject_GetAttrString(value, "code"));
          Py_DECREF(value);
          PyErr_Clear();
          if ( request != self->request ) MrhttpApp_release_request( self->app, request );
          if(!protocol_write_redirect_response(self, code, PyUnicode_AsUTF8(msg))) return NULL;
          return self;
        }

        Py_DECREF(value);
      }
      PyErr_Clear();

      printf("Unhandled exception:\n");
      PyObject_Print( type, stdout, 0 ); printf("\n");
      PyObject_Print( value, stdout, 0 ); printf("\n");
 
      self->num_requests_popped++; 
      if ( request != self->request ) MrhttpApp_release_request( self->app, request );
      protocol_write_error_response(self, 500,"Internal Server Error","The server encountered an unexpected condition which prevented it from fulfilling the request.");
      return self;
    }
    Py_XDECREF(get_result);

  } else {
    response = task;
  }
  self->num_requests_popped++; 

  //if ( PyBytes_Check( response ) ) {
    //response = PyUnicode_FromEncodedObject( response, "utf-8", "strict" );
    //printf("WARNING: Page handler should return a string. Bytes object returned from the page handler is being converted to unicode using utf-8\n");
  //}

  //if ( !PyUnicode_Check( response ) ) {
  if ( !( PyUnicode_Check( response ) || PyBytes_Check( response ) ) ) {
    if ( request != self->request ) MrhttpApp_release_request( self->app, request );

    protocol_write_error_response(self, 500,"Internal Server Error","The server encountered an unexpected condition which prevented it from fulfilling the request.");
    if ( PyCoro_CheckExact( response ) ) {
      PyErr_SetString(PyExc_ValueError, "Page handler must return a string, did you forget to await an async function?");
    } else {
      PyErr_SetString(PyExc_ValueError, "Page handler must return a string");
    }
    return NULL;
  }

  if(!self->closed) {
    if(!protocol_write_response(self, request, response)) goto error;
  } else {
    // If closed then just drop it
    DBG printf("Connection closed, response dropped prot %p\n",self);
  }

  // important: this breaks a cycle in case of an exception
  //Py_CLEAR(((Request*)request)->exception);

  goto finally;

  error:
  self = NULL;

  finally:
  if(pipeline_is_task(r)) Py_XDECREF(response);
  return self;
}


PyObject* protocol_task_done(Protocol* self, PyObject* task)
{
  DBG printf("  coro task done proto %p\n",self);
  
  PyObject* result = Py_True;
  PipelineRequest *r;

  // task is the task that just finished.  
  //   If task is at the head of the Q then return the response.
  //     then continue looping through the Q and return a response for anything that is already done


  // This code loops the whole Q and asks each task if they are done and if so calls ready on each one 
  //   until we hit a task that is still waiting or the Q is empty
  for(r = self->queue + self->queue_start;
      r < self->queue + self->queue_end; r++) {
    PyObject* done = NULL;
    PyObject* done_result = NULL;
    result = Py_True;

    if(pipeline_is_task(*r)) {
      task = pipeline_get_task(*r);

      if(!(done = PyObject_GetAttrString(task, "done"))) goto loop_error;
      if(!(done_result = PyObject_CallFunctionObjArgs(done, NULL))) goto loop_error;

      if(done_result == Py_False) {
        result = Py_False;
        goto loop_finally;
      }
    } else {
    }
    if(!protocol_pipeline_ready(self, *r)) goto loop_error;

    pipeline_DECREF(*r);

    goto loop_finally;

    loop_error:
    result = NULL;

    loop_finally:
    Py_XDECREF(done_result);
    Py_XDECREF(done);
    if(!result) goto error;
    if(result == Py_False) break;
  }

  self->queue_start = r - self->queue;

  goto finally;

  error:
  result = NULL;

  finally:
  Py_XINCREF(result);
  return result;
}


void* Protocol_pipeline_cancel(Protocol* self)
{
  void* result = self;
  PipelineRequest *r;

  for(r = self->queue + self->queue_start;
      r < self->queue + self->queue_end; r++) {

    if(!pipeline_is_task(*r)) continue;

    PyObject* task = pipeline_get_task(*r);
    PyObject* cancel = NULL;

    if(!(cancel = PyObject_GetAttrString(task, "cancel")))
      goto loop_error;

    PyObject* tmp;
    if(!(tmp = PyObject_CallFunctionObjArgs(cancel, NULL)))
      goto loop_error;
    Py_DECREF(tmp);

    goto loop_finally;

    loop_error:
    result = NULL;

    loop_finally:
    Py_XDECREF(cancel);

    if(!result)
      break;
  }

  return result;
}

PyObject* Protocol_get_pipeline_empty(Protocol* self)
{
  if(PIPELINE_EMPTY(self)) {
    Py_RETURN_TRUE;
  }
  Py_RETURN_FALSE;
}


PyObject* Protocol_get_transport(Protocol* self)
{
  Py_INCREF(self->transport);
  return self->transport;
}

// TODO Is task_done call the correct thing? Redo the pipeline
void Protocol_timeout_request(Protocol* self) {
  if ( !PIPELINE_EMPTY(self) ) {
    self->queue_start++; // Drop the request
    protocol_write_error_response(self, 503,"Service unavailable","The server timed out responding to your request and may be overloaded.  Please try again later.");
    protocol_task_done(self, NULL);
  }

}




