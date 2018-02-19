/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * whirlwind-rng.c - Clean room Whirlwind RNG implementation.
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
 * Clean room implementation of Whirlwind RNG idea:
 *
 *  - Use blake2b (64 byte hash)
 *  - h() from the paper is actually a full blake2b() computation
 *  - Reseeding when extracting output is a keyed blake2b() construction
 *  - State is explicit for the RNG; thus the implementation is
 *    lockfree. Caller must ensure that each thread or CPU uses its
 *    own instance.
 *  - Init() (bootstrap) requires the caller to pass an IV.
 *
 * Original paper:
 * http://www.ieee-security.org/TC/SP2014/papers/Not-So-RandomNumbersinVirtualizedLinuxandtheWhirlwindRNG.pdf
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "blake2.h"

#include "whirlwind-rng.h"


/*
 * Used for extracting output
 */
struct ww_output_state
{
    uint32_t dom;
    uint32_t pid;
    uint8_t  s1[WW_SEED_BYTES];
    uint8_t  s2[WW_SEED_BYTES];
    uint64_t ctr;
    uint64_t cc;
    uint64_t hw;
};
typedef struct ww_output_state ww_output_state;


/*
 * Temp structure used only during init.
 */
struct ww_init_temp
{
    uint32_t dom;
    uint32_t pid;
    uint32_t ctr;
};
typedef struct ww_init_temp ww_init_temp;



/*
 * Yuck. There is no portable intrinsics to get the TSC for various
 * CPUs.
 */

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



/*
 * Handy defines.
 */
#define __sys_get_cycle_counter()   read_tsc()
#define HASH_SIZE                   WW_SEED_BYTES


/*
 * XXX Ginormous hack.
 */
#define __sys_hw_rand64()           0


/*
 * Horrible trick to fool the optimizer.
 */
static void* (* volatile ___zeroer)(void* x, int c, size_t n) = memset;

#define memz(p, n)      do { \
                            (*___zeroer)(p, 0, n); \
                        } while (0)

void
sodium_memzero(void * x, size_t n)
{
    memz(x, n);
}


/*
 * h() from the paper.
 *
 * In our implementation, we do a full blake2b computation.
 */
static inline void
hash_seed(ww_seed* d)
{
    blake2b_state s;

    blake2b_init(&s, HASH_SIZE);
    blake2b_update(&s, d->seed, sizeof d->seed);
    blake2b_update(&s, (const uint8_t*)d->inp,  sizeof d->inp);
    blake2b_final(&s,  d->seed, HASH_SIZE);
    memz(&s, sizeof s);
}


/*
 * H() from the paper.
 */
static inline void
hash_buffer(uint8_t* obuf, void* inbuf, size_t n)
{
    blake2b_state s;

    blake2b_init(&s, HASH_SIZE);
    blake2b_update(&s, inbuf, n);
    blake2b_final(&s, obuf, HASH_SIZE);

    memz(&s, sizeof s);
}


/*
 * Reseed at the end of random bytes extraction.
 * Again, we do a full keyed blake2b computation.
 */
static inline void
hash_reseed(uint8_t* seed)
{
    static const uint8_t zero[1024] = { 0 };
    blake2b_state s;

    blake2b_init_key(&s, HASH_SIZE, seed, HASH_SIZE);
    blake2b_update(&s, zero, sizeof zero);
    blake2b_final(&s,  seed, HASH_SIZE);

    memz(&s, sizeof s);
}


/*
 * Update the input buffer and hash if needed.
 */
static inline void
update_seed(ww_seed* s, uint64_t inp)
{
    // Add to the input buffer and hash if needed
    s->inp[s->idx] = inp;
    if (++s->idx == WW_INPUT_WORDS) {
        hash_seed(s);
        s->idx = 0;
    }
}



/*
 * Initialize a seed with an IV.
 * Keep this as a macro! We need the C preprocessor to generate
 * proper values for __COUNTER__.
 */
