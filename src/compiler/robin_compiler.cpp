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

  parser = new RecursiveDecentParser(scanner);
  semantic = new SemanticAnalyzer(parser);
  generator = new IRGenerator(semantic);
}

vector<Token> RobinCompiler::tokenize()
{
  return scanner->get_tokens_stream();
}

AstNode *RobinCompiler::parse_ast()
{
  return parser->parse_ast();
}

void RobinCompiler::typecheck()
{
  semantic->analyze();
}

void RobinCompiler::generate_ir(const string &filename)
{
  generator->generate(filename);
}
