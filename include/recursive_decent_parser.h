#pragma once

#include <typeinfo>

#include "parser_base.h"
#include "json.h"

using namespace std;

class RecursiveDecentParser : public ParserBase
{
public:
  RecursiveDecentParser(ScannerBase *sc);
  AstNode *parse_ast();

private:
  Source *parse_source();
  ProgramDefinition *parse_program();
  ReturnType *parse_return_type();
  FunctionDefinition *parse_function();

  DataType *parse_data_type();
  VariableDefinition *parse_var_def();

  Statement *parse_command();
  SkipStatement *parse_skip_expr();
  StopStatement *parse_stop_expr();
  ReadStatement *parse_read();
  WriteStatement *parse_write();
  ReturnStatement *parse_return_stmt();
  IfStatement *parse_if();
  ForLoop *parse_for();
  AssignmentExpression *parse_int_assign();
  WhileLoop *parse_while();
  Expression *parse_bool_expr();

  Identifier *parse_identifier();
  IntegerLiteral *parse_int();
  FloatLiteral *parse_float();
  StringLiteral *parse_string();
  BooleanLiteral *parse_bool();
  ArrayLiteral *parse_array();
  vector<Expression *> parse_array_value();

  Literal *parse_literal();
  Expression *parse_expr_stmt();
  Expression *parse_expr();
  Expression *parse_assign_expr();
  AssignableExpression *parse_assignable_expr(Expression *expr);
  Expression *parse_or_expr();
  Expression *parse_and_expr();
  Expression *parse_equality_expr();
  Expression *parse_relational_expr();
  Expression *parse_additive_expr();
  Expression *parse_multiplicative_expr();
  Expression *parse_unary_expr();
  Expression *parse_call_expr(Identifier *id);
  Expression *parse_index_expr();
  Expression *parse_primary_expr();

  bool match(TokenType ty);
};
