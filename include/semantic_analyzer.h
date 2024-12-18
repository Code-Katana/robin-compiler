#pragma once

#include "parser_base.h"
#include "json.h"
#include "ast.h"
#include "symbol_table.h"

#include <iostream>
#include <stack>

using namespace std;

class SemanticAnalyzer
{
public:
  SemanticAnalyzer(ParserBase *pr);
  void analyze();

private:
  ParserBase *parser;
  stack<SymbolTable *> call_stack;

  void semantic_source(Source *source);
  void semantic_program(Program *program);
  void sematic_function(Function *func);
  void semantic_command(Statement *stmt, string name_parent);
  void semantic_if(IfStatement *ifstmt, string name_parent);
  void semantic_return(ReturnStatement *rtnStmt, string name_parent);
  void semantic_read(ReadStatement *readStmt);
  void semantic_write(WriteStatement *writeStmt);
  void semantic_while(WhileLoop *loop, string name_parent);
  void semantic_for(ForLoop *loop, string name_parent);
  void semantic_int_assign(AssignmentExpression *assign);
  void semantic_var_def(VariableDefinition *var, SymbolTable *st);
  void semantic_all_expr(Expression *expr);
  void semantic_assign_expr(AssignmentExpression *assignExpr);
  SymbolType semantic_expr(Expression *expr);
  SymbolType semantic_bool_expr(Expression *expr);
  SymbolType semantic_literal(Literal *lit);
  SymbolType semantic_or_expr(OrExpression *orExpr);
  SymbolType semantic_and_expr(AndExpression *andExpr);
  SymbolType semantic_equality_expr(EqualityExpression *eqExpr);
  SymbolType semantic_relational_expr(RelationalExpression *relExpr);
  SymbolType semantic_additive_expr(AdditiveExpression *addExpr);
  SymbolType semantic_multiplicative_expr(MultiplicativeExpression *mulExpr);
  SymbolType semantic_unary_expr(UnaryExpression *unaryExpr);
  SymbolType semantic_call_function_expr(CallFunctionExpression *cfExpr);
  SymbolType semantic_primary_expr(PrimaryExpression *primaryExpr);
  SymbolType semantic_id(Identifier *id);
  pair<SymbolType, int> semantic_array(ArrayLiteral *arrNode);
  pair<SymbolType, int> semantic_array_value(vector<Expression *> elements);
};
