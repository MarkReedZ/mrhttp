
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

#define IS_PRINTABLE_ASCII(c) ((unsigned char)(c)-040u < 0137u)

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

std::size_t slow_hparse(std::string &text) noexcept
{
  std::size_t spaces = 0;
  std::string keys[1000];
  std::string vals[1000];
  int last = 0;
  int index = 0;

  for (std::uint64_t i = 0; i < text.length(); i++)
  {
    if ( text[i] == ':' ) {
        keys[index] = text.substr( last, i-last );
        last = i+1;
    }
    if ( text[i] == '\n' ) {
        vals[index] = text.substr( last, i-last );
        last += 2;
        index += 1;
    }
  }
  return spaces;
}

static const char *findchar(const char *buf, const char *buf_end, const char *ranges, size_t ranges_size, int *found)
{
    *found = 0;
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
    return buf;
}
static const char *adv_token(const char *buf, int *ret) {
        const char *tok_start = buf;                                                                                               
        const char *buf_end = buf+512;
        static const char ranges2[] = "\000\042\177\177";                                                              
        int found2;                                                                                                               
        buf = findchar(buf, buf+512, ranges2, sizeof(ranges2) - 1, &found2);                                                       
        if (!found2) {                                                                                                             
            CHECK_END();                                                                                                           
        } else if ( unlikely(*buf != ' ' )) {                                                                                      
            *ret = -1;                                                                                                             
            return NULL;                                                                                                           
        }                                                                                                                          
        while (1) {                                                                                                                
            if (*buf == ' ') {                                                                                                    
                return buf;                                                                                                             
            } else if (unlikely(!IS_PRINTABLE_ASCII(*buf))) {                                                                      
                if ((unsigned char)*buf < '\040' || *buf == '\177') {                                                          
                    *ret = -1;                                                                                                     
                    return NULL;
                }                                                                                                             
            }                                                                                                                
            ++buf;                                                                                                         
            CHECK_END();                                                                                                  
        }                                                                                                                
        *ret = buf - tok_start;                                                                                      
        return tok_start;
}

static const char *get_token_to_eol(const char *buf, const char *buf_end, int *ret)
{

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
    } else if (*buf == '\012') {
        ++buf;
    } else {
        *ret = -1;
        return NULL;
    }

    return buf;
}
static const char *my_get_eol128(const char *buf) {
  //__m128i* pSrc1 = (__m128i *)string;         // init pointer to start of string
  __m128i m0 = _mm_set1_epi8(13);              // vector of 16 `\0` characters

  while (1)
  {
    __m128i v0 = _mm_loadu_si128((const __m128i *)buf);
    __m128i v1 = _mm_cmpeq_epi8(v0, m0);    // compare all 16 chars
    unsigned int vmask = _mm_movemask_epi8(v1);      // get 16 comparison result bits
    if (vmask != 0) {
        buf += TZCNT(vmask) + 2;
        break;                              // we found a `\0`, break out of loop
    }
    buf += 16; //pSrc1++;                                // next 16 characters...
  }
  return buf;
}

 //64bits  256bits  bytes 8 * 32 
__m256i m13 = _mm256_set1_epi8(13);             
__m256i m32 = _mm256_set1_epi8(32);             
static const char *my_get_eol(const char *buf) {

  while (1)
  {
    __m256i v0 = _mm256_loadu_si256((const __m256i *)buf);
    __m256i v1 = _mm256_cmpeq_epi8(v0, m13);     
    unsigned long vmask = _mm256_movemask_epi8(v1);  
    if (vmask != 0) {
        buf += TZCNT(vmask) + 2;
        break;                             
    }
    buf += 32; //pSrc1++;                 
  }
  return buf;
}
static const char *get_to_space(const char *buf, int *len) {
  const char *orig = buf;
  while (1)
  {
    __m256i v0 = _mm256_loadu_si256((const __m256i *)buf);
    __m256i v1 = _mm256_cmpeq_epi8(v0, m32);     
    unsigned long vmask = _mm256_movemask_epi8(v1);  
    if (vmask != 0) {
        buf += TZCNT(vmask) + 1;
        break;                             
    }
    buf += 32; 
  }
  *len = buf-orig-1;
  return buf;
}



