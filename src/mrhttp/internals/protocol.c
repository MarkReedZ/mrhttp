
#include "perproc.h"

#include "protocol.h"
#include "common.h"
#include "module.h"
#include "request.h"
#include "response.h"
#include "Python.h"
#include <errno.h>
#include <string.h>
//#include "response.h"

static PyObject* socket_str;

#include <netinet/in.h>
#include <netinet/tcp.h>

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

  DBG printf("proto new self = %p\n",(void*)self);

  finally:
  return (PyObject*)self;
}

void Protocol_dealloc(Protocol* self)
{
  router_dealloc(&self->router);
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

  if(!(self->request  = (Request*) PyObject_GetAttrString(self->app, "request" ))) return -1;
  if(!(self->response = (Response*)PyObject_GetAttrString(self->app, "response"))) return -1;

  if(!(socket_str = PyUnicode_FromString("socket"))) return -1;

  if ( !router_init(&self->router, self) ) return -1;
  if ( !parser_init(&self->parser, self)   ) return -1;

  PyObject* loop = NULL;
  if(!(loop = PyObject_GetAttrString(self->app, "_loop"  ))) return -1;
  
  self->queue_start = 0;
  self->queue_end = 0;
  if(!(self->task_done   = PyObject_GetAttrString((PyObject*)self, "task_done"  ))) return -1;
  if(!(self->create_task = PyObject_GetAttrString(loop, "create_task"))) return -1;
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
  if(PySet_Add(connections, (PyObject*)self) == -1) return NULL;
  DBG printf("connection made num %ld\n", PySet_GET_SIZE(connections));
  Py_XDECREF(connections);

  //PyObject *mc = PyObject_GetAttrString(self->app, "mc");
  //if ( !mc ) return NULL;
  //self->memprotocol = (MemcachedProtocol*)PyObject_GetAttrString(mc, "conn");
  //if(!self->memprotocol) {
    //printf("DELME no mem proto \n");
    //return NULL;
  //}

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

  PyObject* connections = NULL;

  //if(!Parser_feed_disconnect(&self->parser)) goto error;

  // Remove the connection from app.connections
  if(!(connections = PyObject_GetAttrString(self->app, "_connections"))) return NULL;
  int rc = PySet_Discard(connections, (PyObject*)self);
  Py_XDECREF(connections);
  if ( rc == -1 ) return NULL;

  //if(!Pipeline_cancel(&self->pipeline)) goto error;

  Py_RETURN_NONE;
}

