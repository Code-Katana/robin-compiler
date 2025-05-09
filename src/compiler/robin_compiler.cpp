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
  case ParserOptions::LL1:
    parser = new LL1Parser(scanner);
    break;
  }

  semantic = new SemanticAnalyzer(parser);
  generator = new IRGenerator(semantic);
  optimization = new CodeOptimization(generator->getModule(), options->get_optimization_option());
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

void RobinCompiler::optimize(const string &filename)
{
  optimization->optimize_and_write(filename);
}
