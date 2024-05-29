
"use strict"

const BUILD = "1";

//{{{  misc

const MATE = 32000;

var mSilent = 0;

//}}}
//{{{  timing

var tBestMove    = 0;
var tDone        = 0;
var tNodes       = 0;
var tTargetDepth = 0;
var tTargetNodes = 0;
var tFinishTime  = 0;

function areWeDone() {
  return tBestMove && ((tFinishTime  && (Date.now() >  tFinishTime)) || (tTargetNodes && (tNodes >= tTargetNodes)));
}

//}}}
//{{{  board

//{{{  constants

const WHITE = 0x0;
const BLACK = 0x8;

const PIECE_MASK  = 0x7;
const COLOUR_MASK = 0x8;

const PAWN   = 1;
const KNIGHT = 2;
const BISHOP = 3;
const ROOK   = 4;
const QUEEN  = 5;
const KING   = 6;
const EDGE   = 7;

const W_PAWN   = PAWN;
const W_KNIGHT = KNIGHT;
const W_BISHOP = BISHOP;
const W_ROOK   = ROOK;
const W_QUEEN  = QUEEN;
const W_KING   = KING;

const B_PAWN   = PAWN   | BLACK;
const B_KNIGHT = KNIGHT | BLACK;
const B_BISHOP = BISHOP | BLACK;
const B_ROOK   = ROOK   | BLACK;
const B_QUEEN  = QUEEN  | BLACK;
const B_KING   = KING   | BLACK;

const ADJACENT = [1,1,0,0,0,0,0,0,0,0,0,1,1,1];

