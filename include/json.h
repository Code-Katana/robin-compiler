#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <typeinfo>

#include "token.h"
#include "ast.h"

using namespace std;

class JSON
{
public:
  // Writing to json file
  static bool debug_file(string path, string json);
  static string format(string json);

  // Tokens
  static string stringify_token(Token tk);
  static string stringify_tokens_stream(vector<Token> tokens);

  // AST Nodes
  static string stringify_node(const AstNode *node);

private:
  JSON();

  // Helpers
  static string quote(string str);
  static string node_type(const AstNode *node);
  static string node_loc(const AstNode *node);
  static string stringify_ast_node(const AstNode *node);

  // Root Node
  static string stringify_source(const Source *src);

  // Declarations
  static string stringify_program(const ProgramDefinition *node);
  static string stringify_function(const FunctionDefinition *node);
  static string stringify_var_def(const VariableDefinition *node);
  static string stringify_var_dec(const VariableDeclaration *node);
  static string stringify_var_init(const VariableInitialization *node);

  // Statements
  static string stringify_stmt(const Statement *stmt);
  static string stringify_if_stmt(const IfStatement *stmt);
  static string stringify_return_stmt(const ReturnStatement *stmt);
  static string stringify_skip_stmt(const SkipStatement *stmt);
  static string stringify_stop_stmt(const StopStatement *stmt);
  static string stringify_read_stmt(const ReadStatement *stmt);
  static string stringify_write_stmt(const WriteStatement *stmt);

  // Loops
  static string stringify_while_loop(const WhileLoop *loop);
  static string stringify_for_loop(const ForLoop *loop);

  // Expressions
  static string stringify_expr(const Expression *expr);
  static string stringify_assignable_expr(const AssignableExpression *expr);
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

  // Types
  static string stringify_type(const DataType *type);
  static string stringify_primitive_type(const PrimitiveDataType *type);
  static string stringify_array_type(const ArrayDataType *type);
  static string stringify_return_type(const ReturnType *type);

  // Literals
  static string stringify_literal(const Literal *node);
  static string stringify_identifier(const Identifier *node);
  static string stringify_integer_literal(const IntegerLiteral *node);
  static string stringify_float_literal(const FloatLiteral *node);
  static string stringify_string_literal(const StringLiteral *node);
  static string stringify_boolean_literal(const BooleanLiteral *node);
  static string stringify_array_literal(const ArrayLiteral *node);
};
