=============
Whirlwind-rng
=============

Cleanroom implementation of the whirlwind RNG.

This implementation follows the construction laid out in the
original paper_. However, this implementation deviates slightly from
the paper in the following ways:

* Uses blake2b as the hash function
* Uses full hash output of blake2b in lieu of SHA-512 transform
  (``h()``)
* Uses keyed blake2b for hash reseeding
* Requires the caller to provide an initialization seed ("IV")

This implementation uses an explicit state argument for every
function. Thus, this is fully thread-safe.

The blake2b implementation is borrowed from libsodium.

Guide to Source
===============
* ``whirlwind-rng.c``: The implementation of Whirlwind RNG
* ``whirlwind-rng.h``: Public interface to the implementation
* ``blake*``: Blake2b implementation
* ``t-ww.c``: Basic sanity test
* ``bench-ww.c``: Simple benchmark test


Tests
=====
``bench-ww.c`` is a simple benchmark that prints the cost of adding
entropy and generating random bytes of various sizes. Example usage::
    
    bench-ww  4 8 16 32 64 128 256 512 1024

This will generate random bytes of 4, 8, 16 etc. bytes and show
associated timing information in CPU cycles. On my Late 2013 Retina
MacBook Pro (2.8GHz Core i7), the above invocation shows ::

       4,  151.0537 cy/add, 3160.2849 cy/byte, 12641.1394 cy/blk
       8,  168.7217 cy/add, 1769.0247 cy/byte, 14152.1978 cy/blk
      16,  155.4158 cy/add,  812.3566 cy/byte, 12997.7051 cy/blk
      32,  149.5706 cy/add,  379.3949 cy/byte, 12140.6355 cy/blk
      64,  164.0715 cy/add,  217.1170 cy/byte, 13895.4868 cy/blk
     128,  386.3782 cy/add,  122.8829 cy/byte, 15729.0155 cy/blk
     256,  196.8602 cy/add,   77.3959 cy/byte, 19813.3384 cy/blk
     512,  208.2917 cy/add,   56.1442 cy/byte, 28745.8385 cy/blk
    1024,  190.6415 cy/add,   44.6681 cy/byte, 45740.1093 cy/blk

``t-ww.c`` is a most basic sanity test. Ultimately, I hope to
enhance this to generate outputs suitable for use in a statistical
test-suite to measure randomness.

.. _paper: http://www.ieee-security.org/TC/SP2014/papers/Not-So-RandomNumbersinVirtualizedLinuxandtheWhirlwindRNG.pdf

.. vim: ft=rst:sw=4:ts=4:tw=68:
