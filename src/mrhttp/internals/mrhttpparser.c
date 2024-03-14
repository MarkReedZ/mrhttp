
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

#define ADVANCE_TOKEN(tok, toklen)                                                                                                 \
    do {                                                                                                                           \
        const char *tok_start = buf;                                                                                               \
        static const char ALIGNED(16) ranges2[] = "\000\042\177\177";                                                              \
        int found2;                                                                                                                \
        buf = findchar(buf, buf_end, ranges2, sizeof(ranges2) - 1, &found2);                                                       \
        if (!found2) {                                                                                                             \
            CHECK_END();                                                                                                           \
        } else if ( unlikely(*buf != ' ' )) {                                                                                      \
            *ret = -1;                                                                                                             \
            return NULL;                                                                                                           \
        }                                                                                                                          \
        while (1) {                                                                                                                \
            if (*buf == ' ') {                                                                                                     \
                break;                                                                                                             \
            } else if (unlikely(!IS_PRINTABLE_ASCII(*buf))) {                                                                      \
                if ((unsigned char)*buf < '\040' || *buf == '\177') {                                                              \
                    *ret = -1;                                                                                                     \
                    return NULL;                                                                                                   \
                }                                                                                                                  \
            }                                                                                                                      \
            ++buf;                                                                                                                 \
            CHECK_END();                                                                                                           \
        }                                                                                                                          \
        tok = tok_start;                                                                                                           \
        toklen = buf - tok_start;                                                                                                  \
    } while (0)


static const char *token_char_map = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                    "\0\1\0\1\1\1\1\1\0\0\1\1\0\1\1\0\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0"
                                    "\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\1\1"
                                    "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\1\0\1\0"
                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

static const char *findchar(const char *buf, const char *buf_end, const char *ranges, size_t ranges_size, int *found)
{
    *found = 0;
#if __SSE4_2__
    if (likely(buf_end - buf >= 16)) {
        __m128i ranges16 = _mm_loadu_si128((const __m128i *)ranges);

        size_t left = (buf_end - buf) & ~15;
        do {
            __m128i b16 = _mm_loadu_si128((const __m128i *)buf);
            int r = _mm_cmpestri(ranges16, ranges_size, b16, 16, _SIDD_LEAST_SIGNIFICANT | _SIDD_CMP_RANGES | _SIDD_UBYTE_OPS);
            if (unlikely(r != 16)) {
                buf += r;
                *found = 1;
                break;
            }
            buf += 16;
            left -= 16;
        } while (likely(left != 0));
    }
#else
    /* suppress unused parameter warning */
    (void)buf_end;
    (void)ranges;
    (void)ranges_size;
#endif
    return buf;
}

static const char *get_token_to_eol(const char *buf, const char *buf_end, const char **token, size_t *token_len, int *ret)
{
    const char *token_start = buf;

#ifdef __SSE4_2__
    static const char ranges1[] = "\0\010"
                                  /* allow HT */
                                  "\012\037"
                                  /* allow SP and up to but not including DEL */
                                  "\177\177"
        /* allow chars w. MSB set */
        ;
    int found;
    buf = findchar(buf, buf_end, ranges1, sizeof(ranges1) - 1, &found);
    if (found)
        goto FOUND_CTL;
#else
    /* find non-printable char within the next 8 bytes, this is the hottest code; manually inlined */
    while (likely(buf_end - buf >= 8)) {
#define DOIT()                                                                                                                     \
    do {                                                                                                                           \
        if (unlikely(!IS_PRINTABLE_ASCII(*buf)))                                                                                   \
            goto NonPrintable;                                                                                                     \
        ++buf;                                                                                                                     \
    } while (0)
        DOIT();
        DOIT();
        DOIT();
        DOIT();
        DOIT();
        DOIT();
        DOIT();
        DOIT();
#undef DOIT
        continue;
    NonPrintable:
        if ((likely((unsigned char)*buf < '\040') && likely(*buf != '\011')) || unlikely(*buf == '\177')) {
            goto FOUND_CTL;
        }
        ++buf;
    }
#endif
    for (;; ++buf) {
        CHECK_END();
        if (unlikely(!IS_PRINTABLE_ASCII(*buf))) {
            if ((likely((unsigned char)*buf < '\040') && likely(*buf != '\011')) || unlikely(*buf == '\177')) {
                goto FOUND_CTL;
            }
        }
    }
FOUND_CTL:
    if (likely(*buf == '\015')) {
        ++buf;
        EXPECT_CHAR('\012');
        *token_len = buf - 2 - token_start;
    } else if (*buf == '\012') {
        *token_len = buf - token_start;
        ++buf;
    } else {
        *ret = -1;
        return NULL;
    }
    *token = token_start;

    return buf;
}