//
// E == EMPTY, X = OFF BOARD, - == CANNOT HAPPEN
//
//                  0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15
//                  E  W  W  W  W  W  W  X  -  B  B  B  B  B  B  -
//                  E  P  N  B  R  Q  K  X  -  P  N  B  R  Q  K  -
//
const IS_O       = [0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0];
const IS_E       = [1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
const IS_OE      = [1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0];

const IS_P       = [0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0];
const IS_N       = [0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0];
const IS_NBRQ    = [0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0]
const IS_NBRQKE  = [1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0]
const IS_RQKE    = [1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0]
const IS_Q       = [0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0]
const IS_QKE     = [1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0]
const IS_K       = [0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0];
const IS_KN      = [0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0];
const IS_SLIDER  = [0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0];

const IS_W       = [0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0];
const IS_WE      = [1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0];
const IS_WP      = [0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
const IS_WN      = [0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
const IS_WNBRQ   = [0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
const IS_WPNBRQ  = [0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
const IS_WPNBRQE = [1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
const IS_WB      = [0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
const IS_WR      = [0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
const IS_WBQ     = [0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
const IS_WRQ     = [0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
const IS_WQ      = [0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
const IS_WK      = [0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0]

const IS_B       = [0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0];
const IS_BE      = [1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0];
const IS_BP      = [0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0];
const IS_BN      = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0];
const IS_BNBRQ   = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0]
const IS_BPNBRQ  = [0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0]
const IS_BPNBRQE = [1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0]
const IS_BB      = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0];
const IS_BR      = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0];
const IS_BBQ     = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0];
const IS_BRQ     = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0];
const IS_BQ      = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0]
const IS_BK      = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0]

const OBJ_CHAR = ['.','P','N','B','R','Q','K','x','y','p','n','b','r','q','k','z'];

const A1 = 110, B1 = 111, C1 = 112, D1 = 113, E1 = 114, F1 = 115, G1 = 116, H1 = 117;
const           B2 = 99,  C2 = 100,                               G2 = 104, H2 = 105;
const           B7 = 39,  C7 = 40,                                G7 = 44,  H7 = 45;
const A8 = 26,  B8 = 27,  C8 = 28,  D8 = 29,  E8 = 30,  F8 = 31,  G8 = 32,  H8 = 33;

const B88 = [26, 27, 28, 29, 30, 31, 32, 33,
             38, 39, 40, 41, 42, 43, 44, 45,
             50, 51, 52, 53, 54, 55, 56, 57,
             62, 63, 64, 65, 66, 67, 68, 69,
             74, 75, 76, 77, 78, 79, 80, 81,
             86, 87, 88, 89, 90, 91, 92, 93,
             98, 99, 100,101,102,103,104,105,
             110,111,112,113,114,115,116,117];

const COORDS = ['??', '??', '??', '??', '??', '??', '??', '??', '??', '??', '??', '??',
                '??', '??', '??', '??', '??', '??', '??', '??', '??', '??', '??', '??',
                '??', '??', 'a8', 'b8', 'c8', 'd8', 'e8', 'f8', 'g8', 'h8', '??', '??',
                '??', '??', 'a7', 'b7', 'c7', 'd7', 'e7', 'f7', 'g7', 'h7', '??', '??',
                '??', '??', 'a6', 'b6', 'c6', 'd6', 'e6', 'f6', 'g6', 'h6', '??', '??',
                '??', '??', 'a5', 'b5', 'c5', 'd5', 'e5', 'f5', 'g5', 'h5', '??', '??',
                '??', '??', 'a4', 'b4', 'c4', 'd4', 'e4', 'f4', 'g4', 'h4', '??', '??',
                '??', '??', 'a3', 'b3', 'c3', 'd3', 'e3', 'f3', 'g3', 'h3', '??', '??',
                '??', '??', 'a2', 'b2', 'c2', 'd2', 'e2', 'f2', 'g2', 'h2', '??', '??',
                '??', '??', 'a1', 'b1', 'c1', 'd1', 'e1', 'f1', 'g1', 'h1', '??', '??',
                '??', '??', '??', '??', '??', '??', '??', '??', '??', '??', '??', '??',
                '??', '??', '??', '??', '??', '??', '??', '??', '??', '??', '??', '??'];

const RANK = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

const FILE = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

const CENTRE = [0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0,
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
                0, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0];

//}}}

const bBoard  = Array(144);
var   bTurn   = 0;
var   bRights = 0;
var   bEP     = 0;
var   bPly    = 0;
const bKings  = [0,0];

//{{{  board primitives

function objColour (obj) {
  return obj & COLOUR_MASK;
}

function objPiece (obj) {
  return obj & PIECE_MASK;
}

function colourIndex (c) {
  return c >>> 3;
}

function colourtoggleIndex (i) {
  return Math.abs(i-1);
}

function colourMultiplier (c) {
  return (-c >> 31) | 1;
}

function colourToggle (c) {
  return ~c & COLOUR_MASK;
}

//}}}
//{{{  printBoard

function printBoard () {

  const b = bBoard;

  for (var rank=7; rank >= 0; rank--) {
    process.stdout.write((rank+1)+' ');
    for (var file=0; file <= 7; file++) {
      process.stdout.write(OBJ_CHAR[b[B88[(7-rank)*8+file]]] + ' ');
    }
    process.stdout.write('\r\n');
  }

  console.log('  a b c d e f g h');

  if (bTurn == WHITE)
    process.stdout.write('w');
  else
    process.stdout.write('b');
  process.stdout.write(' ');

  if (bRights) {
    if (bRights & WHITE_RIGHTS_KING)
      process.stdout.write('K');
   if (bRights & WHITE_RIGHTS_QUEEN)
      process.stdout.write('Q');
   if (bRights & BLACK_RIGHTS_KING)
      process.stdout.write('k');
   if (bRights & BLACK_RIGHTS_QUEEN)
      process.stdout.write('q');
    process.stdout.write(' ');
  }
  else
    process.stdout.write('- ');

  if (bEP)
    process.stdout.write(COORDS[bEP]);
  else
    process.stdout.write('-');

  console.log();
  console.log('hash',hLo[0],hHi[0]);
}

//}}}

//}}}
//{{{  eval

//{{{  constants

const MATERIAL = [100,320,330,500,900,20000];

const WPAWN_PST = [0, 0, 0,  0,  0,   0,   0,   0,   0,  0,  0, 0,
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
                   0, 0, 0,  0,  0,   0,   0,   0,   0,  0,  0, 0];

const WKNIGHT_PST = [0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
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
                     0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0];

const WBISHOP_PST = [0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
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
                     0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0];

const WROOK_PST = [0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0,
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
                   0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0];

const WQUEEN_PST = [0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
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
                    0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0];

const WKING_MID_PST = [0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0,
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
                       0, 0, 0,   0,   0,   0,   0,   0,   0,   0,   0, 0];

const WKING_END_PST = [0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0,
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
                       0, 0, 0,   0,   0,   0,   0,   0,   0,   0,  0, 0];

const BPAWN_PST     = Array(144);
const BKNIGHT_PST   = Array(144);
const BBISHOP_PST   = Array(144);
const BROOK_PST     = Array(144);
const BQUEEN_PST    = Array(144);
const BKING_MID_PST = Array(144);
const BKING_END_PST = Array(144);

const WHITE_MID_PST = [WPAWN_PST, WKNIGHT_PST, WBISHOP_PST, WROOK_PST, WQUEEN_PST, WKING_MID_PST];
const WHITE_END_PST = [WPAWN_PST, WKNIGHT_PST, WBISHOP_PST, WROOK_PST, WQUEEN_PST, WKING_END_PST];
const BLACK_MID_PST = [BPAWN_PST, BKNIGHT_PST, BBISHOP_PST, BROOK_PST, BQUEEN_PST, BKING_MID_PST];
const BLACK_END_PST = [BPAWN_PST, BKNIGHT_PST, BBISHOP_PST, BROOK_PST, BQUEEN_PST, BKING_END_PST];

const WB_MID_PST = [WHITE_MID_PST, BLACK_MID_PST];
const WB_END_PST = [WHITE_END_PST, BLACK_END_PST];

//}}}

//{{{  evaluate

function evaluate () {

  const b = bBoard;

  const cx = colourMultiplier(bTurn)

  var e = 10 * cx;

  var pst_mid = 0;
  var pst_end = 0;

  var q = 0;

  for (let sq=0; sq<64; sq++) {

    const fr    = B88[sq];
    const frObj = b[fr];

    if (!frObj)
      continue;

    const frPiece  = objPiece(frObj) - 1;
    const frColour = objColour(frObj);
    const frIndex  = colourIndex(frColour);
    const frMult   = colourMultiplier(frColour);

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

//}}}
//{{{  flip

function flip (sq) {
  var m = (143-sq)/12|0;
  return 12*m + sq%12;
}

//}}}
//{{{  evalInitOnce

function evalInitOnce () {

  for (let i=0; i < 144; i++) {

    let j = flip(i);

    BPAWN_PST[j]     = WPAWN_PST[i];
    BKNIGHT_PST[j]   = WKNIGHT_PST[i];
    BBISHOP_PST[j]   = WBISHOP_PST[i];
    BROOK_PST[j]     = WROOK_PST[i];
    BQUEEN_PST[j]    = WQUEEN_PST[i];
    BKING_MID_PST[j] = WKING_MID_PST[i];
    BKING_END_PST[j] = WKING_END_PST[i];
  }

}

//}}}

//}}}
//{{{  movelist

//{{{  constants

const MAX_PLY   = 1024;
const MAX_MOVES = 256;

const ALL_MOVES   = 0
const NOISY_MOVES = 1

const MOVE_TO_BITS      = 0;
const MOVE_FR_BITS      = 8;
const MOVE_TOOBJ_BITS   = 16;
const MOVE_FROBJ_BITS   = 20;
const MOVE_PROMAS_BITS  = 29;

const MOVE_TO_MASK       = 0x000000FF;
const MOVE_FR_MASK       = 0x0000FF00;
const MOVE_TOOBJ_MASK    = 0x000F0000;
const MOVE_FROBJ_MASK    = 0x00F00000;
const MOVE_KINGMOVE_MASK = 0x01000000;
const MOVE_EPTAKE_MASK   = 0x02000000;
const MOVE_EPMAKE_MASK   = 0x04000000;
const MOVE_CASTLE_MASK   = 0x08000000;
const MOVE_PROMOTE_MASK  = 0x10000000;
const MOVE_PROMAS_MASK   = 0x60000000;  // NBRQ.
const MOVE_SPARE_MASK    = 0x80000000;

const MOVE_CAPTURE_MASK = MOVE_TOOBJ_MASK | MOVE_EPTAKE_MASK;
const MOVE_IKKY_MASK    = MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | MOVE_PROMOTE_MASK | MOVE_EPTAKE_MASK | MOVE_EPMAKE_MASK;
const MOVE_DRAW_MASK    = MOVE_TOOBJ_MASK | MOVE_CASTLE_MASK | MOVE_PROMOTE_MASK | MOVE_EPTAKE_MASK;

const MOVE_E1G1 = MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | (W_KING << MOVE_FROBJ_BITS) | (E1 << MOVE_FR_BITS) | G1;
const MOVE_E1C1 = MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | (W_KING << MOVE_FROBJ_BITS) | (E1 << MOVE_FR_BITS) | C1;
const MOVE_E8G8 = MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | (B_KING << MOVE_FROBJ_BITS) | (E8 << MOVE_FR_BITS) | G8;
const MOVE_E8C8 = MOVE_KINGMOVE_MASK | MOVE_CASTLE_MASK | (B_KING << MOVE_FROBJ_BITS) | (E8 << MOVE_FR_BITS) | C8;

const QPRO = (QUEEN-2)  << MOVE_PROMAS_BITS | MOVE_PROMOTE_MASK;
const RPRO = (ROOK-2)   << MOVE_PROMAS_BITS | MOVE_PROMOTE_MASK;
const BPRO = (BISHOP-2) << MOVE_PROMAS_BITS | MOVE_PROMOTE_MASK;
const NPRO = (KNIGHT-2) << MOVE_PROMAS_BITS | MOVE_PROMOTE_MASK;

const WHITE_RIGHTS_KING  = 0x00000001;
const WHITE_RIGHTS_QUEEN = 0x00000002;
const BLACK_RIGHTS_KING  = 0x00000004;
const BLACK_RIGHTS_QUEEN = 0x00000008;
const WHITE_RIGHTS       = WHITE_RIGHTS_QUEEN | WHITE_RIGHTS_KING;
const BLACK_RIGHTS       = BLACK_RIGHTS_QUEEN | BLACK_RIGHTS_KING;

const MASK_RIGHTS = [15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
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
                     15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15];

const W_OFFSET_ORTH  = -12;
const W_OFFSET_DIAG1 = -13;
const W_OFFSET_DIAG2 = -11;

const B_OFFSET_ORTH  = 12;
const B_OFFSET_DIAG1 = 13;
const B_OFFSET_DIAG2 = 11;

const KNIGHT_OFFSETS  = [25,-25,23,-23,14,-14,10,-10];
const BISHOP_OFFSETS  = [11,-11,13,-13];
const ROOK_OFFSETS    = [1,-1,12,-12];
const QUEEN_OFFSETS   = [11,-11,13,-13,1,-1,12,-12];
const KING_OFFSETS    = [11,-11,13,-13,1,-1,12,-12];

const SLIDER_OFFSETS = [0, 0, 0, BISHOP_OFFSETS, ROOK_OFFSETS, QUEEN_OFFSETS];

const WB_CAN_CAPTURE  = [IS_BPNBRQ,      IS_WPNBRQ];
const WB_OUR_PIECE    = [IS_W,           IS_B];
const WB_OFFSET_ORTH  = [W_OFFSET_ORTH,  B_OFFSET_ORTH];
const WB_OFFSET_DIAG1 = [W_OFFSET_DIAG1, B_OFFSET_DIAG1];
const WB_OFFSET_DIAG2 = [W_OFFSET_DIAG2, B_OFFSET_DIAG2];
const WB_HOME_RANK    = [2,              7];
const WB_PROMOTE_RANK = [7,              2];
const WB_EP_RANK      = [5,              4];
const WB_RQ           = [IS_WRQ,         IS_BRQ];
const WB_BQ           = [IS_WBQ,         IS_BBQ];
const WB_PAWN         = [W_PAWN,         B_PAWN];

//}}}

//{{{  movelistStruct

function movelistStruct () {

  this.quietMoves  = Array(MAX_MOVES);
  this.noisyMoves  = Array(MAX_MOVES);
  this.quietRanks  = Array(MAX_MOVES);
  this.noisyRanks  = Array(MAX_MOVES);
  this.quietNum    = 0;
  this.noisyNum    = 0;
  this.nextMove    = 0;
  this.noisy       = 0;
  this.stage       = 0;
}

//}}}

const movelist = Array(MAX_PLY);

//{{{  move primitives

function moveFromSq (move) {
  return (move & MOVE_FR_MASK) >>> MOVE_FR_BITS;
}

function moveToSq (move) {
  return (move & MOVE_TO_MASK) >>> MOVE_TO_BITS;
}

function moveToObj (move) {
  return (move & MOVE_TOOBJ_MASK) >>> MOVE_TOOBJ_BITS;
}

function moveFromObj (move) {
  return (move & MOVE_FROBJ_MASK) >>> MOVE_FROBJ_BITS;
}

function movePromotePiece (move) {
  return ((move & MOVE_PROMAS_MASK) >>> MOVE_PROMAS_BITS) + 2;
}

//}}}

//{{{  movelistInitOnce

function movelistInitOnce() {

  for (let i=0; i < movelist.length; i++)
    movelist[i] = new movelistStruct();
}

//}}}

//{{{  initMoveGen

function initMoveGen (noisy) {

  const ml = movelist[bPly];

  ml.stage = 0;
  ml.noisy = noisy;

}

//}}}
//{{{  getNextMove

function getNextMove () {

  const ml = movelist[bPly];

  switch (ml.stage) {

    case 0:

      ml.noisyNum = 0;
      ml.quietNum = 0;

      genMoves();

      ml.nextMove = 0;
      ml.stage++;

    case 1:

      if (ml.nextMove < ml.noisyNum)
        return nextStagedMove(ml.nextMove++, ml.noisyNum, ml.noisyMoves, ml.noisyRanks);

      if (ml.noisy)
        return 0;

      ml.nextMove = 0;
      ml.stage++;

    case 2:

      if (ml.nextMove < ml.quietNum)
        return nextStagedMove(ml.nextMove++, ml.quietNum, ml.quietMoves, ml.quietRanks);

      return 0;
  }
}

//}}}
//{{{  nextStagedMove

function nextStagedMove (next, num, moves, ranks) {

  var maxR = -100000;
  var maxI = 0;

  for (let i=next; i < num; i++) {
    if (ranks[i] > maxR) {
      maxR = ranks[i];
      maxI = i;
    }
  }

  const maxM = moves[maxI]

  moves[maxI] = moves[next];
  ranks[maxI] = ranks[next];

  return maxM;
}

//}}}
//{{{  addNoisy
//
// Must handle EP captures.
//

const RANK_ATTACKER = [0,    600,  500,  400,  300,  200,  100, 0, 0,    600,  500,  400,  300,  200,  100];
const RANK_DEFENDER = [2000, 1000, 3000, 3000, 5000, 9000, 0,   0, 2000, 1000, 3000, 3000, 5000, 9000, 0];

function addNoisy (move) {

  const ml = movelist[bPly];

  ml.noisyMoves[ml.noisyNum]   = move;
  ml.noisyRanks[ml.noisyNum++] = RANK_ATTACKER[moveFromObj(move)] + RANK_DEFENDER[moveToObj(move)];

}

//}}}
//{{{  addQuiet

function addQuiet (move) {

  const ml = movelist[bPly];

  ml.quietMoves[ml.quietNum]   = move;
  ml.quietRanks[ml.quietNum++] = 100 + CENTRE[moveToSq(move)] - CENTRE[moveFromSq(move)];
}

//}}}

//{{{  genMoves

function genMoves () {

  const b = bBoard;

  const cx           = colourIndex(bTurn);
  const OUR_PIECE    = WB_OUR_PIECE[cx];
  const HOME_RANK    = WB_HOME_RANK[cx];
  const PROMOTE_RANK = WB_PROMOTE_RANK[cx];
  const EP_RANK      = WB_EP_RANK[cx];

  if (bTurn == WHITE)
    genWhiteCastlingMoves();
  else
    genBlackCastlingMoves();

  for (let i=0; i<64; i++) {

    const fr    = B88[i];
    const frObj = b[fr];

    if (!OUR_PIECE[frObj])
      continue;

    const frPiece   = objPiece(frObj);
    const frMove    = (frObj << MOVE_FROBJ_BITS) | (fr << MOVE_FR_BITS);

    switch (frPiece) {

      case KING:
        genKingMoves(frMove);
        break;

      case PAWN:
        const frRank = RANK[fr];
        switch (frRank) {
          case HOME_RANK:
            genHomePawnMoves(frMove);
            break;
          case PROMOTE_RANK:
            genPromotePawnMoves(frMove);
            break;
          case EP_RANK:
            genPawnMoves(frMove);
            if (bEP)
              genEnPassPawnMoves(frMove);
            break;
          default:
            genPawnMoves(frMove);
            break;
        }
        break;

      case KNIGHT:
        genKnightMoves(frMove);
        break;

      default:
        genSliderMoves(frMove);
        break;
    }
  }
}

//}}}

//{{{  genWhiteCastlingMoves

function genWhiteCastlingMoves () {

  const b = bBoard;

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

//}}}
//{{{  genBlackCastlingMoves

function genBlackCastlingMoves () {

  const b = bBoard;

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

//}}}

//{{{  genPawnMoves

function genPawnMoves (frMove) {

  const b = bBoard;

  const fr           = moveFromSq(frMove);
  const cx           = colourIndex(bTurn);
  const CAN_CAPTURE  = WB_CAN_CAPTURE[cx];
  const OFFSET_ORTH  = WB_OFFSET_ORTH[cx];
  const OFFSET_DIAG1 = WB_OFFSET_DIAG1[cx];
  const OFFSET_DIAG2 = WB_OFFSET_DIAG2[cx];

  var to = fr + OFFSET_ORTH;
  if (!b[to])
    addQuiet(frMove | to);

  var to = fr + OFFSET_DIAG1;
  var toObj = b[to];
  if (CAN_CAPTURE[toObj])
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to);

  var to = fr + OFFSET_DIAG2;
  var toObj = b[to];
  if (CAN_CAPTURE[toObj])
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to);
}

//}}}
//{{{  genEnPassPawnMoves

function genEnPassPawnMoves (frMove) {

  const b = bBoard;

  const fr           = moveFromSq(frMove);
  const cx           = colourIndex(bTurn);
  const OFFSET_DIAG1 = WB_OFFSET_DIAG1[cx];
  const OFFSET_DIAG2 = WB_OFFSET_DIAG2[cx];

  var to = fr + OFFSET_DIAG1;
  if (to == bEP && !b[to])
    addNoisy(frMove | to | MOVE_EPTAKE_MASK);

  var to = fr + OFFSET_DIAG2;
  if (to == bEP && !b[to])
    addNoisy(frMove | to | MOVE_EPTAKE_MASK);
}

//}}}
//{{{  genHomePawnMoves

function genHomePawnMoves (frMove) {

  const b = bBoard;

  const fr           = moveFromSq(frMove);
  const cx           = colourIndex(bTurn);
  const CAN_CAPTURE  = WB_CAN_CAPTURE[cx];
  const OFFSET_ORTH  = WB_OFFSET_ORTH[cx];
  const OFFSET_DIAG1 = WB_OFFSET_DIAG1[cx];
  const OFFSET_DIAG2 = WB_OFFSET_DIAG2[cx];

  var to = fr + OFFSET_ORTH;
  if (!b[to]) {
    addQuiet(frMove | to);
    to += OFFSET_ORTH;
    if (!b[to])
      addQuiet(frMove | to | MOVE_EPMAKE_MASK);
  }

  var to    = fr + OFFSET_DIAG1;
  var toObj = b[to];
  if (CAN_CAPTURE[toObj])
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to);

  var to    = fr + OFFSET_DIAG2;
  var toObj = b[to];
  if (CAN_CAPTURE[toObj])
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to);
}

//}}}
//{{{  genPromotePawnMoves

function genPromotePawnMoves (frMove) {

  const b = bBoard;

  const fr           = moveFromSq(frMove);
  const cx           = colourIndex(bTurn);
  const CAN_CAPTURE  = WB_CAN_CAPTURE[cx];
  const OFFSET_ORTH  = WB_OFFSET_ORTH[cx];
  const OFFSET_DIAG1 = WB_OFFSET_DIAG1[cx];
  const OFFSET_DIAG2 = WB_OFFSET_DIAG2[cx];

  var to = fr + OFFSET_ORTH;
  if (!b[to]) {
    addQuiet(frMove | to | QPRO);
    addQuiet(frMove | to | RPRO);
    addQuiet(frMove | to | BPRO);
    addQuiet(frMove | to | NPRO);
  }

  var to    = fr + OFFSET_DIAG1;
  var toObj = b[to];
  if (CAN_CAPTURE[toObj]) {
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | QPRO);
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | RPRO);
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | BPRO);
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | NPRO);
  }

  var to    = fr + OFFSET_DIAG2;
  var toObj = b[to];
  if (CAN_CAPTURE[toObj]) {
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | QPRO);
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | RPRO);
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | BPRO);
    addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | NPRO);
  }
}

