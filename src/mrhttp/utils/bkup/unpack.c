#include "unpack.h"
#include "py_defines.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct _decoder
{
  unsigned char *start, *end, *s;
  int depth;
} Decoder;


static char    *sbuf = NULL;
static uint32_t sbuf_len;

static void print_buffer( unsigned char* b, int len ) {
  for ( int z = 0; z < len; z++ ) {
    printf( "%02x ",(int)b[z]);
  }
  printf("\n");
}

PyObject *decode( Decoder *d ) { //unsigned char *s, unsigned char *end) {
  if ( *(d->s) == 0x60 ) { d->s += 1; Py_INCREF(Py_None); return Py_None; }
  else if ( (*(d->s) & 0xE0) == 0x80 ) {  // String
    int l = *(d->s) & 0x1F; 
    if ( l > sbuf_len ) { 
      while ( l > sbuf_len ) { sbuf_len <<= 1; } 
      sbuf = (char*)realloc(sbuf, sbuf_len);
    }
    d->s += 1;
    memcpy( sbuf, d->s, l );
    d->s += l;
    return PyUnicode_FromStringAndSize( sbuf, l );
  }
  else if ( *(d->s) == 0x61 ) { d->s += 1; Py_INCREF(Py_True); return Py_True; }
  else if ( *(d->s) == 0x62 ) { d->s += 1; Py_INCREF(Py_False); return Py_False; }
  else if ( *(d->s) == 0x63 ) { 
    d->s++;
    union { double d; uint64_t i; } mem;
    uint64_t *p = (uint64_t*)d->s;
    mem.i = *p;
    d->s += 8;
    return PyFloat_FromDouble(mem.d);
  }
  else if ( *(d->s) == 0x64 ) { 
    d->s++;
    long long *p = (long long*)d->s;
    d->s += 8;
    return PyLong_FromLong(*p);
  }
  else if ( *(d->s) == 0x65 ) { 
    d->s++;
    uint64_t *p = (uint64_t*)d->s;
    d->s += 8;
    return PyLong_FromUnsignedLong(*p);
  }
  else if ( *(d->s) == 0x68 ) { 
    d->s++;
    uint32_t *p = (uint32_t*)d->s;
    d->s += 4;
    return PyLong_FromUnsignedLong(*p);
  }
  else if ( *(d->s) == 0x67 ) { 
    d->s++;
    int32_t *p = (int32_t*)d->s;
    d->s += 4;
    return PyLong_FromLong(*p);
  }
  else if ( *(d->s) == 0x66 ) { 
    d->s++;
    uint32_t *p = (uint32_t*)d->s;
    uint32_t l = *p;
    d->s+=4;
    if ( l > sbuf_len ) { 
      while ( l > sbuf_len ) { sbuf_len <<= 1; } 
      sbuf = (char*)realloc(sbuf, sbuf_len);
    }
    memcpy( sbuf, d->s, l );
    d->s += l;
    return PyUnicode_FromStringAndSize( sbuf, l );
  }
  else if ( (*(d->s) & 0xE0) == 0xC0 ) {  // tiny int
    int i = *(d->s) & 0x1F; 
    d->s += 1;
    return PyLong_FromLong(i);
  }
  else if ( (*(d->s) & 0xE0) == 0x40 ) {  // List
    int l = *(d->s) & 0x1F; 
    d->s += 1;
    d->depth += 1;
    PyObject *ret = PyList_New(l);
    //printf("new list of len %d", l);
    for (Py_ssize_t i = 0; i < l; i++) {
      if (Py_EnterRecursiveCall(" while unpacking list object")) return 0;
      PyList_SetItem( ret, i, decode( d ) );
      //PyObject_Print( ret, stdout, 0 );
      Py_LeaveRecursiveCall();
    }
    d->depth -= 1;
    return ret;
  }
  else if ( (*(d->s) & 0xE0) == 0x20 ) {  // dict
    int l = *(d->s) & 0x1F; 
    d->s += 1;
    d->depth += 1;
    PyObject *ret = PyDict_New();
    for (Py_ssize_t i = 0; i < l; i++) {
      PyObject* k = decode(d);
      PyObject* v = decode(d);
      PyDict_SetItem( ret, k, v );
    }
    d->depth -= 1;
    return ret;
  }
  else {
    PyErr_Format(PyExc_ValueError, "Parser error");
    return NULL;
  }
}


PyObject* unpackc( unsigned char *p, int len ) {
  Decoder d = { p,p+len,p,0 };
  PyObject *ret = decode( &d );
  return ret;
}
void initmrpacker(void) {
  if ( sbuf == NULL ) {
    sbuf_len = 256;
    sbuf = (char*)malloc(sbuf_len);
  }
}

PyObject* unpack(PyObject* self, PyObject *args, PyObject *kwargs)
{
  static char *kwlist[] = {"obj", NULL};
  PyObject *arg;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &arg)) return NULL;

  if (! PyBytes_Check(arg))
  {
    PyErr_Format(PyExc_TypeError, "Expected bytes");
    return NULL;
  }

  //sbuf_len = 256;
  //sbuf = (char*)malloc(sbuf_len);

  unsigned char* p;
  Py_ssize_t l;

  if(PyBytes_AsStringAndSize(arg, &p, &l) == -1) {
    PyErr_Format(PyExc_TypeError, "Failed to convert byte object to c string. Out of memory?");
    return NULL;
  }

  //print_buffer( p, l );

  Decoder d = { p,p+l,p,0 };

  return decode( &d );

/*
  char *endptr;
  //ret = jsonParse(PyString_AS_STRING(sarg), &endptr, PyString_GET_SIZE(sarg));

  if (sarg != arg)
  {
    Py_DECREF(sarg);
  }

  return ret;
*/
}

