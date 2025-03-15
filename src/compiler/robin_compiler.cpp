#include "robin_compiler.h"

RobinCompiler::RobinCompiler(CompilerOptions *options)
{
  string program = options->get_program();

  switch (options->get_scanner_option())
  {
  case ScannerOptions::HandCoded:
    scanner = new HandCodedScanner(program);
    break;
  case ScannerOptions::FiniteAutomaton:
    scanner = new FAScanner(program);
    break;
  }

  switch (options->get_parser_option())
  {
  case ParserOptions::RecursiveDecent:
    parser = new RecursiveDecentParser(scanner);
    break;
  }
}

vector<Token> RobinCompiler::tokenize()
{
  return scanner->get_tokens_stream();
}
