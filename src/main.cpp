#include <iostream>

#include "wren_compiler.h"
#include "json.h"

using namespace std;

int main()
{
  string Source = "for i = 0; i < 5; ++i do write arr[i + 1]; end for";

  WrenCompiler program(Source, ScannerOptions::FA);

  cout << JSON::stringify_tokens_stream(program.scanner->get_tokens_stream()) << endl;

  system("pause");
  return 0;
}