
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <cstring>
#include <x86intrin.h>
#ifdef __AVX2__
#include <immintrin.h>
#endif

#include <benchmark/benchmark.h>

#if __GNUC__ >= 3
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#ifdef _MSC_VER
#define ALIGNED(n) _declspec(align(n))
#else
#define ALIGNED(n) __attribute__((aligned(n)))
#endif


#define IS_PRINTABLE_ASCII(c) ((unsigned char)(c)-040u < 0137u)

#define hex_to_dec(x) \
  ((x <= '9' ? 0 : 9) + (x & 0x0f))
#define is_hex(x) ((x >= '0' && x <= '9') || (x >= 'A' && x <= 'F'))


#define CHAR4_TO_INT(a, b, c, d)         \
   (unsigned int)((d << 24) | (c << 16) | (b << 8) | a)


#define CHECK_END()                                                                                                                \
    if (buf == buf_end) {                                                                                                          \
        *ret = -2;                                                                                                                 \
        return NULL;                                                                                                               \
    }


#define CHECK_EOF()                                                                                                                \
    if (buf == buf_end) {                                                                                                          \
        *ret = -2;                                                                                                                 \
        return NULL;                                                                                                               \
    }

#define EXPECT_CHAR_NO_CHECK(ch)                                                                                                   \
    if (*buf++ != ch) {                                                                                                            \
        *ret = -1;                                                                                                                 \
        return NULL;                                                                                                               \
    }

#define EXPECT_CHAR(ch)                                                                                                            \
    CHECK_EOF();                                                                                                                   \
    EXPECT_CHAR_NO_CHECK(ch);


// Table for converting to lower case
#define TOLC(c) __lct[(unsigned char)c]
static const unsigned char __lct[] __attribute__((aligned(64))) = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
  0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
  0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
  0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
  0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
  0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
  0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
  0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
  0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
  0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
  0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
  0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
  0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
  0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
  0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
  0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
  0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
  0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};


static const char *token_char_map = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                    "\0\1\0\1\1\1\1\1\0\0\1\1\0\1\1\0\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0"
                                    "\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\1\1"
                                    "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\1\0\1\0"
                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

