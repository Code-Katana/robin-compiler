#pragma once

#include <string>
#include <vector>
// compiler configuration options
#include "robin/core/options.h"

// lexical analysis phase
#include "robin/lexical/scanner_base.h"
#include "robin/lexical/handcoded_scanner.h"
#include "robin/lexical/fa_scanner.h"

// syntax analysis phase
#include "robin/syntax/parser_base.h"
#include "robin/syntax/recursive_decent_parser.h"
#include "robin/syntax/ll1_parser.h"

// semantic analysis phase
#include "robin/semantic/semantic_analyzer.h"

// intermediate representation phase
#include "robin/ir-generation/ir_generator.h"

// code optimization phase
#include "robin/optimization/code_optimization.h"

namespace rbn
{
  class RobinCompiler
  {
  public:
    RobinCompiler(options::CompilerOptions *options);
    virtual ~RobinCompiler() = default;

    vector<core::Token> tokenize();
    ast::AstNode *parse_ast();
    void typecheck();
    void generate_ir(const string &filename);
    void optimize(const string &filename);

  private:
    lexical::ScannerBase *scanner;
    syntax::ParserBase *parser;
    semantic::SemanticAnalyzer *semantic;
    ir::IRGenerator *generator;
    optimization::CodeOptimization *optimization;
  };
}