static const char *is_complete(const char *buf, const char *buf_end, size_t last_len, int *ret)
{
    int ret_cnt = 0;
    buf = last_len < 3 ? buf : buf + last_len - 3;

    while (1) {
        CHECK_END();
        if (*buf == '\015') {
            ++buf;
            CHECK_END();
            EXPECT_CHAR('\012');
            ++ret_cnt;
        } else if (*buf == '\012') {
            ++buf;
            ++ret_cnt;
        } else {
            ++buf;
            ret_cnt = 0;
        }
        if (ret_cnt == 2) {
            return buf;
        }
    }

    *ret = -2;
    return NULL;
}

#define PARSE_INT(valp_, mul_)                                                                                                     \
    if (*buf < '0' || '9' < *buf) {                                                                                                \
        buf++;                                                                                                                     \
        *ret = -1;                                                                                                                 \
        return NULL;                                                                                                               \
    }                                                                                                                              \
    *(valp_) = (mul_) * (*buf++ - '0');

#define PARSE_INT_3(valp_)                                                                                                         \
    do {                                                                                                                           \
        int res_ = 0;                                                                                                              \
        PARSE_INT(&res_, 100)                                                                                                      \
        *valp_ = res_;                                                                                                             \
        PARSE_INT(&res_, 10)                                                                                                       \
        *valp_ += res_;                                                                                                            \
        PARSE_INT(&res_, 1)                                                                                                        \
        *valp_ += res_;                                                                                                            \
    } while (0)


