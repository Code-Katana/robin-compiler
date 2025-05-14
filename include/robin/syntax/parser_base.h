#pragma once

#include <iostream>
#include <string>

#include "robin/core/ast.h"
#include "robin/lexical/scanner_base.h"

using namespace std;

namespace rbn::syntax
{
  class ParserBase
  {
  public:
    ParserBase(lexical::ScannerBase *sc);
    void reset_parser();
    virtual ~ParserBase() = default;

    virtual ast::AstNode *parse_ast() = 0;

  protected:
    lexical::ScannerBase *sc;
    core::Token current_token;
    core::Token previous_token;
    ast::ErrorNode *error_node;
    bool has_error;

    virtual bool match(core::TokenType type) = 0;
    ast::ErrorNode *syntax_error(string message);
    bool lookahead(core::TokenType type);
  };
}