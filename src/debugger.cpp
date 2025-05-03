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
  string input_file = DEBUGGING_FOLDER + "/" + PROGRAM_FILE;
  string output_file = DEBUGGING_FOLDER + "/" + PROGRAM_FILE + ".ll";
  string program = read_program(input_file);
  CompilerOptions *options = new CompilerOptions(program);

  RobinCompiler *rc = new RobinCompiler(options);

  vector<Token> tokens = rc->tokenize();
  AstNode *tree = rc->parse_ast();

  if (auto error = dynamic_cast<ErrorNode *>(tree))
  {
    cout << error->message << endl;
  }
  else
  {
    JSON::debug_file(Debugger::DEBUGGING_FOLDER + "/tokens.json", JSON::stringify_tokens_stream(tokens));
    JSON::debug_file(Debugger::DEBUGGING_FOLDER + "/tree.json", JSON::stringify_node(tree));

    rc->typecheck();
  }
  rc->generate_ir(output_file);

  return 0;
}
