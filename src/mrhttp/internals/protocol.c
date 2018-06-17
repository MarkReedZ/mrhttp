#include "protocol.h"
#include "common.h"
#include "module.h"
#include "request.h"
#include "response.h"
#include "router.h"

#include "Python.h"
#include <errno.h>
#include <string.h>
//#include "response.h"

#include <netinet/in.h>
#include <netinet/tcp.h>

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
  DBG printf("protocol new\n");

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
  parser_dealloc(&self->parser);
  //Py_XDECREF(self->request);
  //Py_XDECREF(self->response);
  Py_XDECREF(self->router);
  Py_XDECREF(self->app);
  Py_XDECREF(self->transport);
  Py_XDECREF(self->write);
  Py_XDECREF(self->writelines);
  Py_XDECREF(self->create_task);
  Py_XDECREF(self->task_done);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

int Protocol_init(Protocol* self, PyObject *args, PyObject *kw)
{
  DBG printf("protocol init\n");
  self->closed = true;

  if(!PyArg_ParseTuple(args, "O", &self->app)) return -1;
  Py_INCREF(self->app);

  self->request = (Request*)MrhttpApp_get_request( (MrhttpApp*)self->app );
  //if(!(self->request  = (Request*) PyObject_GetAttrString(self->app, "request" ))) return -1;
  //if(!(self->response = (Response*)PyObject_GetAttrString(self->app, "response"))) return -1;
  if(!(self->router   = (Router*)  PyObject_GetAttrString(self->app, "router"  ))) return -1;

  if ( !parser_init(&self->parser, self) ) return -1;

  PyObject* loop = NULL;
  if(!(loop = PyObject_GetAttrString(self->app, "_loop"  ))) return -1;
  
  self->queue_start = 0;
  self->queue_end = 0;
  if(!(self->task_done   = PyObject_GetAttrString((PyObject*)self, "task_done"  ))) return -1;
  if(!(self->create_task = PyObject_GetAttrString(loop, "create_task"))) return -1;
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
// TODO keep a request timer so we can timeout
//self._request_timeout_handler = self.loop.call_later( self.request_timeout, self.request_timeout_callback)
//if self._request_timeout_handler: self._request_timeout_handler.cancel()

  PyObject* connections = NULL;
  self->transport = transport;
  Py_INCREF(self->transport);

  // Set TCP_NODELAY which disables Nagles. TODO dynamic based on response size?
/* Doesn't work with .91 psuedo socket
  PyObject* get_extra_info = NULL;
  PySocketSockObject* socket = NULL;
  if(!(get_extra_info   = PyObject_GetAttrString(transport, "get_extra_info"))) return NULL;
  if(!(socket = (PySocketSockObject*)PyObject_CallFunctionObjArgs( get_extra_info, socket_str, NULL))) return NULL;

  const int on = 1;
  if(setsockopt(socket->sock_fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) != 0) {
    char msg[80]; sprintf(msg, "Error setting socket options: %s\n", strerror(errno));
    PyErr_SetString(PyExc_ConnectionError,msg);
    return NULL;
  }
  Py_XDECREF(socket);
  Py_XDECREF(get_extra_info);
*/


  if(!(self->write      = PyObject_GetAttrString(transport, "write"))) return NULL;
  if(!(self->writelines = PyObject_GetAttrString(transport, "writelines"))) return NULL;

  if(!(connections = PyObject_GetAttrString(self->app, "_connections"))) return NULL;
  if(PySet_Add(connections, (PyObject*)self) == -1) { Py_XDECREF(connections); return NULL; }
  DBG printf("connection made num %ld\n", PySet_GET_SIZE(connections));
  Py_XDECREF(connections);

  // TODO Only if session is enabled?
  self->memclient = (MemcachedClient*)PyObject_GetAttrString(self->app, "_mc");
  self->mrqclient = (MrqClient*)PyObject_GetAttrString(self->app, "_mrq");

  self->closed = false;
  Py_RETURN_NONE;
}

void* Protocol_close(Protocol* self)
{
  void* result = self;

  PyObject* close = PyObject_GetAttrString(self->transport, "close");
  if(!close) return NULL;
  PyObject* tmp = PyObject_CallFunctionObjArgs(close, NULL);
  Py_XDECREF(close);
  if(!tmp) return NULL;
  Py_DECREF(tmp);
  self->closed = true;

  return result;

}

