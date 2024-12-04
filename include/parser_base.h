#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "ast.h"
#include "symbol_table.h"
#include "scanner_base.h"

using namespace std;

class ParserBase
{
public:
  ParserBase(ScannerBase *sc, vector<SymbolTable *> *vector);
  virtual ~ParserBase() = default;

  virtual AstNode *parse_ast() = 0;

protected:
  ScannerBase *sc;
  vector<SymbolTable *> *env;
  Token current_token;
  Token previous_token;
  vector<Token> tokens_stream;

  Token match(TokenType type);
  void set_token_stream(vector<Token> stream);
  void syntax_error(string message);
  bool lookahead(TokenType type);

  SymbolTable *create_scope();
  SymbolTable *init_global_scope();
  SymbolTable *delete_scope();
};