
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <string.h>
#include <x86intrin.h>
#ifdef __AVX2__
#include <immintrin.h>
#endif
#include "fastPRNG.h"

#include <benchmark/benchmark.h>

static char buf[8196];

void rand_bytes (int num_bytes)
{
  for (int i = 0; i < num_bytes; i++) { buf[i] = rand (); }
}
void rand_bytes2 (int num_bytes)
{
  uint64_t *p = (uint64_t*)buf;
  for (int i = 0; i < num_bytes; p++, i+=8 ) { 
    *p = fastPRNG::fastXS64s::xoshiro256p(); 
  } 
}


static void BM_rand1(benchmark::State& state) {
  for (auto _ : state) { 
    long x;
    benchmark::DoNotOptimize(x);
    x = fastPRNG::fastXS64s::xoshiro256p();
  }
}
static void BM_rand_bytes(benchmark::State& state) {
  srand ((unsigned int) time (NULL));
  for (auto _ : state) { 
    rand_bytes(1024);
  }
}
static void BM_rand_bytes2(benchmark::State& state) {
  srand ((unsigned int) time (NULL));
  for (auto _ : state) { 
    rand_bytes2(1024);
  }
}



BENCHMARK(BM_rand1);
BENCHMARK(BM_rand_bytes);
BENCHMARK(BM_rand_bytes2);
BENCHMARK_MAIN();

/*
int main() {
  char buf[8096] = "123z4 ";
  //strcpy(buf,"942312");

  printf(" my strtol %d\n", my_strtol(buf, 4));

}
*/
