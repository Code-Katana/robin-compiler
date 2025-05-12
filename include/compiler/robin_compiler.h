#pragma once

#include <string>
#include <vector>
// compiler configuration options
#include "compiler/core/compiler_options.h"

// lexical analysis phase
#include "compiler/lexical/scanner_base.h"
#include "compiler/lexical/handcoded_scanner.h"
#include "compiler/lexical/fa_scanner.h"

// syntax analysis phase
#include "compiler/syntax/parser_base.h"
#include "compiler/syntax/recursive_decent_parser.h"
#include "compiler/syntax/ll1_parser.h"

// semantic analysis phase
#include "compiler/semantic/semantic_analyzer.h"

// intermediate representation phase
#include "compiler/ir-generation/ir_generator.h"

// code optimization phase
#include "compiler/optimization/code_optimization.h"

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