

#include "utils.h"

PyObject* myrandint(PyObject* self, PyObject* args)
{
  int min, max;
  if (!PyArg_ParseTuple(args, "ii", &min, &max)) return NULL;
  return PyLong_FromLong(min + (rand() / (RAND_MAX / (max + 1) + 1)));
}

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

// Valgrind doesn't support mm_cmpestri so replace findchar
char *valgrind_zfindchar(char *buf, char *buf_end, char *ranges, size_t ranges_size, int *found)
{
    //printf("DELME ranges sz %d\n", ranges_size);
  *found = 0;
  char *p = buf;
  while ( p < buf_end ) {
    for ( int i = 0; i < ranges_size; i += 2 ) {
      if ( p >= ranges[i] && p <= ranges[i+1] ) {
        *found = 1;
        return p;
    	}
    }
    p++;
  }
  return p;    
}
// Search for a range of characters and return a pointer to the location or buf_end if none are found
char *findchar(char *buf, char *buf_end, char *ranges, size_t ranges_size, int *found)
{
    *found = 0;
    __m128i ranges16 = _mm_loadu_si128((const __m128i *)ranges);
    if (likely(buf_end - buf >= 16)) {

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

    size_t left = buf_end - buf;
    if ( left != 0 ) {
      static char sbuf[16] = {0};
      memcpy( sbuf, buf, left );
      __m128i b16 = _mm_loadu_si128((const __m128i *)sbuf);
      size_t r = _mm_cmpestri(ranges16, ranges_size, b16, 16, _SIDD_LEAST_SIGNIFICANT | _SIDD_CMP_RANGES | _SIDD_UBYTE_OPS);
      if (unlikely(r != 16) && r < left) {
        buf += r;
        *found = 1;
        return buf;
      } else {
        buf = buf_end;
      }
    }

    *found = 0;
    return buf;
}

static char escbuf[16*1024];


PyObject *escape_html(PyObject *self, PyObject *s) {

  Py_ssize_t l;
  char *st = PyUnicode_AsUTF8AndSize( s, &l );
  if ( l == 0 ) { Py_INCREF(s); return s; }

  if ( l > 16000 ) return NULL; // TODO malloc and realloc a buffer

  const char *p = st;
  const char *end = st+l;
  const char *last = st;
  char *o = escbuf;
  int found;
  static char ranges1[] = "<<" ">>" "\x22\x22" "&&";

  while ( p < end ) {
    p = findchar(p, end, ranges1, sizeof(ranges1) - 1, &found);
    if ( found ) { 
      if ( p[0] == '<' ) {
        memcpy( o, last, p-last ); o += p-last;
        memcpy( o, "&lt;", 4); o += 4;
        p++; 
        last = p;
      }
      if ( p[0] == '&' ) {
        memcpy( o, last, p-last ); o += p-last;
        memcpy( o, "&amp;", 5); o += 5;
        p++; 
        last = p;
      }
      if ( p[0] == '>' ) {
        memcpy( o, last, p-last ); o += p-last;
        memcpy( o, "&gt;", 4); o += 4;
        p++; 
        last = p;
      }
      if ( p[0] == 0x22 ) {
        memcpy( o, last, p-last ); o += p-last;
        memcpy( o, "&quot;", 6); o += 6;
        p++; 
        last = p;
      }
    }
  }
  // Return the string if nothing to escape
  if ( o == escbuf ) { Py_INCREF(s); return s; }

  memcpy( o, last, end-last ); o += end-last;

  return PyUnicode_FromStringAndSize( escbuf, o-escbuf );
  
}

PyObject *to64     (PyObject *self, PyObject *num) {

  if ( !PyLong_Check(num) ) { PyErr_SetString(PyExc_ValueError, "to64 requires a number and was passed something else"); return NULL; }

  char ret[128];
  int i = 127;
  uint64_t n = PyLong_AsUnsignedLongMask(num);
  while (n) {
    ret[i--] = to_64[n & 0x3F];
    n >>= 6;
  }
  char *p = ret+i+1;

  return PyUnicode_FromStringAndSize( p, 127-i );
}
PyObject *from64   (PyObject *self, PyObject *str) {

  if ( !PyUnicode_Check(str) ) { PyErr_SetString(PyExc_ValueError, "from64 requires a string and was passed something else"); return NULL; }

  uint64_t num = 0;
  Py_ssize_t l;
  char *p = PyUnicode_AsUTF8AndSize( str, &l );
  int skip = 1;
  for ( int i = 0; i < l; i++) {
    if ( skip ) skip = 0;
    else num <<= 6;
    num |= from_64[(unsigned char)p[i]];
  }

  return PyLong_FromUnsignedLongLong(num);

}

static char hoursago[16]   = "   hours ago";
static char minutesago[16] = "   minutes ago";
static char daysago[16]    = "   days ago";
static char monthsago[16]  = "   months ago";
static char yearsago[16]   = "   years ago";

PyObject *timesince( PyObject *self, PyObject *ts ) {

  if ( !PyLong_Check(ts) ) { PyErr_SetString(PyExc_ValueError, "timesince requires a number and was passed something else"); return NULL; }

  uint64_t then = PyLong_AsUnsignedLong(ts);
  if (then == (unsigned long)-1) {
    PyErr_SetString(PyExc_ValueError, "timesince only accepts a number greater than 0");
    return NULL;
  }
  uint64_t diff = time(NULL)-then;

  char *st = hoursago;
  int sl = 12;
  if (diff < 86400) {
    if (diff > 3600) {
      diff /= 3600;
    } else {
      diff /= 60;
      st = minutesago;
      sl = 14;
    }
  } else {
    if ( diff > 31536000 ) {
      diff /= 31536000;
      st = yearsago;
    } else if ( diff > 2592000 ) {
      diff /= 2592000;
      st = monthsago;
      sl = 13;
    } else {
      diff /= 86400;
      st = daysago;
      sl = 11;
    }
  }
  *st = ' ';
  char *s  = st+1;
  do *s-- = (char)(48 + (diff % 10ULL)); while(diff /= 10ULL);
  return PyUnicode_FromStringAndSize( s, sl-(s-st));

}


PyObject *hot(PyObject *self, PyObject *args) {

  int64_t rtg;
  uint64_t ts;
  if(!PyArg_ParseTuple(args, "LK", &rtg, &ts)) return NULL;

  uint64_t r;
  if ( rtg < 0 ) {
    r = (long) ((((ts-1134028003) / 45000) - log10(-rtg))  *10000);
  } else if ( rtg > 0 ) {
    r = (long) ((((ts-1134028003) / 45000) + log10(rtg) )  *10000);
  } else {
    r = (long) (ts-1134028003) / 45;
  }

  return PyLong_FromUnsignedLong(r);

}

