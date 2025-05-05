#pragma once

#include <string>
#include <vector>

#include "compiler_options.h"

#include "scanner_base.h"
#include "handcoded_scanner.h"
#include "fa_scanner.h"

#include "parser_base.h"
#include "recursive_decent_parser.h"
#include "ll1_parser.h"

#include "semantic_analyzer.h"

class RobinCompiler
{
public:
  RobinCompiler(CompilerOptions *options);
  virtual ~RobinCompiler() = default;

  vector<Token> tokenize();
  AstNode *parse_ast();
  void typecheck();

private:
  ScannerBase *scanner;
  ParserBase *parser;
  SemanticAnalyzer *semantic;
};