#pragma once

#include "parser_base.h"

class RecursiveDecentParser : public ParserBase
{
public:
  RecursiveDecentParser(ScannerBase* sc);
  AstNode *parse_ast();

private:
  AstNode *parse_source(); // todo change return type to Source *
  ReturnStatement *parse_return_stmt();
  Expression *parse_expr();
  PrimaryExpression *parse_primary_expr();
  IntegerLiteral *parse_int_literal();
};