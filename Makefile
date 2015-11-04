
#
# Simple makefile to build the standalone Whirlwind implementation.
# 
# Requires GNU make.
#
# Notes:
#    o to produce debug builds, 'gmake'
#    o to produce optimized builds, 'gmake OPTIMIZE=1'
#    o to clean build products 'gmake clean'  or 'gmake realclean'
#


CC = gcc
AR = ar
LD = $(CC)

OFLAGS = -O3
CFLAGS = -Wall -Wextra -g
ARFLAGS = rv
LDFLAGS =

ifneq ($(OPTIMIZE),)
	CFLAGS += $(OFLAGS)
endif

libobjs = blake2b-ref.o whirlwind-rng.o
lib     = ww.a
tests   = t-ww bench-ww

objs    = $(libobjs) $(addsuffix %.d, $(tests))
deps    = $(objs:.o=.d)

all: $(tests)

t-ww: t-ww.o $(lib)
	$(CC) -o $@ $(LDFLAGS) $^

bench-ww: bench-ww.o $(lib)
	$(CC) -o $@ $(LDFLAGS) $^


$(lib): $(libobjs)
	-rm -f $@
	$(AR) $(ARFLAGS) $@ $^


.PHONY: clean

clean:
	rm -f $(objs) $(lib) $(tests)

realclean: clean
	rm -f $(deps)


%.o: %.c
	$(CC) -MMD $(_MP) -MT '$@ $(@:.o=.d)' -MF "$(@:.o=.d)" $(CFLAGS) -c -o $@ $<

ifneq ($(findstring clean, $(MAKECMDGOALS)),clean)
-include $(deps)
endif

# vim: ft=make:noexpandtab:sw=4:ts=4:notextmode:
