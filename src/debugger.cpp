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
  string opt_file = DEBUGGING_FOLDER + "/" + PROGRAM_FILE + "_opt.ll";

  string program = read_program(input_file);
  rbn::options::CompilerOptions *options = new rbn::options::CompilerOptions(
      program,
      rbn::options::ScannerOptions::FiniteAutomaton,
      rbn::options::ParserOptions::LL1,
      rbn::options::OptimizationLevels::O3);

  rbn::RobinCompiler *rc = new rbn::RobinCompiler(options);

  vector<rbn::core::Token> tokens = rc->tokenize();
  rbn::ast::AstNode *tree = rc->parse_ast();

  if (auto error = dynamic_cast<rbn::ast::ErrorNode *>(tree))
  {
    cout << error->message << endl;
  }

  rc->generate_ir(output_file);
  return 0;
}
