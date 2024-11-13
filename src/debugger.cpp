#include "debugger.h"

string Debugger::DEBUGGING_FOLDER = "./debug";

int Debugger::run()
{
  string program = R"(

  func [[integer]] sum has
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
    var x : [[boolean]] = { { not(true) },{ false or false },{ true } };
  end

  )";

  WrenCompiler wc(program, ScannerOptions::FA, ParserOptions::RecursiveDecent);

  // stringify to json
  // cout << JSON::stringify_tokens_stream(wc.scanner->get_tokens_stream()) << endl;

  // cout << JSON::stringify_node(wc.parser->parse_ast()) << endl;
  JSON::debug_file(DEBUGGING_FOLDER + "/parse_tree.json", JSON::stringify_node(wc.parser->parse_ast()));

  return 0;
}