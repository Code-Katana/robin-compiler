#include "utils/debugger.h"

string Debugger::DEBUGGING_FOLDER = "./debug";
string Debugger::PROGRAM_FILE = "main.rbn";

int Debugger::run()
{
  string input_file = DEBUGGING_FOLDER + "/" + PROGRAM_FILE;
  string output_file = DEBUGGING_FOLDER + "/" + PROGRAM_FILE + ".ll";
  string opt_file = DEBUGGING_FOLDER + "/" + PROGRAM_FILE + "_opt.ll";

  string program = read_file(input_file);
  rbn::options::CompilerOptions *options = new rbn::options::CompilerOptions(
      program,
      rbn::options::ScannerOptions::FiniteAutomaton,
      rbn::options::ParserOptions::RecursiveDecent,
      rbn::options::OptimizationLevels::O2);

  rbn::RobinCompiler *rc = new rbn::RobinCompiler(options);

  vector<rbn::core::Token> tokens = rc->tokenize();
  rbn::ast::AstNode *tree = rc->parse_ast();

  if (auto error = dynamic_cast<rbn::ast::ErrorNode *>(tree))
  {
    cout << error->message << endl;
  }

  rc->generate_ir(output_file);
  rc->optimize(opt_file);
  system("pause");
  return 0;
}
