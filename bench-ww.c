/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * bench-ww.c - Simple benchmark for the Whirlwind RNG.
 *
 * Copyright (c) 2015 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 * Usage:
 * ======
 *
 *   $0  size [size ..]
 *
 * The program takes one or more integer arguments and generates
 * random bytes of that size. It does this in a loop and prints the
 * average times.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "whirlwind-rng.h"


#if defined(__i386__)

static inline uint64_t read_tsc(void)
{
    uint64_t x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}

#elif defined(__x86_64__)


static inline uint64_t read_tsc(void)
{
    uint64_t res;
    uint32_t hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));

    res = hi;
    res <<= 32;
    return res | lo;
}

#else

#error "I don't know how to get CPU cycle counter for this machine!"

#endif /* x86, x86_64, ppc */


static uint8_t IV[1024];


/*
 * Generate 'siz' byte RNG in a tight loop and provide averages.
 */
static void
bench(ww_state* ww, size_t siz, size_t niter)
{
    size_t j;
    uint8_t *buf = malloc(siz);
    uint64_t s0, s1;
    uint64_t t1 = 0,
             t2 = 0;

    for (j = 0; j < niter; ++j) {
        s0 = read_tsc();
        ww_add_input(ww, s0^j);

        s1 = read_tsc(); t1 += s1 - s0;

        ww_random_bytes(ww, buf, siz);

        s0 = read_tsc(); t2 += s0 - s1;
    }

#define _d(x)   ((double)(x))
    double a0 = _d(t1) / _d(niter);
    double a1 = _d(t2) / _d(niter);
    double a2 = a1 / _d(siz);

    printf("%6lu, %9.4f cy/add, %9.4f cy/byte, %9.4f cy/blk\n",
            siz, a0, a2, a1);

    free(buf);
}


#define NITER       8192

int
main(int argc, char* argv[])
{
    int i;

    // Yes. I know. don't shoot me.
    memset(IV, 33, sizeof IV);

    ww_state w;

    ww_init(&w, IV, sizeof IV);

    for (i = 1; i < argc; ++i) {
        int z = atoi(argv[i]);
        if (z <= 0) continue;
        bench(&w, z, NITER);
    }

    return 0;
}

/* EOF */
