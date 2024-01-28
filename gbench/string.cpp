

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

static void _strtol( char* buf ) {
  char * endptr = buf+4;
  long n = strtol(buf, &endptr, 10);
}
static void my_strtol( char* s ) {
  long l;
  while (_isdigit(*s)) {
    l = (l * 10) + (*s++ - '0');
  }
}

static void BM_strtol(benchmark::State& state) {
  char buf[8096] = "1234 ";
  for (auto _ : state) { _strtol(buf); }
}
static void BM_my_strtol(benchmark::State& state) {
  char buf[8096] = "1234 ";
  for (auto _ : state) { my_strtol(buf); }
}

BENCHMARK(BM_strtol);
BENCHMARK(BM_my_strtol);
BENCHMARK_MAIN();

