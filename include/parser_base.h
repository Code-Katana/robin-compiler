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
  virtual ~ParserBase() = default;

  virtual AstNode *parse_ast() = 0;

protected:
  ScannerBase *sc;
  int node_start;
  int start_line;
  int node_end;
  int end_line;
  Token current_token;

  Token match(TokenType type);
  void syntax_error(string message);
  bool lookahead(TokenType type);
  void set_start();
  void set_end();
};