static const char *parse_headers(const char *buf, const char *buf_end, int *ret)
{
    int num_headers = 0;
    int max_headers = 20;
    if ( buf_end <= buf ) {
      *ret = -2;
      return NULL;
    }
    for (;; ++num_headers) {
        CHECK_EOF();
        if (*buf == '\015') {
            ++buf;
            EXPECT_CHAR('\012');
            break;
        } else if (*buf == '\012') {
            ++buf;
            break;
        }
        if (num_headers == max_headers) {
            *ret = -1;
            return NULL;
        }
        //printf(">%.*s<", 10, buf);
        // Listed small to larger - probably best as most used TODO check bounds
        switch ( TOLC(*buf) ) {
          case 'h': // Host
            ////headers[*num_headers].name = buf;
            //headers[*num_headers].name_len = 4;
            buf += 6;
            goto hvalue;
          case 'c': 
            if ( buf[6] == ':' ) { // Cookie:
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 6;
              buf += 8;
              goto hvalue;
            } 
            if ( buf[10] == ':' ) { // Connection: 
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 10;
              buf += 12;
              goto hvalue;
            }
            if ( buf[11] == ':' ) { // Content-MD5: 
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 11;
              buf += 13;
              goto hvalue;
            }
            if ( buf[12] == ':' ) { // Content-Type: 
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 12;
              buf += 14;
              //goto hvalue;
              //if ( buf[0] == 'a' && buf[13] == 'r' ) { //"application/mrpacker"
                //mrr->flags = 2;
              //} 
              //buf = get_token_to_eol(buf, buf_end, ret); 
              buf = my_get_eol(buf);
              goto skipvalue;
            }
            if ( buf[13] == ':' ) { // Cache-Control:
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 13;
              buf += 15;
              goto hvalue;
            }
            if ( buf[14] == ':' ) { // Content-Length:   
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 14;
              buf += 16;
              goto hvalue;
            }
            if ( buf[16] == ':' ) { // CF-Connecting-IP
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 16;
              buf += 18;
              //mrr->ip = buf;
              //buf = get_token_to_eol(buf, buf_end, ret); 
              buf = my_get_eol(buf);
              //mrr->ip_len = headers[*num_headers].value_len;
              goto skipvalue;
            }
            break;
            //printf( "%.*s\n" , 10, buf);
            //printf( "Host: %08x == %08x\n" , MR_CHAR4_INT('o', 's', 't',':'), *((unsigned int *)(buf+1)));
          case 'd':
            if ( buf[4] == ':' ) { // Date:
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 4;
              buf += 6;
              goto hvalue;
            }
            if ( buf[3] == ':' ) { // DNT:       
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 3;
              buf += 5;
              goto hvalue;
            }
            break;
          case 'x':
            if ( buf[9] == ':' ) { // X-Real-IP
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 9;
              buf += 11;
              //mrr->ip = buf;
              //buf = get_token_to_eol(buf, buf_end, ret); 
              buf = my_get_eol(buf);
              //mrr->ip_len = headers[*num_headers].value_len;
              goto skipvalue;
            }
            if ( buf[15] == ':' ) { // X-Forwarded-For:       
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 15;
              buf += 17;
              //mrr->ip = buf;
              buf = get_token_to_eol(buf, buf_end, ret); 
              //mrr->ip_len = headers[*num_headers].value_len;
              goto skipvalue;
              //goto hvalue;
            }
            if ( buf[16] == ':' ) { // X-Forwarded-Host:       
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 16;
              buf += 18;
              goto hvalue;
            }
            break;
          case 'f':
            if ( buf[5] == ':' ) { // From:
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 5;
              buf += 7;
              goto hvalue;
            }
            if ( buf[9] == ':' ) { // Forwarded:     
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 9;
              buf += 11;
              goto hvalue;
            }
            break;
          case 'i': 
            if ( buf[13] == ':' ) { // If-None-Match:  
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 13;
              buf += 15;
              goto hvalue;
            }
            if ( buf[17] == ':' ) { // If-Modified-Since:  
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 17;
              buf += 19;
              goto hvalue;
            }
            break;
          case 'o':
            //headers[*num_headers].name = buf;
            //headers[*num_headers].name_len = 6;
            buf += 8;
            goto hvalue;
          case 'r':
            //headers[*num_headers].name = buf;
            //headers[*num_headers].name_len = 7;
            buf += 9;
            goto hvalue;
          case 't': // Transfer-Encoding:
            //headers[*num_headers].name = buf;
            //headers[*num_headers].name_len = 17;
            buf += 19;
            goto hvalue;
          case 'u':
            if ( buf[10] == ':' ) { // User-Agent:     
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 10;
              buf += 12;
              goto hvalue;
            }
            if ( buf[25] == ':' ) { // Upgrade-Insecure-Requests:     
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 25;
              buf += 27;
              goto hvalue;
            }
            break;
          case 'a':
            if ( buf[6] == ':' ) { // Accept: 
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 6;
              buf += 8;
              goto hvalue;
            }
            if ( buf[13] == ':' ) { // Authorization:   
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 13;
              buf += 15;
              goto hvalue;
            }
            if ( buf[14] == ':' ) { // Accept-Charset:           
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 14;
              buf += 16;
              goto hvalue;
            }
            if ( buf[15] == ':' ) { // Accept-Encoding: -Datetime
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 15;
              buf += 17;
              goto hvalue;
            }
            if ( buf[16] == ':' ) { // Accept-Language:
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 15;
              buf += 17;
              goto hvalue;
            }
            if ( buf[29] == ':' ) { // Access-Control-Request-Method:     
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 29;
              buf += 31;
              goto hvalue;
            }
            if ( buf[30] == ':' ) { // Access-Control-Request-Headers:     
              //headers[*num_headers].name = buf;
              //headers[*num_headers].name_len = 30;
              buf += 32;
              goto hvalue;
            }
            break;

        }
        if (!(num_headers != 0 && (*buf == ' ' || *buf == '\t'))) {
            /* parsing name, but do not discard SP before colon, see
             * http://www.mozilla.org/security/announce/2006/mfsa2006-33.html */
            //headers[*num_headers].name = buf;
            static const char ranges1[] = "\x00 "  /* control chars and up to SP */
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
            //if ((headers[*num_headers].name_len = buf - headers[*num_headers].name) == 0) {
                //*ret = -1;
                //return NULL;
            //}
            ++buf;
            for (;; ++buf) {
                CHECK_EOF();
                if (!(*buf == ' ' || *buf == '\t')) {
                    break;
                }
            }
        } else {
            //headers[*num_headers].name = NULL;
            //headers[*num_headers].name_len = 0;
        }
hvalue:
        //if ((buf = get_token_to_eol(buf, buf_end, ret)) == NULL) {
        if ((buf = my_get_eol(buf)) == NULL) {
            return NULL;
        }
skipvalue:
      ;
    }
    return buf;
}

