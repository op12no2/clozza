
#include "prng.h"

static uint64_t rand64_seed = 0xDEADBEEFCAFEBABEULL;

uint64_t rand64(void) {

  rand64_seed ^= rand64_seed >> 12;
  rand64_seed ^= rand64_seed << 25;
  rand64_seed ^= rand64_seed >> 27;

  return rand64_seed * 2685821657736338717ULL;

}

