
#ifndef UCI_H
#define UCI_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "tc.h"
#include "board.h"
#include "perft.h"

#define UCI_LINE_LENGTH 8192
#define UCI_TOKENS      1024

void uci_exec(char *line);

#endif