static void find_ranges32(__m256i b0, unsigned long *range0, unsigned long *range1) {
  const __m256i rr0 = _mm256_set1_epi8(0x00 - 1);
  const __m256i rr1 = _mm256_set1_epi8(0x1f + 1);
  const __m256i rr2 = _mm256_set1_epi8(0x3a);
  const __m256i rr4 = _mm256_set1_epi8(0x7f);
  const __m256i rr7 = _mm256_set1_epi8(0x09);

  /* 0<=x */
  __m256i gz0 = _mm256_cmpgt_epi8(b0, rr0);
  /* 0=<x<=1f */
  __m256i z_1f_0 = _mm256_and_si256(_mm256_cmpgt_epi8(rr1, b0), gz0);
  /* 0<=x<=1f || x==3a */
  __m256i range0_0 = _mm256_or_si256(_mm256_cmpeq_epi8(rr2, b0), z_1f_0);
  /* 0<=x<9 || 9<x<=1f || x==7f */
  __m256i range1_0 = _mm256_or_si256(_mm256_cmpeq_epi8(rr4, b0), _mm256_andnot_si256(_mm256_cmpeq_epi8(b0, rr7), z_1f_0));
  /* Generate bit masks */
  unsigned int r0 = _mm256_movemask_epi8(range0_0);
  /* Combine 32bit masks into a single 64bit mask */
  *range0 = r0;
  r0 = _mm256_movemask_epi8(range1_0);
  *range1 = r0;
}

/* Parse only 64 bytes */
static void find_ranges64(__m256i b0, __m256i b1, unsigned long *range0, unsigned long *range1) {
  const __m256i rr0 = _mm256_set1_epi8(0x00 - 1);
  const __m256i rr1 = _mm256_set1_epi8(0x1f + 1);
  const __m256i rr2 = _mm256_set1_epi8(0x3a);
  const __m256i rr4 = _mm256_set1_epi8(0x7f);
  const __m256i rr7 = _mm256_set1_epi8(0x09);

  /* 0<=x */
  __m256i gz0 = _mm256_cmpgt_epi8(b0, rr0);
  __m256i gz1 = _mm256_cmpgt_epi8(b1, rr0);
  /* 0=<x<=1f */
  __m256i z_1f_0 = _mm256_and_si256(_mm256_cmpgt_epi8(rr1, b0), gz0);
  __m256i z_1f_1 = _mm256_and_si256(_mm256_cmpgt_epi8(rr1, b1), gz1);
  /* 0<=x<=1f || x==3a */
  __m256i range0_0 = _mm256_or_si256(_mm256_cmpeq_epi8(rr2, b0), z_1f_0);
  __m256i range0_1 = _mm256_or_si256(_mm256_cmpeq_epi8(rr2, b1), z_1f_1);
  /* 0<=x<9 || 9<x<=1f || x==7f */
  __m256i range1_0 = _mm256_or_si256(_mm256_cmpeq_epi8(rr4, b0), _mm256_andnot_si256(_mm256_cmpeq_epi8(b0, rr7), z_1f_0));
  __m256i range1_1 = _mm256_or_si256(_mm256_cmpeq_epi8(rr4, b1), _mm256_andnot_si256(_mm256_cmpeq_epi8(b1, rr7), z_1f_1));
  /* Generate bit masks */
  unsigned int r0 = _mm256_movemask_epi8(range0_0);
  unsigned int r1 = _mm256_movemask_epi8(range0_1);
  /* Combine 32bit masks into a single 64bit mask */
  *range0 = r0 ^ ((unsigned long)r1 << 32);
  r0 = _mm256_movemask_epi8(range1_0);
  r1 = _mm256_movemask_epi8(range1_1);
  *range1 = r0 ^ ((unsigned long)r1 << 32);     
}