static unsigned long TZCNT(unsigned long long in) {
  unsigned long res;
  asm("tzcnt %1, %0\n\t" : "=r"(res) : "r"(in));
  return res;
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
//          1         2         3         4         5         6         7
// 1234567890123456789012345678901234567890123456789012345678901234567890
// /spanish/objetos%20voladores%20no%20identificados?foo=bar
// /spanish/objetos%20voladores/
static inline size_t sse_decode(char* path, ssize_t length, int *qs_len) {
  //DBG printf("sse_decode >%.*s<\n", (int)length, path);
  if (length == 0) return length;
  char *pat = path;
  static char ranges1[] = "%%" "??";
  char *end = path + length;
  int found;

  // We only findchar once - benchmark one or more % encodings with continuing to use findchar ( Spanish / Chinese )
  do {
    //printf("sse_decode >%.*s<\n", (int)length, path);
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
  //printf("sse_decode len %d path >%.*s<\n", (int)length, (int)length, path);
  //printf(" qs %d\n",*qs_len);

  return length;
}

__m256i m37 = _mm256_set1_epi8(37); // %
__m256i m63 = _mm256_set1_epi8(63); // ?
//          1         2         3         4         5         6         7
// 1234567890123456789012345678901234567890123456789012345678901234567890
// /print/%E4%B8%8D%E5%8F%AF%E5%86%8D%E7%94%9F%E8%B5%84%E6%BA%90/?test";
// /spanish/objetos%20voladores%20no%20identificados?foo=bar
// /spanish/objetos%20voladores/
static inline int path_decode(char* buf, int len, int *qs_len) {
  unsigned int msk;
  int i=0,tz; // 32B index
  unsigned int shifted;
  char *sbuf = buf;
  char *obuf = buf;
  char *buf_end = buf+len;
  char *wbuf;
  int found = 0;

  do {
    const char *block_start = obuf+32*i;
    __m256i b0 = _mm256_loadu_si256((const __m256i *) block_start);
    msk = _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b0, m37), _mm256_cmpeq_epi8(b0, m63) ) );
    while (1) {

      //if ( buf >= buf_end ) { goto decdone; }
      shifted = buf-block_start;
      if ( shifted >= 32 ) break;
      tz = TZCNT((msk >> shifted));
      if ( tz < 32 ) {
        buf += tz;
        //printf( " fnd >%.*s<\n", (int)(buf-sbuf), sbuf );  
        if ( buf >= buf_end ) { goto decdone; }
        if ( *buf == '?' ) { 
          len -= buf_end-buf;
          *qs_len = buf_end-buf-1;
          //printf("path_decode len %d path >%.*s<\n", (int)len, (int)len, obuf);
          goto decdone;
        }
        if ( *buf == '%' ) { 
          if ( found ) {
            //printf( " copy >%.*s<\n", (int)(buf-sbuf), sbuf );  
            memcpy( wbuf, sbuf, buf-sbuf );
            //printf( " to   >%.*s<\n", (int)(buf-sbuf), wbuf );  
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
        buf += 32 - shifted;
        break;
      }

    }
    i+=1;
    if ( buf >= buf_end ) { goto decdone; }
  } while ( buf < buf_end ); // Why doesn't this work

decdone:
  if ( found ) {
    //printf( " copy >%.*s<\n", (int)(buf-sbuf), sbuf );  
    memcpy( wbuf, sbuf, buf-sbuf );
    //printf( " to   >%.*s<\n", (int)(buf-sbuf), wbuf );  
  }
  //printf( " fnd >%.*s<\n", (int)(buf-sbuf), sbuf );  
  //printf("path_decode len %d path >%.*s<\n", (int)len, (int)len, obuf);
  //printf(" qs %d\n",*qs_len);
  return len;
}
static inline int path_decode2(char* buf, int len, int *qs_len) {
  unsigned long long msk;
  int i=0,tz; // 32B index
  unsigned int shifted;
  char *sbuf = buf;
  char *obuf = buf;
  char *buf_end = buf+len;
  char *wbuf;
  int found = 0;

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
          *qs_len = buf_end-buf-1;
          //printf("path_decode len %d path >%.*s<\n", (int)len, (int)len, obuf);
          goto decdone;
        }
        if ( *buf == '%' ) { 
          if ( found ) {
            //printf( " copy >%.*s<\n", (int)(buf-sbuf), sbuf );  
            memcpy( wbuf, sbuf, buf-sbuf );
            //printf( " to   >%.*s<\n", (int)(buf-sbuf), wbuf );  
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
    //printf( " copy >%.*s<\n", (int)(buf-sbuf), sbuf );  
    memcpy( wbuf, sbuf, buf-sbuf );
    //printf( " to   >%.*s<\n", (int)(buf-sbuf), wbuf );  
  }
  //printf( " fnd >%.*s<\n", (int)(buf-sbuf), sbuf );  
  //printf("path_decode len %d path >%.*s<\n", (int)len, (int)len, obuf);
  //printf(" qs %d\n",*qs_len);
  return len;
}
static inline int path_decode3(char* buf, int len, int *qs_len) {
  if ( len > 32 ) return path_decode2(buf,len,qs_len);
  else            return path_decode(buf,len,qs_len);
}

static inline void parse_query_args_old( char *buf, size_t buflen ) {
  char *end = buf + buflen;
  char *last = buf;
  //PyObject* args = PyDict_New();

  if ( buflen == 0 ) return;


  //PyObject* key = NULL; PyObject* value = NULL;

  static char ALIGNED(16) ranges1[] = "==" "&&";
  int found;
  int state = 0;
  int grab_session = 0;
  int ignore_me = 0;
  size_t len;
  // foo=bar&key=23%28
  do {
    buf = findchar(buf, end, ranges1, sizeof(ranges1) - 1, &found);
    if ( found ) {
      if ( *buf == '=' ) {
        len = sse_decode( last, buf-last, &ignore_me );
        //key = PyUnicode_FromStringAndSize(last, len); //TODO error
        //printf( " key >%.*s<\n", (int)(buf-last), last);
        state = 1;
        buf+=1;
        last = buf;
      }
      else if ( *buf == '&' ) {
        //if ( state == 0 ) key  = PyUnicode_FromString("");

       len = sse_decode( last, buf-last, &ignore_me );
        //value = PyUnicode_FromStringAndSize(last, len);
        //printf( " val >%.*s<\n", (int)(buf-last), last);
        state = 0;
        //PyDict_SetItem(args, key, value);  //  == -1) goto loop_error;
        //Py_XDECREF(key);
        //Py_XDECREF(value);
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
    //if ( state == 0 ) key  = PyUnicode_FromString("");
    if ( buf == end && *(buf-1) == ' ' ) {
      len = path_decode( last, buf-last-1, &ignore_me );
      //value = PyUnicode_FromStringAndSize(last, len); //TODO error
      //printf( " val >%.*s<\n", (int)(buf-last-1), last);
    } else {
      len = path_decode( last, buf-last, &ignore_me );
      //value = PyUnicode_FromStringAndSize(last, len); //TODO error
      //printf( " val >%.*s<\n", (int)(buf-last), last);
    }
    state = 0;
    //PyDict_SetItem(args, key, value);  //  == -1) goto loop_error;
    //Py_XDECREF(key);
    //Py_XDECREF(value);
  }

  return;
}
static inline void parse_query_args( char *buf, size_t buflen ) {
  char *end;
  char *last = buf;
  size_t len;
  int ignore_me = 0;
  //PyObject* args = PyDict_New();

  if ( buflen == 0 ) return;

  len = path_decode2( buf, buflen, &ignore_me );
  //printf( " decoded >%.*s<\n", (int)(len), buf);
  end = buf + len;

  //PyObject* key = NULL; PyObject* value = NULL;

  static char ALIGNED(16) ranges1[] = "==" "&&";
  int found;
  int state = 0;
  int grab_session = 0;
  // foo=bar&key=23%28
  do {
    buf = findchar(buf, end, ranges1, sizeof(ranges1) - 1, &found);
    if ( found ) {
      if ( *buf == '=' ) {
        //len = path_decode( last, buf-last, &ignore_me );
        //key = PyUnicode_FromStringAndSize(last, len); //TODO error
        //printf( " key >%.*s<\n", (int)(buf-last), last);
        state = 1;
        buf+=1;
        last = buf;
      }
      else if ( *buf == '&' ) {
        //if ( state == 0 ) key  = PyUnicode_FromString("");

       //len = path_decode( last, buf-last, &ignore_me );
        //value = PyUnicode_FromStringAndSize(last, len);
        //printf( " val >%.*s<\n", (int)(buf-last), last);
        state = 0;
        //PyDict_SetItem(args, key, value);  //  == -1) goto loop_error;
        //Py_XDECREF(key);
        //Py_XDECREF(value);
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
    //if ( state == 0 ) key  = PyUnicode_FromString("");
    if ( buf == end && *(buf-1) == ' ' ) {
      //len = path_decode( last, buf-last-1, &ignore_me );
      //value = PyUnicode_FromStringAndSize(last, len); //TODO error
      //printf( " val >%.*s<\n", (int)(buf-last-1), last);
    } else {
      //len = path_decode( last, buf-last, &ignore_me );
      //value = PyUnicode_FromStringAndSize(last, len); //TODO error
      //printf( " val >%.*s<\n", (int)(buf-last), last);
    }
    state = 0;
    //PyDict_SetItem(args, key, value);  //  == -1) goto loop_error;
    //Py_XDECREF(key);
    //Py_XDECREF(value);
  }

  return;
}
__m256i m38 = _mm256_set1_epi8(38); // &
__m256i m61 = _mm256_set1_epi8(61); // =
static inline void parse_query_args2( char *buf, size_t len ) {
  unsigned long long msk;
  int i=0,tz; // 32B index
  unsigned int shifted;
  char *sbuf = buf;
  char *obuf = buf;
  int state = 0;
  int ignore_me = 0;

  if ( len == 0 ) return;
  char *buf_end = buf+len;

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
          //printf( " key >%.*s<\n", (int)(buf-sbuf), sbuf );  
          len = path_decode2( sbuf, buf-sbuf, &ignore_me );
          state = 1;
          buf += 1;
        }
        if ( *buf == '&' ) { 
          //printf( " val >%.*s<\n", (int)(buf-sbuf), sbuf);
          len = path_decode2( sbuf, buf-sbuf, &ignore_me );
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
  len = path_decode2( sbuf, buf-sbuf, &ignore_me );
  //printf( " done >%.*s<\n", (int)(buf-sbuf), sbuf );  
  return;
}




static char buf[8096] = "/foo/bar/bazfdas";
static int  blen = strlen("/foo/bar/bazfdas");
static char buf2[8096] = "/foo/bar/bazfdasfffffffffffffffffffffffffffffffffffffffdffffffffffffffffffffffffffffffffffffffffffffffffff1?foo=bar";
static int  blen2 = strlen("/foo/bar/bazfdasfffffffffffffffffffffffffffffffffffffffdffffffffffffffffffffffffffffffffffffffffffffffffff1?foo=bar");
//static char buf[8096]   = "/spanish/objetos%20voladoresentificados";
//static int  blen = strlen("/spanish/objetos%20voladoresentificados");
static char buf3[8096] = "/spanish/objetos%20voladores%20no%20identificados?foo=bar";
static int  blen3 = strlen("/spanish/objetos%20voladores%20no%20identificados?foo=bar");

static char buf4[8096] = "/print/%E4%B8%8D%E5%8F%AF%E5%86%8D%E7%94%9F%E8%B5%84%E6%BA%90/?test";
static int  blen4 = strlen("/print/%E4%B8%8D%E5%8F%AF%E5%86%8D%E7%94%9F%E8%B5%84%E6%BA%90/?test");

static char qbuf[8096] = "p1=v1&param2=value2";
static int  qlen = strlen("p1=v1&param2=value2");
static char qbuf2[8096] = "key%201=%C2%BFPeroc%C3%B3mopuedesdecir%20esto%3F&param2=val2";
static int  qlen2 = strlen("key%201=%C2%BFPeroc%C3%B3mopuedesdecir%20esto%3F&param2=val2");
static char qbuf3[8096] = "key%201=%C2%BFPeroc%C3%B3mopuedesdecir%20esto%3F&param2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222220=val2&ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa1";
static int  qlen3 = strlen("key%201=%C2%BFPeroc%C3%B3mopuedesdecir%20esto%3F&param2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222220=val2&ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa1");


static void BM_sse_decode(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    sse_decode(buf, blen, &qslen);
  }
}
static void BM_path_decode(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    path_decode(buf, blen, &qslen);
  }
}
static void BM_path_decode2(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    path_decode2(buf, blen, &qslen);
  }
}
static void BM_path_decode3(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    path_decode3(buf, blen, &qslen);
  }
}
static void BM_sse_decode_long(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    sse_decode(buf2, blen2, &qslen);
  }
}
static void BM_path_decode_long(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    path_decode(buf2, blen2, &qslen);
  }
}
static void BM_path_decode2_long(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    path_decode2(buf2, blen2, &qslen);
  }
}
static void BM_path_decode3_long(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    path_decode3(buf2, blen2, &qslen);
  }
}
static void BM_sse_decode_complex(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    sse_decode(buf3, blen3, &qslen);
  }
}
static void BM_path_decode_complex(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    path_decode(buf3, blen3, &qslen);
  }
}
static void BM_path_decode2_complex(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    path_decode2(buf3, blen3, &qslen);
  }
}
static void BM_path_decode3_complex(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    path_decode3(buf3, blen3, &qslen);
  }
}
static void BM_sse_decode_chinese(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    sse_decode(buf4, blen4, &qslen);
  }
}
static void BM_path_decode_chinese(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    path_decode(buf4, blen4, &qslen);
  }
}
static void BM_path_decode2_chinese(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    path_decode2(buf4, blen4, &qslen);
  }
}
static void BM_query_args_old(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    parse_query_args_old(qbuf, qlen);
  }
}
static void BM_query_args(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    parse_query_args(qbuf, qlen);
  }
}
static void BM_query_args2(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    parse_query_args2(qbuf, qlen);
  }
}
static void BM_query_args_decodes_old(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    parse_query_args_old(qbuf3, qlen3);
  }
}
static void BM_query_args_decodes(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    parse_query_args(qbuf3, qlen3);
  }
}
static void BM_query_args_decodes2(benchmark::State& state) {
  int qslen = 0;
  for (auto _ : state) {
    parse_query_args2(qbuf3, qlen3);
  }
}

