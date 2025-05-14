#include "robin/compiler.h"

namespace rbn
{
  RobinCompiler::RobinCompiler(options::CompilerOptions *options)
  {
    string program = options->get_program();

    switch (options->get_scanner_option())
    {
    case options::ScannerOptions::HandCoded:
      scanner = new lexical::HandCodedScanner(program);
      break;
    case options::ScannerOptions::FiniteAutomaton:
      scanner = new lexical::FAScanner(program);
      break;
    }

    switch (options->get_parser_option())
    {
    case options::ParserOptions::RecursiveDecent:
      parser = new syntax::RecursiveDecentParser(scanner);
      break;
    case options::ParserOptions::LL1:
      parser = new syntax::LL1Parser(scanner);
      break;
    }

    semantic = new semantic::SemanticAnalyzer(parser);
    generator = new ir::IRGenerator(semantic);
    optimization = new optimization::CodeOptimization(generator->getModule(), options->get_optimization_option());
  }

  vector<core::Token> RobinCompiler::tokenize()
  {
    return scanner->get_tokens_stream();
  }

  ast::AstNode *RobinCompiler::parse_ast()
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
}