PyObject* Protocol_eof_received(Protocol* self) {
  DBG printf("eof received\n");
  Py_RETURN_NONE; // Closes the connection and conn lost will be called next
}
PyObject* Protocol_connection_lost(Protocol* self, PyObject* args)
{
  DBG printf("conn lost\n");
  self->closed = true;
  //MrhttpApp_release_request( (MrhttpApp*)self->app, self->request );

  PyObject* connections = NULL;

  //if(!Parser_feed_disconnect(&self->parser)) goto error;

  // Remove the connection from app.connections
  if(!(connections = PyObject_GetAttrString(self->app, "_connections"))) return NULL;
  int rc = PySet_Discard(connections, (PyObject*)self);
  Py_XDECREF(connections);
  if ( rc == -1 ) return NULL;

  //if(!Protocol_pipeline_cancel(self)) return NULL;

  //Py_XDECREF(self->write);
  //Py_XDECREF(self->writelines);
  Py_XDECREF(self->task_done); // TODO Why is this helping

  Py_RETURN_NONE;
}

PyObject* Protocol_data_received(Protocol* self, PyObject* data)
{
  self->num_data_received++;
  DBG printf("protocol data recvd %ld\n", Py_SIZE(data));
        //# Check for the request itself getting too large and exceeding
        //# memory limits
        //self._total_request_size += len(data)
        //if self._total_request_size > self.request_max_size:
            //exception = PayloadTooLarge('Payload Too Large')
            //self.write_error(exception)

//if self._header_fragment == b'Content-Length' \ and int(value) > self.request_max_size:
                //exception = PayloadTooLarge('Payload Too Large')

  if(parser_data_received(&self->parser, data, self->request) == -1) {
    //  TODO Write a bad request 400 response
    return NULL; 
  }

  Py_RETURN_NONE;
}

