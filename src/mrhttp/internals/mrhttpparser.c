
/*
 * Copyright (c) 2013-2018 Mark Reed
 *
 * The software is licensed under either the MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 */


//DELME
#include <stdio.h>


#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#ifdef __AVX2__
#include <immintrin.h>
#else
#include <x86intrin.h>
#endif
#include "mrhttpparser.h"

#if __GNUC__ >= 3
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#define ALIGNED(x) __attribute__ ((aligned(x)))

#define IS_PRINTABLE_ASCII(c) ((unsigned char)(c)-040u < 0137u)

static void print_buffer( char* b, int len ) {
  for ( int z = 0; z < len; z++ ) {
    printf( "%02x ",(int)b[z]);
  }
  printf("\n");
}

#define CHECK_END()                                                                                                                \
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
    CHECK_END();                                                                                                                   \
    EXPECT_CHAR_NO_CHECK(ch);


static unsigned long TZCNT(unsigned long long in) {
  unsigned long res;
  asm("tzcnt %1, %0\n\t" : "=r"(res) : "r"(in));
  return res;
}
// TODO just len
static int get_len_to_space(const char *buf, const char *buf_end) {
  const char *orig = buf;
  __m256i m32 = _mm256_set1_epi8(32);
  while (1)
  {
    __m256i v0 = _mm256_loadu_si256((const __m256i *)buf);
    __m256i v1 = _mm256_cmpeq_epi8(v0, m32);
    unsigned long vmask = _mm256_movemask_epi8(v1);
    if (vmask != 0) {
        buf += TZCNT(vmask);
        return buf-orig;
    }
    buf += 32;
    if ( buf >= buf_end ) return -1;
  }
}

static const char *parse_headers_avx2(const char *buf, const char *buf_end, struct mr_header *headers, size_t *num_headers,
                                 size_t max_headers, int *ret, struct mr_request *mrr)
{
  unsigned long msk;
  int i=0, tz; // 32B index
  unsigned int shifted;
  const char *sbuf = buf;
  const char *obuf = buf;
  int name_or_value = 0;

  __m256i m13 = _mm256_set1_epi8(13); // \r
  __m256i m58 = _mm256_set1_epi8(58); // :

  do {
    const char *block_start = obuf+64*i;
    if ( block_start > buf_end ) { printf("DELME hdr too big\n"); *ret = -1; return NULL; }

    __m256i b0 = _mm256_loadu_si256((const __m256i *) block_start);
    __m256i b1 = _mm256_loadu_si256((const __m256i *) (block_start+32));
    msk = (unsigned int) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b0, m13), _mm256_cmpeq_epi8(b0, m58) ) )  | 
        ((unsigned long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b1, m13), _mm256_cmpeq_epi8(b1, m58) ) ) << 32);

    while (1) {
    
      shifted = buf-block_start;
      if ( shifted >= 64 ) break;
      tz = TZCNT((msk >> shifted));
      if ( tz < 64 ) {
        buf += tz;

        if ( name_or_value == 1 ) {
          if ( *buf == ':' ) { buf += 1; continue; } // : in value field
          headers[*num_headers].value = sbuf;
          headers[*num_headers].value_len = buf-sbuf;
          ++*num_headers;
          if (*num_headers >= max_headers) { printf("DELME hdr too many\n"); *ret = -1; return NULL; }
          name_or_value = 0;
          buf += 2; if ( *buf == '\r' ) { buf+=2; *ret=0; return buf; } // \r\n\r\n marks the end
        } else {
          headers[*num_headers].name = sbuf;
          headers[*num_headers].name_len = buf-sbuf;
          name_or_value = 1;
          buf += 2;
        }
        sbuf = buf;
      } else {
        buf += 64 - shifted;
        break;
      }

    }
    i++;
  } while ( buf < buf_end );
  *ret = -1;
  return buf;
}

static const char *parse_request(const char *buf, const char *buf_end, const char **method, size_t *method_len, const char **path,
                                 size_t *path_len, int *minor_version, struct mr_header *headers, size_t *num_headers,
                                 size_t max_headers, int *ret, struct mr_request *mrr)
{
    // skip first empty line (some clients add CRLF after POST content)
    CHECK_END();
    if (*buf == '\015') {
        ++buf;
        EXPECT_CHAR('\012');
    } else if (*buf == '\012') {
        ++buf;
    }

    // parse request line
    // TODO Support other methods
    switch (*(unsigned int *)buf) {
      case CHAR4_TO_INT('G', 'E', 'T', ' '):
        *method = buf; *method_len = 3; buf += 4; break;
      case CHAR4_TO_INT('P', 'O', 'S', 'T'):
        *method = buf; *method_len = 4; buf += 5; break;
      default:
        *ret = -1;
        return NULL;
    }
    *path = buf;
    int l = get_len_to_space(buf, buf_end);
    if ( l == -1 ) {
        *ret = -1; // TODO Should we return -2 (needs more bytes?)
        return NULL;
    }
    buf += l; *path_len = l;
    ++buf;
    switch (*(unsigned long *)buf) {
      case CHAR8_TO_LONG('H', 'T', 'T', 'P','/','1','.','0'):
        *minor_version = 0; buf += 8; break;
      case CHAR8_TO_LONG('H', 'T', 'T', 'P','/','1','.','1'):
        *minor_version = 1; buf += 8; break;
      default:
        *ret = -2;
        return NULL;
    }
    
    if (*buf == '\015') {
        ++buf;
        EXPECT_CHAR('\012');
    } else if (*buf == '\012') {
        ++buf;
    } else {
        *ret = -2;
        return NULL;
    }
    return parse_headers_avx2(buf, buf_end, headers, num_headers, max_headers, ret, mrr);
    //return parse_headers(buf, buf_end, headers, num_headers, max_headers, ret, mrr);
}

static __inline__ unsigned long long rdtsc(void)
{
  unsigned long long int x;
     __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
     return x;
}

int mr_parse_request(const char *buf_start, size_t len, const char **method, size_t *method_len, const char **path,
                      size_t *path_len, int *minor_version, struct mr_header *headers, size_t *num_headers, struct mr_request *mrr)
{
    const char *buf = buf_start, *buf_end = buf_start + len;
    size_t max_headers = *num_headers;
    int r;

    //unsigned long long cycles = rdtsc();

    *method = NULL;
    *method_len = 0;
    *path = NULL;
    *path_len = 0;
    *minor_version = -1;
    *num_headers = 0;

    if ((buf = parse_request(buf, buf_end, method, method_len, path, path_len, minor_version, headers, num_headers, max_headers, &r, mrr)) == NULL) {
        return r;
    }
    //cycles = rdtsc() - cycles;
    //printf("Cycles spent is %d\n", (unsigned)cycles);

    return (int)(buf - buf_start);
}


#undef CHECK_END
#undef EXPECT_CHAR
