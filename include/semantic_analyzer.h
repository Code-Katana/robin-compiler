#pragma once

#include "parser_base.h"
#include "json.h"
#include "type_checker.h"
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
  ErrorSymbol get_error();

private:
  ParserBase *parser;
  stack<SymbolTable *> call_stack;
  ErrorSymbol error_symbol;
  bool has_error;

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
  SymbolType semantic_assignable_expr(AssignableExpression *assignable);
  SymbolType semantic_expr(Expression *expr);
  SymbolType semantic_assign_expr(Expression *assignExpr);
  SymbolType semantic_literal(Expression *lit);
  SymbolType semantic_or_expr(Expression *orExpr);
  SymbolType semantic_and_expr(Expression *andExpr);
  SymbolType semantic_equality_expr(Expression *eqExpr);
  SymbolType semantic_relational_expr(Expression *relExpr);
  SymbolType semantic_additive_expr(Expression *addExpr);
  SymbolType semantic_multiplicative_expr(Expression *mulExpr);
  SymbolType semantic_unary_expr(Expression *unaryExpr);
  SymbolType semantic_index_expr(Expression *indexExpr, bool set_init = false, bool allow_partial_indexing = false);
  SymbolType semantic_call_function_expr(Expression *cfExpr);
  SymbolType semantic_primary_expr(Expression *primaryExpr);
  SymbolType semantic_id(Identifier *id, bool set_init = false);
  pair<SymbolType, int> semantic_array(ArrayLiteral *arrNode);
  pair<SymbolType, int> semantic_array_value(vector<Expression *> elements);

  VariableSymbol *is_initialized_var(Identifier *id);
  SymbolTable *retrieve_scope(string sn);
  void is_array(Expression *Expr);

  ErrorSymbol semantic_error(string name, SymbolType st, string err);
};
