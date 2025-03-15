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
  CompilerOptions *options = new CompilerOptions(program);

  RobinCompiler *rc = new RobinCompiler(options);
  string tokens_stream = JSON::stringify_tokens_stream(rc->tokenize());

  JSON::debug_file(DEBUGGING_FOLDER + "/tokens.json", tokens_stream);

  return 0;
}