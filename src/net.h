
#ifndef NET_H
#define NET_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h> // PRIu64 PRId64 PRIx64

#include "config.h"
#include "node.h"
#include "zob.h"

#define NET_I_SIZE 768
#define NET_QA 255
#define NET_QB 64
#define NET_QAB (NET_QA * NET_QB)
#define NET_SCALE 400

void net_init();
void net_rebuild_accs(Node *const node);
int32_t net_eval(Node *const node);
void net_copy(const Node *const from_node, Node *const to_node);
void net_prepare_move(const int a0, const int a1, const int a2);
void net_prepare_capture(const int a0, const int a1, const int a2, const int a3);
void net_prepare_promote(const int a0, const int a1, const int a2, const int a3, const int a4);
void net_prepare_ep_capture(const int a0, const int a1, const int a2, const int a3, const int a4);
void net_prepare_castle(const int a0, const int a1, const int a2, const int a3, const int a4, const int a5);

#endif

