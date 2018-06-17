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
#include <string.h>
#ifdef __AVX2__
#include <immintrin.h>
#elif defined __SSE4_2__
#ifdef _MSC_VER
#include <nmmintrin.h>
#else
#include <x86intrin.h>
#endif
#endif
#include "mrhttpparser.h"

/* $Id: a707070d11d499609f99d09f97535642cec910a8 $ */

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

#define TFW_LC_INT  0x20202020
#define TFW_LC_LONG 0x2020202020202020UL
#define TFW_CHAR4_INT(a, b, c, d)         \
   (unsigned int)((d << 24) | (c << 16) | (b << 8) | a)
#define TFW_CHAR8_INT(a, b, c, d, e, f, g, h)       \
   (((long)h << 56) | ((long)g << 48) | ((long)f << 40)   \
    | ((long)e << 32) | (d << 24) | (c << 16) | (b << 8) | a)
#define TFW_P2LCINT(p)  ((*(unsigned int *)(p)) | TFW_LC_INT)
/*
 * Match 4 or 8 characters with conversion to lower case
 * and type conversion to int or long type.
 */
#define C4_INT_LCM(p, a, b, c, d)         \
   !((*(unsigned int *)(p) | TFW_LC_INT) ^ TFW_CHAR4_INT(a, b, c, d))
#define C8_INT_LCM(p, a, b, c, d, e, f, g, h)       \
   !((*(unsigned long *)(p) | TFW_LC_LONG)      \
     ^ TFW_CHAR8_INT(a, b, c, d, e, f, g, h))

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