/* This function parses 128 bytes at a time, creating bitmap of all interesting tokens */
static void find_ranges(const char* buf, const char* buf_end, unsigned long *range0, unsigned long *range1) {
  const __m256i rr0 = _mm256_set1_epi8(0x00 - 1);
  const __m256i rr1 = _mm256_set1_epi8(0x1f + 1);
  const __m256i rr2 = _mm256_set1_epi8(0x3a);
  const __m256i rr4 = _mm256_set1_epi8(0x7f);
  const __m256i rr7 = _mm256_set1_epi8(0x09);

  __m256i b0, b1, b2, b3;
  unsigned char tmpbuf[32];
  int i;
  int dist;

  if((dist = buf_end - buf) < 128) {
    //memcpy(tmpbuf, buf + (dist & (-32)), dist & 31);
    for (i=0; i < (dist & 31); i++) tmpbuf[i] = buf[ (dist & (-32)) + i];
    if (dist >= 96) {
      b0 = _mm256_loadu_si256((const __m256i_u*) buf + 32*0);
      b1 = _mm256_loadu_si256((const __m256i_u*) buf + 32*1);
      b2 = _mm256_loadu_si256((const __m256i_u*) buf + 32*2);
      b3 = _mm256_loadu_si256((const __m256i_u*) tmpbuf);
    } else if (dist >= 64) {
      b0 = _mm256_loadu_si256((const __m256i_u*) buf + 32*0);
      b1 = _mm256_loadu_si256((const __m256i_u*) buf + 32*1);
      b2 = _mm256_loadu_si256((const __m256i_u*) tmpbuf);
      b3 = _mm256_setzero_si256();
    } else {
      if(dist < 32) {
        b0 = _mm256_loadu_si256((const __m256i_u*)tmpbuf);
        return find_ranges32(b0, range0, range1);
      } else {
        b0 = _mm256_loadu_si256((const __m256i_u*) buf + 32*0);
        b1 = _mm256_loadu_si256((const __m256i_u*)tmpbuf);
        return find_ranges64(b0, b1, range0, range1);
      }
    }
  } else {
    /* Load 128 bytes */
    b0 = _mm256_loadu_si256((const __m256i_u*) buf + 32*0);
    b1 = _mm256_loadu_si256((const __m256i_u*) buf + 32*1);
    b2 = _mm256_loadu_si256((const __m256i_u*) buf + 32*2);
    b3 = _mm256_loadu_si256((const __m256i_u*) buf + 32*3);
  }

  /* 0<=x */
  __m256i gz0 = _mm256_cmpgt_epi8(b0, rr0);
  __m256i gz1 = _mm256_cmpgt_epi8(b1, rr0);
  __m256i gz2 = _mm256_cmpgt_epi8(b2, rr0);
  __m256i gz3 = _mm256_cmpgt_epi8(b3, rr0);
  /* 0=<x<=1f */
  __m256i z_1f_0 = _mm256_and_si256(_mm256_cmpgt_epi8(rr1, b0), gz0);
  __m256i z_1f_1 = _mm256_and_si256(_mm256_cmpgt_epi8(rr1, b1), gz1);
  __m256i z_1f_2 = _mm256_and_si256(_mm256_cmpgt_epi8(rr1, b2), gz2);
  __m256i z_1f_3 = _mm256_and_si256(_mm256_cmpgt_epi8(rr1, b3), gz3);
  /* 0<=x<=1f || x==3a */
  __m256i range0_0 = _mm256_or_si256(_mm256_cmpeq_epi8(rr2, b0), z_1f_0);
  __m256i range0_1 = _mm256_or_si256(_mm256_cmpeq_epi8(rr2, b1), z_1f_1);
  __m256i range0_2 = _mm256_or_si256(_mm256_cmpeq_epi8(rr2, b2), z_1f_2);
  __m256i range0_3 = _mm256_or_si256(_mm256_cmpeq_epi8(rr2, b3), z_1f_3);
  /* 0<=x<9 || 9<x<=1f || x==7f */
  __m256i range1_0 = _mm256_or_si256(_mm256_cmpeq_epi8(rr4, b0), _mm256_andnot_si256(_mm256_cmpeq_epi8(b0, rr7), z_1f_0));
  __m256i range1_1 = _mm256_or_si256(_mm256_cmpeq_epi8(rr4, b1), _mm256_andnot_si256(_mm256_cmpeq_epi8(b1, rr7), z_1f_1));
  __m256i range1_2 = _mm256_or_si256(_mm256_cmpeq_epi8(rr4, b2), _mm256_andnot_si256(_mm256_cmpeq_epi8(b2, rr7), z_1f_2));
  __m256i range1_3 = _mm256_or_si256(_mm256_cmpeq_epi8(rr4, b3), _mm256_andnot_si256(_mm256_cmpeq_epi8(b3, rr7), z_1f_3));
  /* Generate bit masks */
  unsigned int r0 = _mm256_movemask_epi8(range0_0);
  unsigned int r1 = _mm256_movemask_epi8(range0_1);
  /* Combine 32bit masks into a single 64bit mask */
  *range0 = r0 ^ ((unsigned long)r1 << 32);

  r0 = _mm256_movemask_epi8(range0_2);
  r1 = _mm256_movemask_epi8(range0_3);
  range0[1] = r0 ^ ((unsigned long)r1 << 32);

  r0 = _mm256_movemask_epi8(range1_0);
  r1 = _mm256_movemask_epi8(range1_1);

  *range1 = r0 ^ ((unsigned long)r1 << 32);     
  r0 = _mm256_movemask_epi8(range1_2);
  r1 = _mm256_movemask_epi8(range1_3);

  range1[1] = r0 ^ ((unsigned long)r1 << 32);
}

