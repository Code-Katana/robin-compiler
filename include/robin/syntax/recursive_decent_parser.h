#pragma once

#include <typeinfo>

#include "robin/syntax/parser_base.h"

using namespace std;

namespace rbn::syntax
{
  class RecursiveDecentParser : public ParserBase
  {
  public:
    RecursiveDecentParser(lexical::ScannerBase *sc);
    ast::AstNode *parse_ast();

  private:
    ast::Source *parse_source();
    ast::ProgramDefinition *parse_program();
    ast::ReturnType *parse_return_type();
    ast::FunctionDefinition *parse_function();

    ast::DataType *parse_data_type();
    ast::VariableDefinition *parse_var_def();

    ast::Statement *parse_command();
    ast::SkipStatement *parse_skip_expr();
    ast::StopStatement *parse_stop_expr();
    ast::ReadStatement *parse_read();
    ast::WriteStatement *parse_write();
    ast::ReturnStatement *parse_return_stmt();
    ast::IfStatement *parse_if();
    ast::ForLoop *parse_for();
    ast::AssignmentExpression *parse_int_assign();
    ast::WhileLoop *parse_while();
    ast::Expression *parse_bool_expr();

    ast::Identifier *parse_identifier();
    ast::IntegerLiteral *parse_int();
    ast::FloatLiteral *parse_float();
    ast::StringLiteral *parse_string();
    ast::BooleanLiteral *parse_bool();
    ast::ArrayLiteral *parse_array();
    vector<ast::Expression *> parse_array_value();

    ast::Literal *parse_literal();
    ast::Expression *parse_expr_stmt();
    ast::Expression *parse_expr();
    ast::Expression *parse_assign_expr();
    ast::AssignableExpression *parse_assignable_expr(ast::Expression *expr);
    ast::Expression *parse_or_expr();
    ast::Expression *parse_and_expr();
    ast::Expression *parse_equality_expr();
    ast::Expression *parse_relational_expr();
    ast::Expression *parse_additive_expr();
    ast::Expression *parse_multiplicative_expr();
    ast::Expression *parse_unary_expr();
    ast::Expression *parse_call_expr(ast::Identifier *id);
    ast::Expression *parse_index_expr();
    ast::Expression *parse_primary_expr();

    bool match(core::TokenType ty);
  };
}