
#include "unpack.h"
#include "py_defines.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#define DBG if(0) 

#define MAX_DEPTH 64

static char errmsg[256];
static PyObject* SetErrorInt(const char *message, int pos)
{
  char pstr[32];
  sprintf( pstr, "%d", pos );
  strcpy( errmsg, message );
  strcat( errmsg, pstr );
  PyErr_Format (PyExc_ValueError, "%s", errmsg);
  return NULL;
}


PyObject *decode( char *s, char *end) {
  PyObject *parents[MAX_DEPTH];
  PyObject *keys[MAX_DEPTH];
  int curlen[MAX_DEPTH];
  int maxlen[MAX_DEPTH];
  int types[MAX_DEPTH];
  
  int depth = -1;
  PyObject *o;


  while( s < end ) {
  if ( *s == 0x60 ) { s += 1; Py_INCREF(Py_None); o = Py_None; }
  else if ( (*(s) & 0xE0) == 0x80 ) {  // String
    int l = *(s) & 0x1F; 
    s += 1;
    o = PyUnicode_FromStringAndSize( s, l );
    s += l;
  }
  else if ( *(s) == 0x66 ) { 
    s++;
    uint32_t *p = (uint32_t*)s;
    uint32_t l = *p;
    s+=4;
    o = PyUnicode_FromStringAndSize( s, l );
    s += l;
  }
  else if ( *(s) == 0x61 ) { s += 1; Py_INCREF(Py_True);  o = Py_True; }
  else if ( *(s) == 0x62 ) { s += 1; Py_INCREF(Py_False); o = Py_False; }
  else if ( *(s) == 0x63 ) { 
    s++;
    union { double d; uint64_t i; } mem;
    uint64_t *p = (uint64_t*)s;
    mem.i = *p;
    s += 8;
    o = PyFloat_FromDouble(mem.d);
  }
  else if ( *(s) == 0x64 ) { 
    s++;
    long long *p = (long long*)s;
    s += 8;
    o = PyLong_FromLongLong(*p);
  }
  else if ( *(s) == 0x65 ) { 
    s++;
    uint64_t *p = (uint64_t*)s;
    s += 8;
    o = PyLong_FromUnsignedLongLong(*p);
  }
  else if ( *(s) == 0x68 ) { 
    s++;
    uint32_t *p = (uint32_t*)s;
    s += 4;
    o = PyLong_FromUnsignedLong(*p);
  }
  else if ( *(s) == 0x67 ) { 
    s++;
    int32_t *p = (int32_t*)s;
    s += 4;
    o = PyLong_FromLong(*p);
  }
  else if ( (*(s) & 0xE0) == 0xC0 ) {  // tiny int
    int i = *(s) & 0x1F; 
    s += 1;
    o = PyLong_FromLong(i);
  }
  else if ( (*(s) & 0xE0) == 0x40 ) {  // List
    int l = *(s) & 0x1F; 
    s += 1;
    if ( l == 0 ) o = PyList_New(0);
    else {
      depth += 1;
      if (depth == MAX_DEPTH) return SetErrorInt("Too many nested objects, the max depth is ", MAX_DEPTH);
      DBG printf("depth %d is list\n",depth);
      parents[depth] = PyList_New(l);
      curlen[depth] = 0;
      maxlen[depth] = l;
      types[depth] = 1;
      continue;
    }
  }
  else if ( *(s) == 0x6A ) {  // List
    s++;
    uint32_t *p = (uint32_t*)s;
    uint32_t l = *p;
    s+=4;
    depth += 1;
    if (depth == MAX_DEPTH) return SetErrorInt("Too many nested objects, the max depth is ", MAX_DEPTH);
    DBG printf("depth %d is list\n",depth);
    parents[depth] = PyList_New(l);
    curlen[depth] = 0;
    maxlen[depth] = l;
    types[depth] = 1;
    continue;
  }
  else if ( (*(s) & 0xE0) == 0x20 ) {  // dict
    int l = *(s) & 0x1F; 
    //printf("new dict of len %d\n", l);
    s += 1;
    if ( l > 0 ) {
      depth += 1;
      if (depth == MAX_DEPTH) return SetErrorInt("Too many nested objects, the max depth is ", MAX_DEPTH);
      DBG printf("depth %d is dict\n",depth);
      parents[depth] = PyDict_New();
      curlen[depth] = 0;
      maxlen[depth] = l;
      types[depth] = 2;
      keys[depth] = NULL;
      continue;
    } else {
      o = PyDict_New();
    }
  }
  else if ( *(s) == 0x69 ) {  // Dict
    s++;
    uint32_t *p = (uint32_t*)s;
    uint32_t l = *p;
    s+=4;
    if ( l > 0 ) {
      depth += 1;
      if (depth == MAX_DEPTH) return SetErrorInt("Too many nested objects, the max depth is ", MAX_DEPTH);
      DBG printf("depth %d is dict\n",depth);
      parents[depth] = PyDict_New();
      curlen[depth] = 0;
      maxlen[depth] = l;
      types[depth] = 2;
      keys[depth] = NULL;
      continue;
    } else {
      o = PyDict_New();
    }
  } else {
    PyErr_Format(PyExc_ValueError, "Parser error");
    return NULL;
  }
  //PyObject_Print(o, stdout, 0); printf("\n");

end:
  if ( depth == -1 ) return o;
  if ( types[depth] == 1 ) {
    PyList_SetItem( parents[depth],curlen[depth],o );
    curlen[depth] += 1;
    if ( curlen[depth] == maxlen[depth] ) {
      o = parents[depth];
      depth -= 1;
      goto end;
    }
  } else {
    if ( keys[depth] == NULL ) {
      keys[depth] = o;
    } else {
      PyDict_SetItem( parents[depth], keys[depth], o );
      Py_DECREF(keys[depth]); Py_DECREF(o);
      keys[depth] = NULL;
      curlen[depth] += 1;
      if ( curlen[depth] == maxlen[depth] ) {
        o = parents[depth];
        depth -= 1;
        goto end;
      }
    }
  } 
  } 
  return NULL;
}


PyObject* unpackc( char *p, int len ) {
  PyObject *ret = decode( (char*)p, (char*)p+len );
  return ret;
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

  char *p;
  Py_ssize_t l;

  if(PyBytes_AsStringAndSize(arg, &p, &l) == -1) {
    PyErr_Format(PyExc_TypeError, "Failed to convert byte object to c string. Out of memory?");
    return NULL;
  }

  return decode( p, p+l );

}