//}}}

//{{{  genKingMoves

function genKingMoves (frMove) {

  const b = bBoard;

  const fr          = moveFromSq(frMove);
  const cx          = colourIndex(bTurn);
  const cy          = colourIndex(colourToggle(bTurn));
  const CAN_CAPTURE = WB_CAN_CAPTURE[cx];
  const theirKingSq = bKings[cy];

  var dir = 0;

  while (dir < 8) {

    const to = fr + KING_OFFSETS[dir++];

    if (!ADJACENT[Math.abs(to-theirKingSq)]) {

      const toObj = b[to];

      if (!toObj)
        addQuiet(frMove | to | MOVE_KINGMOVE_MASK);

      else if (CAN_CAPTURE[toObj])
        addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to | MOVE_KINGMOVE_MASK);
    }
  }
}

//}}}
//{{{  genKnightMoves

function genKnightMoves (frMove) {

  const b = bBoard;

  const fr          = moveFromSq(frMove);
  const cx          = colourIndex(bTurn);
  const CAN_CAPTURE = WB_CAN_CAPTURE[cx];

  var dir = 0;

  while (dir < 8) {

    const to    = fr + KNIGHT_OFFSETS[dir++];
    const toObj = b[to];

    if (!toObj)
      addQuiet(frMove | to);

    else if (CAN_CAPTURE[toObj])
      addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to);
  }
}