BENCHMARK(BM_query_args_old);
BENCHMARK(BM_query_args);
BENCHMARK(BM_query_args2);
BENCHMARK(BM_query_args_decodes_old);
BENCHMARK(BM_query_args_decodes);
BENCHMARK(BM_query_args_decodes2);
/*
BENCHMARK(BM_sse_decode);
BENCHMARK(BM_path_decode);
BENCHMARK(BM_path_decode2);
BENCHMARK(BM_path_decode3);
BENCHMARK(BM_sse_decode_long);
BENCHMARK(BM_path_decode_long);
BENCHMARK(BM_path_decode2_long);
BENCHMARK(BM_path_decode3_long);
BENCHMARK(BM_sse_decode_complex);
BENCHMARK(BM_path_decode_complex);
BENCHMARK(BM_path_decode2_complex);
BENCHMARK(BM_path_decode3_complex);
BENCHMARK(BM_sse_decode_chinese);
BENCHMARK(BM_path_decode_chinese);
BENCHMARK(BM_path_decode2_chinese);
*/
BENCHMARK_MAIN();
/*
int main() {

  int qslen = 0;
  //printf("%.*s\n",blen4,buf4);
  //path_decode2(buf4, blen4, &qslen);
  //sse_decode(buf, blen, &qslen);

  parse_query_args2(qbuf3, qlen3);

}


*/
