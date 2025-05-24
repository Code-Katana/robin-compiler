#pragma once

#include <stack>
#include <typeinfo>

#include "robin/core/ast.h"
#include "robin/syntax/parser_base.h"
#include "robin/semantic/type_checker.h"
#include "robin/semantic/symbol_table.h"

using namespace std;

namespace rbn::semantic
{
  class SemanticAnalyzer
  {
  public:
    SemanticAnalyzer(syntax::ParserBase *pr);
    ast::Source *analyze();
    core::ErrorSymbol get_error();

  private:
    syntax::ParserBase *parser;
    stack<SymbolTable *> call_stack;
    core::ErrorSymbol error_symbol;
    bool has_error;

    void semantic_source(ast::Source *source);
    void semantic_program(ast::ProgramDefinition *program);
    void sematic_function(ast::FunctionDefinition *func);
    void semantic_command(ast::Statement *stmt, string name_parent);
    void semantic_if(ast::IfStatement *ifstmt, string name_parent);
    void semantic_return(ast::ReturnStatement *rtnStmt, string name_parent);
    void semantic_read(ast::ReadStatement *readStmt);
    void semantic_write(ast::WriteStatement *writeStmt);
    void semantic_while(ast::WhileLoop *loop, string name_parent);
    void semantic_for(ast::ForLoop *loop, string name_parent);
    void semantic_int_assign(ast::AssignmentExpression *assign);
    void semantic_var_def(ast::VariableDefinition *var, SymbolTable *st);
    core::SymbolType semantic_assignable_expr(ast::AssignableExpression *assignable);
    core::SymbolType semantic_expr(ast::Expression *expr);
    core::SymbolType semantic_assign_expr(ast::Expression *assignExpr);
    core::SymbolType semantic_literal(ast::Expression *lit);
    core::SymbolType semantic_or_expr(ast::Expression *orExpr);
    core::SymbolType semantic_and_expr(ast::Expression *andExpr);
    core::SymbolType semantic_equality_expr(ast::Expression *eqExpr);
    core::SymbolType semantic_relational_expr(ast::Expression *relExpr);
    core::SymbolType semantic_additive_expr(ast::Expression *addExpr);
    core::SymbolType semantic_multiplicative_expr(ast::Expression *mulExpr);
    core::SymbolType semantic_unary_expr(ast::Expression *unaryExpr);
    core::SymbolType semantic_index_expr(ast::Expression *indexExpr, bool set_init = false, bool allow_partial_indexing = false);
    core::SymbolType semantic_call_function_expr(ast::Expression *cfExpr);
    core::SymbolType semantic_primary_expr(ast::Expression *primaryExpr);
    core::SymbolType semantic_id(ast::Identifier *id, bool set_init = false);
    pair<core::SymbolType, int> semantic_array(ast::ArrayLiteral *arrNode);
    pair<core::SymbolType, int> semantic_array_value(vector<ast::Expression *> elements);

    core::VariableSymbol *is_initialized_var(ast::Identifier *id);
    SymbolTable *retrieve_scope(string sn);
    void is_array(ast::Expression *Expr);

    core::ErrorSymbol semantic_error(string name, core::SymbolType st, string err);
    core::ErrorSymbol forword_syntax_error(ast::ErrorNode *err);
  };
}