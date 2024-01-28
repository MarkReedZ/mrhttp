

#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <x86intrin.h>
#ifdef __AVX2__
#include <immintrin.h>
#endif


#if __GNUC__ >= 3
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#define IS_PRINTABLE_ASCII(c) ((unsigned char)(c)-040u < 0137u)

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

static unsigned long TZCNT(unsigned long long in) {
  unsigned long res;
  asm("tzcnt %1, %0\n\t" : "=r"(res) : "r"(in));
  return res;
}

__m256i m13 = _mm256_set1_epi8(13);
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

static void parse_sse4( const char* buf ) {
  int ret = 0;
  while ( ret == 0 && buf != NULL && buf[0] != '\r' ) {
    buf = get_token_to_eol( buf, buf+512, &ret); 
    printf("%d - %.16s\n",(int)buf[0],buf);
  }
}
static void parse_mysse4( const char* buf ) {
  int ret = 0;
  while ( ret == 0 && buf != NULL && buf[0] != '\r' ) {
    buf = my_get_eol( buf );
    printf("%d - %.16s\n",(int)buf[0],buf);
  }
}

//__m256i m13 = _mm256_set1_epi8(13);
__m256i m58 = _mm256_set1_epi8(58);   //  0x1313131313131313...
                                      //  0x32333435363713   //  abcdef\r
                                      //  32 bit number 0x40

static void parse_mine( const char* buf ) {
  unsigned long msk;
  int i=0,t; // 32B index
  int cnt = 0;
  const char *sbuf = buf;
  const char *obuf = buf;
  do {
    int shifted = 0;
    const char *block_start = obuf+32*i;
    __m256i b0 = _mm256_loadu_si256((const __m256i *) block_start);
    msk = _mm256_movemask_epi8( _mm256_or_si256(_mm256_cmpeq_epi8(b0, m13), _mm256_cmpeq_epi8(b0, m58) ) );
    printf(" m%d : 0x%08x\n", i, msk);

    while (1) {

      shifted = buf-block_start;
      t = TZCNT((msk >> shifted)); 
      printf("DELME shifted %d tzcnt %d\n", shifted, t);
      if ( t < 32 ) {
        if ( t == 0 ) break;
        buf += t+2;
        printf("L=%d str=%.*s\n", buf-sbuf-2, buf-sbuf-2, sbuf);
        sbuf = buf;
      } else {
        buf = block_start + 32;
        break;
      }

    }
    i+=1;
    cnt += 1;
  } while ( cnt < 20 && buf[0] != '\r' );


}

static void parse_mine3( const char* buf ) {

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

  const char *obuf = buf;
  const char *sbuf = buf;
  // "Host: server\r\n"
  int i = 0;  // msk[i] 
  int cnt = 0, t;
  //int cnt = 0;
  do {

    const char *block_start = obuf+64*i;

    while(1) {
      t = TZCNT((msk[i]>>(buf-block_start))); 
      printf("DELME shifted %d tzcnt %d msk %016llx\n", buf-block_start, t, (msk[i]>>(buf-block_start)));
      if ( t < 64 ) {
        buf += t+2;  
        printf("L=%d str=%.*s\n", buf-sbuf-2, buf-sbuf-2, sbuf);
        sbuf = buf;
        if ( (buf-block_start)> 64 ) break; // TODO?
      } else {
        buf = block_start + 64;
        break;
      }
  
    }
  
    cnt += 1; 
    i+=1; if ( i > 7 ) break; // TODO
  } while ( buf[0] != '\r' );
  
  
}


int main() { 
  char buf[8096] = "Host: server\r\n"
"User-Agent: Mozilla/5.0 (X11; Linux x86_64) Gecko/20130501 Firefox/30.0 AppleWebKit/600.00 Chrome/30.0.0000.0 Trident/10.0 Safari/600.00\r\n"
"Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
"Accept-Language: en-US,en;q=0.5\r\n"
"Connection: keep-alive\r\n\r\nzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz           "
"                                                                         "
"                                                                         ";

  parse_mine3(buf);

}
