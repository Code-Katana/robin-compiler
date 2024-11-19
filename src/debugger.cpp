#include "debugger.h"

string Debugger::DEBUGGING_FOLDER = "./debug";
string Debugger::PROGRAM_FILE = "main.wren";

string read_program(string path)
{
  ifstream file(path);

  if (!file.is_open())
  {
    cout << "Program file `" << path << "` was not found." << endl;
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

  WrenCompiler wc(program, ScannerOptions::FA, ParserOptions::RecursiveDecent);

  vector<Token> tokens_stream = wc.scanner->get_tokens_stream();

  JSON::debug_file(DEBUGGING_FOLDER + "/tokens_stream.json", JSON::stringify_tokens_stream(tokens_stream));

  for (auto token : tokens_stream)
  {
    cout << token.value << " @" << token.line << endl;
  }

  system("pause");
  return 0;
}