PyObject* Protocol_data_received(Protocol* self, PyObject* data)
{
  DBG printf("protocol data recvd %ld\n", Py_SIZE(data));
        //# Check for the request itself getting too large and exceeding
        //# memory limits
        //self._total_request_size += len(data)
        //if self._total_request_size > self.request_max_size:
            //exception = PayloadTooLarge('Payload Too Large')
            //self.write_error(exception)

//if self._header_fragment == b'Content-Length' \ and int(value) > self.request_max_size:
                //exception = PayloadTooLarge('Payload Too Large')

  if(parser_data_received(&self->parser, data) == -1) {
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

  // TODO Doesn't this mean both are 0?  
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
  DBG printf("path >%.*s<\n", (int)self->request->path_len, self->request->path );
  return result;
}

PyObject *protocol_callPageHandler( Protocol* self, PyObject *func ) {
  //PyObject* result = NULL;

  int numArgs = self->request->numArgs;
  if ( numArgs ) {
    //PyObject *args = PyTuple_New( numArgs );
    PyObject **args = malloc( numArgs * sizeof(PyObject*) );
    
    for (int i=0; i<numArgs; i++) { 
      //printf(" arg%d len %d\n", i, self->request->argLens[i]);
      //printf(" arg%d %.*s\n", i, self->request->argLens[i], self->request->args[i] );

      //PyObject *arg = PyUnicode_FromStringAndSize( self->request->args[i],  self->request->argLens[i] );
      args[i] = PyUnicode_FromStringAndSize( self->request->args[i],  self->request->argLens[i] );
      //int rc = PyTuple_SetItem( args, i, arg );
      //if (rc) return NULL;
    }
    switch (numArgs) {
      case 1: return PyObject_CallFunctionObjArgs(func, args[0], NULL);
      case 2: return PyObject_CallFunctionObjArgs(func, args[0], args[1], NULL);
      case 3: return PyObject_CallFunctionObjArgs(func, args[0], args[1], args[2], NULL);
      case 4: return PyObject_CallFunctionObjArgs(func, args[0], args[1], args[2], args[3], NULL);
      case 5: return PyObject_CallFunctionObjArgs(func, args[0], args[1], args[2], args[3], args[4], NULL);
      case 6: return PyObject_CallFunctionObjArgs(func, args[0], args[1], args[2], args[3], args[4], args[5], NULL);
    }
    //return PyObject_CallFunctionObjArgs(func, args, NULL);
  } else {
    return PyObject_CallFunctionObjArgs(func, NULL);
  }
  //TODO num args > 6 or fail at start 
  return NULL;
}

Protocol* Protocol_on_body(Protocol* self, char* body, size_t body_len) {
  DBG printf("protocol - on body\n");

  PyObject* result = NULL;

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
  Py_INCREF(self->transport);
  request->app = self->app;
  Py_INCREF(self->app);

  Route *r = router_getRoute( &self->router, self->request );
  if ( r == NULL ) {
    // TODO pipeline..?
    protocol_write_error_response(self, 404,"Not Found","The requested page was not found");
    return self;
  }

  // If the route requires a session we need to check for a cookie session id, and fetch the session
  // from the backend.
  if ( r->session ) {
    DBG printf("Route requires a session\n"); 
    //TODO
  }

  if ( r->mtype ) {
    self->response->mtype = r->mtype;
  }

  if ( r->iscoro || !PIPELINE_EMPTY(self)) {
    //self->gather.enabled = false;
    // clone request
  }


  if(!(result = protocol_callPageHandler(self, r->func)) ) {
  //if(!(result = PyObject_CallFunctionObjArgs(r->func, NULL))) {
    DBG printf("Page handler call failed with an exception\n");
    PyObject *type, *value, *traceback;
    PyErr_Fetch(&type, &value, &traceback);
    Py_XDECREF(traceback);
    Py_XDECREF(type);
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
        return self;
      }
    }
    PyErr_Clear();
    printf("Unhandled exception:\n");
    PyObject_Print( type, stdout, 0 ); printf("\n");
    PyObject_Print( value, stdout, 0 ); printf("\n");
   
    Py_XDECREF(value);
    protocol_write_error_response(self, 500,"Internal Server Error","The server encountered an unexpected condition which prevented it from fulfilling the request.");
    return self;
    //PyErr_Restore(type, value, traceback);
  }

  if ( r->iscoro ) {
    DBG printf("protocol request is a coroutine\n");
    PyObject *task;
    if(!(task = PyObject_CallFunctionObjArgs(self->create_task, result, NULL))) return NULL;
    DBG printf("after create task\n");
    PyObject *ret = pipeline_queue(self, (PipelineRequest){true, self->request, task});
    Py_XDECREF(task);
    Py_DECREF(result);
    if ( !ret ) return NULL;
    return self;
  }

  if(!PIPELINE_EMPTY(self))
  {
    if(!pipeline_queue(self, (PipelineRequest){false, self->request, result})) goto error;
    Py_DECREF(result);
    return self;
  }


  if ( PyBytes_Check( result ) ) {
    result = PyUnicode_FromEncodedObject( result, "utf-8", "strict" );
    printf("WARNING: Page handler should return a string. Bytes object returned from the page handler is being converted to unicode using utf-8\n");
  }

  // Make sure the result is a string
  if ( !PyUnicode_Check( result ) ) {
    PyErr_SetString(PyExc_ValueError, "Page handler did not return a string");
    protocol_write_error_response(self, 500,"Internal Server Error","The server encountered an unexpected condition which prevented it from fulfilling the request.");
    goto error;
  }

  if(!protocol_write_response(self, self->request, result)) goto error;


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
  char *rbuf = self->response->rbuf;

  int headerLen = response_updateHeaders(self->response); // Add user headers 
  if (!headerLen) return NULL;

  // TODO Would unicode to bytes (PyUnicode_AsEncodedString) then concatenate body and header be faster? Could check those functions
  char *r = PyUnicode_AsUTF8AndSize( resp, &rlen ); 
  t = rlen;
  DBG_RESP printf("Response body >%.*s<\n", (int)rlen, r);
  memcpy( rbuf + headerLen, r, rlen );

  // Faster than sprintf
    //int result = sprintf( rbuf + 176, "%ld", rlen);
  int off = 33;
  char *s = rbuf + off;  
  do *s++ = (char)(48 + (t % 10ULL)); while(t /= 10ULL);
  reverse( rbuf+off, s-1 );  

  if ( unlikely(req->minor_version == 0) ) {
    rbuf[7] = '0';
  }

  if ( (0) ) {
    rbuf[88] = 'c'; rbuf[89] = 'l'; rbuf[90] = 'o'; rbuf[91] = 's'; rbuf[92] = 'e'; rbuf[93] = ' '; rbuf[94] = ' '; rbuf[95] = ' '; rbuf[96] = ' '; rbuf[97] = ' ';
  }

  DBG_RESP printf( "Sending response:\n\n%.*s\n", (int)rlen + headerLen, rbuf );
  PyObject *bytes;
  bytes = PyBytes_FromStringAndSize( rbuf, rlen + headerLen );
  PyObject *o;
  if(!(o = PyObject_CallFunctionObjArgs(self->write, bytes, NULL))) return NULL;
  Py_DECREF(o);


  return self;
}