//}}}
//{{{  genSliderMoves

function genSliderMoves (frMove) {

  const b = bBoard;

  const fr          = moveFromSq(frMove);
  const frObj       = moveFromObj(frMove);
  const frPiece     = objPiece(frObj);
  const cx          = colourIndex(bTurn);
  const CAN_CAPTURE = WB_CAN_CAPTURE[cx];
  const OFFSETS     = SLIDER_OFFSETS[frPiece];
  const len         = OFFSETS.length;

  var dir = 0;

  while (dir < len) {

    const offset = OFFSETS[dir++];

    let to = fr + offset;
    while (!b[to]) {
      addQuiet(frMove | to);
      to += offset;
    }

    const toObj = b[to];
    if (CAN_CAPTURE[toObj])
      addNoisy(frMove | (toObj << MOVE_TOOBJ_BITS) | to);
  }
}

//}}}

//}}}
//{{{  hash

const hSeed      = new Uint32Array(1);
const hLo        = new Uint32Array(1);
const hHi        = new Uint32Array(1);
const hLoTurn    = new Uint32Array(9);
const hHiTurn    = new Uint32Array(9);
const hLoRights  = new Uint32Array(16);
const hHiRights  = new Uint32Array(16);
const hLoEP      = new Uint32Array(144);
const hHiEP      = new Uint32Array(144);
const hLoObj     = Array(16);
const hHiObj     = Array(16);
const hLoHistory = new Uint32Array(MAX_PLY);
const hHiHistory = new Uint32Array(MAX_PLY);

var hHistoryOffset = 0;
var hHistoryLimit = 0;

//{{{  rand

function rand () {  // thanks Maksim

  hSeed[0] ^= hSeed[0] << 13 | 0;
  hSeed[0] ^= hSeed[0] >>> 17 | 0;
  hSeed[0] ^= hSeed[0] << 5  | 0;

  return hSeed[0];
}

//}}}
//{{{  hashInitOnce

function hashInitOnce () {

  hSeed[0] = 1804289383;

  for (let i=0; i < hLoTurn.length; i++) {
    hLoTurn[i] = rand();
    hHiTurn[i] = rand();
  }

  for (let i=0; i < hLoRights.length; i++) {
    hLoRights[i] = rand();
    hHiRights[i] = rand();
  }

  for (let i=0; i < hLoEP.length; i++) {
    hLoEP[i] = rand();
    hHiEP[i] = rand();
  }

  for (let i=1; i < hLoObj.length; i++) {
    hLoObj[i] = new Uint32Array(144);
    hHiObj[i] = new Uint32Array(144);
    for (let j=0; j < 144; j++) {
      hLoObj[i][j] = rand();
      hHiObj[i][j] = rand();
    }
  }
  hLoObj[0] = new Uint32Array(144);
  hHiObj[0] = new Uint32Array(144);
  for (let j=0; j < 144; j++) {
    hLoObj[0][j] = 0;
    hHiObj[0][j] = 0;
  }
}

//}}}
//{{{  hashReset

function hashReset() {

  hLo[0] = 0;
  hHi[0] = 0;
}

//}}}
//{{{  hashCalc

function hashCalc() {

  hashReset();

  for (let i=0; i < 64; i++) {
    let sq  = B88[i];
    let obj = bBoard[sq];
    hashObj(obj,sq);
  }

  hashTurn(bTurn);
  hashRights(bRights);
  hashEP(bEP);
}

//}}}
//{{{  hashTurn

function hashTurn(turn) {
  hLo[0] ^= hLoTurn[turn];
  hHi[0] ^= hHiTurn[turn];
}

//}}}
//{{{  hashRights

function hashRights (rights) {
  hLo[0] ^= hLoRights[rights];
  hHi[0] ^= hHiRights[rights];
}

//}}}
//{{{  hashEP

function hashEP(ep) {
  hLo[0] ^= hLoEP[ep];
  hHi[0] ^= hHiEP[ep];
}

//}}}
//{{{  hashObj

function hashObj(obj, sq) {
  hLo[0] ^= hLoObj[obj][sq];
  hHi[0] ^= hHiObj[obj][sq];
}

//}}}
//{{{  hashIsDraw

function hashIsDraw() {

  if ((bPly + hHistoryOffset - hHistoryLimit) > 100)
    return 1;

  var limit = bPly + hHistoryOffset - 4;
  var count = 0;

  while (limit >= hHistoryLimit) {
    if (hLo[0] == hLoHistory[limit] && hHi[0] == hHiHistory[limit]) {
      if (++count == 2)
        return 1;
    }
    limit -= 2
  }

  return 0;
}

//}}}

//}}}
//{{{  tt

const TT_SIZE  = 1 << 20;
const TT_MASK  = TT_SIZE - 1;
const TT_EXACT = 0x01;
const TT_ALPHA = 0x02;
const TT_BETA  = 0x04;

const ttLo    = new Uint32Array(TT_SIZE);
const ttHi    = new Uint32Array(TT_SIZE);
const ttFlags = new Uint8Array(TT_SIZE);
const ttScore = new Int16Array(TT_SIZE);
const ttDepth = new Uint8Array(TT_SIZE);

function ttInit () {
  ttFlags.fill(0);
}

function ttPut (flags, depth, score) {

  var i = hLo[0] & TT_MASK;

  ttLo[i] = hLo[0];
  ttHi[i] = hHi[0];

  ttFlags[i] = flags;
  ttDepth[i] = depth;
  ttScore[i] = score;
}

function ttIndex () {

  let i = hLo[0] & TT_MASK;

  if (ttFlags[i] && ttLo[i] == hLo[0] && ttHi[i] == hHi[0])
    return i;
  else
    return 0;
}

//}}}
//{{{  cache

//{{{  cacheStruct

function cacheStruct () {

  this.bRights       = 0;
  this.bEP           = 0;
  this.hLo           = new Uint32Array(1);
  this.hHi           = new Uint32Array(1);
  this.hHistoryLimit = 0
}

//}}}

const cache = Array(MAX_PLY);

//{{{  cacheInitOnce

function cacheInitOnce() {

  for (let i=0; i < cache.length; i++)
    cache[i] = new cacheStruct();
}

//}}}

//{{{  cacheSave

function cacheSave () {

  const c = cache[bPly];

  c.bRights       = bRights;
  c.bEP           = bEP;
  c.hLo[0]        = hLo[0];
  c.hHi[0]        = hHi[0];
  c.hHistoryLimit = hHistoryLimit;
}

//}}}
//{{{  cacheUnsave

function cacheUnsave () {

  const c = cache[bPly];

  bRights       = c.bRights;
  bEP           = c.bEP;
  hLo[0]        = c.hLo[0];
  hHi[0]        = c.hHi[0];
  hHistoryLimit = c.hHistoryLimit;
}

//}}}

//}}}

//{{{  makeMove

function makeMove (move) {

  const b = bBoard;

  const fr    = moveFromSq(move);
  const to    = moveToSq(move);
  const frObj = moveFromObj(move);
  const toObj = moveToObj(move);

  hLoHistory[bPly+hHistoryOffset] = hLo[0];
  hHiHistory[bPly+hHistoryOffset] = hHi[0];

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

  if ((move & MOVE_DRAW_MASK) || objPiece(frObj) == PAWN)
    hHistoryLimit = bPly + hHistoryOffset;
}

//}}}
//{{{  makeIkkyMove

function makeIkkyMove (move) {

  const b = bBoard;

  const to    = moveToSq(move);
  const frObj = moveFromObj(move);
  const frCol = objColour(frObj);

  if (frCol == WHITE) {
    //{{{  white
    
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
    
    //}}}
  }

  else {
    //{{{  black
    
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
    
    //}}}
  }
}

