#include "debugger.h"

string Debugger::DEBUGGING_FOLDER = "./debug";
string Debugger::PROGRAM_FILE = "main.wren";

string read_program(string path)
{
  ifstream file(path);

  if (!file.is_open())
  {
    cout << "Program file " << path << " was not found." << endl;
    system("pause");
    exit(1);
  }

  string program;
  string line;
  while (getline(file, line))
  {
    program += line + '\n';
  }

  return program;
}

int Debugger::run()
{
  string program = read_program(DEBUGGING_FOLDER + "/" + PROGRAM_FILE);

  WrenCompiler *wc = new WrenCompiler(program, ScannerOptions::HandCoded, ParserOptions::RecursiveDecent);

  AstNode *tree = wc->parser->parse_ast();

  JSON::debug_file(DEBUGGING_FOLDER + "/parse_tree.json", JSON::stringify_node(tree));

  wc->semantic->analyze();

  system("pause");
  return 0;
}