#ifdef __AVX2__
static unsigned long TZCNT(unsigned long long in) {
  unsigned long res;
  asm("tzcnt %1, %0\n\t" : "=r"(res) : "r"(in));
  return res;
}
static const char *parse_headers_avx2(const char *buf, const char *buf_end, struct mr_header *headers, size_t *num_headers,
                                 size_t max_headers, int *ret, struct mr_request *mrr)
{
  unsigned long long msk[8];  // 1 bit for each of 512 bytes matching  : or \r

  //__m256i b0,b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15;
  __m256i b0,b1,b2,b3,b4,b5,b6,b7;

  __m256i m13 = _mm256_set1_epi8(13); // \r
  __m256i m58 = _mm256_set1_epi8(58); // :

  const char *obuf = buf;
  const char *sbuf = buf;

  int i;  // msk[i] 
  int t;
  unsigned int s = 0;
  int name_or_value = 0;

  const char *block_start = obuf;

av_new512:
  i = 0;
  buf = obuf;

  b0 = _mm256_loadu_si256((const __m256i *) (buf + 32*0)); // buf[0]
  b1 = _mm256_loadu_si256((const __m256i *) (buf + 32*1)); // buf[32]
  b2 = _mm256_loadu_si256((const __m256i *) (buf + 32*2)); // buf[64]
  b3 = _mm256_loadu_si256((const __m256i *) (buf + 32*3)); // buf[96]
  b4 = _mm256_loadu_si256((const __m256i *) (buf + 32*4)); // buf[128]
  b5 = _mm256_loadu_si256((const __m256i *) (buf + 32*5));
  b6 = _mm256_loadu_si256((const __m256i *) (buf + 32*6));
  b7 = _mm256_loadu_si256((const __m256i *) (buf + 32*7)); // 256 bytes

  msk[0] = (unsigned int)_mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b0, m13), _mm256_cmpeq_epi8(b0, m58) ) )  |
        ((unsigned long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b1, m13), _mm256_cmpeq_epi8(b1, m58) ) ) << 32);
  msk[1] = (unsigned int)_mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b2, m13), _mm256_cmpeq_epi8(b2, m58) ) )  |
        ((unsigned long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b3, m13), _mm256_cmpeq_epi8(b3, m58) ) ) << 32);
  msk[2] = (unsigned int)_mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b4, m13), _mm256_cmpeq_epi8(b4, m58) ) )  |
        ((unsigned long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b5, m13), _mm256_cmpeq_epi8(b5, m58) ) ) << 32);
  msk[3] = (unsigned int)_mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b6, m13), _mm256_cmpeq_epi8(b6, m58) ) )  |
        ((unsigned long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b7, m13), _mm256_cmpeq_epi8(b7, m58) ) ) << 32);

  b0 = _mm256_loadu_si256((const __m256i *) (buf + 32*8));
  b1 = _mm256_loadu_si256((const __m256i *) (buf + 32*9));
  b2 = _mm256_loadu_si256((const __m256i *) (buf + 32*10));
  b3 = _mm256_loadu_si256((const __m256i *) (buf + 32*11));
  b4 = _mm256_loadu_si256((const __m256i *) (buf + 32*12));
  b5 = _mm256_loadu_si256((const __m256i *) (buf + 32*13));
  b6 = _mm256_loadu_si256((const __m256i *) (buf + 32*14));
  b7 = _mm256_loadu_si256((const __m256i *) (buf + 32*15));

  msk[4] = (unsigned int)_mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b0, m13), _mm256_cmpeq_epi8(b0, m58) ) )  ^
        ((unsigned long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b1, m13), _mm256_cmpeq_epi8(b1, m58) ) ) << 32);
  msk[5] = (unsigned int)_mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b2, m13), _mm256_cmpeq_epi8(b2, m58) ) )  ^
        ((unsigned long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b3, m13), _mm256_cmpeq_epi8(b3, m58) ) ) << 32);
  msk[6] = (unsigned int)_mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b4, m13), _mm256_cmpeq_epi8(b4, m58) ) )  ^
        ((unsigned long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b5, m13), _mm256_cmpeq_epi8(b5, m58) ) ) << 32);
  msk[7] = (unsigned int)_mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b6, m13), _mm256_cmpeq_epi8(b6, m58) ) )  ^
        ((unsigned long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b7, m13), _mm256_cmpeq_epi8(b7, m58) ) ) << 32);


  // "Host: server\r\n"
  do {

    block_start = obuf+64*i;

    while(1) {
      s = buf-block_start;
      t = TZCNT((msk[i]>>s));
      if ( t < 64 ) {
        buf += t;
        if ( name_or_value == 1 ) {
          if ( *buf == ':' ) { buf += 1; continue; } // : in value field
          headers[*num_headers].value = sbuf;
          headers[*num_headers].value_len = buf-sbuf;
          ++*num_headers;
          if (*num_headers >= max_headers) { printf("DELME hdr too many\n"); *ret = -1; return NULL; }
          name_or_value = 0;
          buf += 2; if ( *buf == '\r' ) { goto av_done; } // \r\n\r\n marks the end
        } else {
          headers[*num_headers].name = sbuf;
          headers[*num_headers].name_len = buf-sbuf;
          name_or_value = 1;
          buf += 2;
        }
        sbuf = buf;
        if ( (buf-block_start)> 64 ) break; // TODO?
      } else {
        buf = block_start + 64;
        break;
      }

    }

    i+=1;
    if ( buf[0] == '\r' ) goto av_done;
  } while ( i < 8 && buf[0] != '\r' );

  obuf += 512;
  goto av_new512;