//}}}
//{{{  unmakeMove

function unmakeMove (move) {

  const b = bBoard;

  const fr    = moveFromSq(move);
  const to    = moveToSq(move);
  const toObj = moveToObj(move);
  const frObj = moveFromObj(move);

  b[fr] = frObj;
  b[to] = toObj;

  if (move & MOVE_IKKY_MASK)
    unmakeIkkyMove(move);

  bTurn = colourToggle(bTurn);
  bPly--;
}

//}}}
//{{{  unmakeIkkyMove

function unmakeIkkyMove (move) {

  const b = bBoard;

  const fr    = moveFromSq(move);
  const to    = moveToSq(move);
  const frObj = moveFromObj(move);
  const frCol = objColour(frObj);

  if (frCol == WHITE) {
    //{{{  white
    
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
    
    //}}}
  }

  else {
    //{{{  black
    
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
    
    //}}}
  }
}

//}}}
//{{{  isKingAttacked

function isKingAttacked (to, byCol) {

  const b = bBoard;

  const cx = colourIndex(byCol);

  const OFFSET_DIAG1 = -WB_OFFSET_DIAG1[cx];
  const OFFSET_DIAG2 = -WB_OFFSET_DIAG2[cx];
  const RQ           = WB_RQ[cx];
  const BQ           = WB_BQ[cx];
  const BY_PAWN      = WB_PAWN[cx];
  const N            = KNIGHT | byCol;

  var fr = 0;

  //{{{  pawns
  
  if (b[to+OFFSET_DIAG1] == BY_PAWN || b[to+OFFSET_DIAG2] == BY_PAWN)
    return 1;
  
  //}}}
  //{{{  knights
  
  if ((b[to + -10] == N) ||
      (b[to + -23] == N) ||
      (b[to + -14] == N) ||
      (b[to + -25] == N) ||
      (b[to +  10] == N) ||
      (b[to +  23] == N) ||
      (b[to +  14] == N) ||
      (b[to +  25] == N)) return 1;
  
  //}}}
  //{{{  queen, bishop, rook
  
  fr = to + 1;  while (!b[fr]) fr += 1;  if (RQ[b[fr]]) return 1;
  fr = to - 1;  while (!b[fr]) fr -= 1;  if (RQ[b[fr]]) return 1;
  fr = to + 12; while (!b[fr]) fr += 12; if (RQ[b[fr]]) return 1;
  fr = to - 12; while (!b[fr]) fr -= 12; if (RQ[b[fr]]) return 1;
  
  fr = to + 11; while (!b[fr]) fr += 11; if (BQ[b[fr]]) return 1;
  fr = to - 11; while (!b[fr]) fr -= 11; if (BQ[b[fr]]) return 1;
  fr = to + 13; while (!b[fr]) fr += 13; if (BQ[b[fr]]) return 1;
  fr = to - 13; while (!b[fr]) fr -= 13; if (BQ[b[fr]]) return 1;
  
  //}}}

  return 0;
}


//}}}
//{{{  formatMove

function formatMove (move) {

  if (!move)
    return 'NULL MOVE';

  const fr = moveFromSq(move);
  const to = moveToSq(move);

  const frObj = moveFromObj(move);
  const toObj = moveToObj(move);

  const frCoord = COORDS[fr];
  const toCoord = COORDS[to];

  const frPiece = objPiece(frObj)
  const frCol   = objColour(frObj);
  const frName  = OBJ_CHAR[frObj];

  const toPiece = objPiece(toObj);
  const toCol   = objColour(toObj);
  const toName  = OBJ_CHAR[toObj];

  const pro = (move & MOVE_PROMOTE_MASK) ? OBJ_CHAR[movePromotePiece(move)|BLACK] : '';

  return frCoord + toCoord + pro;
}

//}}}
//{{{  newGame

function newGame () {
  ttInit();
}

//}}}
//{{{  position

function position (sb, st, sr, sep) {

  const b = bBoard;

  b.fill(EDGE);

  for (let i=0; i < 64; i++)
    b[B88[i]] = 0;

  //{{{  board board
  
  var sq   = 0;
  var rank = 7;
  var file = 0;
  
  for (let i=0; i < sb.length; i++) {
  
    const ch   = sb.charAt(i);
    const sq88 = (7-rank) * 8 + file;
    const sq   = B88[sq88];
  
    switch (ch) {
      //{{{  1-8
      
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
      
      //}}}
      //{{{  /
      
      case '/':
        rank--;
        file = 0;
        break;
      
      //}}}
      //{{{  black
      
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
      
      //}}}
      //{{{  white
      
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
      
      //}}}
      default:
        console.log('unknown board char','|'+ch+'|');
    }
  }
  
  //}}}
  //{{{  board turn
  
  if (st == 'w')
    bTurn = WHITE;
  
  else if (st == 'b')
    bTurn = BLACK;
  
  else
    console.log('unknown board colour', st)
  
  //}}}
  //{{{  board rights
  
  bRights = 0;
  
  for (let i=0; i < sr.length; i++) {
  
    const ch = sr.charAt(i);
  
    if (ch == 'K') bRights |= WHITE_RIGHTS_KING;
    if (ch == 'Q') bRights |= WHITE_RIGHTS_QUEEN;
    if (ch == 'k') bRights |= BLACK_RIGHTS_KING;
    if (ch == 'q') bRights |= BLACK_RIGHTS_QUEEN;
  }
  
  //}}}
  //{{{  board ep
  
  if (sep.length == 2)
    bEP = COORDS.indexOf(sep)
  
  else
    bEP = 0;
  
  //}}}

  hashCalc()

  bPly           = 0;
  hHistoryLimit  = 0;
  hHistoryOffset = 0;

}

//}}}
//{{{  playMove

function playMove (uciMove) {

  initMoveGen(ALL_MOVES);

  var move = 0;

  while (move = getNextMove()) {

    if (formatMove(move) == uciMove) {
      makeMove(move);
      hHistoryOffset++;
      return;
    }
  }

  console.log('cannot play uci move', uciMove);
}

//}}}
//{{{  go

function go () {

  bPly = 0;

  tBestMove = 0;
  tDone     = 0;
  tNodes    = 0;

  var score = 0;
  var alpha = 0;
  var beta  = 0;
  var delta = 0;

  for (let depth=1; depth <= tTargetDepth; depth++) {

    alpha = -MATE;
    beta  = MATE;
    delta = 10;

    if (depth >= 4) {
      alpha = Math.max(-MATE, score - delta);
      beta  = Math.min(MATE,  score + delta);
    }

    while (1) {

      score = search(alpha, beta, depth);

      if (tDone)
        break;

      if (score > alpha && score < beta) {
        uciSend('info', 'depth', depth, 'nodes', tNodes, 'score', score, 'pv', formatMove(tBestMove));
        break;
      }

      delta += delta / 2 | 0;

      if (score <= alpha) {
        uciSend('info', 'depth', depth, 'nodes', tNodes, 'lowerbound', score);
        beta  = Math.min(MATE, ((alpha + beta) / 2) | 0);
        alpha = Math.max(-MATE, score - delta);
        //tBestMove = 0;
      }
      else if (score >= beta) {
        uciSend('info', 'depth', depth, 'nodes', tNodes, 'upperbound', score);
        alpha = Math.max(-MATE, ((alpha + beta) / 2) | 0);
        beta  = Math.min(MATE,  score + delta);
      }
    }

    if (tDone)
      break;
  }

  uciSend('bestmove', formatMove(tBestMove));
}

//}}}
//{{{  search

