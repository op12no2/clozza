
#ifndef BB_H
#define BB_H

#include <stdio.h>
#include <stdint.h>

void print_bb(const uint64_t bb, const char *tag);
int bsf(const uint64_t bb);
int popcount(const uint64_t bb);

#endif