static const char* parse_headers_avx2(const char* buf, const char* buf_end, int* ret)
{
  // 128 bit token mask
  unsigned long bm[8] = {0};
  // Pointer to the start of the currently parsed block of 128 bytes
  const char* prep_start = buf;
  const char *p = buf;

  // Load the \r and : mask into rr13 and rr58
  // Load 512 bytes at a time into the bit mask bm[8]
  //   Load 32 bytes from the buffer into each register and compare against the mask registers

  __m256i b0, b1, b2, b3;
  const __m256i rr13    = _mm256_set1_epi8(13);
  const __m256i rr58    = _mm256_set1_epi8(58);
  int state = 0;

  // Process 512b per loop
  while(1) {
    int i = 0;
    // Load 512b into bm[0-7]
    while ( i < 8 ) {
      b0 = _mm256_loadu_si256((__m256i *)(buf + (64*i)+ 0));
      b1 = _mm256_loadu_si256((__m256i *)(buf + (64*i)+32));
      b2 = _mm256_loadu_si256((__m256i *)(buf + (64*i)+64));
      b3 = _mm256_loadu_si256((__m256i *)(buf + (64*i)+96));
      bm[i++]   =       _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(rr13, b0),_mm256_cmpeq_epi8(rr58, b0)) ) |
        ((unsigned long)_mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(rr13, b1),_mm256_cmpeq_epi8(rr58, b1)) ) << 32);
      bm[i++] =         _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(rr13, b2),_mm256_cmpeq_epi8(rr58, b2)) ) |
        ((unsigned long)_mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(rr13, b3),_mm256_cmpeq_epi8(rr58, b3)) ) << 32);
    }

    // Each bit in the mask is either a : or a \r
    int off = 0;
    int shft = 0;
    int bmOff = 0;
    unsigned long bitmap, tz;
    int slen = 0; //DELME

    do {
      bitmap = bm[ bmOff ] >> shft;
      tz = TZCNT(bitmap);
      if ( tz < 64 ) { // tz is 64 if not found
        p += tz;
        //printf( " fnd >%.*s<\n", p-buf, buf );  
        if ( state == 0 ) { // :
          state = 1;
          p += 2; buf = p;
        } else { // \r
          state = 0;
          p += 2; buf = p;
          if ( *p == '\r' ) goto wedone;
        }
      } else {
        p += 64 - shft;
        //printf("DELMEZ %.*s\n", 3, p) ;
      }
      off = p-prep_start;
      //printf("DELME off=%d\n",off);
      shft = off&0x3F;
      bmOff = off/64;
    } while ( bmOff < 8 ) ;
    prep_start += 512;
    buf = prep_start;
  }