function search (alpha, beta, depth) {

  //{{{  housekeeping
  
  tNodes++;
  
  if (areWeDone() || bPly == MAX_PLY) {
  
    tDone = 1;
  
    return 0;
  }
  
  //}}}

  const turn     = bTurn;
  const nextTurn = colourToggle(turn);
  const cx       = colourIndex(turn);
  const inCheck  = isKingAttacked(bKings[cx], nextTurn);

  if (depth <= 0 && !inCheck)
    return qsearch(alpha, beta, depth);

  depth = Math.max(0, depth);

  const pvNode = alpha != (beta - 1);

  //{{{  check TT
  
  let i = ttIndex();
  
  if (i) {
    if (ttDepth[i] >= depth && (depth == 0 || !pvNode)) {
      if (ttFlags[i] == TT_EXACT || (ttFlags[i] == TT_BETA && ttScore[i] >= beta) || (ttFlags[i] == TT_ALPHA && ttScore[i] <= alpha)) {
        return ttScore[i];
      }
    }
  }
  
  //}}}

  if (hashIsDraw())
    return 0;

  const oAlpha   = alpha;
  const rootNode = bPly == 0;

  var bestScore = -MATE;
  var bestMove  = 0;

  var move   = 0;
  var score  = 0;
  var played = 0;

  cacheSave();
  initMoveGen(ALL_MOVES);

  while (move = getNextMove()) {

    makeMove(move);

    if (isKingAttacked(bKings[cx], nextTurn)) {
      //{{{  illegal move
      
      unmakeMove(move);
      cacheUnsave();
      
      continue;
      
      //}}}
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

//}}}
//{{{  qsearch

function qsearch (alpha, beta, depth) {

  //{{{  housekeeping
  
  tNodes++;
  
  if (bPly == MAX_PLY) {
  
    tDone = 1;
  
    return evaluate();
  }
  
  //}}}

  //{{{  check TT
  
  let i = ttIndex();
  
  if (i) {
    if (ttFlags[i] == TT_EXACT || (ttFlags[i] == TT_BETA && ttScore[i] >= beta) || (ttFlags[i] == TT_ALPHA && ttScore[i] <= alpha)) {
      return ttScore[i];
    }
  }
  
  //}}}

  const e = evaluate();

  if (e >= beta)
    return e;

  if (alpha < e)
    alpha = e;

  const turn     = bTurn;
  const nextTurn = colourToggle(turn);
  const cx       = colourIndex(turn);

  var move   = 0;
  var score  = 0;
  var played = 0;

  cacheSave();
  initMoveGen(NOISY_MOVES);

  while (move = getNextMove()) {

    makeMove(move);

    if (isKingAttacked(bKings[cx], nextTurn)) {
      //{{{  illegal move
      
      unmakeMove(move);
      cacheUnsave();
      
      continue;
      
      //}}}
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

//}}}
//{{{  perft

function perft (depth) {

  if (depth == 0)
    return 1;

  const turn      = bTurn;
  const nextTurn  = colourToggle(turn);
  const cx        = colourIndex(turn);

  var count = 0;
  var move  = 0;

  cacheSave();
  initMoveGen(ALL_MOVES);

  while (move = getNextMove()) {

    makeMove(move);

    if (isKingAttacked(bKings[cx], nextTurn)) {
      //{{{  illegal move
      
      unmakeMove(move);
      cacheUnsave();
      
      continue;
      
      //}}}
    }

    count += perft(depth-1);

    unmakeMove(move);
    cacheUnsave();
  }

  return count;
}

//}}}
//{{{  uciArgv

function uciArgv() {

  if (process.argv.length > 2) {
    for (let i=2; i < process.argv.length; i++)
      uciExec(process.argv[i]);
  }

}

//}}}
//{{{  uciPost

function uciPost (s) {
  console.log(s);
}

//}}}
//{{{  uciSend

function uciSend () {

  if (mSilent)
    return;

  var s = '';

  for (var i = 0; i < arguments.length; i++)
    s += arguments[i] + ' ';

  uciPost(s);
}

//}}}
//{{{  uciExec

function uciExec(e) {

  var messageList = e.split('\n');

  for (var messageNum=0; messageNum < messageList.length; messageNum++ ) {

    var message = messageList[messageNum].replace(/(\r\n|\n|\r)/gm,"");

    message = message.trim();
    message = message.replace(/\s+/g,' ');

    var tokens  = message.split(' ');
    var command = tokens[0];

    if (!command)
      continue;

    switch (command) {

      case 'go':
      case 'g': {
        //{{{  go
        
        const slop = 1;
        
        let wTime     = 0;
        let bTime     = 0;
        let wInc      = 0;
        let bInc      = 0;
        let moveTime  = 0;
        let movesToGo = 0;
        let depth     = 0;
        let nodes     = 0;
        
        let i = 1;
        
        while (i < tokens.length) {
          switch (tokens[i]) {
            case 'depth':
            case 'd': {
              depth = parseInt(tokens[i+1]);
              i += 2;
              break;
            }
            case 'nodes': {
              nodes = parseInt(tokens[i+1]);
              i += 2;
              break;
            }
            case 'movestogo': {
              movesToGo = parseInt(tokens[i+1]);
              i += 2;
              break;
            }
            case 'movetime':
            case 'mt': {
              moveTime = parseInt(tokens[i+1]);
              i += 2;
              break;
            }
            case 'winc': {
              wInc = parseInt(tokens[i+1]);
              i += 2;
              break;
            }
            case 'binc': {
              bInc = parseInt(tokens[i+1]);
              i += 2;
              break;
            }
            case 'wtime': {
              wTime = parseInt(tokens[i+1]);
              i += 2;
              break;
            }
            case 'btime': {
              bTime = parseInt(tokens[i+1]);
              i += 2;
              break;
            }
            default: {
              console.log('unknown go token', tokens[i]);
              i++;
            }
          }
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
          tFinishTime = Date.now() + moveTime + slop;
        
        else {
        
          if (movesToGo)
            movesToGo += 2;
          else
            movesToGo = 30;
        
          if (wTime && bTurn == WHITE)
            tFinishTime = Date.now() + 0.95 * (wTime/movesToGo + wInc) + slop;
          else if (bTime && bTurn == BLACK)
            tFinishTime = Date.now() + 0.95 * (bTime/movesToGo + bInc) + slop;
          else
            tFinishTime = 0;
        }
        
        go();
        
        break;
        
        //}}}
      }

      case 'stop': {
        //{{{  stop
        
        break;
        
        //}}}
      }

      case 'uci': {
        //{{{  uci
        
        uciSend('id name clozza',BUILD);
        uciSend('id author Colin Jenkins');
        uciSend('uciok');
        
        break;
        
        //}}}
      }

      case 'ucinewgame':
      case 'u': {
        //{{{  ucinewgame
        
        newGame();
        
        break;
        
        //}}}
      }

      case 'isready': {
        //{{{  isready
        
        uciSend('readyok');
        
        break;
        
        //}}}
      }

      case 'position':
      case 'p': {
        //{{{  position
        
        switch (tokens[1]) {
        
          case 'startpos':
          case 's':
        
            position('rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR', 'w', 'KQkq', '-');
            if (tokens[2] == 'moves') {
              for (var i=3; i < tokens.length; i++)
                playMove(tokens[i]);
            }
            break;
        
          case 'fen':
          case 'f':
        
            position(tokens[2], tokens[3], tokens[4], tokens[5]);
            if (tokens[8] == 'moves') {
              for (var i=9; i < tokens.length; i++)
                playMove(tokens[i]);
            }
            break;
        
          default:
        
            console.log(command, tokens[1], 'not implemented');
            break;
        }
        
        break;
        
        //}}}
      }

      case 'board':
      case 'b': {
        //{{{  board
        
        printBoard();
        
        break;
        
        //}}}
      }

      case 'eval':
      case 'e': {
        //{{{  eval
        
        const e = evaluate();
        
        console.log(e);
        
        break;
        
        //}}}
      }

      case 'quit':
      case 'q': {
        //{{{  quit
        
        process.exit();
        
        break;
        
        //}}}
      }

      case 'perft': {
        //{{{  perft
        
        const depth  = parseInt(tokens[1]);
        const t      = Date.now();
        const pmoves = perft(depth);
        
        console.log(pmoves,'moves',Date.now()-t,'ms');
        
        break;
        
        //}}}
      }

      case 'bench': {
        //{{{  bench
        
        //{{{  bench fens
        
        const bFens = [
          "r3k2r/2pb1ppp/2pp1q2/p7/1nP1B3/1P2P3/P2N1PPP/R2QK2R w KQkq a6 0 14",
          "4rrk1/2p1b1p1/p1p3q1/4p3/2P2n1p/1P1NR2P/PB3PP1/3R1QK1 b - - 2 24",
          "r3qbrk/6p1/2b2pPp/p3pP1Q/PpPpP2P/3P1B2/2PB3K/R5R1 w - - 16 42",
          "6k1/1R3p2/6p1/2Bp3p/3P2q1/P7/1P2rQ1K/5R2 b - - 4 44",
          "8/8/1p2k1p1/3p3p/1p1P1P1P/1P2PK2/8/8 w - - 3 54",
          "7r/2p3k1/1p1p1qp1/1P1Bp3/p1P2r1P/P7/4R3/Q4RK1 w - - 0 36",
          "r1bq1rk1/pp2b1pp/n1pp1n2/3P1p2/2P1p3/2N1P2N/PP2BPPP/R1BQ1RK1 b - - 2 10",
          "3r3k/2r4p/1p1b3q/p4P2/P2Pp3/1B2P3/3BQ1RP/6K1 w - - 3 87",
          "2r4r/1p4k1/1Pnp4/3Qb1pq/8/4BpPp/5P2/2RR1BK1 w - - 0 42",
          "4q1bk/6b1/7p/p1p4p/PNPpP2P/KN4P1/3Q4/4R3 b - - 0 37",
          "2q3r1/1r2pk2/pp3pp1/2pP3p/P1Pb1BbP/1P4Q1/R3NPP1/4R1K1 w - - 2 34",
          "1r2r2k/1b4q1/pp5p/2pPp1p1/P3Pn2/1P1B1Q1P/2R3P1/4BR1K b - - 1 37",
          "r3kbbr/pp1n1p1P/3ppnp1/q5N1/1P1pP3/P1N1B3/2P1QP2/R3KB1R b KQkq b3 0 17",
          "8/6pk/2b1Rp2/3r4/1R1B2PP/P5K1/8/2r5 b - - 16 42",
          "1r4k1/4ppb1/2n1b1qp/pB4p1/1n1BP1P1/7P/2PNQPK1/3RN3 w - - 8 29",
          "8/p2B4/PkP5/4p1pK/4Pb1p/5P2/8/8 w - - 29 68",
          "3r4/ppq1ppkp/4bnp1/2pN4/2P1P3/1P4P1/PQ3PBP/R4K2 b - - 2 20",
          "5rr1/4n2k/4q2P/P1P2n2/3B1p2/4pP2/2N1P3/1RR1K2Q w - - 1 49",
          "1r5k/2pq2p1/3p3p/p1pP4/4QP2/PP1R3P/6PK/8 w - - 1 51",
          "q5k1/5ppp/1r3bn1/1B6/P1N2P2/BQ2P1P1/5K1P/8 b - - 2 34",
          "r1b2k1r/5n2/p4q2/1ppn1Pp1/3pp1p1/NP2P3/P1PPBK2/1RQN2R1 w - - 0 22",
          "r1bqk2r/pppp1ppp/5n2/4b3/4P3/P1N5/1PP2PPP/R1BQKB1R w KQkq - 0 5",
          "r1bqr1k1/pp1p1ppp/2p5/8/3N1Q2/P2BB3/1PP2PPP/R3K2n b Q - 1 12",
          "r1bq2k1/p4r1p/1pp2pp1/3p4/1P1B3Q/P2B1N2/2P3PP/4R1K1 b - - 2 19",
          "r4qk1/6r1/1p4p1/2ppBbN1/1p5Q/P7/2P3PP/5RK1 w - - 2 25",
          "r7/6k1/1p6/2pp1p2/7Q/8/p1P2K1P/8 w - - 0 32",
          "r3k2r/ppp1pp1p/2nqb1pn/3p4/4P3/2PP4/PP1NBPPP/R2QK1NR w KQkq - 1 5",
          "3r1rk1/1pp1pn1p/p1n1q1p1/3p4/Q3P3/2P5/PP1NBPPP/4RRK1 w - - 0 12",
          "5rk1/1pp1pn1p/p3Brp1/8/1n6/5N2/PP3PPP/2R2RK1 w - - 2 20",
          "8/1p2pk1p/p1p1r1p1/3n4/8/5R2/PP3PPP/4R1K1 b - - 3 27",
          "8/4pk2/1p1r2p1/p1p4p/Pn5P/3R4/1P3PP1/4RK2 w - - 1 33",
          "8/5k2/1pnrp1p1/p1p4p/P6P/4R1PK/1P3P2/4R3 b - - 1 38",
          "8/8/1p1kp1p1/p1pr1n1p/P6P/1R4P1/1P3PK1/1R6 b - - 15 45",
          "8/8/1p1k2p1/p1prp2p/P2n3P/6P1/1P1R1PK1/4R3 b - - 5 49",
          "8/8/1p4p1/p1p2k1p/P2npP1P/4K1P1/1P6/3R4 w - - 6 54",
          "8/8/1p4p1/p1p2k1p/P2n1P1P/4K1P1/1P6/6R1 b - - 6 59",
          "8/5k2/1p4p1/p1pK3p/P2n1P1P/6P1/1P6/4R3 b - - 14 63",
          "8/1R6/1p1K1kp1/p6p/P1p2P1P/6P1/1Pn5/8 w - - 0 67",
          "1rb1rn1k/p3q1bp/2p3p1/2p1p3/2P1P2N/PP1RQNP1/1B3P2/4R1K1 b - - 4 23",
          "4rrk1/pp1n1pp1/q5p1/P1pP4/2n3P1/7P/1P3PB1/R1BQ1RK1 w - - 3 22",
          "r2qr1k1/pb1nbppp/1pn1p3/2ppP3/3P4/2PB1NN1/PP3PPP/R1BQR1K1 w - - 4 12",
          "2r2k2/8/4P1R1/1p6/8/P4K1N/7b/2B5 b - - 0 55",
          "6k1/5pp1/8/2bKP2P/2P5/p4PNb/B7/8 b - - 1 44",
          "2rqr1k1/1p3p1p/p2p2p1/P1nPb3/2B1P3/5P2/1PQ2NPP/R1R4K w - - 3 25",
          "r1b2rk1/p1q1ppbp/6p1/2Q5/8/4BP2/PPP3PP/2KR1B1R b - - 2 14",
          "6r1/5k2/p1b1r2p/1pB1p1p1/1Pp3PP/2P1R1K1/2P2P2/3R4 w - - 1 36",
          "rnbqkb1r/pppppppp/5n2/8/2PP4/8/PP2PPPP/RNBQKBNR b KQkq c3 0 2",
          "2rr2k1/1p4bp/p1q1p1p1/4Pp1n/2PB4/1PN3P1/P3Q2P/2RR2K1 w - f6 0 20",
          "3br1k1/p1pn3p/1p3n2/5pNq/2P1p3/1PN3PP/P2Q1PB1/4R1K1 w - - 0 23",
          "2r2b2/5p2/5k2/p1r1pP2/P2pB3/1P3P2/K1P3R1/7R w - - 23 93"
        ];
        
        //}}}
        
        mSilent = 1;
        
        let nodes = 0;
        
        const t1 = Date.now();
        
        for (let i=0; i < bFens.length; i++) {
        
          var fen = bFens[i];
        
          uciExec('ucinewgame');
          uciExec('position fen ' + fen);
          uciExec('go depth 6');
        
          nodes += tNodes;
        }
        
        const t2  = Date.now();
        const sec = (Math.round((t2-t1)/100)/10);
        
        mSilent = 0;
        
        console.log(sec,nodes);
        
        break;
        
        //}}}
      }

      case 'pt': {
        //{{{  perft tests
        
        //{{{  testfens
        
        const pbFens = [
          ['fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR             w KQkq -  0 1', 2, 400,       'cpw-pos1-2'],
          ['fen 4k3/8/8/8/8/8/R7/R3K2R                                  w Q    -  0 1', 3, 4729,      'castling-2'],
          ['fen 4k3/8/8/8/8/8/R7/R3K2R                                  w K    -  0 1', 3, 4686,      'castling-3'],
          ['fen 4k3/8/8/8/8/8/R7/R3K2R                                  w -    -  0 1', 3, 4522,      'castling-4'],
          ['fen r3k2r/r7/8/8/8/8/8/4K3                                  b kq   -  0 1', 3, 4893,      'castling-5'],
          ['fen r3k2r/r7/8/8/8/8/8/4K3                                  b q    -  0 1', 3, 4729,      'castling-6'],
          ['fen r3k2r/r7/8/8/8/8/8/4K3                                  b k    -  0 1', 3, 4686,      'castling-7'],
          ['fen r3k2r/r7/8/8/8/8/8/4K3                                  b -    -  0 1', 3, 4522,      'castling-8'],
          ['fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR             w KQkq -  0 1', 0, 1,         'cpw-pos1-0'],
          ['fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR             w KQkq -  0 1', 1, 20,        'cpw-pos1-1'],
          ['fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR             w KQkq -  0 1', 3, 8902,      'cpw-pos1-3'],
          ['fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR             w KQkq -  0 1', 4, 197281,    'cpw-pos1-4'],
          ['fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR             w KQkq -  0 1', 5, 4865609,   'cpw-pos1-5'],
          ['fen rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R        w KQkq -  0 1', 1, 42,        'cpw-pos5-1'],
          ['fen rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R        w KQkq -  0 1', 2, 1352,      'cpw-pos5-2'],
          ['fen rnbqkb1r/pp1p1ppp/2p5/4P3/2B5/8/PPP1NnPP/RNBQK2R        w KQkq -  0 1', 3, 53392,     'cpw-pos5-3'],
          ['fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -  0 1', 1, 48,        'cpw-pos2-1'],
          ['fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -  0 1', 2, 2039,      'cpw-pos2-2'],
          ['fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -  0 1', 3, 97862,     'cpw-pos2-3'],
          ['fen 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8                         w -    -  0 1', 5, 674624,    'cpw-pos3-5'],
          ['fen n1n5/PPPk4/8/8/8/8/4Kppp/5N1N                           b -    -  0 1', 1, 24,        'prom-1    '],
          ['fen 8/5bk1/8/2Pp4/8/1K6/8/8                                 w -    d6 0 1', 6, 824064,    'ccc-1     '],
          ['fen 8/8/1k6/8/2pP4/8/5BK1/8                                 b -    d3 0 1', 6, 824064,    'ccc-2     '],
          ['fen 8/8/1k6/2b5/2pP4/8/5K2/8                                b -    d3 0 1', 6, 1440467,   'ccc-3     '],
          ['fen 8/5k2/8/2Pp4/2B5/1K6/8/8                                w -    d6 0 1', 6, 1440467,   'ccc-4     '],
          ['fen 5k2/8/8/8/8/8/8/4K2R                                    w K    -  0 1', 6, 661072,    'ccc-5     '],
          ['fen 4k2r/8/8/8/8/8/8/5K2                                    b k    -  0 1', 6, 661072,    'ccc-6     '],
          ['fen 3k4/8/8/8/8/8/8/R3K3                                    w Q    -  0 1', 6, 803711,    'ccc-7     '],
          ['fen r3k3/8/8/8/8/8/8/3K4                                    b q    -  0 1', 6, 803711,    'ccc-8     '],
          ['fen r3k2r/1b4bq/8/8/8/8/7B/R3K2R                            w KQkq -  0 1', 4, 1274206,   'ccc-9     '],
          ['fen r3k2r/7b/8/8/8/8/1B4BQ/R3K2R                            b KQkq -  0 1', 4, 1274206,   'ccc-10    '],
          ['fen r3k2r/8/3Q4/8/8/5q2/8/R3K2R                             b KQkq -  0 1', 4, 1720476,   'ccc-11    '],
          ['fen r3k2r/8/5Q2/8/8/3q4/8/R3K2R                             w KQkq -  0 1', 4, 1720476,   'ccc-12    '],
          ['fen 2K2r2/4P3/8/8/8/8/8/3k4                                 w -    -  0 1', 6, 3821001,   'ccc-13    '],
          ['fen 3K4/8/8/8/8/8/4p3/2k2R2                                 b -    -  0 1', 6, 3821001,   'ccc-14    '],
          ['fen 8/8/1P2K3/8/2n5/1q6/8/5k2                               b -    -  0 1', 5, 1004658,   'ccc-15    '],
          ['fen 5K2/8/1Q6/2N5/8/1p2k3/8/8                               w -    -  0 1', 5, 1004658,   'ccc-16    '],
          ['fen 4k3/1P6/8/8/8/8/K7/8                                    w -    -  0 1', 6, 217342,    'ccc-17    '],
          ['fen 8/k7/8/8/8/8/1p6/4K3                                    b -    -  0 1', 6, 217342,    'ccc-18    '],
          ['fen 8/P1k5/K7/8/8/8/8/8                                     w -    -  0 1', 6, 92683,     'ccc-19    '],
          ['fen 8/8/8/8/8/k7/p1K5/8                                     b -    -  0 1', 6, 92683,     'ccc-20    '],
          ['fen K1k5/8/P7/8/8/8/8/8                                     w -    -  0 1', 6, 2217,      'ccc-21    '],
          ['fen 8/8/8/8/8/p7/8/k1K5                                     b -    -  0 1', 6, 2217,      'ccc-22    '],
          ['fen 8/k1P5/8/1K6/8/8/8/8                                    w -    -  0 1', 7, 567584,    'ccc-23    '],
          ['fen 8/8/8/8/1k6/8/K1p5/8                                    b -    -  0 1', 7, 567584,    'ccc-24    '],
          ['fen 8/8/2k5/5q2/5n2/8/5K2/8                                 b -    -  0 1', 4, 23527,     'ccc-25    '],
          ['fen 8/5k2/8/5N2/5Q2/2K5/8/8                                 w -    -  0 1', 4, 23527,     'ccc-26    '],
          ['fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR             w KQkq -  0 1', 6, 119060324, 'cpw-pos1-6'],
          ['fen 8/p7/8/1P6/K1k3p1/6P1/7P/8                              w -    -  0 1', 8, 8103790,   'jvm-7     '],
          ['fen n1n5/PPPk4/8/8/8/8/4Kppp/5N1N                           b -    -  0 1', 6, 71179139,  'jvm-8     '],
          ['fen r3k2r/p6p/8/B7/1pp1p3/3b4/P6P/R3K2R                     w KQkq -  0 1', 6, 77054993,  'jvm-9     '],
          ['fen 8/5p2/8/2k3P1/p3K3/8/1P6/8                              b -    -  0 1', 8, 64451405,  'jvm-11    '],
          ['fen r3k2r/pb3p2/5npp/n2p4/1p1PPB2/6P1/P2N1PBP/R3K2R         w KQkq -  0 1', 5, 29179893,  'jvm-12    '],
          ['fen 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8                         w -    -  0 1', 7, 178633661, 'jvm-10    '],
          ['fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -  0 1', 5, 193690690, 'jvm-6     '],
          ['fen 8/2pkp3/8/RP3P1Q/6B1/8/2PPP3/rb1K1n1r                   w -    -  0 1', 6, 181153194, 'ob1       '],
          ['fen rnbqkb1r/ppppp1pp/7n/4Pp2/8/8/PPPP1PPP/RNBQKBNR         w KQkq f6 0 1', 6, 244063299, 'jvm-5     '],
          ['fen 8/2ppp3/8/RP1k1P1Q/8/8/2PPP3/rb1K1n1r                   w -    -  0 1', 6, 205552081, 'ob2       '],
          ['fen 8/8/3q4/4r3/1b3n2/8/3PPP2/2k1K2R                        w K    -  0 1', 6, 207139531, 'ob3       '],
          ['fen 4r2r/RP1kP1P1/3P1P2/8/8/3ppp2/1p4p1/4K2R                b K    -  0 1', 6, 314516438, 'ob4       '],
          ['fen r3k2r/8/8/8/3pPp2/8/8/R3K1RR                            b KQkq e3 0 1', 6, 485647607, 'jvm-1     '],
          ['fen 8/3K4/2p5/p2b2r1/5k2/8/8/1q6                            b -    -  0 1', 7, 493407574, 'jvm-4     '],
          ['fen r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1   w kq   -  0 1', 6, 706045033, 'jvm-2     '],
          ['fen r6r/1P4P1/2kPPP2/8/8/3ppp2/1p4p1/R3K2R                  w KQ   -  0 1', 6, 975944981, 'ob5       ']
        ];
        
        //}}}
        
        if (tokens.length > 1)
          var num = parseInt(tokens[1]);
        else
          var num = pbFens.length;
        
        var errs = 0;
        
        const t1 = Date.now();
        
        for (var i=0; i < num; i++) {
        
          const p = pbFens[i];
        
          const fen   = p[0];
          const depth = p[1];
          const moves = p[2];
          const id    = p[3];
        
          uciExec('ucinewgame');
          uciExec('position ' + fen);
        
          const pmoves = perft(depth);
          const err    = pmoves - moves;
        
          errs += err;
        
          const t2  = Date.now();
          const sec = (''+Math.round((t2-t1)/100)/10).padEnd(6);
        
          console.log(sec,id,fen,depth,moves,pmoves,err,errs);
        }
        
        const t2  = Date.now();
        const sec = Math.round((t2-t1)/100)/10;
        
        console.log(sec, 'sec', errs, 'perft errors');
        
        break;
        
        //}}}
      }

      default:
        //{{{  ?
        
        console.log(command, '?');
        
        break;
        
        //}}}
    }
  }
}

//}}}

movelistInitOnce();
hashInitOnce();
cacheInitOnce();
evalInitOnce();

//{{{  connect to stdio

var fs = require('fs');

process.stdin.setEncoding('utf8');

process.stdin.on('readable', function() {
  var chunk = process.stdin.read();
  process.stdin.resume();
  if (chunk !== null) {
    uciExec(chunk);
  }
});

process.stdin.on('end', function() {
  process.exit();
});

//}}}

uciArgv();

