#include <iostream>
#include <string>
#include <vector>

#include "wren_compiler.h"
#include "json.h"

using namespace std;

int main()
{
  string program = R"(
  
  program katana is
    var x: integer;
  begin
    read x;
  end

  )";

  WrenCompiler wc(program, ScannerOptions::FA);

  // stringify to json
  cout << JSON::stringify_tokens_stream(wc.scanner->get_tokens_stream()) << endl;

  system("pause");
  return 0;
}