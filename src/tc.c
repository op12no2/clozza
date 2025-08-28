
#include "tc.h"

/*{{{  now_ms*/

uint64_t now_ms(void) {

  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000u + (uint64_t)(ts.tv_nsec / 1000000u);

}

/*}}}*/