wedone:
// Host: server\r\n
// User-Agent: Mozilla/5.0 (X11; Linux x86_64) Gecko/20130501 Firefox/30.0 AppleWebKit/600.00 Chrome/30.0.0000.0 Trident/10.0 Safari/600.00\r\n
  //printf("%.*s\n", p - prep_start, prep_start);
  return buf;
}



//__m256i m13 = _mm256_set1_epi8(13);
__m256i m58 = _mm256_set1_epi8(58);   //  0x1313131313131313...
                                      //  0x32333435363713   //  abcdef\r
                                      //  32 bit number 0x40
static void parse_mine( const char* buf ) {

  unsigned long long msk[8];  // 1 bit for each of 512 bytes matching  : or \r

  //__m256i b0,b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15;
  __m256i b0,b1,b2,b3,b4,b5,b6,b7;

  b0 = _mm256_loadu_si256((const __m256i *) (buf + 32*0)); // buf[0]
  b1 = _mm256_loadu_si256((const __m256i *) (buf + 32*1)); // buf[32]
  b2 = _mm256_loadu_si256((const __m256i *) (buf + 32*2)); // buf[64]
  b3 = _mm256_loadu_si256((const __m256i *) (buf + 32*3)); // buf[96]
  b4 = _mm256_loadu_si256((const __m256i *) (buf + 32*4)); // buf[128]
  b5 = _mm256_loadu_si256((const __m256i *) (buf + 32*5));
  b6 = _mm256_loadu_si256((const __m256i *) (buf + 32*6));
  b7 = _mm256_loadu_si256((const __m256i *) (buf + 32*7)); // 256 bytes

  msk[0] =            _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b0, m13), _mm256_cmpeq_epi8(b0, m58) ) )  |
     ((unsigned long long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b1, m13), _mm256_cmpeq_epi8(b1, m58) ) ) << 32);
  msk[1] =            _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b2, m13), _mm256_cmpeq_epi8(b2, m58) ) )  |
     ((unsigned long long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b3, m13), _mm256_cmpeq_epi8(b3, m58) ) ) << 32);
  msk[2] =            _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b4, m13), _mm256_cmpeq_epi8(b4, m58) ) )  |
     ((unsigned long long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b5, m13), _mm256_cmpeq_epi8(b5, m58) ) ) << 32);
  msk[3] =            _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b6, m13), _mm256_cmpeq_epi8(b6, m58) ) )  |
     ((unsigned long long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b7, m13), _mm256_cmpeq_epi8(b7, m58) ) ) << 32);


  b0 = _mm256_loadu_si256((const __m256i *) (buf + 32*8));
  b1 = _mm256_loadu_si256((const __m256i *) (buf + 32*9));
  b2 = _mm256_loadu_si256((const __m256i *) (buf + 32*10));
  b3 = _mm256_loadu_si256((const __m256i *) (buf + 32*11));
  b4 = _mm256_loadu_si256((const __m256i *) (buf + 32*12));
  b5 = _mm256_loadu_si256((const __m256i *) (buf + 32*13));
  b6 = _mm256_loadu_si256((const __m256i *) (buf + 32*14));
  b7 = _mm256_loadu_si256((const __m256i *) (buf + 32*15));

  msk[4] =            _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b0, m13), _mm256_cmpeq_epi8(b0, m58) ) )  ^
     ((unsigned long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b1, m13), _mm256_cmpeq_epi8(b1, m58) ) ) << 32);
  msk[5] =            _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b2, m13), _mm256_cmpeq_epi8(b2, m58) ) )  ^
     ((unsigned long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b3, m13), _mm256_cmpeq_epi8(b3, m58) ) ) << 32);
  msk[6] =            _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b4, m13), _mm256_cmpeq_epi8(b4, m58) ) )  ^
     ((unsigned long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b5, m13), _mm256_cmpeq_epi8(b5, m58) ) ) << 32);
  msk[7] =            _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b6, m13), _mm256_cmpeq_epi8(b6, m58) ) )  ^
     ((unsigned long) _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b7, m13), _mm256_cmpeq_epi8(b7, m58) ) ) << 32);

  //for ( int i = 0; i < 8; i++ ) {
    //printf(" m%d : 0x%016llx\n", i, msk[i]);
  //}

  //  uint64 msk[8]  -- 512 bits
  //  Loop until crlfcrlf or 0xA
  //    Name =  string(buf, tzcnt(msk[i]) )
  //    msk >>= len,  buf += len // TODO increment i for each 64bits
  //    Value = string(buf, tzcnt(msk[i]))

  // "Host: server\r\n"
  int i = 0;  // msk[i] 
  int l, dist, t;
  //int cnt = 0;
  while (1) {

    // msk[0] is only 64 bits 
    l = 0;
    while(1) {
      t = TZCNT(msk[i]); // tz is 6,  'server\r\n'
                             // msk[0] is all 0s and I get 64+2
      if ( t == 64 ) {
        l += t-dist; 
        dist = 0;
        i += 1;
        if ( i > 7 ) break; 
      } else {
        l += t;
        dist += t+2;
        buf += l+2;  
        if ( t+2 > 64 ) {
          msk[i] = 0;
          i += 1;
          if ( i > 7 ) break; 
          dist = t+2-64;
          msk[i] >>= (t+2-64);
        } else {
          msk[i] >>= t+2;
        }
    
        break;
      }
  
    }
   
    if ( i > 7 ) break; 
    if ( buf[0] == '\r' ) break;
  }
  
  
}
static void parse_mine3( const char* buf ) {

  unsigned long long msk[8];  // 1 bit for each of 512 bytes matching  : or \r

  //__m256i b0,b1,b2,b3,b4,b5,b6,b7,b8,b9,b10,b11,b12,b13,b14,b15;
  __m256i b0,b1,b2,b3,b4,b5,b6,b7;

  const char *obuf = buf;
  const char *sbuf = buf;

  int i;  // msk[i] 
  int t;
  unsigned int s = 0;
  int name_or_value = 0;

  const char *block_start = obuf;

new512:
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
      //printf("DELME mski %016llx shift %d\n", msk[i], s );
      //printf("DELME shft %016llx\n", msk[i]>>s );
      if ( t < 64 ) {
        buf += t;
        if ( name_or_value == 1 ) {
          if ( *buf == ':' ) { buf += 1; continue; } // : in value field
          name_or_value = 0;
        } else {
          name_or_value = 1;
        }
        //printf( " fnd >%.*s<\n", buf-sbuf, sbuf );  
        buf += 2; if ( *buf == '\r' ) break; // \r\n\r\n marks the end
        sbuf = buf;
        if ( (buf-block_start)> 64 ) break; // TODO?
      } else {
        buf = block_start + 64;
        break;
      }

    }

    i+=1;
    if ( buf[0] == '\r' ) goto done;
  } while ( i < 8 && buf[0] != '\r' );

  obuf += 512; 
  goto new512;
