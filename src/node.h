
#ifndef NODES_H
#define NODES_H

#include "config.h"
#include "position.h"

#define MAX_PLY 128
#define MAX_MOVES 256

typedef struct {

  Position pos;

  uint32_t moves[MAX_MOVES];
  int num_moves;
  int next_move;
  uint8_t in_check;
  uint8_t stage;

  int32_t acc1[NET_H1_SIZE] __attribute__((aligned(64)));  // us acc
  int32_t acc2[NET_H1_SIZE] __attribute__((aligned(64)));  // them acc

} __attribute__((aligned(64))) Node;

extern Node ss[MAX_PLY];

#endif