#ifdef __OpenBSD__
#ifndef __COUNTER__
#define __COUNTER__   __LINE__
#endif
#endif

#define init_seed(ss, dd, iv, ivsiz) do {   \
                                        blake2b_state s;                        \
                                        ww_init_temp zz = { .dom = dd,          \
                                                            .pid = getpid(),    \
                                                            .ctr = __COUNTER__  \
                                                          };                    \
                                        blake2b_init(&s, HASH_SIZE);            \
                                        blake2b_update(&s, (const uint8_t*)&zz, sizeof zz);     \
                                        blake2b_update(&s, iv, ivsiz);          \
                                        blake2b_final(&s, ss, HASH_SIZE);       \
                                        memz(&s, sizeof s);                     \
                                    } while (0)



/*
 * Initiaize the Whirlwind RNG
 *
 * Caller must provide IV.
 */

void
ww_init(ww_state* ww, void* iv, size_t ivsiz)
{
    uint32_t i, j;
    uint64_t a = 0;

    assert(iv);
    assert(ivsiz > 0);

#define L       100
#define Lmax    1024

    memset(ww, 0, sizeof *ww);

    /* Now, initialize slow and fast seeds */
    init_seed(ww->slow_seed.seed, 1, iv, ivsiz);
    init_seed(ww->slow_seed.seed, 2, iv, ivsiz);

    /* Create a data dependent loop */
    for (i = 0; i < L; ++i)  {
        uint64_t cc = __sys_get_cycle_counter();
        uint32_t k  = cc % Lmax;

        ww_add_input(ww, cc);


        for (j = 0; j < k; ++j) {
            a = (cc * (j+1)) - (a * i);
        }
    }

    

    /*
     * Add the result of the variable length inner loop to RNG. This won't
     * add any predictable amount of entropy to the RNG; but it prevents
     * the compiler from optimizing away the loop above.
     */
    ww_add_input(ww, a);
}



/*
 * Add 64 bits of entropy
 */
void
ww_add_input(ww_state* ww, uint64_t inp)
{
    uint64_t v = ++ww->ctr;

    if (0 == (v % WW_SLOW_SEED_MAX)) {
        ww_seed* s = &ww->slow_seed;

        update_seed(s, inp);

        if (++s->chains == WW_SLOW_CHAIN_MAX) {
            memcpy(ww->oseed, s->seed, sizeof ww->oseed);
            s->chains = 0;
        }
    } else {
        update_seed(&ww->fast_seed, inp);
    }
}




/*
 * Extract n bytes of random output from the generator
 */
void
ww_random_bytes(ww_state* ww, void* bufx, size_t n)
{
    ww_output_state o;
    uint8_t ohash[HASH_SIZE];

    uint8_t* buf = (uint8_t*)bufx;
    size_t blks  = (n / HASH_SIZE) + (0 != (n % HASH_SIZE));

    memcpy(o.s1, ww->fast_seed.seed, sizeof o.s1);
    memcpy(o.s2, ww->oseed,          sizeof o.s2);

    o.dom = 3;
    o.pid = getpid();
    o.ctr = ww->ctr;
    o.hw  = __sys_hw_rand64();

    ww->ctr += blks;
    ww_add_input(ww, o.ctr);

    while (blks > 1) {
        o.cc   = __sys_get_cycle_counter();
        o.ctr += 1;

        hash_buffer(buf, &o, sizeof o);

        buf  += HASH_SIZE;
        n    -= HASH_SIZE;
        blks -= 1;
    }

    /* Mop up last remaining block */
    o.cc   = __sys_get_cycle_counter();
    o.ctr += 1;
    hash_buffer(ohash, &o, sizeof o);
    memcpy(buf, ohash, n);

    memz(ohash, sizeof ohash);
    memz(&o,    sizeof o);

    /* Update the fast seed */
    hash_reseed(ww->fast_seed.seed);
}


/* EOF */