av_done:
  buf += 2;
  *ret = 0;
  return buf;
}



static const char *parse_headers_avx2_old(const char *buf, const char *buf_end, struct mr_header *headers, size_t *num_headers,
                                 size_t max_headers, int *ret, struct mr_request *mrr)
{
  unsigned long msk;
  int i=0,tz; // 32B index
  int shifted;
  const char *sbuf = buf;
  const char *obuf = buf;
  int name_or_value = 0;

  __m256i m13 = _mm256_set1_epi8(13); // \r
  __m256i m58 = _mm256_set1_epi8(58); // :

  do {
    const char *block_start = obuf+32*i; i += 1;
    if ( block_start > buf_end ) { printf("DELME hdr too big\n"); *ret = -1; return NULL; }
    __m256i b0 = _mm256_loadu_si256((const __m256i *) block_start);
    msk = _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b0, m13), _mm256_cmpeq_epi8(b0, m58) ) );

    while (1) {
    
    // "Host: server\r\n"
    // Headers end on \r\n\r\n
      shifted = buf-block_start;
      tz = TZCNT((msk >> shifted));
      if ( tz < 32 ) {
        buf += tz;

        if ( name_or_value == 1 ) {
          if ( *buf == ':' ) { buf += 1; continue; } // : in value field
          headers[*num_headers].value = sbuf;
          headers[*num_headers].value_len = buf-sbuf;
          ++*num_headers;
          if (*num_headers >= max_headers) { printf("DELME hdr too many\n"); *ret = -1; return NULL; }
          name_or_value = 0;
          buf += 2; if ( *buf == '\r' ) { break; } // \r\n\r\n marks the end
        } else {
          headers[*num_headers].name = sbuf;
          headers[*num_headers].name_len = buf-sbuf;
          name_or_value = 1;
          buf += 2;
        }
        sbuf = buf;
      } else {
        buf += 32 - shifted;
        break;
      }

    }
  } while ( *buf != '\r' );
  buf += 2;
  *ret = 0;
  return buf;
}
#endif

