
#include "main.h"

int main(int argc, char **argv) {

  setvbuf(stdout, NULL, _IONBF, 0);

  magic_init();
  zob_init();
  net_init();

  for (int i=1; i < argc; i++) {
    char buf[UCI_LINE_LENGTH];
    snprintf(buf, sizeof(buf), "%s", argv[i]);
    uci_exec(buf);
  }

  char chunk[UCI_LINE_LENGTH];
  while (fgets(chunk, sizeof(chunk), stdin))
    uci_exec(chunk);

  return 0;

}

