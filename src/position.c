
#include "position.h"

/*{{{  colour_index*/

inline int colour_index(const int colour) {

  return colour * 6;

}

/*}}}*/
/*{{{  piece_index*/

inline int piece_index(const int piece, const int colour) {

  return piece + colour * 6;

}

/*}}}*/

