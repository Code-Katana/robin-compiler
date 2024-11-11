#include <iostream>
#include <string>
#include <vector>

#include "wren_compiler.h"
#include "json.h"

using namespace std;

int main()
{
  string program = R"(
  
  // program katana is
  //   var x: integer;
  // begin
  //   read x;
  // end

  return 69;

  )";

  WrenCompiler wc(program, ScannerOptions::FA, ParserOptions::RecursiveDecent);

  // stringify to json
  cout << JSON::stringify_node(wc.parser->parse_ast()) << endl;

  system("pause");
  return 0;
}