static inline Protocol* protocol_write_redirect_response(Protocol* self, int code, char *url ) {
  PyObject *bytes = response_getRedirectResponse( self->response, code, url );
  if ( !bytes ) return NULL;
  PyObject *o;
  if(!(o = PyObject_CallFunctionObjArgs(self->write, bytes, NULL))) return NULL;
  Py_DECREF(o);
  return self;
}

// TODO rename
static inline Protocol* protocol_write_error_response(Protocol* self, int code, char *reason, char *msg) { 
  PyObject *bytes = response_getErrorResponse( self->response, code, reason, msg );
  if ( !bytes ) return NULL;
  PyObject *o;
  if(!(o = PyObject_CallFunctionObjArgs(self->write, bytes, NULL))) return NULL;
  Py_DECREF(o);
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

    DBG printf("  pipeline ready2\n");
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
          if(!protocol_write_error_response(self, code, PyUnicode_AsUTF8(reason), PyUnicode_AsUTF8(msg))) return NULL;
          return self;
        }
        // Check for HTTPRedirect
        msg = PyObject_GetAttrString(value, "url");
        if ( msg ) {
          int code = PyLong_AsLong(PyObject_GetAttrString(value, "code"));
          Py_DECREF(value);
          PyErr_Clear();
          if(!protocol_write_redirect_response(self, code, PyUnicode_AsUTF8(msg))) return NULL;
          return self;
        }

        Py_DECREF(value);
      }
      PyErr_Clear();

      printf("Unhandled exception:\n");
      PyObject_Print( type, stdout, 0 ); printf("\n");
      PyObject_Print( value, stdout, 0 ); printf("\n");
  
      protocol_write_error_response(self, 500,"Internal Server Error","The server encountered an unexpected condition which prevented it from fulfilling the request.");
      return self;
    }

  } else {
    response = task;
  }
  DBG printf("  pipeline ready3\n");

  if ( !PyUnicode_Check( response ) ) {
    PyErr_SetString(PyExc_ValueError, "Page handler did not return a string");
    protocol_write_error_response(self, 500,"Internal Server Error","The server encountered an unexpected condition which prevented it from fulfilling the request.");
    return NULL;
  }

  if(!self->closed) {
    if(!protocol_write_response(self, request, response))
      goto error;
  } else {
    // TODO: Send that to protocol_error
    printf("Connection closed, response dropped\n");
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

