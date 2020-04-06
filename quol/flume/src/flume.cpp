// flume - fast log utility for monitoring and email

#include "config.h"

#include <signal.h>
#include <unistd.h>

#include <iostream>

using namespace flume;

static volatile bool gQuitReq = false;


static void
handleSig(int)
{
  gQuitReq = true;
}


int
main(int argc, char **argv) {
  if (argc != 2)
    return 1;

  signal(SIGINT,  handleSig);
  signal(SIGQUIT, handleSig);
  signal(SIGTERM, handleSig);

  try {
    ConfigT cfg;
    cfg.readCfg(argv[1]);
    cfg.start();

    while (!gQuitReq)
      pause();

    cfg.stop();
  }
  catch (const std::exception &ex) {
    std::cout << "Exception: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