#define ADVANCE_TOKEN(tok, toklen)                                                                                                 \
    do {                                                                                                                           \
        const char *tok_start = buf;                                                                                               \
        static const char ALIGNED(16) ranges2[] = "\000\042\177\177";                                                              \
        int found2;                                                                                                                \
        buf = findchar_fast(buf, buf_end, ranges2, sizeof(ranges2) - 1, &found2);                                                  \
        if (!found2) {                                                                                                             \
            CHECK_EOF();                                                                                                           \
        } else if ( unlikely(*buf != ' ' )) {                                                                                       \
            *ret = -1;                                                                                                                \
            return NULL;                                                                                                                \
        }                                                                                                                \
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
            CHECK_EOF();                                                                                                           \
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

static const char *findchar_fast(const char *buf, const char *buf_end, const char *ranges, size_t ranges_size, int *found)
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
    buf = findchar_fast(buf, buf_end, ranges1, sizeof(ranges1) - 1, &found);
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
        CHECK_EOF();
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
        CHECK_EOF();
        if (*buf == '\015') {
            ++buf;
            CHECK_EOF();
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


static const char *parse_headers(const char *buf, const char *buf_end, struct mr_header *headers, size_t *num_headers,
                                 size_t max_headers, int *ret)
{
    if ( buf_end <= buf ) {
      *ret = -2;
      return NULL;
    }
    for (;; ++*num_headers) {
        CHECK_EOF();
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
          case 'c': 
            if ( buf[6] == ':' && TFW_CHAR4_INT('o', 'o', 'k','i') == *((unsigned int *)(buf+1)) ) { // Cookie:
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
              goto hvalue;
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
            break;
            //printf( "%.*s\n" , 10, buf);
            //printf( "Host: %08x == %08x\n" , TFW_CHAR4_INT('o', 's', 't',':'), *((unsigned int *)(buf+1)));
          case 'd':
            if ( buf[3] == ':' ) { // DNT:       
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 3;
              buf += 5;
              goto hvalue;
            }
            if ( buf[4] == ':' ) { // Date:
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 4;
              buf += 6;
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
          case 'h': // Host
            if ( TFW_CHAR4_INT('o', 's', 't',':') == *((unsigned int *)(buf+1)) ) {
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 4;
              buf += 6;
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
            if ( buf[6] == ':' ) { // Origin:       
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 6;
              buf += 8;
              goto hvalue;
            }
            break;
          case 'r':
            if ( buf[7] == ':' ) { // referer
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 7;
              buf += 9;
              goto hvalue;
            }
            break;
          case 't': // Transfer-Encoding:
            if ( buf[8] == '-' && buf[17] == ':' ) {
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 17;
              buf += 19;
              goto hvalue;
            }
            break;
          case 'u':
            if ( buf[5] == '-' && buf[10] == ':' ) { // User-Agent:     
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
          case 'x':
            if ( buf[1] == '-' && buf[15] == ':' ) { // X-Forwarded-For:       
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 15;
              buf += 17;
              goto hvalue;
            }
            if ( buf[1] == '-' && buf[16] == ':' ) { // X-Forwarded-Host:       
              headers[*num_headers].name = buf;
              headers[*num_headers].name_len = 16;
              buf += 18;
              goto hvalue;
            }
            break;
          case 'a':
            //printf("?? %c\n", buf[13] );
            //printf(">%.*s<", 16, buf);
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
            buf = findchar_fast(buf, buf_end, ranges1, sizeof(ranges1) - 1, &found);
            if (!found) {
                CHECK_EOF();
            }
            while (1) {
                if (*buf == ':') {
                    break;
                } else if (!token_char_map[(unsigned char)*buf]) {
                    *ret = -1;
                    return NULL;
                }
                ++buf;
                CHECK_EOF();
            }
            if ((headers[*num_headers].name_len = buf - headers[*num_headers].name) == 0) {
                *ret = -1;
                return NULL;
            }
            ++buf;
            for (;; ++buf) {
                CHECK_EOF();
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
    }
    return buf;
}
//#endif


static const char *parse_request(const char *buf, const char *buf_end, const char **method, size_t *method_len, const char **path,
                                 size_t *path_len, int *minor_version, struct mr_header *headers, size_t *num_headers,
                                 size_t max_headers, int *ret)
{
    /* skip first empty line (some clients add CRLF after POST content) */
    CHECK_EOF();
    if (*buf == '\015') {
        ++buf;
        EXPECT_CHAR('\012');
    } else if (*buf == '\012') {
        ++buf;
    }

    /* parse request line */
    //ADVANCE_TOKEN(*method, *method_len);
    switch (*(unsigned int *)buf) {
      case TFW_CHAR4_INT('G', 'E', 'T', ' '):
        *method = buf; *method_len = 3; buf += 4; goto uri;
      case TFW_CHAR4_INT('P', 'O', 'S', 'T'):
        *method = buf; *method_len = 4; buf += 5; goto uri;
    }
    ++buf;
uri:
    ADVANCE_TOKEN(*path, *path_len);
    ++buf;
    switch (*(unsigned long *)buf) {
      case TFW_CHAR8_INT('H', 'T', 'T', 'P','/','1','.','0'):
        *minor_version = 0; buf += 8; goto endfirst;
      case TFW_CHAR8_INT('H', 'T', 'T', 'P','/','1','.','1'):
        *minor_version = 1; buf += 8; goto endfirst;
      default:
        *ret = -2;
        return NULL;
    }
    
endfirst:
    if (*buf == '\015') {
        ++buf;
        EXPECT_CHAR('\012');
    } else if (*buf == '\012') {
        ++buf;
    } else {
        *ret = -2;
        return NULL;
    }

    return parse_headers(buf, buf_end, headers, num_headers, max_headers, ret);
}

static __inline__ unsigned long long rdtsc(void)
{
  unsigned long long int x;
     __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
     return x;
}

int mr_parse_request(const char *buf_start, size_t len, const char **method, size_t *method_len, const char **path,
                      size_t *path_len, int *minor_version, struct mr_header *headers, size_t *num_headers, size_t last_len)
{
    const char *buf = buf_start, *buf_end = buf_start + len;
    size_t max_headers = *num_headers;
    int r;

    //printf("1\n");
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

    if ((buf = parse_request(buf, buf_end, method, method_len, path, path_len, minor_version, headers, num_headers, max_headers,
                             &r)) == NULL) {
        return r;
    }
    //cycles = rdtsc() - cycles;
    //printf("Cycles spent is %d\n", (unsigned)cycles);

    return (int)(buf - buf_start);
}

static const char *parse_response(const char *buf, const char *buf_end, int *minor_version, int *status, const char **msg,
                                  size_t *msg_len, struct mr_header *headers, size_t *num_headers, size_t max_headers, int *ret)
{
    /* parse "HTTP/1.x" */
    if ( buf_end - buf < 16 ) {
      *ret = -2;
      return NULL;
    }
    switch (*(unsigned long *)buf) {
      case TFW_CHAR8_INT('H', 'T', 'T', 'P','/','1','.','0'):
        *minor_version = 0; buf += 8; break;
      case TFW_CHAR8_INT('H', 'T', 'T', 'P','/','1','.','1'):
        *minor_version = 1; buf += 8; break;
      default:
        *ret = -2;
        return NULL;
    }
    if ( *(unsigned long*)buf == TFW_CHAR8_INT(' ', '2', '0', '0',' ','O','K','\r') ) {
      *status = 200;
      *msg = buf+5; *msg_len = 2;
      buf += 9;
      //printf(">%.*s<", 16, buf);
      return parse_headers(buf, buf_end, headers, num_headers, max_headers, ret);
    }
    /* skip space */
    if (*buf++ != ' ') {
        *ret = -1;
        return NULL;
    }
    /* parse status code, we want at least [:digit:][:digit:][:digit:]<other char> to try to parse */
    if (buf_end - buf < 4) {
        *ret = -2;
        return NULL;
    }
    PARSE_INT_3(status);

    /* skip space */
    if (*buf++ != ' ') {
        *ret = -1;
        return NULL;
    }
    /* get message */
    if ((buf = get_token_to_eol(buf, buf_end, msg, msg_len, ret)) == NULL) {
        return NULL;
    }

    return parse_headers(buf, buf_end, headers, num_headers, max_headers, ret);
}

int mr_parse_response(const char *buf_start, size_t len, int *minor_version, int *status, const char **msg, size_t *msg_len,
                       struct mr_header *headers, size_t *num_headers, size_t last_len)
{
    const char *buf = buf_start, *buf_end = buf + len;
    size_t max_headers = *num_headers;
    int r;

    *minor_version = -1;
    *status = 0;
    *msg = NULL;
    *msg_len = 0;
    *num_headers = 0;

    /* if last_len != 0, check if the response is complete (a fast countermeasure
       against slowloris */
    if (last_len != 0 && is_complete(buf, buf_end, last_len, &r) == NULL) {
        return r;
    }

    if ((buf = parse_response(buf, buf_end, minor_version, status, msg, msg_len, headers, num_headers, max_headers, &r)) == NULL) {
        return r;
    }

    return (int)(buf - buf_start);
}

int mr_parse_headers(const char *buf_start, size_t len, struct mr_header *headers, size_t *num_headers, size_t last_len)
{
    const char *buf = buf_start, *buf_end = buf + len;
    size_t max_headers = *num_headers;
    int r;

    *num_headers = 0;

    /* if last_len != 0, check if the response is complete (a fast countermeasure
       against slowloris */
    if (last_len != 0 && is_complete(buf, buf_end, last_len, &r) == NULL) {
        return r;
    }

    if ((buf = parse_headers(buf, buf_end, headers, num_headers, max_headers, &r)) == NULL) {
        return r;
    }

    return (int)(buf - buf_start);
}

enum {
    CHUNKED_IN_CHUNK_SIZE,
    CHUNKED_IN_CHUNK_EXT,
    CHUNKED_IN_CHUNK_DATA,
    CHUNKED_IN_CHUNK_CRLF,
    CHUNKED_IN_TRAILERS_LINE_HEAD,
    CHUNKED_IN_TRAILERS_LINE_MIDDLE
};

static int decode_hex(int ch)
{
    if ('0' <= ch && ch <= '9') {
        return ch - '0';
    } else if ('A' <= ch && ch <= 'F') {
        return ch - 'A' + 0xa;
    } else if ('a' <= ch && ch <= 'f') {
        return ch - 'a' + 0xa;
    } else {
        return -1;
    }
}

ssize_t mr_decode_chunked(struct mr_chunked_decoder *decoder, char *buf, size_t *_bufsz)
{
    size_t dst = 0, src = 0, bufsz = *_bufsz;
    ssize_t ret = -2; /* incomplete */

    while (1) {
        switch (decoder->_state) {
        case CHUNKED_IN_CHUNK_SIZE:
            for (;; ++src) {
                int v;
                if (src == bufsz)
                    goto Exit;
                if ((v = decode_hex(buf[src])) == -1) {
                    if (decoder->_hex_count == 0) {
                        ret = -1;
                        goto Exit;
                    }
                    break;
                }
                if (decoder->_hex_count == sizeof(size_t) * 2) {
                    ret = -1;
                    goto Exit;
                }
                decoder->bytes_left_in_chunk = decoder->bytes_left_in_chunk * 16 + v;
                ++decoder->_hex_count;
            }
            decoder->_hex_count = 0;
            decoder->_state = CHUNKED_IN_CHUNK_EXT;
        /* fallthru */
        case CHUNKED_IN_CHUNK_EXT:
            /* RFC 7230 A.2 "Line folding in chunk extensions is disallowed" */
            for (;; ++src) {
                if (src == bufsz)
                    goto Exit;
                if (buf[src] == '\012')
                    break;
            }
            ++src;
            if (decoder->bytes_left_in_chunk == 0) {
                if (decoder->consume_trailer) {
                    decoder->_state = CHUNKED_IN_TRAILERS_LINE_HEAD;
                    break;
                } else {
                    goto Complete;
                }
            }
            decoder->_state = CHUNKED_IN_CHUNK_DATA;
        /* fallthru */
        case CHUNKED_IN_CHUNK_DATA: {
            size_t avail = bufsz - src;
            if (avail < decoder->bytes_left_in_chunk) {
                if (dst != src)
                    memmove(buf + dst, buf + src, avail);
                src += avail;
                dst += avail;
                decoder->bytes_left_in_chunk -= avail;
                goto Exit;
            }
            if (dst != src)
                memmove(buf + dst, buf + src, decoder->bytes_left_in_chunk);
            src += decoder->bytes_left_in_chunk;
            dst += decoder->bytes_left_in_chunk;
            decoder->bytes_left_in_chunk = 0;
            decoder->_state = CHUNKED_IN_CHUNK_CRLF;
        }
        /* fallthru */
        case CHUNKED_IN_CHUNK_CRLF:
            for (;; ++src) {
                if (src == bufsz)
                    goto Exit;
                if (buf[src] != '\015')
                    break;
            }
            if (buf[src] != '\012') {
                ret = -1;
                goto Exit;
            }
            ++src;
            decoder->_state = CHUNKED_IN_CHUNK_SIZE;
            break;
        case CHUNKED_IN_TRAILERS_LINE_HEAD:
            for (;; ++src) {
                if (src == bufsz)
                    goto Exit;
                if (buf[src] != '\015')
                    break;
            }
            if (buf[src++] == '\012')
                goto Complete;
            decoder->_state = CHUNKED_IN_TRAILERS_LINE_MIDDLE;
        /* fallthru */
        case CHUNKED_IN_TRAILERS_LINE_MIDDLE:
            for (;; ++src) {
                if (src == bufsz)
                    goto Exit;
                if (buf[src] == '\012')
                    break;
            }
            ++src;
            decoder->_state = CHUNKED_IN_TRAILERS_LINE_HEAD;
            break;
        default:
            assert(!"decoder is corrupt");
        }
    }

Complete:
    ret = bufsz - src;
Exit:
    if (dst != src)
        memmove(buf + dst, buf + src, bufsz - src);
    *_bufsz = dst;
    return ret;
}

int mr_decode_chunked_is_in_data(struct mr_chunked_decoder *decoder)
{
    return decoder->_state == CHUNKED_IN_CHUNK_DATA;
}

#undef CHECK_EOF
#undef EXPECT_CHAR
#undef ADVANCE_TOKEN