PyObject* pipeline_queue(Protocol* self, PipelineRequest r)
{ 
  DBG printf("Queueing\n");
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
  DBG printf("Queueing in position %ld\n", self->queue_end);
  
  if(pipeline_is_task(r)) {
    DBG printf(" is task\n");
    PyObject* task = pipeline_get_task(r);
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
  request_load( self->request, method, method_len, path, path_len, minor_version, headers, num_headers);
  //DBG printf("path >%.*s<\n", (int)self->request->path_len, self->request->path );
  return result;
}

PyObject *protocol_callPageHandler( Protocol* self, PyObject *func, Request *request ) {
  PyObject* ret = NULL;

  int numArgs = request->numArgs;
  DBG printf("num args %d\n",numArgs);
  if ( numArgs ) {
    //PyObject *args = PyTuple_New( numArgs );
    PyObject **args = malloc( numArgs * sizeof(PyObject*) );
    
    for (int i=0; i<numArgs; i++) { 
      //printf(" arg%d len %d\n", i, self->request->argLens[i]);
      //printf(" arg%d %.*s\n", i, self->request->argLens[i], self->request->args[i] );

      //PyObject *arg = PyUnicode_FromStringAndSize( self->request->args[i],  self->request->argLens[i] );
      args[i] = PyUnicode_FromStringAndSize( request->args[i],  request->argLens[i] );
      //TODO error?
      //int rc = PyTuple_SetItem( args, i, arg );
      //if (rc) return NULL;
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
    free(args);
    //return PyObject_CallFunctionObjArgs(func, args, NULL);
  } else {
    DBG printf("num args is 0 so just call func %p\n",func);
    return PyObject_CallFunctionObjArgs(func, request, NULL);
  }
  //TODO num args > 6 fail at start 
  return ret;
}

void Protocol_on_memcached_reply( MemcachedCallbackData *mcd, char *data, int data_sz ) {
  Protocol *self = (Protocol*)mcd->protocol;
  Request  *req  = mcd->request;

  DBG_MEMCAC printf(" memcached reply data len %d data %.*s\n",data_sz,data_sz,data);

  if ( data_sz ) {
    PyObject *session = PyUnicode_FromStringAndSize( data, data_sz );
    PyObject_CallFunctionObjArgs(req->set_user, session, NULL);
  } 
  
  free(mcd);
  if ( !self->closed ) {
    Protocol_handle_request( self, req, req->route );
  } else {
    printf("DELME closed?\n");
  }
  Py_DECREF(self);
}

Protocol* Protocol_on_body(Protocol* self, char* body, size_t body_len) {
  DBG printf("protocol - on body\n");

  //Protocol* result = self;
  //PyObject* response_text = NULL;
  //if(!(func = PyObject_GetAttrString(self->app, "default"))) {
    //printf("app.default handler doesn't exist");
    //return NULL;
  //}

  Request* request = self->request;

  request->body     = body;
  request->body_len = body_len;

  request->transport = self->transport;
  request->app = self->app; //TODO need this?
  //Py_INCREF(self->transport);//TODO non static req
  //Py_INCREF(self->app);//TODO non static req

  Route *r = router_getRoute( self->router, self->request );
  if ( r == NULL ) {
    // TODO If the pipeline isn't empty...
    protocol_write_error_response(self, 404,"Not Found","The requested page was not found");
    return self;
  }

  // TODO session and mrq?

  // If the route requires a session we need to check for a cookie session id, and fetch the session
  if ( r->session ) {
    DBG printf("Route requires a session\n"); 

    self->request->route = r;
    Request_load_cookies(self->request);

    // If we found a session id in the cookies lets fetch it
    if ( self->request->session_id != NULL ) {

      // We need to call asyncGet and save this request so when our CB is called we continue processing
      MemcachedCallbackData *mcd = malloc( sizeof(MemcachedCallbackData));
      mcd->protocol = self;
      Py_INCREF(self);
      mcd->request  = self->request;
      MemcachedClient_get( self->memclient, self->request->session_id, (tMemcachedCallback)&Protocol_on_memcached_reply, mcd );
      //MemcachedProtocol_asyncGet( self->memprotocol, self->request->session_id, (tMemcachedCallback)&Protocol_on_memcached_reply, mcd );
      // Get a new request object so we don't overwrite this one while waiting for the reply
      self->request = (Request*)MrhttpApp_get_request( (MrhttpApp*)self->app );
      return self;

    }

    //?  PyObject *ret = pipeline_queue(self, (PipelineRequest){true, self->request, task});
  }
  if ( r->mrq ) { //TODO
    DBG printf("Route uses mrq\n"); 
    // TODO Get slot from url A
       //rte->segs[rte->numSegs-1] = rest; //TODO last one?
        //rte->segLens[j] = rest_len;
    int slot = 0;

    // Send body to mrq
    //TODO request->body / body_len
    MrqClient_push( self->mrqclient, slot, request->body, request->body_len );

    // TODO set a client member to say success/fail? Have to start failing if slow consumer / connection gone.

  }
  // TODO for memcached
  //  Need to set app->request or pass to page handlers
  //  memcached CB needs to set the user based on the returned json  see aiomcache wrapper

  return Protocol_handle_request( self, self->request, r );
}

Protocol* Protocol_handle_request(Protocol* self, Request* request, Route* r) {
  PyObject* result = NULL;
  DBG printf("protocol handle request\n");

  if ( r->mtype ) {
    request->response->mtype = r->mtype;
  }

  if ( r->iscoro || !PIPELINE_EMPTY(self)) {
    //self->gather.enabled = false;
    // If we can't finish now save this request by grabbing a new free request if we haven't already done so
    if ( self->request == request ) self->request = (Request*)MrhttpApp_get_request( (MrhttpApp*)self->app );
  }

  if(!(result = protocol_callPageHandler(self, r->func, request)) ) {
  //if(!(result = PyObject_CallFunctionObjArgs(r->func, NULL))) {
    DBG printf("Page handler call failed with an exception\n");
    PyObject *type, *value, *traceback;
    PyErr_Fetch(&type, &value, &traceback);
    PyErr_NormalizeException(&type, &value, &traceback);
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

/*
#include "frameobject.h"
    PyThreadState *tstate = PyThreadState_GET();
    if (NULL != tstate && NULL != tstate->frame) {
      PyFrameObject *frame = tstate->frame;

      printf("Python stack trace:\n");
      while (NULL != frame) {
        // int line = frame->f_lineno;
         //frame->f_lineno will not always return the correct line number
         //you need to call PyCode_Addr2Line().
        int line = PyCode_Addr2Line(frame->f_code, frame->f_lasti);
        const char *filename = PyUnicode_AsUTF8AndSize(frame->f_code->co_filename,NULL);
        const char *funcname = PyUnicode_AsUTF8AndSize(frame->f_code->co_name,NULL);
        printf("    %s(%d): %s\n", filename, line, funcname);
        frame = frame->f_back;
      }
    }
*/
   
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

  if ( PyBytes_Check( result ) ) {
    result = PyUnicode_FromEncodedObject( result, "utf-8", "strict" );
    printf("WARNING: Page handler should return a string. Bytes object returned from the page handler is being converted to unicode using utf-8\n");
  }

  // Make sure the result is a string
  if ( !PyUnicode_Check( result ) ) {
    //DELME
    PyObject_Print(result, stdout, 0); printf("\n");
    PyErr_SetString(PyExc_ValueError, "Page handler did not return a string");
    protocol_write_error_response(self, 500,"Internal Server Error","The server encountered an unexpected condition which prevented it from fulfilling the request.");
    goto error;
  }

  if(!protocol_write_response(self, request, result)) goto error;

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

  long int rlen, t;
  DBG_RESP printf("protocol write response\n");

  int headerLen = response_updateHeaders(req->response); // Add user headers 
  if (!headerLen) return NULL;

  // TODO Would unicode to bytes (PyUnicode_AsEncodedString) then concatenate body and header be faster? Could check those functions
  char *r = PyUnicode_AsUTF8AndSize( resp, &rlen ); 
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

  if ( req != self->request ) MrhttpApp_release_request( (MrhttpApp*)self->app, req );
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

// TODO rename
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
      DBG printf("  exception in coro page handler\n");
      // TODO exception
      //Protocol_catch_exception(request);
      PyObject *type, *value, *traceback;
      PyErr_Fetch(&type, &value, &traceback);
      Py_XDECREF(traceback);
      Py_XDECREF(type);
      if (value) {

        // Check for HTTPError
        PyObject *msg = PyObject_GetAttrString(value, "_message");
        if ( msg ) {
          int code = PyLong_AsLong(PyObject_GetAttrString(value, "code"));
          PyObject *reason = PyObject_GetAttrString(value, "reason");
          Py_DECREF(value);
          PyErr_Clear();
          if ( request != self->request ) MrhttpApp_release_request( (MrhttpApp*)self->app, request );
          if(!protocol_write_error_response(self, code, PyUnicode_AsUTF8(reason), PyUnicode_AsUTF8(msg))) return NULL;
          return self;
        }
        // Check for HTTPRedirect
        msg = PyObject_GetAttrString(value, "url");
        if ( msg ) {
          int code = PyLong_AsLong(PyObject_GetAttrString(value, "code"));
          Py_DECREF(value);
          PyErr_Clear();
          if ( request != self->request ) MrhttpApp_release_request( (MrhttpApp*)self->app, request );
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
      if ( request != self->request ) MrhttpApp_release_request( (MrhttpApp*)self->app, request );
      protocol_write_error_response(self, 500,"Internal Server Error","The server encountered an unexpected condition which prevented it from fulfilling the request.");
      return self;
    }

  } else {
    response = task;
  }
  self->num_requests_popped++; 

  if ( PyBytes_Check( response ) ) {
    response = PyUnicode_FromEncodedObject( response, "utf-8", "strict" );
    printf("WARNING: Page handler should return a string. Bytes object returned from the page handler is being converted to unicode using utf-8\n");
  }

  if ( !PyUnicode_Check( response ) ) {
    PyErr_SetString(PyExc_ValueError, "Page handler did not return a string");
    if ( request != self->request ) MrhttpApp_release_request( (MrhttpApp*)self->app, request );
    protocol_write_error_response(self, 500,"Internal Server Error","The server encountered an unexpected condition which prevented it from fulfilling the request.");
    return NULL;
  }

  if(!self->closed) {
    if(!protocol_write_response(self, request, response))
      goto error;
  } else {
    // TODO: Send that to protocol_error
    //printf("Connection closed, response dropped\n");
  }

  // important: this breaks a cycle in case of an exception
  //Py_CLEAR(((Request*)request)->exception);

  goto finally;

  error:
  self = NULL;

  finally:
  if(pipeline_is_task(r)) Py_XDECREF(response);
  Py_XDECREF(get_result);
  return self;
}


PyObject* protocol_task_done(Protocol* self, PyObject* task)
{
  DBG printf("  coro task done\n");
  PyObject* result = Py_True;
  PipelineRequest *r;

  for(r = self->queue + self->queue_start;
      r < self->queue + self->queue_end; r++) {
    PyObject* done = NULL;
    PyObject* done_result = NULL;
    result = Py_True;

    if(pipeline_is_task(*r)) {
      task = pipeline_get_task(*r);

      if(!(done = PyObject_GetAttrString(task, "done")))
        goto loop_error;

      if(!(done_result = PyObject_CallFunctionObjArgs(done, NULL)))
        goto loop_error;

      if(done_result == Py_False) {
        result = Py_False;
        goto loop_finally;
      }
    }
    if(!protocol_pipeline_ready(self, *r))
      goto loop_error;

    pipeline_DECREF(*r);

    goto loop_finally;

    loop_error:
    result = NULL;

    loop_finally:
    Py_XDECREF(done_result);
    Py_XDECREF(done);
    if(!result)
      goto error;
    if(result == Py_False)
      break;
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

void Protocol_timeout_request(Protocol* self) {
  if ( !PIPELINE_EMPTY(self) ) {
    self->queue_start++; // Drop the request
    protocol_write_error_response(self, 503,"Service unavailable","The server timed out responding to your request and may be overloaded.  Please try again later.");
    protocol_task_done(self, NULL);
  }

}




