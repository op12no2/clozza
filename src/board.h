
#ifndef BOARD_H
#define BOARD_H

#include <string.h>
#include <ctype.h>

#include "zob.h"
#include "node.h"
#include "net.h"
#include "movegen.h"

void position(Node *const node, const char *board_fen, const char *stm_str, const char *rights_str, const char *ep_str);
void print_board(Node *const node);
void make_move(Position *const pos, const uint32_t move);

#endif

