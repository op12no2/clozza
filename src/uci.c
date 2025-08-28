
#include "uci.h"

/*{{{  find_token*/

static int find_token(char *token, int n, char **tokens) {

  for (int i=0; i < n; i++) {
    if (!strcmp(token, tokens[i]))
      return i;
  }

  return -1;

}

/*}}}*/
/*{{{  uci_tokens*/

static void uci_tokens(int num_tokens, char **tokens) {

  Node *const root_node = &ss[0];

  if (num_tokens == 0)
    return;

  if (strlen(tokens[0]) == 0)
    return;

  const char *cmd = tokens[0];
  const char *sub = tokens[num_tokens > 1 ? 1 : 0];

  if (!strcmp(cmd, "isready")) {
    /*{{{  isready*/
    
    printf("readyok\n");
    
    /*}}}*/
  }
  else if (!strcmp(cmd, "position") || !strcmp(cmd, "p")) {
    /*{{{  position*/
    
    if (!strcmp(sub, "startpos") || !strcmp(sub, "s"))
      position(root_node, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "KQkq", "-");
    
    else if (!strcmp(sub, "fen") || !strcmp(sub, "f") )
      position(root_node, tokens[2], tokens[3], tokens[4], tokens[5]);
    
    //int n = find_token("moves", num_tokens, tokens);
    //if (n > 0) {
      //for (int i=n+1; i < num_tokens; i++)
        //play_move(root_node, tokens[i]);
    //hack}
    
    /*}}}*/
  }
  else if (!strcmp(cmd, "ucinewgame") || !strcmp(cmd, "u")) {
  }
  else if (!strcmp(cmd, "go") || !strcmp(cmd, "g")) {
  }
  else if (!strcmp(cmd, "uci")) {
    /*{{{  uci*/
    
    printf("id name Clozza 8\n");
    printf("id author Colin Jenkins\n");
    printf("uciok\n");
    
    /*}}}*/
  }
  else if (!strcmp(cmd, "board")|| !strcmp(cmd, "b")) {
    /*{{{  board*/
    
    print_board(root_node);
    
    /*}}}*/
  }
  else if (!strcmp(cmd, "e")) {
  }
  else if (!strcmp(cmd, "perft") || !strcmp(cmd, "f")) {
    /*{{{  perft*/
    
    const int depth = atoi(sub);
    uint64_t start_ms = now_ms();
    uint64_t total_nodes = 0;
    
    uint64_t num_nodes = perft(0, depth);
    total_nodes += num_nodes;
    
    uint64_t end_ms     = now_ms();
    uint64_t elapsed_ms = end_ms - start_ms;
    uint64_t nps        = (total_nodes * 1000ULL) / (elapsed_ms ? elapsed_ms : 1);
    
    printf("time %" PRIu64 " nodes %" PRIu64 " nps %" PRIu64 "\n", elapsed_ms, total_nodes, nps);
    
    /*}}}*/
  }
  else if (!strcmp(cmd, "bench")) {
  }
  else if (!strcmp(cmd, "pt")) {
    /*{{{  perft tests*/
    
    perft_tests();
    
    /*}}}*/
  }
  else if (!strcmp(cmd, "q")) {
    /*{{{  quit*/
    
    exit(0);
    
    /*}}}*/
  }
  else {
    printf("?|%s|\n", cmd);
  }

  return;

}

/*}}}*/
/*{{{  uci_exec*/

void uci_exec(char *line) {

  char *tokens[UCI_TOKENS];
  char *token;

  int num_tokens = 0;

  token = strtok(line, " \t\n");

  while (token != NULL && num_tokens < UCI_TOKENS) {

    tokens[num_tokens++] = token;
    token = strtok(NULL, " \r\t\n");

  }

  uci_tokens(num_tokens, tokens);

}

/*}}}*/

