#include "utils/debugger.h"
#include "lsp/lifecycle/start.h"

int main(int argc, char **argv)
{
  lsp::lifecycle::start_lsp();
  return 0;
}
