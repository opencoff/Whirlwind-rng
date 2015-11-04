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

.. _paper: http://www.ieee-security.org/TC/SP2014/papers/Not-So-RandomNumbersinVirtualizedLinuxandtheWhirlwindRNG.pdf

.. vim: ft=rst:sw=4:ts=4:tw=68:
