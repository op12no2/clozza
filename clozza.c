
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>

/*{{{  misc*/

#define uint8  uint8_t
#define int16  int16_t
#define uint16 uint16_t
#define uint32 uint32_t
#define uint64 uint64_t
#define move_t uint32_t

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define len(s) ((int)strlen(s))

#define MAX_PLY 1024

const int MATE = 32000;

int mSilent = 0;

/*}}}*/
/*{{{  timing*/

int    tDone        = 0;
int    tTargetDepth = 0;
uint32 tTargetNodes = 0;
uint32 tFinishTime  = 0;
uint32 tNodes       = 0;
move_t tBestMove    = 0;

/*{{{  clock*/

uint32 clock() {

  struct timeval tv;

  gettimeofday(&tv, NULL);

  return (uint32)(tv.tv_sec * 1000) + (uint32)(tv.tv_usec / 1000);
}

/*}}}*/
/*{{{  areWeDone*/

int areWeDone() {
  return tBestMove && ((tFinishTime && (clock() > tFinishTime)) || (tTargetNodes && (tNodes >= tTargetNodes)));
}

/*}}}*/

/*}}}*/
/*{{{  board/hash*/

/*{{{  constants*/

#define WHITE 0x0
#define BLACK 0x8

#define PIECE_MASK  0x7
#define COLOUR_MASK 0x8

#define PAWN   1
#define KNIGHT 2
#define BISHOP 3
#define ROOK   4
#define QUEEN  5
#define KING   6
#define EDGE   7

#define W_PAWN   PAWN
#define W_KNIGHT KNIGHT
#define W_BISHOP BISHOP
#define W_ROOK   ROOK
#define W_QUEEN  QUEEN
#define W_KING   KING

#define B_PAWN   (PAWN   | BLACK)
#define B_KNIGHT (KNIGHT | BLACK)
#define B_BISHOP (BISHOP | BLACK)
#define B_ROOK   (ROOK   | BLACK)
#define B_QUEEN  (QUEEN  | BLACK)
#define B_KING   (KING   | BLACK)

#define WHITE_RIGHTS_KING  0x1
#define WHITE_RIGHTS_QUEEN 0x2
#define BLACK_RIGHTS_KING  0x4
#define BLACK_RIGHTS_QUEEN 0x8

#define WHITE_RIGHTS  (WHITE_RIGHTS_QUEEN | WHITE_RIGHTS_KING)
#define BLACK_RIGHTS  (BLACK_RIGHTS_QUEEN | BLACK_RIGHTS_KING)

const int MASK_RIGHTS[144] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                              15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                              15, 15, ~8, 15, 15, 15, ~12,15, 15, ~4, 15, 15,
                              15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                              15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                              15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                              15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                              15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                              15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                              15, 15, ~2, 15, 15, 15, ~3, 15, 15, ~1, 15, 15,
                              15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                              15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15};


const int ADJACENT[144] = {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0};

//
// E == EMPTY, X = OFF BOARD, - == CANNOT HAPPEN
//
//                  0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15
//                  E  W  W  W  W  W  W  X  -  B  B  B  B  B  B  -
//                  E  P  N  B  R  Q  K  X  -  P  N  B  R  Q  K  -
//

#define W_OFFSET_ORTH  -12
#define W_OFFSET_DIAG1 -13
#define W_OFFSET_DIAG2 -11

#define B_OFFSET_ORTH  12
#define B_OFFSET_DIAG1 13
#define B_OFFSET_DIAG2 11

const int KNIGHT_OFFSETS[] = {25,-25,23,-23,14,-14,10,-10};
const int BISHOP_OFFSETS[] = {11,-11,13,-13};
const int ROOK_OFFSETS[]   = {1,-1,12,-12};
const int QUEEN_OFFSETS[]  = {11,-11,13,-13,1,-1,12,-12};
const int KING_OFFSETS[]   = {11,-11,13,-13,1,-1,12,-12};

const int IS_O[]       = {0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0};
const int IS_E[]       = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int IS_OE[]      = {1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0};

const int IS_P[]       = {0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
const int IS_N[]       = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0};
const int IS_NBRQ[]    = {0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0};
const int IS_NBRQKE[]  = {1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0};
const int IS_RQKE[]    = {1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0};
const int IS_Q[]       = {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0};
const int IS_QKE[]     = {1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0};
const int IS_K[]       = {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0};
const int IS_KN[]      = {0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0};
const int IS_SLIDER[]  = {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0};

const int IS_W[]       = {0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int IS_WE[]      = {1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int IS_WP[]      = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int IS_WN[]      = {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int IS_WNBRQ[]   = {0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int IS_WPNBRQ[]  = {0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int IS_WPNBRQE[] = {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int IS_WB[]      = {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int IS_WR[]      = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int IS_WBQ[]     = {0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int IS_WRQ[]     = {0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int IS_WQ[]      = {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const int IS_WK[]      = {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const int IS_B[]       = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0};
const int IS_BE[]      = {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0};
const int IS_BP[]      = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
const int IS_BN[]      = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0};
const int IS_BNBRQ[]   = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0};
const int IS_BPNBRQ[]  = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0};
const int IS_BPNBRQE[] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0};
const int IS_BB[]      = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0};
const int IS_BR[]      = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0};
const int IS_BBQ[]     = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0};
const int IS_BRQ[]     = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0};
const int IS_BQ[]      = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0};
const int IS_BK[]      = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0};

const int *WB_CAN_CAPTURE[] = {IS_BPNBRQ, IS_WPNBRQ};
const int *WB_OUR_PIECE[]   = {IS_W,      IS_B};
const int *WB_RQ[]          = {IS_WRQ,    IS_BRQ};
const int *WB_BQ[]          = {IS_WBQ,    IS_BBQ};

const int WB_PAWN[] = {W_PAWN, B_PAWN};

const int WB_OFFSET_ORTH[]  = {W_OFFSET_ORTH,  B_OFFSET_ORTH};
const int WB_OFFSET_DIAG1[] = {W_OFFSET_DIAG1, B_OFFSET_DIAG1};
const int WB_OFFSET_DIAG2[] = {W_OFFSET_DIAG2, B_OFFSET_DIAG2};

const int WB_HOME_RANK[]    = {2, 7};
const int WB_PROMOTE_RANK[] = {7, 2};
const int WB_EP_RANK[]      = {5, 4};

const char OBJ_CHAR[] = {'.','P','N','B','R','Q','K','x','y','p','n','b','r','q','k','z'};

#define A1 110
#define B1 111
#define C1 112
#define D1 113
#define E1 114
#define F1 115
#define G1 116
#define H1 117
#define B2 99
#define C2 100
#define G2 10
#define H2 105
#define B7 39
#define C7 40
#define G7 44
#define H7 45
#define A8 26
#define B8 27
#define C8 28
#define D8 29
#define E8 30
#define F8 31
#define G8 32
#define H8 33

const int B88[] = {26, 27, 28, 29, 30, 31, 32, 33,
                   38, 39, 40, 41, 42, 43, 44, 45,
                   50, 51, 52, 53, 54, 55, 56, 57,
                   62, 63, 64, 65, 66, 67, 68, 69,
                   74, 75, 76, 77, 78, 79, 80, 81,
                   86, 87, 88, 89, 90, 91, 92, 93,
                   98, 99, 100,101,102,103,104,105,
                   110,111,112,113,114,115,116,117};

const char * const COORDS[] = {"??", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??",
                               "??", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??",
                               "??", "??", "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8", "??", "??",
                               "??", "??", "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", "??", "??",
                               "??", "??", "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", "??", "??",
                               "??", "??", "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", "??", "??",
                               "??", "??", "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", "??", "??",
                               "??", "??", "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", "??", "??",
                               "??", "??", "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", "??", "??",
                               "??", "??", "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "??", "??",
                               "??", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??",
                               "??", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??", "??"};

const int RANK[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 8, 8, 8, 8, 8, 8, 8, 8, 0, 0,
                    0, 0, 7, 7, 7, 7, 7, 7, 7, 7, 0, 0,
                    0, 0, 6, 6, 6, 6, 6, 6, 6, 6, 0, 0,
                    0, 0, 5, 5, 5, 5, 5, 5, 5, 5, 0, 0,
                    0, 0, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0,
                    0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0,
                    0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0,
                    0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const int FYLE[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0,
                    0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0,
                    0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0,
                    0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0,
                    0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0,
                    0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0,
                    0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0,
                    0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const int CENTRE[] = {0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0,
                      0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0,
                      0, 0, 1, 2,  3,  4,  4,  3,  2,  1, 0, 0,
                      0, 0, 2, 6,  8,  10, 10, 8,  6,  2, 0, 0,
                      0, 0, 3, 8,  15, 18, 18, 15, 8,  3, 0, 0,
                      0, 0, 4, 10, 18, 28, 28, 18, 10, 4, 0, 0,
                      0, 0, 4, 10, 18, 28, 28, 19, 10, 4, 0, 0,
                      0, 0, 3, 8,  15, 18, 18, 15, 8,  3, 0, 0,
                      0, 0, 2, 6,  8,  10, 10, 8,  6,  2, 0, 0,
                      0, 0, 1, 2,  3,  4,  4,  3,  2,  1, 0, 0,
                      0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0,
                      0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0};

/*}}}*/

int bBoard[144];
int bTurn     = 0;
int bRights   = 0;
int bEP       = 0;
int bPly      = 0;
int bKings[2] = {0,0};

uint32 hSeed = 0;

uint32 hLo = 0;
uint32 hHi = 0;

uint32 hLoTurn[9];
uint32 hHiTurn[9];
uint32 hLoRights[16];
uint32 hHiRights[16];
uint32 hLoEP[144];
uint32 hHiEP[144];
uint32 hLoObj[16][144] = {0};
uint32 hHiObj[16][144] = {0};
uint32 hLoHistory[MAX_PLY];
uint32 hHiHistory[MAX_PLY];

int hHistoryOffset = 0;
int hHistoryLimit  = 0;

/*{{{  rand32*/

uint32 rand32 () {

  hSeed ^= hSeed << 13;
  hSeed ^= hSeed >> 17;
  hSeed ^= hSeed << 5;

  return hSeed;
}

/*}}}*/
/*{{{  hashInitOnce*/

void hashInitOnce () {

  hSeed = 1804289383;

  for (int i=0; i < 9; i++) {
    hLoTurn[i] = rand32();
    hHiTurn[i] = rand32();
  }

  for (int i=0; i < 16; i++) {
    hLoRights[i] = rand32();
    hHiRights[i] = rand32();
  }

  for (int i=0; i < 144; i++) {
    hLoEP[i] = rand32();
    hHiEP[i] = rand32();
  }

  for (int i=1; i < 16; i++) {
    for (int j=0; j < 144; j++) {
      hLoObj[i][j] = rand32();
      hHiObj[i][j] = rand32();
    }
  }
}

/*}}}*/
/*{{{  hashReset*/

void hashReset() {

  hLo = 0;
  hHi = 0;
}

/*}}}*/
/*{{{  hashTurn*/

void hashTurn(int turn) {
  hLo ^= hLoTurn[turn];
  hHi ^= hHiTurn[turn];
}

/*}}}*/
/*{{{  hashRights*/

void hashRights (int rights) {
  hLo ^= hLoRights[rights];
  hHi ^= hHiRights[rights];
}

/*}}}*/
/*{{{  hashEP*/

void hashEP(int ep) {
  hLo ^= hLoEP[ep];
  hHi ^= hHiEP[ep];
}

/*}}}*/
/*{{{  hashObj*/

void hashObj(int obj, int sq) {
  hLo ^= hLoObj[obj][sq];
  hHi ^= hHiObj[obj][sq];
}

/*}}}*/
/*{{{  hashCalc*/

void hashCalc() {

  hashReset();

  for (int i=0; i < 64; i++) {
    const int sq  = B88[i];
    const int obj = bBoard[sq];
    hashObj(obj,sq);
  }

  hashTurn(bTurn);
  hashRights(bRights);
  hashEP(bEP);
}

/*}}}*/
/*{{{  hashIsDraw*/

int hashIsDraw() {

  if ((bPly + hHistoryOffset - hHistoryLimit) > 100)
    return 1;

  int limit = bPly + hHistoryOffset - 4;
  int count = 0;

  while (limit >= hHistoryLimit) {
    if (hLo == hLoHistory[limit] && hHi == hHiHistory[limit]) {
      if (++count == 2)
        return 1;
    }
    limit -= 2;
  }

  return 0;
}

/*}}}*/

/*{{{  board primitives*/

int objColour (int obj) {
  return obj & COLOUR_MASK;
}

int objPiece (int obj) {
  return obj & PIECE_MASK;
}

int colourIndex (int c) {
  return c >> 3;
}

int colourtoggleIndex (int i) {
  return abs(i-1);
}

int colourMultiplier (int c) {
  return (-c >> 31) | 1;
}

int colourToggle (int c) {
  return ~c & COLOUR_MASK;
}

/*}}}*/
/*{{{  printBoard*/

void printBoard () {

  int *b = bBoard;

  for (int rank=7; rank >= 0; rank--) {
    printf("%d ", (rank+1));
    for (int file=0; file <= 7; file++) {
      printf("%c ", OBJ_CHAR[b[B88[(7-rank)*8+file]]]);
    }
    printf("\n");
  }
  printf("  a b c d e f g h\n");

  if (bTurn == WHITE)
    printf("w");
  else
    printf("b");
  printf(" ");

  if (bRights) {
    if (bRights & WHITE_RIGHTS_KING)
      printf("K");
   if (bRights & WHITE_RIGHTS_QUEEN)
      printf("Q");
   if (bRights & BLACK_RIGHTS_KING)
      printf("k");
   if (bRights & BLACK_RIGHTS_QUEEN)
      printf("q");
    printf(" ");
  }
  else
    printf("- ");

  if (bEP)
    printf("%s",COORDS[bEP]);
  else
    printf("-");

  printf("\n");

  printf("hash %u %u\n",hLo,hHi);
}

/*}}}*/
/*{{{  position*/

void position (char *sb, char *st, char *sr, char *sep) {

  int *b = bBoard;

  for (int i=0; i < 144; i++)
    b[i] = EDGE;

  for (int i=0; i < 64; i++)
    b[B88[i]] = 0;

  /*{{{  board board*/
  
  int rank = 7;
  int file = 0;
  
  for (int i=0; i < len(sb); i++) {
  
    const char ch  = sb[i];
    const int sq88 = (7-rank) * 8 + file;
    const int sq   = B88[sq88];
  
    switch (ch) {
      /*{{{  1-8*/
      
      case '1':
        file += 1;
        break;
      case '2':
        file += 2;
        break;
      case '3':
        file += 3;
        break;
      case '4':
        file += 4;
        break;
      case '5':
        file += 5;
        break;
      case '6':
        file += 6;
        break;
      case '7':
        file += 7;
        break;
      case '8':
        break;
      
      /*}}}*/
      /*{{{  /*/
      
      case '/':
        rank--;
        file = 0;
        break;
      
      /*}}}*/
      /*{{{  black*/
      
      case 'p':
        b[sq] = B_PAWN;
        file++;
        break;
      case 'n':
        b[sq] = B_KNIGHT;
        file++;
        break;
      case 'b':
        b[sq] = B_BISHOP;
        file++;
        break;
      case 'r':
        b[sq] = B_ROOK;
        file++;
        break;
      case 'q':
        b[sq] = B_QUEEN;
        file++;
        break;
      case 'k':
        b[sq] = B_KING;
        bKings[1] = sq;
        file++;
        break;
      
      /*}}}*/
      /*{{{  white*/
      
      case 'P':
        b[sq] = W_PAWN;
        file++;
        break;
      case 'N':
        b[sq] = W_KNIGHT;
        file++;
        break;
      case 'B':
        b[sq] = W_BISHOP;
        file++;
        break;
      case 'R':
        b[sq] = W_ROOK;
        file++;
        break;
      case 'Q':
        b[sq] = W_QUEEN;
        file++;
        break;
      case 'K':
        b[sq] = W_KING;
        bKings[0] = sq;
        file++;
        break;
      
      /*}}}*/
    }
  }
  
  /*}}}*/
  /*{{{  board turn*/
  
  if (st[0] == 'w')
    bTurn = WHITE;
  
  else if (st[0] == 'b')
    bTurn = BLACK;
  
  else
    fprintf(stderr,"unknown board colour %s\n", st);
  
  /*}}}*/
  /*{{{  board rights*/
  
  bRights = 0;
  
  for (int i=0; i < len(sr); i++) {
  
    const char ch = sr[i];
  
    if (ch == 'K') bRights |= WHITE_RIGHTS_KING;
    if (ch == 'Q') bRights |= WHITE_RIGHTS_QUEEN;
    if (ch == 'k') bRights |= BLACK_RIGHTS_KING;
    if (ch == 'q') bRights |= BLACK_RIGHTS_QUEEN;
  }
  
  /*}}}*/
  /*{{{  board ep*/
  
  bEP = 0;
  
  if (len(sep) == 2) {
    for (int i=0; i < 144; i++) {
      if (!strcmp(COORDS[i],sep)) {
        bEP = i;
        break;
      }
    }
  }
  
  /*}}}*/

  hashCalc();

  bPly           = 0;
  hHistoryLimit  = 0;
  hHistoryOffset = 0;
}

/*}}}*/
/*{{{  isKingAttacked*/

int isKingAttacked (int to, int byCol) {

  int *b = bBoard;

  const int cx = colourIndex(byCol);

  const int OFFSET_DIAG1 = -WB_OFFSET_DIAG1[cx];
  const int OFFSET_DIAG2 = -WB_OFFSET_DIAG2[cx];
  const int BY_PAWN      = WB_PAWN[cx];
  const int *RQ          = WB_RQ[cx];
  const int *BQ          = WB_BQ[cx];
  const int N            = KNIGHT | byCol;

  int fr = 0;

  /*{{{  pawns*/
  
  if (b[to+OFFSET_DIAG1] == BY_PAWN || b[to+OFFSET_DIAG2] == BY_PAWN)
    return 1;
  
  /*}}}*/
  /*{{{  knights*/
  
  if ((b[to + -10] == N) ||
      (b[to + -23] == N) ||
      (b[to + -14] == N) ||
      (b[to + -25] == N) ||
      (b[to +  10] == N) ||
      (b[to +  23] == N) ||
      (b[to +  14] == N) ||
      (b[to +  25] == N)) return 1;
  
  /*}}}*/
  /*{{{  queen, bishop, rook*/
  
  fr = to + 1;  while (!b[fr]) fr += 1;  if (RQ[b[fr]]) return 1;
  fr = to - 1;  while (!b[fr]) fr -= 1;  if (RQ[b[fr]]) return 1;
  fr = to + 12; while (!b[fr]) fr += 12; if (RQ[b[fr]]) return 1;
  fr = to - 12; while (!b[fr]) fr -= 12; if (RQ[b[fr]]) return 1;
  
  fr = to + 11; while (!b[fr]) fr += 11; if (BQ[b[fr]]) return 1;
  fr = to - 11; while (!b[fr]) fr -= 11; if (BQ[b[fr]]) return 1;
  fr = to + 13; while (!b[fr]) fr += 13; if (BQ[b[fr]]) return 1;
  fr = to - 13; while (!b[fr]) fr -= 13; if (BQ[b[fr]]) return 1;
  
  /*}}}*/

  return 0;
}


/*}}}*/

/*}}}*/
/*{{{  tt*/

#define TT_SIZE  (1 << 20)
#define TT_MASK  ((TT_SIZE) - 1)
#define TT_EXACT 0x01
#define TT_ALPHA 0x02
#define TT_BETA  0x04

uint32 ttLo[TT_SIZE];
uint32 ttHi[TT_SIZE];
uint8  ttFlags[TT_SIZE];
int16  ttScore[TT_SIZE];
uint8  ttDepth[TT_SIZE];

void ttInit () {
  for (int i=0; i < TT_SIZE; i++)
    ttFlags[i] = 0;
}

void ttPut (int flags, int depth, int score) {

  const uint32 i = hLo & TT_MASK;

  ttLo[i] = hLo;
  ttHi[i] = hHi;

  ttFlags[i] = (uint8)flags;
  ttDepth[i] = (uint8)depth;
  ttScore[i] = (int16)score;
}

uint32 ttIndex () {

  const uint32 i = hLo & TT_MASK;

  if (ttFlags[i] && ttLo[i] == hLo && ttHi[i] == hHi)
    return i;
  else
    return 0;
}

/*}}}*/
/*{{{  eval*/

/*{{{  constants*/

int MATERIAL[] = {100,320,330,500,900,20000};

int WPAWN_PST[] = {0, 0, 0,  0,  0,   0,   0,   0,   0,  0,  0, 0,
                   0, 0, 0,  0,  0,   0,   0,   0,   0,  0,  0, 0,
                   0, 0, 0,  0,  0,   0,   0,   0,   0,  0,  0, 0,
                   0, 0, 50, 50, 50,  50,  50,  50,  50, 50, 0, 0,
                   0, 0, 10, 10, 20,  30,  30,  20,  10, 10, 0, 0,
                   0, 0, 5,  5,  10,  25,  25,  10,  5,  5,  0, 0,
                   0, 0, 0,  0,  0,   20,  20,  0,   0,  0,  0, 0,
                   0, 0, 5,  -5, -10, 0,   0,   -10, -5, 5,  0, 0,
                   0, 0, 5,  10, 10,  -20, -20, 10,  10, 5,  0, 0,
                   0, 0, 0,  0,  0,   0,   0,   0,   0,  0,  0, 0,
                   0, 0, 0,  0,  0,   0,   0,   0,   0,  0,  0, 0,
                   0, 0, 0,  0,  0,   0,   0,   0,   0,  0,  0, 0};

int WKNIGHT_PST[] = {0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
                     0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
                     0, 0, -50, -40, -30, -30, -30, -30, -40, -50, 0, 0,
                     0, 0, -40, -20, 0,   0,   0,   0,   -20, -40, 0, 0,
                     0, 0, -30, 0,   10,  15,  15,  10,   0,  -30, 0, 0,
                     0, 0, -30, 5,   15,  20,  20,  15,   5,  -30, 0, 0,
                     0, 0, -30, 0,   15,  20,  20,  15,   0,  -30, 0, 0,
                     0, 0, -30, 5,   10,  15,  15,  10,   5,  -30, 0, 0,
                     0, 0, -40, -20, 0,   5,   5,   0,   -20, -40, 0, 0,
                     0, 0, -50, -40, -30, -30, -30, -30, -40, -50, 0, 0,
                     0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
                     0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0};

int WBISHOP_PST[] = {0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
                     0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
                     0, 0, -20, -10, -10, -10, -10, -10, -10, -20, 0, 0,
                     0, 0, -10, 0,   0,   0,   0,   0,    0,  -10, 0, 0,
                     0, 0, -10, 0,   5,   10,  10,  5,    0,  -10, 0, 0,
                     0, 0, -10, 5,   5,   10,  10,  5,    5,  -10, 0, 0,
                     0, 0, -10, 0,   10,  10,  10,  10,   0,  -10, 0, 0,
                     0, 0, -10, 10,  10,  10,  10,  10,   10, -10, 0, 0,
                     0, 0, -10, 5 ,   0,   0,   0,   0,   5,  -10, 0, 0,
                     0, 0, -20, -10, -10, -10, -10, -10, -10, -20, 0, 0,
                     0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
                     0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0};
int WROOK_PST[] = {0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0,
                   0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0,
                   0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0,
                   0, 0, 5,   10,  10,  10,  10,  10,  10,  5,  0, 0,
                   0, 0, -5,  0,   0,   0,   0,   0,   0,   -5, 0, 0,
                   0, 0, -5,  0,   0,   0,   0,   0,   0,   -5, 0, 0,
                   0, 0, -5,  0,   0,   0,   0,   0,   0,   -5, 0, 0,
                   0, 0, -5,  0,   0,   0,   0,   0,   0,   -5, 0, 0,
                   0, 0, -5,  0,   0,   0,   0,   0,   0,   -5, 0, 0,
                   0, 0, 0,   0,   0,   5,   5,   0,   0,   0,  0, 0,
                   0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0,
                   0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0};

int WQUEEN_PST[] = {0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
                    0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
                    0, 0, -20, -10, -10, -5,  -5,  -10, -10, -20, 0, 0,
                    0, 0, -10, 0,   0,   0,   0,   0,    0,  -10, 0, 0,
                    0, 0, -10, 0,   5,   5,   5,   5,    0,  -10, 0, 0,
                    0, 0, -5,  0,   5,   5,   5,   5,    0,  -5,  0, 0,
                    0, 0,  0,  0,   5,   5,   5,   5,    0,  -5,  0, 0,
                    0, 0, -10, 5,   5,   5,   5,   5,    0,  -10, 0, 0,
                    0, 0, -10, 0,   5,   0,   0,   0,    0,  -10, 0, 0,
                    0, 0, -20, -10, -10, -5,  -5,  -10, -10, -20, 0, 0,
                    0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
                    0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0};

int WKING_MID_PST[] = {0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
                       0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
                       0, 0, -30, -40, -40, -50, -50, -40, -40, -30, 0, 0,
                       0, 0, -30, -40, -40, -50, -50, -40, -40, -30, 0, 0,
                       0, 0, -30, -40, -40, -50, -50, -40, -40, -30, 0, 0,
                       0, 0, -30, -40, -40, -50, -50, -40, -40, -30, 0, 0,
                       0, 0, -20, -30, -30, -40, -40, -30, -30, -20, 0, 0,
                       0, 0, -10, -20, -20, -20, -20, -20, -20, -10, 0, 0,
                       0, 0, 20,  20,  0,   0,   0,   0,   20,  20,  0, 0,
                       0, 0, 20,  30,  10,  0,   0,   10,  30,  20,  0, 0,
                       0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
                       0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0};

int WKING_END_PST[] = {0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0,
                       0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0,
                       0, 0, -50, -40, -30, -20, -20, -30, -40, -50,0, 0,
                       0, 0, -30, -20, -10, 0,   0,   -10, -20, -30,0, 0,
                       0, 0, -30, -10, 20,  30,  30,  20,  -10, -30,0, 0,
                       0, 0, -30, -10, 30,  40,  40,  30,  -10, -30,0, 0,
                       0, 0, -30, -10, 30,  40,  40,  30,  -10, -30,0, 0,
                       0, 0, -30, -10, 20,  30,  30,  20,  -10, -30,0, 0,
                       0, 0, -30, -30, 0,   0,   0,   0,   -30, -30,0, 0,
                       0, 0, -50, -30, -30, -30, -30, -30, -30, -50,0, 0,
                       0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0,
                       0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0};

int BPAWN_PST[144];
int BKNIGHT_PST[144];
int BBISHOP_PST[144];
int BROOK_PST[144];
int BQUEEN_PST[144];
int BKING_MID_PST[144];
int BKING_END_PST[144];

int *WHITE_MID_PST[] = {WPAWN_PST, WKNIGHT_PST, WBISHOP_PST, WROOK_PST, WQUEEN_PST, WKING_MID_PST};
int *WHITE_END_PST[] = {WPAWN_PST, WKNIGHT_PST, WBISHOP_PST, WROOK_PST, WQUEEN_PST, WKING_END_PST};
int *BLACK_MID_PST[] = {BPAWN_PST, BKNIGHT_PST, BBISHOP_PST, BROOK_PST, BQUEEN_PST, BKING_MID_PST};
int *BLACK_END_PST[] = {BPAWN_PST, BKNIGHT_PST, BBISHOP_PST, BROOK_PST, BQUEEN_PST, BKING_END_PST};

int **WB_MID_PST[] = {WHITE_MID_PST, BLACK_MID_PST};
int **WB_END_PST[] = {WHITE_END_PST, BLACK_END_PST};

/*}}}*/

/*{{{  evaluate*/

int evaluate () {

  int *b = bBoard;

  const int cx = colourMultiplier(bTurn);

  int e = 10 * cx;

  int pst_mid = 0;
  int pst_end = 0;

  int q = 0;

  for (int sq=0; sq<64; sq++) {

    const int fr    = B88[sq];
    const int frObj = b[fr];

    if (!frObj)
      continue;

    const int frPiece  = objPiece(frObj) - 1;
    const int frColour = objColour(frObj);
    const int frIndex  = colourIndex(frColour);
    const int frMult   = colourMultiplier(frColour);

    e += MATERIAL[frPiece] * frMult;

    pst_mid += WB_MID_PST[frIndex][frPiece][fr] * frMult;
    pst_end += WB_END_PST[frIndex][frPiece][fr] * frMult;

    q += IS_Q[frObj];
  }

  if (q)
    return (e + pst_mid) * cx;
  else
    return (e + pst_end) * cx;
}

/*}}}*/
/*{{{  flip*/

int flip (int sq) {
  const int m = (143-sq)/12;
  return 12*m + sq%12;
}

/*}}}*/
/*{{{  evalInitOnce*/

void evalInitOnce () {

  for (int i=0; i < 144; i++) {

    const int j = flip(i);

    BPAWN_PST[j]     = WPAWN_PST[i];
    BKNIGHT_PST[j]   = WKNIGHT_PST[i];
    BBISHOP_PST[j]   = WBISHOP_PST[i];
    BROOK_PST[j]     = WROOK_PST[i];
    BQUEEN_PST[j]    = WQUEEN_PST[i];
    BKING_MID_PST[j] = WKING_MID_PST[i];
    BKING_END_PST[j] = WKING_END_PST[i];
  }

}

/*}}}*/

/*}}}*/
/*{{{  cache*/

/*{{{  cacheStruct*/

struct cacheStruct {

  int bRights;
  int bEP;

  uint32 hLo;
  uint32 hHi;

  int hHistoryLimit;
};

/*}}}*/

struct cacheStruct cache[MAX_PLY];

/*{{{  cacheSave*/

void cacheSave () {

  struct cacheStruct *c = &cache[bPly];

  c->bRights       = bRights;
  c->bEP           = bEP;
  c->hLo           = hLo;
  c->hHi           = hHi;
  c->hHistoryLimit = hHistoryLimit;
}

/*}}}*/
/*{{{  cacheUnsave*/

void cacheUnsave () {

  struct cacheStruct *c = &cache[bPly];

  bRights       = c->bRights;
  bEP           = c->bEP;
  hLo           = c->hLo;
  hHi           = c->hHi;
  hHistoryLimit = c->hHistoryLimit;
}

/*}}}*/

/*}}}*/
/*{{{  movelist*/

/*{{{  constants*/

#define MAX_MOVES 256

#define ALL_MOVES   0
#define NOISY_MOVES 1

#define MOVE_TO_BITS      0
#define MOVE_FR_BITS      8
#define MOVE_TOOBJ_BITS   16
#define MOVE_FROBJ_BITS   20
#define MOVE_PROMAS_BITS  29

#define MOVE_TO_MASK       0x000000FF
#define MOVE_FR_MASK       0x0000FF00
#define MOVE_TOOBJ_MASK    0x000F0000
#define MOVE_FROBJ_MASK    0x00F00000
#define MOVE_KINGMOVE_MASK 0x01000000
#define MOVE_EPTAKE_MASK   0x02000000
#define MOVE_EPMAKE_MASK   0x04000000
#define MOVE_CASTLE_MASK   0x08000000
#define MOVE_PROMOTE_MASK  0x10000000
#define MOVE_PROMAS_MASK   0x60000000
#define MOVE_SPARE_MASK    0x80000000

#define MOVE_CAPTURE_MASK (MOVE_TOOBJ_MASK | MOVE_EPTAKE_MASK)
#define MOVE_IKKY_MASK    (MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | MOVE_PROMOTE_MASK | MOVE_EPTAKE_MASK | MOVE_EPMAKE_MASK)
#define MOVE_DRAW_MASK    (MOVE_TOOBJ_MASK | MOVE_CASTLE_MASK | MOVE_PROMOTE_MASK | MOVE_EPTAKE_MASK)

#define MOVE_E1G1 (MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | (W_KING << MOVE_FROBJ_BITS) | (E1 << MOVE_FR_BITS) | G1)
#define MOVE_E1C1 (MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | (W_KING << MOVE_FROBJ_BITS) | (E1 << MOVE_FR_BITS) | C1)
#define MOVE_E8G8 (MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | (B_KING << MOVE_FROBJ_BITS) | (E8 << MOVE_FR_BITS) | G8)
#define MOVE_E8C8 (MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | (B_KING << MOVE_FROBJ_BITS) | (E8 << MOVE_FR_BITS) | C8)

#define QPRO (((QUEEN-2)  << MOVE_PROMAS_BITS) | MOVE_PROMOTE_MASK)
#define RPRO (((ROOK-2)   << MOVE_PROMAS_BITS) | MOVE_PROMOTE_MASK)
#define BPRO (((BISHOP-2) << MOVE_PROMAS_BITS) | MOVE_PROMOTE_MASK)
#define NPRO (((KNIGHT-2) << MOVE_PROMAS_BITS) | MOVE_PROMOTE_MASK)

/*}}}*/

/*{{{  movelistStruct*/

struct movelistStruct {

  move_t quietMoves[MAX_MOVES];
  move_t noisyMoves[MAX_MOVES];

  int quietRanks[MAX_MOVES];
  int noisyRanks[MAX_MOVES];

  int quietNum;
  int noisyNum;
  int nextMove;
  int noisy;
  int stage;
};

/*}}}*/

struct movelistStruct movelist[MAX_PLY];

/*{{{  move primitives*/

int moveFromSq (move_t move) {
  return (int)((move & MOVE_FR_MASK) >> MOVE_FR_BITS);
}

int moveToSq (move_t move) {
  return (int)((move & MOVE_TO_MASK) >> MOVE_TO_BITS);
}

int moveToObj (move_t move) {
  return (int)((move & MOVE_TOOBJ_MASK) >> MOVE_TOOBJ_BITS);
}

int moveFromObj (move_t move) {
  return (int)((move & MOVE_FROBJ_MASK) >> MOVE_FROBJ_BITS);
}

int movePromotePiece (move_t move) {
  return (int)(((move & MOVE_PROMAS_MASK) >> MOVE_PROMAS_BITS) + 2);
}

/*}}}*/

/*{{{  initMoveGen*/

void initMoveGen (int noisy) {

  struct movelistStruct *ml = &movelist[bPly];

  ml->stage = 0;
  ml->noisy = noisy;

}

/*}}}*/
/*{{{  addQuiet*/

void addQuiet (move_t move) {

  struct movelistStruct *ml = &movelist[bPly];

  ml->quietMoves[ml->quietNum] = move;
  ml->quietRanks[ml->quietNum] = 100 + CENTRE[moveToSq(move)] - CENTRE[moveFromSq(move)];

  ml->quietNum = ml->quietNum + 1;
}

/*}}}*/
/*{{{  addNoisy*/
//
// Must handle EP captures.
//

const int RANK_ATTACKER[] = {0,    600,  500,  400,  300,  200,  100, 0, 0,    600,  500,  400,  300,  200,  100};
const int RANK_DEFENDER[] = {2000, 1000, 3000, 3000, 5000, 9000, 0,   0, 2000, 1000, 3000, 3000, 5000, 9000, 0};

void addNoisy (move_t move) {

  struct movelistStruct *ml = &movelist[bPly];

  ml->noisyMoves[ml->noisyNum] = move;
  ml->noisyRanks[ml->noisyNum] = RANK_ATTACKER[moveFromObj(move)] + RANK_DEFENDER[moveToObj(move)];

  ml->noisyNum = ml->noisyNum + 1;
}

/*}}}*/
/*{{{  genWhiteCastlingMoves*/

void genWhiteCastlingMoves () {

  int *b = bBoard;

  if ((bRights & WHITE_RIGHTS_KING)  && !b[F1]
                                     && !b[G1]
                                     && b[G2] != B_KING
                                     && b[H2] != B_KING
                                     && !isKingAttacked(E1,BLACK)
                                     && !isKingAttacked(F1,BLACK)) {
    addQuiet(MOVE_E1G1);
  }

  if ((bRights & WHITE_RIGHTS_QUEEN) && !b[B1]
                                     && !b[C1]
                                     && !b[D1]
                                     && b[B2] != B_KING
                                     && b[C2] != B_KING
                                     && !isKingAttacked(E1,BLACK)
                                     && !isKingAttacked(D1,BLACK)) {
    addQuiet(MOVE_E1C1);
  }
}

/*}}}*/
/*{{{  genBlackCastlingMoves*/

void genBlackCastlingMoves () {

  int *b = bBoard;

  if ((bRights & BLACK_RIGHTS_KING)  && b[F8] == 0
                                     && b[G8] == 0
                                     && b[G7] != W_KING
                                     && b[H7] != W_KING
                                     && !isKingAttacked(E8,WHITE)
                                     && !isKingAttacked(F8,WHITE)) {
    addQuiet(MOVE_E8G8);
  }

  if ((bRights & BLACK_RIGHTS_QUEEN) && b[B8] == 0
                                     && b[C8] == 0
                                     && b[D8] == 0
                                     && b[B7] != W_KING
                                     && b[C7] != W_KING
                                     && !isKingAttacked(E8,WHITE)
                                     && !isKingAttacked(D8,WHITE)) {
    addQuiet(MOVE_E8C8);
  }
}

/*}}}*/
/*{{{  genPawnMoves*/

void genPawnMoves (move_t frMove) {

  int *b = bBoard;

  const int fr           = moveFromSq(frMove);
  const int cx           = colourIndex(bTurn);
  const int *CAN_CAPTURE = WB_CAN_CAPTURE[cx];
  const int OFFSET_ORTH  = WB_OFFSET_ORTH[cx];
  const int OFFSET_DIAG1 = WB_OFFSET_DIAG1[cx];
  const int OFFSET_DIAG2 = WB_OFFSET_DIAG2[cx];

  int to, toObj;

  to = fr + OFFSET_ORTH;
  if (!b[to])
    addQuiet(frMove | to);

  to    = fr + OFFSET_DIAG1;
  toObj = b[to];
  if (CAN_CAPTURE[toObj])
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to);

  to = fr + OFFSET_DIAG2;
  toObj = b[to];
  if (CAN_CAPTURE[toObj])
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to);
}

/*}}}*/
/*{{{  genEnPassPawnMoves*/

void genEnPassPawnMoves (move_t frMove) {

  int *b = bBoard;

  const int fr           = moveFromSq(frMove);
  const int cx           = colourIndex(bTurn);
  const int OFFSET_DIAG1 = WB_OFFSET_DIAG1[cx];
  const int OFFSET_DIAG2 = WB_OFFSET_DIAG2[cx];

  int to;

  to = fr + OFFSET_DIAG1;
  if (to == bEP && !b[to])
    addNoisy(frMove | to | MOVE_EPTAKE_MASK);

  to = fr + OFFSET_DIAG2;
  if (to == bEP && !b[to])
    addNoisy(frMove | to | MOVE_EPTAKE_MASK);
}

/*}}}*/
/*{{{  genHomePawnMoves*/

void genHomePawnMoves (move_t frMove) {

  int *b = bBoard;

  const int fr           = moveFromSq(frMove);
  const int cx           = colourIndex(bTurn);
  const int *CAN_CAPTURE = WB_CAN_CAPTURE[cx];
  const int OFFSET_ORTH  = WB_OFFSET_ORTH[cx];
  const int OFFSET_DIAG1 = WB_OFFSET_DIAG1[cx];
  const int OFFSET_DIAG2 = WB_OFFSET_DIAG2[cx];

  int to, toObj;

  to = fr + OFFSET_ORTH;
  if (!b[to]) {
    addQuiet(frMove | to);
    to += OFFSET_ORTH;
    if (!b[to])
      addQuiet(frMove | to | MOVE_EPMAKE_MASK);
  }

  to    = fr + OFFSET_DIAG1;
  toObj = b[to];
  if (CAN_CAPTURE[toObj])
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to);

  to    = fr + OFFSET_DIAG2;
  toObj = b[to];
  if (CAN_CAPTURE[toObj])
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to);
}

/*}}}*/
/*{{{  genPromotePawnMoves*/

void genPromotePawnMoves (move_t frMove) {

  int *b = bBoard;

  const int fr           = moveFromSq(frMove);
  const int cx           = colourIndex(bTurn);
  const int *CAN_CAPTURE = WB_CAN_CAPTURE[cx];
  const int OFFSET_ORTH  = WB_OFFSET_ORTH[cx];
  const int OFFSET_DIAG1 = WB_OFFSET_DIAG1[cx];
  const int OFFSET_DIAG2 = WB_OFFSET_DIAG2[cx];

  int to, toObj;

  to = fr + OFFSET_ORTH;
  if (!b[to]) {
    addQuiet(frMove | to | QPRO);
    addQuiet(frMove | to | RPRO);
    addQuiet(frMove | to | BPRO);
    addQuiet(frMove | to | NPRO);
  }

  to    = fr + OFFSET_DIAG1;
  toObj = b[to];
  if (CAN_CAPTURE[toObj]) {
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | QPRO);
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | RPRO);
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | BPRO);
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | NPRO);
  }

  to    = fr + OFFSET_DIAG2;
  toObj = b[to];
  if (CAN_CAPTURE[toObj]) {
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | QPRO);
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | RPRO);
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | BPRO);
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | NPRO);
  }
}

/*}}}*/
/*{{{  genKingMoves*/

void genKingMoves (move_t frMove) {

  int *b = bBoard;

  const int fr           = moveFromSq(frMove);
  const int cx           = colourIndex(bTurn);
  const int cy           = colourIndex(colourToggle(bTurn));
  const int *CAN_CAPTURE = WB_CAN_CAPTURE[cx];
  const int theirKingSq  = bKings[cy];

  int dir = 0;

  while (dir < 8) {

    const int to = fr + KING_OFFSETS[dir++];

    if (!ADJACENT[abs(to-theirKingSq)]) {

      const int toObj = b[to];

      if (!toObj)
        addQuiet(frMove | to | MOVE_KINGMOVE_MASK);

      else if (CAN_CAPTURE[toObj])
        addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | MOVE_KINGMOVE_MASK);
    }
  }
}

/*}}}*/
/*{{{  genKnightMoves*/

void genKnightMoves (move_t frMove) {

  int *b = bBoard;

  const int fr           = moveFromSq(frMove);
  const int cx           = colourIndex(bTurn);
  const int *CAN_CAPTURE = WB_CAN_CAPTURE[cx];

  int dir = 0;

  while (dir < 8) {

    const int to    = fr + KNIGHT_OFFSETS[dir++];
    const int toObj = b[to];

    if (!toObj)
      addQuiet(frMove | to);

    else if (CAN_CAPTURE[toObj])
      addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to);
  }
}

/*}}}*/
/*{{{  genBishopMoves*/

void genBishopMoves (move_t frMove) {

  int *b = bBoard;

  const int fr           = moveFromSq(frMove);
  const int cx           = colourIndex(bTurn);
  const int *CAN_CAPTURE = WB_CAN_CAPTURE[cx];

  int dir = 0;

  while (dir < 4) {

    const int offset = BISHOP_OFFSETS[dir++];

    int to = fr + offset;
    while (!b[to]) {
      addQuiet(frMove | to);
      to += offset;
    }

    const int toObj = b[to];
    if (CAN_CAPTURE[toObj])
      addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to);
  }
}

/*}}}*/
/*{{{  genRookMoves*/

void genRookMoves (move_t frMove) {

  int *b = bBoard;

  const int fr           = moveFromSq(frMove);
  const int cx           = colourIndex(bTurn);
  const int *CAN_CAPTURE = WB_CAN_CAPTURE[cx];

  int dir = 0;

  while (dir < 4) {

    const int offset = ROOK_OFFSETS[dir++];

    int to = fr + offset;
    while (!b[to]) {
      addQuiet(frMove | to);
      to += offset;
    }

    const int toObj = b[to];
    if (CAN_CAPTURE[toObj])
      addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to);
  }
}

/*}}}*/
/*{{{  genQueenMoves*/

void genQueenMoves (move_t frMove) {

  int *b = bBoard;

  const int fr           = moveFromSq(frMove);
  const int cx           = colourIndex(bTurn);
  const int *CAN_CAPTURE = WB_CAN_CAPTURE[cx];

  int dir = 0;

  while (dir < 8) {

    const int offset = QUEEN_OFFSETS[dir++];

    int to = fr + offset;
    while (!b[to]) {
      addQuiet(frMove | to);
      to += offset;
    }

    const int toObj = b[to];
    if (CAN_CAPTURE[toObj])
      addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to);
  }
}

/*}}}*/
/*{{{  genMoves*/

void genMoves () {

  int *b = bBoard;

  const int cx           = colourIndex(bTurn);
  const int *OUR_PIECE   = WB_OUR_PIECE[cx];
  const int HOME_RANK    = WB_HOME_RANK[cx];
  const int PROMOTE_RANK = WB_PROMOTE_RANK[cx];
  const int EP_RANK      = WB_EP_RANK[cx];

  if (bTurn == WHITE)
    genWhiteCastlingMoves();
  else
    genBlackCastlingMoves();

  for (int i=0; i<64; i++) {

    const int fr    = B88[i];
    const int frObj = b[fr];

    if (!OUR_PIECE[frObj])
      continue;

    const int frPiece   = objPiece(frObj);
    const int frRank    = RANK[fr];
    const move_t frMove = (move_t)((frObj << MOVE_FROBJ_BITS) | (fr << MOVE_FR_BITS));

    switch (frPiece) {

      case KING:
        genKingMoves(frMove);
        break;

      case PAWN:
        if (frRank == HOME_RANK) {
          genHomePawnMoves(frMove);
        }
        else if (frRank == PROMOTE_RANK) {
          genPromotePawnMoves(frMove);
        }
        else if (frRank == EP_RANK) {
          genPawnMoves(frMove);
          if (bEP)
            genEnPassPawnMoves(frMove);
        }
        else {
          genPawnMoves(frMove);
        }
        break;

      case ROOK:
        genRookMoves(frMove);
        break;

      case KNIGHT:
        genKnightMoves(frMove);
        break;

      case BISHOP:
        genBishopMoves(frMove);
        break;

      case QUEEN:
        genQueenMoves(frMove);
        break;
    }
  }
}

/*}}}*/
/*{{{  nextStagedMove*/

move_t nextStagedMove (int next, int num, move_t *moves, int *ranks) {

  int maxR = -10000;
  int maxI = 0;

  for (int i=next; i < num; i++) {
    if (ranks[i] > maxR) {
      maxR = ranks[i];
      maxI = i;
    }
  }

  const move_t maxM = moves[maxI];

  moves[maxI] = moves[next];
  ranks[maxI] = ranks[next];

  return maxM;
}

/*}}}*/
/*{{{  getNextMove*/

move_t getNextMove () {

  struct movelistStruct *ml = &movelist[bPly];

  switch (ml->stage) {

    case 0:

      ml->noisyNum = 0;
      ml->quietNum = 0;

      genMoves();

      ml->nextMove = 0;
      ml->stage++;

    case 1:

      if (ml->nextMove < ml->noisyNum)
        return nextStagedMove(ml->nextMove++, ml->noisyNum, ml->noisyMoves, ml->noisyRanks);

      if (ml->noisy)
        return 0;

      ml->nextMove = 0;
      ml->stage++;

    case 2:

      if (ml->nextMove < ml->quietNum)
        return nextStagedMove(ml->nextMove++, ml->quietNum, ml->quietMoves, ml->quietRanks);

      return 0;

    default:
      fprintf(stderr,"get next move stage is %d\n",ml->stage);
      return 0;
  }
}

/*}}}*/
/*{{{  formatMove*/

char *formatMove (move_t move) {

  static char nm[6] = "nullm";
  static char fm[6];

  if (!move)
    return nm;

  const int fr = moveFromSq(move);
  const int to = moveToSq(move);

  const char *frCoord = COORDS[fr];
  const char *toCoord = COORDS[to];

  fm[0] = frCoord[0];
  fm[1] = frCoord[1];
  fm[2] = toCoord[0];
  fm[3] = toCoord[1];

  if (move & MOVE_PROMOTE_MASK) {
    fm[4] = OBJ_CHAR[movePromotePiece(move)|BLACK];  // sic
    fm[5] = '\0';
  }
  else {
    fm[4] = '\0';
  }

  return fm;
}

/*}}}*/

/*{{{  makeIkkyMove*/

void makeIkkyMove (move_t move) {

  int *b = bBoard;

  const int to    = moveToSq(move);
  const int frObj = moveFromObj(move);
  const int frCol = objColour(frObj);

  if (frCol == WHITE) {
    /*{{{  white*/
    
    if (move & MOVE_KINGMOVE_MASK)
    
      bKings[0] = to;
    
    if (move & MOVE_EPMAKE_MASK) {
    
      hashEP(bEP);
      bEP = to+12;
      hashEP(bEP);
    
    }
    
    else if (move & MOVE_EPTAKE_MASK) {
    
      hashObj(b[to+12],to+12);
      b[to+12] = 0;
      hashObj(b[to+12],to+12);
    
    }
    
    else if (move & MOVE_PROMOTE_MASK) {
    
      hashObj(b[to],to);
      b[to] = movePromotePiece(move) | WHITE;
      hashObj(b[to],to);
    
    }
    
    else if (move == MOVE_E1G1) {
    
      hashObj(b[H1],H1);
      b[H1] = 0;
      hashObj(b[H1],H1);
    
      hashObj(b[F1],F1);
      b[F1] = W_ROOK;
      hashObj(b[F1],F1);
    
    }
    
    else if (move == MOVE_E1C1) {
    
      hashObj(b[A1],A1);
      b[A1] = 0;
      hashObj(b[A1],A1);
    
      hashObj(b[D1],D1);
      b[D1] = W_ROOK;
      hashObj(b[D1],D1);
    
    }
    
    /*}}}*/
  }

  else {
    /*{{{  black*/
    
    if (move & MOVE_KINGMOVE_MASK)
    
      bKings[1] = to;
    
    if (move & MOVE_EPMAKE_MASK) {
    
      hashEP(bEP);
      bEP = to-12;
      hashEP(bEP);
    
    }
    
    else if (move & MOVE_EPTAKE_MASK) {
    
      hashObj(b[to-12],to-12);
      b[to-12] = 0;
      hashObj(b[to-12],to-12);
    
    }
    
    else if (move & MOVE_PROMOTE_MASK) {
    
      hashObj(b[to],to);
      b[to] = movePromotePiece(move) | BLACK;
      hashObj(b[to],to);
    
    }
    
    else if (move == MOVE_E8G8) {
    
      hashObj(b[H8],H8);
      b[H8] = 0;
      hashObj(b[H8],H8);
    
      hashObj(b[F8],F8);
      b[F8] = B_ROOK;
      hashObj(b[F8],F8);
    
    }
    
    else if (move == MOVE_E8C8) {
    
      hashObj(b[A8],A8);
      b[A8] = 0;
      hashObj(b[A8],A8);
    
      hashObj(b[D8],D8);
      b[D8] = B_ROOK;
      hashObj(b[D8],D8);
    
    }
    
    /*}}}*/
  }
}

/*}}}*/
/*{{{  makeMove*/

void makeMove (move_t move) {

  int *b = bBoard;

  const int fr    = moveFromSq(move);
  const int to    = moveToSq(move);
  const int frObj = moveFromObj(move);
  const int toObj = moveToObj(move);

  hLoHistory[bPly+hHistoryOffset] = hLo;
  hHiHistory[bPly+hHistoryOffset] = hHi;

  hashObj(frObj,fr);
  b[fr] = 0;
  hashObj(0,fr);

  hashObj(toObj,to);
  b[to] = frObj;
  hashObj(frObj,to);

  hashTurn(bTurn);
  bTurn = colourToggle(bTurn);
  hashTurn(bTurn);

  hashRights(bRights);
  bRights &= MASK_RIGHTS[fr] & MASK_RIGHTS[to];
  hashRights(bRights);

  hashEP(bEP);
  bEP = 0;
  hashEP(bEP);

  if (move & MOVE_IKKY_MASK)
    makeIkkyMove(move);

  bPly++;

  if ((move & MOVE_DRAW_MASK) || objPiece(frObj) == PAWN) {
    hHistoryLimit = bPly+hHistoryOffset;
  }
}

/*}}}*/
/*{{{  unmakeIkkyMove*/

void unmakeIkkyMove (move_t move) {

  int *b = bBoard;

  const int fr    = moveFromSq(move);
  const int to    = moveToSq(move);
  const int frObj = moveFromObj(move);
  const int frCol = objColour(frObj);

  if (frCol == WHITE) {
    /*{{{  white*/
    
    if (move & MOVE_KINGMOVE_MASK)
    
      bKings[0] = fr;
    
    if (move & MOVE_EPTAKE_MASK) {
    
      b[to+12] = B_PAWN;
    
    }
    
    else if (move == MOVE_E1G1) {
    
      b[H1] = W_ROOK;
      b[F1] = 0;
    
    }
    
    else if (move == MOVE_E1C1) {
    
      b[A1] = W_ROOK;
      b[D1] = 0;
    
    }
    
    /*}}}*/
  }

  else {
    /*{{{  black*/
    
    if (move & MOVE_KINGMOVE_MASK)
    
      bKings[1] = fr;
    
    if (move & MOVE_EPTAKE_MASK) {
    
      b[to-12] = W_PAWN;
    
    }
    
    else if (move == MOVE_E8G8) {
    
      b[H8] = B_ROOK;
      b[F8] = 0;
    
    }
    
    else if (move == MOVE_E8C8) {
    
      b[A8] = B_ROOK;
      b[D8] = 0;
    
    }
    
    /*}}}*/
  }
}

/*}}}*/
/*{{{  unmakeMove*/

void unmakeMove (move_t move) {

  int *b = bBoard;

  const int fr    = moveFromSq(move);
  const int to    = moveToSq(move);
  const int toObj = moveToObj(move);
  const int frObj = moveFromObj(move);

  b[fr] = frObj;
  b[to] = toObj;

  if (move & MOVE_IKKY_MASK)
    unmakeIkkyMove(move);

  bTurn = colourToggle(bTurn);
  bPly--;
}

/*}}}*/

/*{{{  playMove*/

void playMove (char *uciMove) {

  initMoveGen(ALL_MOVES);

  move_t move = 0;

  while ((move = getNextMove())) {

    if (!strcmp(formatMove(move),uciMove)) {
      makeMove(move);
      hHistoryOffset++;
      return;
    }
  }

  fprintf(stderr,"cannot play uci move %s\n", uciMove);
}

/*}}}*/

/*}}}*/
/*{{{  search*/

/*{{{  newGame*/

void newGame () {
  ttInit();
}

/*}}}*/
/*{{{  perft*/

uint32 perft (int depth) {

  if (depth == 0)
    return 1;

  const int turn      = bTurn;
  const int nextTurn  = colourToggle(turn);
  const int cx        = colourIndex(turn);

  uint32 count = 0;
  move_t move;

  cacheSave();
  initMoveGen(ALL_MOVES);

  while ((move = getNextMove())) {

    makeMove(move);

    if (isKingAttacked(bKings[cx], nextTurn)) {
      /*{{{  illegal move*/
      
      unmakeMove(move);
      cacheUnsave();
      
      continue;
      
      /*}}}*/
    }

    count += perft(depth-1);

    unmakeMove(move);
    cacheUnsave();
  }

  return count;
}

/*}}}*/
/*{{{  qsearch*/

int qsearch (int alpha, int beta, int depth) {

  /*{{{  housekeeping*/
  
  tNodes++;
  
  if (bPly == MAX_PLY) {
  
    tDone = 1;
  
    return evaluate();
  }
  
  /*}}}*/

  /*{{{  check TT*/
  
  const int i = ttIndex();
  
  if (i) {
    if (ttFlags[i] == TT_EXACT || (ttFlags[i] == TT_BETA && ttScore[i] >= beta) || (ttFlags[i] == TT_ALPHA && ttScore[i] <= alpha)) {
      return ttScore[i];
    }
  }
  
  /*}}}*/

  const int e = evaluate();

  if (e >= beta)
    return e;

  if (alpha < e)
    alpha = e;

  const int turn     = bTurn;
  const int nextTurn = colourToggle(turn);
  const int cx       = colourIndex(turn);

  move_t move;

  int score  = 0;
  int played = 0;

  cacheSave();
  initMoveGen(NOISY_MOVES);

  while ((move = getNextMove())) {

    makeMove(move);

    if (isKingAttacked(bKings[cx], nextTurn)) {
      /*{{{  illegal move*/
      
      unmakeMove(move);
      cacheUnsave();
      
      continue;
      
      /*}}}*/
    }

    played++;

    score = -qsearch(-beta, -alpha, depth-1);

    unmakeMove(move);
    cacheUnsave();

    if (tDone)
      return 0;

    if (score > alpha) {
      alpha = score;
      if (score >= beta) {
        return score;
      }
    }
  }

  return alpha;
}

/*}}}*/
/*{{{  search*/

int search (int alpha, int beta, int depth) {

  /*{{{  housekeeping*/
  
  tNodes++;
  
  if (areWeDone() || bPly == MAX_PLY) {
  
    tDone = 1;
  
    return 0;
  }
  
  /*}}}*/

  const int turn     = bTurn;
  const int nextTurn = colourToggle(turn);
  const int cx       = colourIndex(turn);
  const int inCheck  = isKingAttacked(bKings[cx], nextTurn);

  if (depth <= 0 && !inCheck)
    return qsearch(alpha, beta, depth);

  depth = MAX(0, depth);

  const int pvNode = alpha != (beta - 1);

  /*{{{  check TT*/
  
  const int i = ttIndex();
  
  if (i) {
    if (ttDepth[i] >= depth && (depth == 0 || !pvNode)) {
      if (ttFlags[i] == TT_EXACT || (ttFlags[i] == TT_BETA && ttScore[i] >= beta) || (ttFlags[i] == TT_ALPHA && ttScore[i] <= alpha)) {
        return ttScore[i];
      }
    }
  }
  
  /*}}}*/

  if (hashIsDraw())
    return 0;

  const int oAlpha   = alpha;
  const int rootNode = bPly == 0;

  move_t move;

  int bestScore = -MATE;
  int bestMove  = 0;
  int score     = 0;
  int played    = 0;

  cacheSave();
  initMoveGen(ALL_MOVES);

  while ((move = getNextMove())) {

    makeMove(move);

    if (isKingAttacked(bKings[cx], nextTurn)) {
      /*{{{  illegal move*/
      
      unmakeMove(move);
      cacheUnsave();
      
      continue;
      
      /*}}}*/
    }

    played++;

    score = -search(-beta, -alpha, depth-1);

    unmakeMove(move);
    cacheUnsave();

    if (tDone)
      return 0;

    if (score > bestScore) {
      bestScore = score;
      bestMove  = move;
      if (bestScore > alpha) {
        alpha = bestScore;
        if (rootNode) {
          tBestMove = bestMove;
        }
        if (bestScore >= beta) {
          ttPut(TT_BETA, depth, bestScore);
          return bestScore;
        }
      }
    }
  }

  if (!played)
    return inCheck ? -MATE + bPly : 0;

  if (alpha > oAlpha) {
    ttPut(TT_EXACT, depth, bestScore);
  }
  else {
    ttPut(TT_ALPHA, depth, bestScore);
  }

  return bestScore;
}

/*}}}*/
/*{{{  go*/

void go () {

  bPly = 0;

  tBestMove = 0;
  tDone     = 0;
  tNodes    = 0;

  int score = 0;
  int alpha = 0;
  int beta  = 0;
  int delta = 0;

  for (int depth=1; depth <= tTargetDepth; depth++) {

    alpha = -MATE;
    beta  = MATE;
    delta = 10;

    if (depth >= 4) {
      alpha = MAX(-MATE, score - delta);
      beta  = MIN(MATE,  score + delta);
    }

    while (1) {

      score = search(alpha, beta, depth);

      if (tDone)
        break;

      if (score > alpha && score < beta) {
        if (!mSilent)
          printf("info depth %d nodes %u score %d pv %s\n", depth, tNodes, score, formatMove(tBestMove));
        break;
      }

      delta += delta / 2;

      if (score <= alpha) {
        if (!mSilent)
          printf("info depth %d nodes %u lowerbound %d\n", depth, tNodes, score);
        beta  = MIN(MATE, ((alpha + beta) / 2));
        alpha = MAX(-MATE, score - delta);
        //tBestMove = 0;
      }
      else if (score >= beta) {
        if (!mSilent)
          printf("info depth %d nodes %u upperbound %d\n", depth, tNodes, score);
        alpha = MAX(-MATE, ((alpha + beta) / 2));
        beta  = MIN(MATE,  score + delta);
      }
    }

    if (tDone)
      break;
  }

  if (!mSilent)
    printf("bestmove %s\n", formatMove(tBestMove));
}

/*}}}*/

/*}}}*/
/*{{{  uci*/

#define MAX_LINE_LENGTH 8192
#define MAX_TOKENS 8192

/*{{{  uciTokens*/

int uciTokens(int n, char **tokens) {

  char *cmd = tokens[0];
  char *sub = tokens[1];

  if (!strcmp(cmd,"position") || !strcmp(cmd,"p")) {
    /*{{{  position*/
    
    if (!strcmp(sub,"startpos") || !strcmp(sub,"s")) {
    
      position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-");
    
      if (n > 2 && !strcmp(tokens[2],"moves")) {
        for (int i=3; i < n; i++)
          playMove(tokens[i]);
      }
    }
    
    else if (!strcmp(sub,"fen")) {
    
      position(tokens[2], tokens[3], tokens[4], tokens[5]);
    
      if (n > 8 && !strcmp(tokens[8],"moves")) {
        for (int i=9; i < n; i++)
          playMove(tokens[i]);
      }
    }
    
    /*}}}*/
  }

  else if (!strcmp(cmd,"ucinewgame") || !strcmp(cmd,"u")) {
    /*{{{  ucinewgame*/
    
    newGame();
    
    /*}}}*/
  }

  else if (!strcmp(cmd,"isready")) {
    /*{{{  isready*/
    
    printf("readyok\n");
    
    /*}}}*/
  }

  else if (!strcmp(cmd,"go") || !strcmp(cmd,"g")) {
    /*{{{  go*/
    
    const int slop = 1;
    
    int wTime     = 0;
    int bTime     = 0;
    int wInc      = 0;
    int bInc      = 0;
    int moveTime  = 0;
    int movesToGo = 0;
    int depth     = 0;
    
    uint32 nodes = 0;
    
    int i = 1;
    
    while (i < n) {
      /*{{{  go args*/
      
      if (!strcmp(tokens[i],"depth") || !strcmp(tokens[i],"d")) {
        depth = atoi(tokens[i+1]);
        i += 2;
      }
      
      else if (!strcmp(tokens[i],"nodes")) {
        nodes = atoi(tokens[i+1]);
        i += 2;
      }
      
      else if (!strcmp(tokens[i],"movestogo")) {
        movesToGo = atoi(tokens[i+1]);
        i += 2;
      }
      
      else if (!strcmp(tokens[i],"movetime")) {
        moveTime = atoi(tokens[i+1]);
        i += 2;
      }
      
      else if (!strcmp(tokens[i],"winc")) {
        wInc = atoi(tokens[i+1]);
        i += 2;
      }
      
      else if (!strcmp(tokens[i],"binc")) {
        bInc = atoi(tokens[i+1]);
        i += 2;
      }
      
      else if (!strcmp(tokens[i],"wtime")) {
        wTime = atoi(tokens[i+1]);
        i += 2;
      }
      
      else if (!strcmp(tokens[i],"btime")) {
        bTime = atoi(tokens[i+1]);
        i += 2;
      }
      
      else {
        fprintf(stderr,"unknown go token %s\n", tokens[i]);
        i++;
      }
      
      /*}}}*/
    }
    
    if (depth)
      tTargetDepth = depth;
    else
      tTargetDepth = MAX_PLY - 1;
    
    if (nodes)
      tTargetNodes = nodes;
    else
      tTargetNodes = 0;
    
    if (moveTime > 0)
      tFinishTime = clock() + moveTime + slop;
    
    else {
    
      if (movesToGo)
        movesToGo += 2;
      else
        movesToGo = 30;
    
      if (wTime && bTurn == WHITE)
        tFinishTime = clock() + 0.95 * (wTime/movesToGo + wInc) + slop;
      else if (bTime && bTurn == BLACK)
        tFinishTime = clock() + 0.95 * (bTime/movesToGo + bInc) + slop;
      else
        tFinishTime = 0;
    }
    
    go();
    
    /*}}}*/
  }

  else if (!strcmp(cmd,"uci")) {
    /*{{{  uci*/
    
    printf("id name clozza 1\n");
    printf("id author Colin Jenkins\n");
    printf("uciok\n");
    
    /*}}}*/
  }

  else if (!strcmp(cmd,"b")) {
    /*{{{  b*/
    
    printBoard();
    
    /*}}}*/
  }

  else if (!strcmp(cmd,"e")) {
    /*{{{  e*/
    
    const int e = evaluate();
    
    printf("%d\n",e);
    
    /*}}}*/
  }

  else if (!strcmp(cmd,"q")) {
    /*{{{  q*/
    
    return 1;
    
    /*}}}*/
  }

  else if (!strcmp(cmd,"perft")) {
    /*{{{  perft*/
    
    const int depth     = atoi(sub);
    const uint32 t      = clock();
    const uint32 pmoves = perft(depth);
    
    printf("%u moves %u ms\n", pmoves, clock()-t);
    
    /*}}}*/
  }

  else if (!strcmp(cmd,"pt")) {
    /*{{{  pt*/
    
    /*{{{  testfens*/
    
    typedef struct {
      char   *fen;
      char   *turn;
      char   *rights;
      char   *ep;
      int    depth;
      uint32 moves;
      char   *id;
    } perftTestStruct;
    
    perftTestStruct pFens[] = {
      {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",            "w", "KQkq", "-",  2, 400,       "cpw-pos1-2"},
      {"4k3/8/8/8/8/8/R7/R3K2R",                                 "w", "Q",    "-",  3, 4729,      "castling-2"},
      {"4k3/8/8/8/8/8/R7/R3K2R",                                 "w", "K",    "-",  3, 4686,      "castling-3"},
      {"4k3/8/8/8/8/8/R7/R3K2R",                                 "w", "-",    "-",  3, 4522,      "castling-4"},
      {"r3k2r/r7/8/8/8/8/8/4K3",                                 "b", "kq",   "-",  3, 4893,      "castling-5"},
      {"r3k2r/r7/8/8/8/8/8/4K3",                                 "b", "q",    "-",  3, 4729,      "castling-6"},
      {"r3k2r/r7/8/8/8/8/8/4K3",                                 "b", "k",    "-",  3, 4686,      "castling-7"},
      {"r3k2r/r7/8/8/8/8/8/4K3",                                 "b", "-",    "-",  3, 4522,      "castling-8"},
      {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",            "w", "KQkq", "-",  0, 1,         "cpw-pos1-0"},
      {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",            "w", "KQkq", "-",  1, 20,        "cpw-pos1-1"},
      {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",            "w", "KQkq", "-",  3, 8902,      "cpw-pos1-3"},
      {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",            "w", "KQkq", "-",  4, 197281,    "cpw-pos1-4"},
      {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",            "w", "KQkq", "-",  5, 4865609,   "cpw-pos1-5"},
      {"rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R",       "w", "KQkq", "-",  1, 42,        "cpw-pos5-1"},
      {"rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R",       "w", "KQkq", "-",  2, 1352,      "cpw-pos5-2"},
      {"rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R",       "w", "KQkq", "-",  3, 53392,     "cpw-pos5-3"},
      {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R","w", "KQkq", "-",  1, 48,        "cpw-pos2-1"},
      {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R","w", "KQkq", "-",  2, 2039,      "cpw-pos2-2"},
      {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R","w", "KQkq", "-",  3, 97862,     "cpw-pos2-3"},
      {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8",                        "w", "-",    "-",  5, 674624,    "cpw-pos3-5"},
      {"n1n5/PPPk4/8/8/8/8/4Kppp/5N1N",                          "b", "-",    "-",  1, 24,        "prom-1    "},
      {"8/5bk1/8/2Pp4/8/1K6/8/8",                                "w", "-",    "d6", 6, 824064,    "ccc-1     "},
      {"8/8/1k6/8/2pP4/8/5BK1/8",                                "b", "-",    "d3", 6, 824064,    "ccc-2     "},
      {"8/8/1k6/2b5/2pP4/8/5K2/8",                               "b", "-",    "d3", 6, 1440467,   "ccc-3     "},
      {"8/5k2/8/2Pp4/2B5/1K6/8/8",                               "w", "-",    "d6", 6, 1440467,   "ccc-4     "},
      {"5k2/8/8/8/8/8/8/4K2R",                                   "w", "K",    "-",  6, 661072,    "ccc-5     "},
      {"4k2r/8/8/8/8/8/8/5K2",                                   "b", "k",    "-",  6, 661072,    "ccc-6     "},
      {"3k4/8/8/8/8/8/8/R3K3",                                   "w", "Q",    "-",  6, 803711,    "ccc-7     "},
      {"r3k3/8/8/8/8/8/8/3K4",                                   "b", "q",    "-",  6, 803711,    "ccc-8     "},
      {"r3k2r/1b4bq/8/8/8/8/7B/R3K2R",                           "w", "KQkq", "-",  4, 1274206,   "ccc-9     "},
      {"r3k2r/7b/8/8/8/8/1B4BQ/R3K2R",                           "b", "KQkq", "-",  4, 1274206,   "ccc-10    "},
      {"r3k2r/8/3Q4/8/8/5q2/8/R3K2R",                            "b", "KQkq", "-",  4, 1720476,   "ccc-11    "},
      {"r3k2r/8/5Q2/8/8/3q4/8/R3K2R",                            "w", "KQkq", "-",  4, 1720476,   "ccc-12    "},
      {"2K2r2/4P3/8/8/8/8/8/3k4",                                "w", "-",    "-",  6, 3821001,   "ccc-13    "},
      {"3K4/8/8/8/8/8/4p3/2k2R2",                                "b", "-",    "-",  6, 3821001,   "ccc-14    "},
      {"8/8/1P2K3/8/2n5/1q6/8/5k2",                              "b", "-",    "-",  5, 1004658,   "ccc-15    "},
      {"5K2/8/1Q6/2N5/8/1p2k3/8/8",                              "w", "-",    "-",  5, 1004658,   "ccc-16    "},
      {"4k3/1P6/8/8/8/8/K7/8",                                   "w", "-",    "-",  6, 217342,    "ccc-17    "},
      {"8/k7/8/8/8/8/1p6/4K3",                                   "b", "-",    "-",  6, 217342,    "ccc-18    "},
      {"8/P1k5/K7/8/8/8/8/8",                                    "w", "-",    "-",  6, 92683,     "ccc-19    "},
      {"8/8/8/8/8/k7/p1K5/8",                                    "b", "-",    "-",  6, 92683,     "ccc-20    "},
      {"K1k5/8/P7/8/8/8/8/8",                                    "w", "-",    "-",  6, 2217,      "ccc-21    "},
      {"8/8/8/8/8/p7/8/k1K5",                                    "b", "-",    "-",  6, 2217,      "ccc-22    "},
      {"8/k1P5/8/1K6/8/8/8/8",                                   "w", "-",    "-",  7, 567584,    "ccc-23    "},
      {"8/8/8/8/1k6/8/K1p5/8",                                   "b", "-",    "-",  7, 567584,    "ccc-24    "},
      {"8/8/2k5/5q2/5n2/8/5K2/8",                                "b", "-",    "-",  4, 23527,     "ccc-25    "},
      {"8/5k2/8/5N2/5Q2/2K5/8/8",                                "w", "-",    "-",  4, 23527,     "ccc-26    "},
      {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",            "w", "KQkq", "-",  6, 119060324, "cpw-pos1-6"},
      {"8/p7/8/1P6/K1k3p1/6P1/7P/8",                             "w", "-",    "-",  8, 8103790,   "jvm-7     "},
      {"n1n5/PPPk4/8/8/8/8/4Kppp/5N1N",                          "b", "-",    "-",  6, 71179139,  "jvm-8     "},
      {"r3k2r/p6p/8/B7/1pp1p3/3b4/P6P/R3K2R",                    "w", "KQkq", "-",  6, 77054993,  "jvm-9     "},
      {"8/5p2/8/2k3P1/p3K3/8/1P6/8",                             "b", "-",    "-",  8, 64451405,  "jvm-11    "},
      {"r3k2r/pb3p2/5npp/n2p4/1p1PPB2/6P1/P2N1PBP/R3K2R",        "w", "KQkq", "-",  5, 29179893,  "jvm-12    "},
      {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8",                        "w", "-",    "-",  7, 178633661, "jvm-10    "},
      {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R","w", "KQkq", "-",  5, 193690690, "jvm-6     "},
      {"8/2pkp3/8/RP3P1Q/6B1/8/2PPP3/rb1K1n1r",                  "w", "-",    "-",  6, 181153194, "ob1       "},
      {"rnbqkb1r/ppppp1pp/7n/4Pp2/8/8/PPPP1PPP/RNBQKBNR",        "w", "KQkq", "f6", 6, 244063299, "jvm-5     "},
      {"8/2ppp3/8/RP1k1P1Q/8/8/2PPP3/rb1K1n1r",                  "w", "-",    "-",  6, 205552081, "ob2       "},
      {"8/8/3q4/4r3/1b3n2/8/3PPP2/2k1K2R",                       "w", "K",    "-",  6, 207139531, "ob3       "},
      {"4r2r/RP1kP1P1/3P1P2/8/8/3ppp2/1p4p1/4K2R",               "b", "K",    "-",  6, 314516438, "ob4       "},
      {"r3k2r/8/8/8/3pPp2/8/8/R3K1RR",                           "b", "KQkq", "e3", 6, 485647607, "jvm-1     "},
      {"8/3K4/2p5/p2b2r1/5k2/8/8/1q6",                           "b", "-",    "-",  7, 493407574, "jvm-4     "},
      {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1",  "w", "kq",   "-",  6, 706045033, "jvm-2     "},
      {"r6r/1P4P1/2kPPP2/8/8/3ppp2/1p4p1/R3K2R",                 "w", "KQ",   "-",  6, 975944981, "ob5       "}
    };
    
    /*}}}*/
    
    uint64 tmoves = 0;
    uint32 t1, t2, pmoves, err, errs = 0;
    int sec;
    
    t1 = clock();
    
    for (int i=0; i < 64; i++) {
    
      newGame();
      position(pFens[i].fen,pFens[i].turn,pFens[i].rights,pFens[i].ep);
    
      pmoves = perft(pFens[i].depth);
      err    = pmoves - pFens[i].moves;
    
      errs   += err;
      tmoves += pmoves;
    
      t2  = clock();
      sec = round((t2-t1)/100)/10;
    
      printf("%d %s %s %d %u %u %u %u\n",sec,pFens[i].id,pFens[i].fen,pFens[i].depth,pFens[i].moves,pmoves,err,errs);
    }
    
    t2  = clock();
    sec = round((t2-t1)/100)/10;
    
    printf("%d sec, %lu nodes, %u errors\n",sec,tmoves,errs);
    
    /*}}}*/
  }

  else if (!strcmp(cmd,"bench")) {
    /*{{{  bench*/
    
    /*{{{  bench fens*/
    
    typedef struct {
      char   *fen;
      char   *turn;
      char   *rights;
      char   *ep;
    } benchTestStruct;
    
    benchTestStruct bFens[] = {
      {"r3k2r/2pb1ppp/2pp1q2/p7/1nP1B3/1P2P3/P2N1PPP/R2QK2R", "w", "KQkq", "a6"},
      {"4rrk1/2p1b1p1/p1p3q1/4p3/2P2n1p/1P1NR2P/PB3PP1/3R1QK1", "b", "-", "-"},
      {"r3qbrk/6p1/2b2pPp/p3pP1Q/PpPpP2P/3P1B2/2PB3K/R5R1", "w", "-", "-"},
      {"6k1/1R3p2/6p1/2Bp3p/3P2q1/P7/1P2rQ1K/5R2", "b", "-", "-"},
      {"8/8/1p2k1p1/3p3p/1p1P1P1P/1P2PK2/8/8", "w", "-", "-"},
      {"7r/2p3k1/1p1p1qp1/1P1Bp3/p1P2r1P/P7/4R3/Q4RK1", "w", "-", "-"},
      {"r1bq1rk1/pp2b1pp/n1pp1n2/3P1p2/2P1p3/2N1P2N/PP2BPPP/R1BQ1RK1", "b", "-", "-"},
      {"3r3k/2r4p/1p1b3q/p4P2/P2Pp3/1B2P3/3BQ1RP/6K1", "w", "-", "-"},
      {"2r4r/1p4k1/1Pnp4/3Qb1pq/8/4BpPp/5P2/2RR1BK1", "w", "-", "-"},
      {"4q1bk/6b1/7p/p1p4p/PNPpP2P/KN4P1/3Q4/4R3", "b", "-", "-"},
      {"2q3r1/1r2pk2/pp3pp1/2pP3p/P1Pb1BbP/1P4Q1/R3NPP1/4R1K1", "w", "-", "-"},
      {"1r2r2k/1b4q1/pp5p/2pPp1p1/P3Pn2/1P1B1Q1P/2R3P1/4BR1K", "b", "-", "-"},
      {"r3kbbr/pp1n1p1P/3ppnp1/q5N1/1P1pP3/P1N1B3/2P1QP2/R3KB1R", "b", "KQkq", "b3"},
      {"8/6pk/2b1Rp2/3r4/1R1B2PP/P5K1/8/2r5", "b", "-", "-"},
      {"1r4k1/4ppb1/2n1b1qp/pB4p1/1n1BP1P1/7P/2PNQPK1/3RN3", "w", "-", "-"},
      {"8/p2B4/PkP5/4p1pK/4Pb1p/5P2/8/8", "w", "-", "-"},
      {"3r4/ppq1ppkp/4bnp1/2pN4/2P1P3/1P4P1/PQ3PBP/R4K2", "b", "-", "-"},
      {"5rr1/4n2k/4q2P/P1P2n2/3B1p2/4pP2/2N1P3/1RR1K2Q", "w", "-", "-"},
      {"1r5k/2pq2p1/3p3p/p1pP4/4QP2/PP1R3P/6PK/8", "w", "-", "-"},
      {"q5k1/5ppp/1r3bn1/1B6/P1N2P2/BQ2P1P1/5K1P/8", "b", "-", "-"},
      {"r1b2k1r/5n2/p4q2/1ppn1Pp1/3pp1p1/NP2P3/P1PPBK2/1RQN2R1", "w", "-", "-"},
      {"r1bqk2r/pppp1ppp/5n2/4b3/4P3/P1N5/1PP2PPP/R1BQKB1R", "w", "KQkq", "-"},
      {"r1bqr1k1/pp1p1ppp/2p5/8/3N1Q2/P2BB3/1PP2PPP/R3K2n", "b", "Q", "-"},
      {"r1bq2k1/p4r1p/1pp2pp1/3p4/1P1B3Q/P2B1N2/2P3PP/4R1K1", "b", "-", "-"},
      {"r4qk1/6r1/1p4p1/2ppBbN1/1p5Q/P7/2P3PP/5RK1", "w", "-", "-"},
      {"r7/6k1/1p6/2pp1p2/7Q/8/p1P2K1P/8", "w", "-", "-"},
      {"r3k2r/ppp1pp1p/2nqb1pn/3p4/4P3/2PP4/PP1NBPPP/R2QK1NR", "w", "KQkq", "-"},
      {"3r1rk1/1pp1pn1p/p1n1q1p1/3p4/Q3P3/2P5/PP1NBPPP/4RRK1", "w", "-", "-"},
      {"5rk1/1pp1pn1p/p3Brp1/8/1n6/5N2/PP3PPP/2R2RK1", "w", "-", "-"},
      {"8/1p2pk1p/p1p1r1p1/3n4/8/5R2/PP3PPP/4R1K1", "b", "-", "-"},
      {"8/4pk2/1p1r2p1/p1p4p/Pn5P/3R4/1P3PP1/4RK2", "w", "-", "-"},
      {"8/5k2/1pnrp1p1/p1p4p/P6P/4R1PK/1P3P2/4R3", "b", "-", "-"},
      {"8/8/1p1kp1p1/p1pr1n1p/P6P/1R4P1/1P3PK1/1R6", "b", "-", "-"},
      {"8/8/1p1k2p1/p1prp2p/P2n3P/6P1/1P1R1PK1/4R3", "b", "-", "-"},
      {"8/8/1p4p1/p1p2k1p/P2npP1P/4K1P1/1P6/3R4", "w", "-", "-"},
      {"8/8/1p4p1/p1p2k1p/P2n1P1P/4K1P1/1P6/6R1", "b", "-", "-"},
      {"8/5k2/1p4p1/p1pK3p/P2n1P1P/6P1/1P6/4R3", "b", "-", "-"},
      {"8/1R6/1p1K1kp1/p6p/P1p2P1P/6P1/1Pn5/8", "w", "-", "-"},
      {"1rb1rn1k/p3q1bp/2p3p1/2p1p3/2P1P2N/PP1RQNP1/1B3P2/4R1K1", "b", "-", "-"},
      {"4rrk1/pp1n1pp1/q5p1/P1pP4/2n3P1/7P/1P3PB1/R1BQ1RK1", "w", "-", "-"},
      {"r2qr1k1/pb1nbppp/1pn1p3/2ppP3/3P4/2PB1NN1/PP3PPP/R1BQR1K1", "w", "-", "-"},
      {"2r2k2/8/4P1R1/1p6/8/P4K1N/7b/2B5", "b", "-", "-"},
      {"6k1/5pp1/8/2bKP2P/2P5/p4PNb/B7/8", "b", "-", "-"},
      {"2rqr1k1/1p3p1p/p2p2p1/P1nPb3/2B1P3/5P2/1PQ2NPP/R1R4K", "w", "-", "-"},
      {"r1b2rk1/p1q1ppbp/6p1/2Q5/8/4BP2/PPP3PP/2KR1B1R", "b", "-", "-"},
      {"6r1/5k2/p1b1r2p/1pB1p1p1/1Pp3PP/2P1R1K1/2P2P2/3R4", "w", "-", "-"},
      {"rnbqkb1r/pppppppp/5n2/8/2PP4/8/PP2PPPP/RNBQKBNR", "b", "KQkq", "c3"},
      {"2rr2k1/1p4bp/p1q1p1p1/4Pp1n/2PB4/1PN3P1/P3Q2P/2RR2K1", "w", "-", "f6"},
      {"3br1k1/p1pn3p/1p3n2/5pNq/2P1p3/1PN3PP/P2Q1PB1/4R1K1", "w", "-", "-"},
      {"2r2b2/5p2/5k2/p1r1pP2/P2pB3/1P3P2/K1P3R1/7R", "w", "-", "-"}
    };
    
    /*}}}*/
    
    mSilent = 1;
    
    uint32 nodes = 0;
    
    const uint32 t1 = clock();
    
    for (int i=0; i < 50; i++) {
    
      newGame();
      position(bFens[i].fen,bFens[i].turn,bFens[i].rights,bFens[i].ep);
    
      tTargetDepth = 6;
      tTargetNodes = 0;
      tFinishTime  = 0;
    
      go();
    
      nodes += tNodes;
    }
    
    const uint32 t2 = clock();
    const int sec   = round((t2-t1)/100)/10;
    
    mSilent = 0;
    
    printf("%d %u\n",sec,nodes);
    
    /*}}}*/
  }

  else {
    fprintf(stderr,"?\n");
  }

  return 0;
}

/*}}}*/
/*{{{  uciExec*/

int uciExec (char *line) {

  char *tokens[MAX_TOKENS];
  char *token;

  int num_tokens = 0;

  token = strtok(line, " \t\n");

  while (token != NULL && num_tokens < MAX_TOKENS) {

    tokens[num_tokens++] = token;

    token = strtok(NULL, " \r\t\n");
  }

  return uciTokens(num_tokens, tokens);
}

/*}}}*/

/*}}}*/

int main(int argc, char **argv) {

  setvbuf(stdout, NULL, _IONBF, 0);

  char chunk[MAX_LINE_LENGTH];

  hashInitOnce();
  evalInitOnce();

  /*{{{  exec args*/
  
  for (int i=1; i < argc; i++) {
    if (uciExec(argv[i]))
      return 0;
  }
  
  /*}}}*/
  /*{{{  exec stdio*/
  
  while (fgets(chunk, sizeof(chunk), stdin) != NULL) {
  
    if (uciExec(chunk))
      return 0;
  }
  
  /*}}}*/

  return 0;
}

