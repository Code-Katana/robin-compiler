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
  string program = read_program(input_file);
  CompilerOptions *options = new CompilerOptions(program);

  RobinCompiler *rc = new RobinCompiler(options);
  rc->typecheck();
  system("pause");
  return 0;
}