done: 
  i += 1;
}

static void parse_mine2( const char* buf ) {
  unsigned int msk;
  int i=0,tz; // 32B index
  int cnt = 0;
  unsigned int shifted;
  const char *sbuf = buf;
  const char *obuf = buf;
  int name_or_value = 0;

  do {
    const char *block_start = obuf+32*i;
    __m256i b0 = _mm256_loadu_si256((const __m256i *) block_start);
    msk = _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b0, m13), _mm256_cmpeq_epi8(b0, m58) ) );
    while (1) {

      shifted = buf-block_start;
      if ( shifted >= 32 ) break;
      tz = TZCNT((msk >> shifted));
      if ( tz < 32 ) {
        buf += tz;
        if ( name_or_value == 1 ) {
          if ( *buf == ':' ) { buf += 1; continue; } // : in value field
          name_or_value = 0;
        } else {
          name_or_value = 1;
        }
        //printf( " fnd >%.*s<\n", buf-sbuf, sbuf );  
        buf += 2; if ( *buf == '\r' ) break; // \r\n\r\n marks the end
        sbuf = buf;
      } else {
        buf += 32 - shifted;
        break;
      }

    }
    i+=1;
  } while ( *buf != '\r' );
}

static void parse_sse4( const char* buf ) {
  int ret = 0;
  while ( ret == 0 && buf != NULL && buf[0] != '\r' ) {
    buf = get_token_to_eol( buf, buf+512, &ret);
  }
}
static void parse_mysse4( const char* buf ) {
  int ret = 0;
  while ( ret == 0 && buf != NULL && buf[0] != '\r' ) {
    buf = my_get_eol( buf );
  }
}
static char buf[8096] = "Host: server\r\n"
"User-Agent: Mozilla/5.0 (X11; Linux x86_64) Gecko/20130501 Firefox/30.0 AppleWebKit/600.00 Chrome/30.0.0000.0 Trident/10.0 Safari/600.00\r\n"
"Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\r\n"
"Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\r\n"
"Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\r\n"
"Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\r\n"
"Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\r\n"
"Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\r\n"
"Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\r\n"
"Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,* /*;q=0.8\r\n"
"Accept-Language: en-US,en;q=0.5\r\n"
"Connection: keep-alive\r\n\r\n";
static char buf2[8096] = "Host: localhost:8080\r\nUser-Agent: python-requests/2.31.0\r\nAccept-Encoding: gzip, deflate\r\nAccept: * /*\r\nConnection: keep-alive\r\nCookie: foo=b=ar\r\nContent-Length: 0\r\n\r\n";
static char path[8096] = "/foo/bar/bazfdasfffffffffffffffffffffffffffffffffffffffdfffffffffffffffffffffffffffffffffffffffffffffffffff ";

