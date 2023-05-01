// flume - fast log utility for monitoring and email

#include "config.h"

#include <signal.h>
#include <unistd.h>

#ifdef USE_JEMALLOC
# include <jemalloc/jemalloc.h>
#endif

#include <iostream>

using namespace flume;

static volatile bool gQuitReq = false;


static void
handleSig(int)
{
  gQuitReq = true;
}


#ifdef USE_JEMALLOC
const char *malloc_conf = "junk:true,prof:true,prof_leak:true,prof_final:true,prof_gdump:true,lg_prof_sample:8,prof_prefix:/tmp/jeprof";

namespace {

void getMallCtlStr(const char *name) {
  const char *str;
  size_t len = sizeof(str);
  mallctl(name, &str, &len, nullptr, 0);
  std::cout << "jemalloc " << name << '=' << str << std::endl;
}

void getMallCtlBool(const char *name) {
  bool val;
  size_t len = sizeof(val);
  mallctl(name, &val, &len, nullptr, 0);
  std::cout << "jemalloc " << name << '=' << val << std::endl;
}

} // anonymous
#endif /* USE_JEMALLOC */


int
main(int argc, char **argv) {
#ifdef USE_JEMALLOC
  getMallCtlStr("opt.junk");
  getMallCtlBool("opt.prof");
  getMallCtlBool("opt.prof_leak");
  getMallCtlBool("opt.prof_gdump");
  getMallCtlStr("opt.prof_prefix");
#endif
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
