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
associated timing information in CPU cycles.

``t-ww.c`` is a most basic sanity test. Ultimately, I hope to
enhance this to generate outputs suitable for use in a statistical
test-suite to measure randomness.

.. _paper: http://www.ieee-security.org/TC/SP2014/papers/Not-So-RandomNumbersinVirtualizedLinuxandtheWhirlwindRNG.pdf

.. vim: ft=rst:sw=4:ts=4:tw=68:
