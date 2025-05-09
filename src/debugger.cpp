#include "debugger.h"

string Debugger::DEBUGGING_FOLDER = "./debug";
string Debugger::PROGRAM_FILE = "main.rbn";

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
  CompilerOptions *options1 = new CompilerOptions(program, ParserOptions::RecursiveDecent);
  CompilerOptions *options2 = new CompilerOptions(program, ParserOptions::LL1);

  RobinCompiler *rc1 = new RobinCompiler(options1);
  RobinCompiler *rc2 = new RobinCompiler(options2);
  AstNode *tree1 = rc1->parse_ast();
  AstNode *tree2 = rc2->parse_ast();
  if (dynamic_cast<ErrorNode *>(tree1) || dynamic_cast<ErrorNode *>(tree2))
  {
    ErrorNode *error = dynamic_cast<ErrorNode *>(tree2);
    cout << error->message << endl;
  }
  else
  {
    JSON::debug_file(Debugger::DEBUGGING_FOLDER + "/RecursiveDecent.json", JSON::stringify_node(tree1));
    JSON::debug_file(Debugger::DEBUGGING_FOLDER + "/LL1.json", JSON::stringify_node(tree2));
  }
  system("pause");
  return 0;
}