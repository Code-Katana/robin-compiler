#pragma once

#include <string>
#include <vector>
#include <typeinfo>

#include "token.h"
#include "ast.h"

using namespace std;

class JSON
{
public:
  // Tokens
  static string stringify_token(Token tk);
  static string stringify_tokens_stream(vector<Token> tokens);

  // AST Nodes
  static string stringify_node(const AstNode *node);

private:
  JSON();

  // Helpers
  static string quote(string str);
  static string pair(string type, string value);
  static string quoted_pair(string type, string value);

  static string stringify_ast_node(const AstNode *node);
  // Root Node

  // Declarations

  // Statements

  // Loops

  // Expressions
  static string stringify_expr(const Expression *expr);
  static string stringify_assignment_expr(const AssignmentExpression *expr);
  static string stringify_boolean_expr(const string &exprType, const Expression *exprLeft, const Expression *exprRight);
  static string stringify_binary_expr(const string &exprType, const Expression *exprLeft, const Expression *exprRight, const string &optr);
  static string stringify_or_expr(const OrExpression *expr);
  static string stringify_and_expr(const AndExpression *expr);
  static string stringify_equality_expr(const EqualityExpression *expr);
  static string stringify_relational_expr(const RelationalExpression *expr);
  static string stringify_additive_expr(const AdditiveExpression *expr);
  static string stringify_multiplicative_expr(const MultiplicativeExpression *expr);
  static string stringify_unary_expr(const UnaryExpression *expr);
  static string stringify_call_function_expr(const CallFunctionExpression *expr);
  static string stringify_index_expr(const IndexExpression *expr);
  static string stringify_primary_expr(const PrimaryExpression *expr);
  // Literals
  static string stringify_literal(const Literal *node);
  static string stringify_identifier_literal(const Identifier *node);
  static string stringify_integer_literal(const IntegerLiteral *node);
  static string stringify_float_literal(const FloatLiteral *node);
  static string stringify_string_literal(const StringLiteral *node);
  static string stringify_boolean_literal(const BooleanLiteral *node);
  static string stringify_array_literal(const ArrayLiteral *node);
};
