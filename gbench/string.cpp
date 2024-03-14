

#include <stdlib.h>
#include <stdint.h>
#include <string>
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
static long my_strtol( char* s ) {
  long l;
  while (_isdigit(*s)) {
    l = (l * 10) + (*s++ - '0');
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
  while (IS_DIGIT2(*s)) {
    l = (l * 10) + (*s++ - '0');
  }
  return l;
}



static void BM_strtol(benchmark::State& state) {
  char buf[8096] = "123z4 ";
  for (auto _ : state) { long x = _strtol(buf);  }
}
static void BM_my_strtol(benchmark::State& state) {
  char buf[8096] = "123z4 ";
  for (auto _ : state) { long x = my_strtol(buf); }
}
static void BM_my_strtol2(benchmark::State& state) {
  char buf[8096] = "123z4 ";
  for (auto _ : state) { long x = my_strtol2(buf); }
}
static void BM_my_strtol3(benchmark::State& state) {
  char buf[8096] = "123z4 ";
  for (auto _ : state) { long x = my_strtol3(buf); }
}

BENCHMARK(BM_strtol);
BENCHMARK(BM_my_strtol);
BENCHMARK(BM_my_strtol2);
BENCHMARK(BM_my_strtol3);
BENCHMARK_MAIN();

