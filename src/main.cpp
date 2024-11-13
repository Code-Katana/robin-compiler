#include <iostream>
#include <string>
#include <vector>

#include "wren_compiler.h"
#include "json.h"

using namespace std;

int main()
{
  string program = R"(

  func integer sum has
    var x, y: integer;
  begin
    return x + y;
  end func
  
  program test is
    var a, b: integer;

  begin
    read a, b;
    write sum(a, b);
    for i = 0; i < sum; i++ do
      return sum[i];
    end for
    var x : [[boolean]] = {{not(true)},{false or false},{true}};
  end

  )";

  WrenCompiler wc(program, ScannerOptions::FA, ParserOptions::RecursiveDecent);

  // stringify to json
  // cout << JSON::stringify_tokens_stream(wc.scanner->get_tokens_stream()) << endl;
  cout << JSON::stringify_node(wc.parser->parse_ast()) << endl;

  system("pause");
  return 0;
}