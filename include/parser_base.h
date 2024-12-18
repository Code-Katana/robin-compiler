#pragma once

#include <iostream>
#include <string>

#include "ast.h"
#include "scanner_base.h"

using namespace std;

class ParserBase
{
public:
  ParserBase(ScannerBase *sc);
  void reset_parser();
  virtual ~ParserBase() = default;

  virtual AstNode *parse_ast() = 0;

protected:
  ScannerBase *sc;
  Token current_token;
  Token previous_token;

  Token match(TokenType type);
  void syntax_error(string message);
  bool lookahead(TokenType type);
};