static void BM_SlowParse(benchmark::State& state) {
  // Perform setup here
  std::string text = "Host: server\n"
"User-Agent: Mozilla/5.0 (X11; Linux x86_64) Gecko/20130501 Firefox/30.0 AppleWebKit/600.00 Chrome/30.0.0000.0 Trident/10.0 Safari/600.00\n"
"Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\n"
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,* /*;q=0.8\n"
"Accept-Language: en-US,en;q=0.5\n"
"Connection: keep-alive\n";

  for (auto _ : state) {
    // This code gets timed
    slow_hparse(text);
  }
}
static void BM_sse4_get_eol(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    parse_sse4(buf);
  }
}
static void BM_my_get_eol(benchmark::State& state) {
  // Perform setup here
  for (auto _ : state) {
    // This code gets timed
    parse_mysse4(buf);
  }
}

static void BM_my_header_parse(benchmark::State& state) {
  for (auto _ : state) {
    parse_mine(buf);
  }
}
static void BM_my2_header_parse(benchmark::State& state) {
  for (auto _ : state) {
    parse_mine2(buf);
  }
}
static void BM_my3_header_parse(benchmark::State& state) {
  for (auto _ : state) {
    parse_mine3(buf);
  }
}


static void BM_old_header_parse(benchmark::State& state) {
  int ret = 0;
  for (auto _ : state) {
    parse_headers(buf,buf+2048,&ret);
  }
}

static void BM_avx2_header_parse(benchmark::State& state) {
  int ret = 0;
  for (auto _ : state) {
    parse_headers_avx2(buf,buf+2048,&ret);
  }
}

static void BM_adv_token(benchmark::State& state) {
  int ret = 0;
  int path_len = 0;
  for (auto _ : state) {
    adv_token(path, &path_len);
  }
}
static void BM_adv_token_avx2(benchmark::State& state) {
  int ret = 0;
  int path_len = 0;
  for (auto _ : state) {
    get_to_space(path, &path_len);
  }
}




//BENCHMARK(BM_SlowParse);
//BENCHMARK(BM_sse4_get_eol);
//BENCHMARK(BM_my_get_eol);
BENCHMARK(BM_my3_header_parse);
//BENCHMARK(BM_my2_header_parse);
//BENCHMARK(BM_my_header_parse);
BENCHMARK(BM_old_header_parse);
//BENCHMARK(BM_avx2_header_parse);
BENCHMARK(BM_adv_token);
BENCHMARK(BM_adv_token_avx2);
BENCHMARK_MAIN();

/*

int main() {
  char buf[8096] = "Host: server\r\n"
"User-Agent: Mozilla/5.0 (X11; Linux x86_64) Gecko/20130501 Firefox/30.0 AppleWebKit/600.00 Chrome/30.0.0000.0 Trident/10.0 Safari/600.00\r\n"
"Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\r\n"
"Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\r\n"
"Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\r\n"
"Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\r\n"
"Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\r\n"
"Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,* /*;q=0.8\r\n"
"Accept-Language: en-US,en;q=0.5\r\n"
"Connection: keep-alive\r\n\r\n";
  //strcpy(buf,"Host: localhost:8080\r\nUser-Agent: curl/7.68.0\r\nAccept: * /*\r\n\r\n");
  //strcpy(buf,"Host: localhost:8080\r\nUser-Agent: python-requests/2.31.0\r\nAccept-Encoding: gzip, deflate\r\nAccept: * /*\r\nConnection: keep-alive\r\nCookie: foo=b=ar\r\nContent-Length: 0\r\n\r\n");


  int ret = 0;
  parse_headers_avx2(buf,buf+512,&ret);
  //parse_headers(buf,buf+2048,&ret);
  //parse_mine3(buf);
  printf(" ret=%d\n",ret);

  //unsigned long long l = 0x80008020ull;
  //unsigned int s = 7;
  //printf(" WTF %08x\n", l >> s );

}


*/
