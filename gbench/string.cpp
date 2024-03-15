

#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <string.h>
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

static inline bool _isdigit(char c)  { return  c >= '0'  && c <= '9'; }
static inline bool _isdigit2(unsigned char c)  { return  (c & 0xF0) == 0x30; }
#define IS_DIGIT(c) ((c&0xF0) == 0x30)
#define IS_DIGIT2(c) (c >= '0' && c <= '9')

static long _strtol( char* buf ) {
  char * endptr = buf+4;
  return strtol(buf, &endptr, 10);
}
static long my_strtol( char* s, int maxlen ) {
  long l;
  int n = 0;
  benchmark::DoNotOptimize(n);
  while (_isdigit(*s)) {
    l = (l * 10) + (*s++ - '0');
    n+=1;
    if ( n > maxlen ) return l;
  }
  return l;
}
static long my_strtol2( char* s ) {
  long l;
  while (_isdigit2(*s)) {
    l = (l * 10) + (*s++ - 0x30);
  }
  return l;
}
static long my_strtol3( char* s ) {
  long l;
  while (IS_DIGIT(*s)) {
    l = (l * 10) + (*s++ - '0');
  }
  return l;
}
static long my_strcmp( char* s ) {
  //if ( s[0] == 'C' && s[11] == 'e' ) return 0;
  if ( s[0] == 'C' ) return 0;
  return 1;
}



static void BM_strtol(benchmark::State& state) {
  char buf[8096] = "123z4 ";
  for (auto _ : state) { long x = _strtol(buf);  }
}
static void BM_my_strtol(benchmark::State& state) {
  char buf[8096] = "123z4 ";
  for (auto _ : state) { long x = my_strtol(buf,4); }
}
static void BM_my_strtol2(benchmark::State& state) {
  char buf[8096] = "123z4 ";
  for (auto _ : state) { long x = my_strtol2(buf); }
}
static void BM_my_strtol3(benchmark::State& state) {
  char buf[8096] = "123z4 ";
  for (auto _ : state) { long x = my_strtol3(buf); }
}
static void BM_strcmp(benchmark::State& state) {
  char buf[8096] = "Content-Type";
  for (auto _ : state) { 
    long x;
    benchmark::DoNotOptimize(x);
    x = strcmp(buf, "Cntent-Type"); 
  }
}
static void BM_my_strcmp(benchmark::State& state) {
  char buf[8096] = "Content-Type";
  for (auto _ : state) { 
    long x;
    benchmark::DoNotOptimize(x);
    x = my_strcmp(buf);
  }
}

BENCHMARK(BM_strtol);
BENCHMARK(BM_my_strtol);
BENCHMARK(BM_my_strtol2);
BENCHMARK(BM_my_strtol3);
BENCHMARK(BM_strcmp);
BENCHMARK(BM_my_strcmp);
BENCHMARK_MAIN();

/*
int main() {
  char buf[8096] = "123z4 ";
  //strcpy(buf,"942312");

  printf(" my strtol %d\n", my_strtol(buf, 4));

}
*/
