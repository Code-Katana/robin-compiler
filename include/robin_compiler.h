#pragma once

#include <string>
#include <vector>
// compiler configuration options
#include "compiler_options.h"

// lexical analysis phase
#include "scanner_base.h"
#include "handcoded_scanner.h"
#include "fa_scanner.h"

// syntax analysis phase
#include "parser_base.h"
#include "recursive_decent_parser.h"

// semantic analysis phase
#include "semantic_analyzer.h"

// intermediate representation phase
#include "ir_generator.h"

//code optimization phase
#include "code_optimization.h"

class RobinCompiler
{
public:
  RobinCompiler(CompilerOptions *options);
  virtual ~RobinCompiler() = default;

  vector<Token> tokenize();
  AstNode *parse_ast();
  void typecheck();
  void generate_ir(const string &filename);
  void optimize(const string &filename);

private:
  ScannerBase *scanner;
  ParserBase *parser;
  SemanticAnalyzer *semantic;
  IRGenerator *generator;
  CodeOptimization *optimization;
};