static const char *parse_headers(const char *buf, const char *buf_end, struct mr_header *headers, size_t *num_headers,
                                 size_t max_headers, int *ret, struct mr_request *mrr)
{
    if ( buf_end <= buf ) {
      *ret = -2;
      return NULL;
    }
    for (;; ++*num_headers) {
        CHECK_END();
        if (*buf == '\015') {
            ++buf;
            EXPECT_CHAR('\012');
            break;
        } else if (*buf == '\012') {
            ++buf;
            break;
        }
        if (*num_headers == max_headers) {
            *ret = -1;
            return NULL;
        }
        //printf(">%.*s<", 10, buf);
        // Listed small to larger - probably best as most used TODO check bounds
        switch ( TOLC(*buf) ) {
          case 'h': // Host
            headers[*num_headers].name = buf;
            headers[*num_headers].name_len = 4;
            buf += 6;
            goto hvalue;
          case 'c': 
            if ( buf[6] == ':' ) { // Cookie:
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 6;
              buf += 8;
              goto hvalue;
            } 
            if ( buf[10] == ':' ) { // Connection: 
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 10;
              buf += 12;
              goto hvalue;
            }
            if ( buf[11] == ':' ) { // Content-MD5: 
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 11;
              buf += 13;
              goto hvalue;
            }
            if ( buf[12] == ':' ) { // Content-Type: 
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 12;
              buf += 14;
              //goto hvalue;
              if ( buf[0] == 'a' && buf[13] == 'r' ) { //"application/mrpacker"
                mrr->flags = 2;
              } 
              buf = get_token_to_eol(buf, buf_end, &headers[*num_headers].value, &headers[*num_headers].value_len, ret); 
              goto skipvalue;
            }
            if ( buf[13] == ':' ) { // Cache-Control:
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 13;
              buf += 15;
              goto hvalue;
            }
            if ( buf[14] == ':' ) { // Content-Length:   
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 14;
              buf += 16;
              goto hvalue;
            }
            if ( buf[16] == ':' ) { // CF-Connecting-IP
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 16;
              buf += 18;
              mrr->ip = buf;
              buf = get_token_to_eol(buf, buf_end, &headers[*num_headers].value, &headers[*num_headers].value_len, ret); 
              mrr->ip_len = headers[*num_headers].value_len;
              goto skipvalue;
            }
            break;
            //printf( "%.*s\n" , 10, buf);
            //printf( "Host: %08x == %08x\n" , CHAR4_TO_INT('o', 's', 't',':'), *((unsigned int *)(buf+1)));
          case 'd':
            if ( buf[4] == ':' ) { // Date:
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 4;
              buf += 6;
              goto hvalue;
            }
            if ( buf[3] == ':' ) { // DNT:       
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 3;
              buf += 5;
              goto hvalue;
            }
            break;
          case 'x':
            if ( buf[9] == ':' ) { // X-Real-IP
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 9;
              buf += 11;
              mrr->ip = buf;
              buf = get_token_to_eol(buf, buf_end, &headers[*num_headers].value, &headers[*num_headers].value_len, ret); 
              mrr->ip_len = headers[*num_headers].value_len;
              goto skipvalue;
            }
            if ( buf[15] == ':' ) { // X-Forwarded-For:       
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 15;
              buf += 17;
              mrr->ip = buf;
              buf = get_token_to_eol(buf, buf_end, &headers[*num_headers].value, &headers[*num_headers].value_len, ret); 
              mrr->ip_len = headers[*num_headers].value_len;
              goto skipvalue;
              //goto hvalue;
            }
            if ( buf[16] == ':' ) { // X-Forwarded-Host:       
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 16;
              buf += 18;
              goto hvalue;
            }
            break;
          case 'f':
            if ( buf[5] == ':' ) { // From:
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 5;
              buf += 7;
              goto hvalue;
            }
            if ( buf[9] == ':' ) { // Forwarded:     
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 9;
              buf += 11;
              goto hvalue;
            }
            break;
          case 'i': 
            if ( buf[13] == ':' ) { // If-None-Match:  
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 13;
              buf += 15;
              goto hvalue;
            }
            if ( buf[17] == ':' ) { // If-Modified-Since:  
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 17;
              buf += 19;
              goto hvalue;
            }
            break;
          case 'o':
            headers[*num_headers].name = buf;
            headers[*num_headers].name_len = 6;
            buf += 8;
            goto hvalue;
          case 'r':
            headers[*num_headers].name = buf;
            headers[*num_headers].name_len = 7;
            buf += 9;
            goto hvalue;
          case 't': // Transfer-Encoding:
            headers[*num_headers].name = buf;
            headers[*num_headers].name_len = 17;
            buf += 19;
            goto hvalue;
          case 'u':
            if ( buf[10] == ':' ) { // User-Agent:     
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 10;
              buf += 12;
              goto hvalue;
            }
            if ( buf[25] == ':' ) { // Upgrade-Insecure-Requests:     
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 25;
              buf += 27;
              goto hvalue;
            }
            break;
          case 'a':
            if ( buf[6] == ':' ) { // Accept: 
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 6;
              buf += 8;
              goto hvalue;
            }
            if ( buf[13] == ':' ) { // Authorization:   
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 13;
              buf += 15;
              goto hvalue;
            }
            if ( buf[14] == ':' ) { // Accept-Charset:           
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 14;
              buf += 16;
              goto hvalue;
            }
            if ( buf[15] == ':' ) { // Accept-Encoding: -Datetime
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 15;
              buf += 17;
              goto hvalue;
            }
            if ( buf[16] == ':' ) { // Accept-Language:
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 15;
              buf += 17;
              goto hvalue;
            }
            if ( buf[29] == ':' ) { // Access-Control-Request-Method:     
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 29;
              buf += 31;
              goto hvalue;
            }
            if ( buf[30] == ':' ) { // Access-Control-Request-Headers:     
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 30;
              buf += 32;
              goto hvalue;
            }
            break;

        }
        if (!(*num_headers != 0 && (*buf == ' ' || *buf == '\t'))) {
            /* parsing name, but do not discard SP before colon, see
             * http://www.mozilla.org/security/announce/2006/mfsa2006-33.html */
            headers[*num_headers].name = buf;
            static const char ALIGNED(16) ranges1[] = "\x00 "  /* control chars and up to SP */
                                                      "\"\""   /* 0x22 */
                                                      "()"     /* 0x28,0x29 */
                                                      ",,"     /* 0x2c */
                                                      "//"     /* 0x2f */
                                                      ":@"     /* 0x3a-0x40 */
                                                      "[]"     /* 0x5b-0x5d */
                                                      "{\377"; /* 0x7b-0xff */
            int found;
            buf = findchar(buf, buf_end, ranges1, sizeof(ranges1) - 1, &found);
            if (!found) {
                CHECK_END();
            }
            while (1) {
                if (*buf == ':') {
                    break;
                } else if (!token_char_map[(unsigned char)*buf]) {
                    *ret = -1;
                    return NULL;
                }
                ++buf;
                CHECK_END();
            }
            if ((headers[*num_headers].name_len = buf - headers[*num_headers].name) == 0) {
                *ret = -1;
                return NULL;
            }
            ++buf;
            for (;; ++buf) {
                CHECK_END();
                if (!(*buf == ' ' || *buf == '\t')) {
                    break;
                }
            }
        } else {
            headers[*num_headers].name = NULL;
            headers[*num_headers].name_len = 0;
        }
hvalue:
        if ((buf = get_token_to_eol(buf, buf_end, &headers[*num_headers].value, &headers[*num_headers].value_len, ret)) == NULL) {
            return NULL;
        }
skipvalue:
      ;
    }
    return buf;
}
//#endif


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
    //ADVANCE_TOKEN(*method, *method_len);
    // TODO Support other methods
    switch (*(unsigned int *)buf) {
      case CHAR4_TO_INT('G', 'E', 'T', ' '):
        *method = buf; *method_len = 3; buf += 4; break;
      case CHAR4_TO_INT('P', 'O', 'S', 'T'):
        *method = buf; *method_len = 4; buf += 5; break;
      default:
        *ret = -2;
        return NULL;
    }
    ADVANCE_TOKEN(*path, *path_len);
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
#ifdef __AVX2__
    return parse_headers_avx2(buf, buf_end, headers, num_headers, max_headers, ret, mrr);
#else
    return parse_headers(buf, buf_end, headers, num_headers, max_headers, ret, mrr);
#endif
}

static __inline__ unsigned long long rdtsc(void)
{
  unsigned long long int x;
     __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
     return x;
}

int mr_parse_request(const char *buf_start, size_t len, const char **method, size_t *method_len, const char **path,
                      size_t *path_len, int *minor_version, struct mr_header *headers, size_t *num_headers, size_t last_len,struct mr_request *mrr)
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

    /* if last_len != 0, check if the request is complete (a fast countermeasure
       againt slowloris */
    if (last_len != 0 && is_complete(buf, buf_end, last_len, &r) == NULL) {
        return r;
    }

    if ((buf = parse_request(buf, buf_end, method, method_len, path, path_len, minor_version, headers, num_headers, max_headers, &r, mrr)) == NULL) {
        return r;
    }
    //cycles = rdtsc() - cycles;
    //printf("Cycles spent is %d\n", (unsigned)cycles);

    return (int)(buf - buf_start);
}


#undef CHECK_END
#undef EXPECT_CHAR
#undef ADVANCE_TOKEN
