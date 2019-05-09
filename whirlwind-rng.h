/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * whirlwind-rng.h - Clean room Whirlwind RNG implementation.
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
 * Implementation Notes
 * ====================
 * o  Uses blake2b as the underlying primitive
 * o  Uses full hash as opposed to just one round
 * o  Uses keyed blake2b for hash reseeding.
 */

#ifndef ___WHIRLWIND_RNG_H_3910110_1446620072__
#define ___WHIRLWIND_RNG_H_3910110_1446620072__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>


/*
 * Constants used in this implementation.
 *
 * The paper uses 10 for SLOW_SEED_MAX; we use 8 to make the modulus
 * operation faster.
 *
 * Similarly, we use 64 for SLOW_CHAIN_MAX instead of 50.
 */

#define WW_SEED_BYTES       64
#define WW_SEED_WORDS       (WW_SEED_BYTES / 8)
#define WW_INPUT_WORDS      WW_SEED_WORDS
#define WW_SLOW_SEED_MAX    8
#define WW_SLOW_CHAIN_MAX   64


/*
 * Fast and Slow seeds look like this.
 *
 * The chains counter is only used for slow-seed; it is used to
 * determine when to populate the actual output slow-seed.
 */
struct ww_seed
{
    uint32_t idx;
    uint32_t chains;
    uint8_t  seed[WW_SEED_BYTES];
    uint64_t inp[WW_INPUT_WORDS];
};
typedef struct ww_seed ww_seed;


/*
 * Whirlwind state.
 */
struct ww_state
{
    uint64_t ctr;
    ww_seed  slow;
    ww_seed  fast;
    uint8_t  oseed[WW_SEED_BYTES];
};
typedef struct ww_state ww_state;



/*
 * Initialize instance with the supplied IV.
 */
extern void ww_init(ww_state* ww, void* iv, size_t ivsiz);


/*
 * Add 64 bits of entropy.
 */
extern void ww_add_input(ww_state* ww, uint64_t inp);


/*
 * Fill 'buf' with 'n' bytes of random data.
 */
extern void ww_random_bytes(ww_state* ww, void* buf, size_t n);


/*
 * Return a random 32-bit unsigned int
 */
static inline uint32_t
ww_random(ww_state* ww)
{
    uint32_t z;
    ww_random_bytes(ww, &z, sizeof z);
    return z;
}


/*
 * Platform dependent functions
 */
extern uint64_t __sys_hw_rand64(void);
extern uint64_t __sys_get_cycle_counter(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___WHIRLWIND_RNG_H_3910110_1446620072__ */

/* EOF */
