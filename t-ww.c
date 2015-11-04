#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "whirlwind-rng.h"



static uint32_t
rand32(ww_state* ww)
{
    uint32_t z;

    ww_random_bytes(ww, &z, sizeof z);
    return z;
}


static uint8_t IV[1024];


int
main(int argc, char* argv[])
{
    uint32_t i, 
             n = 10;

    if (argc > 1) {
        n = atoi(argv[1]);
        if (n <= 0) n = 10;
    }

    memset(IV, 33, sizeof IV);

    ww_state w;

    ww_init(&w, IV, sizeof IV);

    for (i = 0; i < n; ++i) {
        printf("%u\n", rand32(&w));
    }

    return 0;
}
