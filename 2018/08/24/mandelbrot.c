/* The Computer Language Benchmarks Game
* http://benchmarksgame.alioth.debian.org/

  contributed by Greg Buchholz

*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <memory.h>

#ifdef USEAVX512
#include <x86intrin.h>
#endif

void* mandelbrot(int32_t w, int32_t h, uint8_t *output) {
  printf("mandelbrot start with %d %d \n", w,h);
  int bit_num = 0;
  char byte_acc = 0;
  int i, iter = 50;
  double x, y, limit = 2.0;
  double Zr, Zi, Cr, Ci, Tr, Ti;
#ifdef USEAVX512
  __m512i a = _mm512_set1_epi32(0);
  __m512i b = _mm512_set1_epi32(1);
  __m512i t;
#endif
  for (y = 0; y < h; ++y) {
#ifdef USEAVX512
    t = a;
    a = b;
#ifdef USEHEAVYAVX512
    b = _mm512_mul_epi32(b, t);
#else
    b = _mm512_add_epi32(b, t);
#endif
#endif
    for (x = 0; x < w; ++x) {
      Zr = Zi = Tr = Ti = 0.0;
      Cr = (2.0 * x / w - 1.5);
      Ci = (2.0 * y / h - 1.0);

      for (i = 0; i < iter && (Tr + Ti <= limit * limit); ++i) {
        Zi = 2.0 * Zr * Zi + Ci;
        Zr = Tr - Ti + Cr;
        Tr = Zr * Zr;
        Ti = Zi * Zi;
      }

      byte_acc <<= 1;
      if (Tr + Ti <= limit * limit)
        byte_acc |= 0x01;

      ++bit_num;

      if (bit_num == 8) {
        // putc(byte_acc,stdout);
        byte_acc = 0;
        bit_num = 0;
      } else if (x == w - 1) {
        byte_acc <<= (8 - w % 8);
        // putc(byte_acc,stdout);
        byte_acc = 0;
        bit_num = 0;
      }
    }
  }
  *output = byte_acc; // to avoid optimization
  printf("mandelbrot finish \n");
#ifdef USEAVX512
  _mm512_storeu_si512(output + 1, b);
#endif
  return output;
}


struct mandelparam {
  int32_t h;
  int32_t w;
  uint8_t output[64 - 2 * 4];
};

void* mandelbrotthread(void * data) {
  struct mandelparam  *x = (struct mandelparam  *) data;
  return mandelbrot(x->w, x->h, x->output);
}

int main(int argc, char **argv) {

  size_t nthreads = 1;
  if (argc > 1) {
    nthreads = atoi(argv[1]);
  }

  int w = argc > 2 ? atoi(argv[2]) : 32000;
  printf("Using %zu threads with size param = %d \n", nthreads, w);

#if defined(__AVX512F__)
  printf("avx512 is available\n");
#else
  printf("avx512 is unavailable \n");
#endif
  pthread_t threads[nthreads];
  struct mandelparam params[nthreads];
  for (int i = 0; i < nthreads; i++) {
    params[i].w = w;
    params[i].h = w;
  }
  for (int i = 0; i < nthreads; i++) {
    pthread_create(&threads[i], NULL, mandelbrotthread, &params[i]);
  }
  for (int i = 0; i < nthreads; i++) {
    pthread_join(threads[i], NULL);
  }
  printf("bogus results: ");
  for (int i = 0; i < nthreads; i++) {
    printf("%d ", (int)params[i].output[0]);
  }
  printf("\n");
#ifdef USEAVX512
  printf("We used avx512 deliberately \n");
#ifdef USEHEAVYAVX512
  printf("In fact, we used multiplications!\n");
#endif
  printf("bogus avx512: ");
  for (int i = 0; i < nthreads; i++) {
    __m512i b = _mm512_loadu_si512(params[i].output + 1);
    printf("%d ", _mm256_extract_epi32(_mm512_extracti64x4_epi64(b, 1), 7));
  }
  printf("\n");
#else
  printf("We did not try to use avx512.\n");
#endif
  return 0;
}