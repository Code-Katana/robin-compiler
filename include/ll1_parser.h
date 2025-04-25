#pragma once

#include <stack>

#include "parser_base.h"
#include "non_terminal.h"

using namespace std;

struct Symbol
{
  bool isTerm;
  TokenType term;
  NonTerminal nt;
  int reduceRule = 0;
  AstNode *node = nullptr;

  Symbol(TokenType t) : isTerm(true), term(t) {}
  Symbol(NonTerminal n) : isTerm(false), nt(n) {}
  Symbol(int rule) : isTerm(false), reduceRule(rule) {}
};

class LL1Parser : public ParserBase
{
public:
  LL1Parser(ScannerBase *sc);
  AstNode *parse_ast();

private:
  stack<Symbol> st;
  vector<AstNode *> nodes;
  int parseTable[(int)NonTerminal::Call_Expr_Tail_NT + 1][(int)TokenType::END_OF_FILE + 1];

  void fill_table();
  void get_token();
  void match(TokenType t);
  void push_rule(int rule);
  void builder(int rule);
  void syntax_error(string msg);

  void build_source();
  void build_program();
  void build_function();
  void build_variable_definition();
  void build_variable_declaration();
  void build_variable_initialization();
  void build_return_type();
  void build_primitive_type();
  void build_array_type();
  void build_if_statement();
  void build_return_statement();
  void build_skip_statement();
  void build_stop_statement();
  void build_read_statement();
  void build_write_statement();
  void build_for_loop();
  void build_while_loop();
  void build_assignment_expression();
  void build_or_expression();
  void build_and_expression();
  void build_equality_expression();
  void build_relational_expression();
  void build_additive_expression();
  void build_multiplicative_expression();
  void build_unary_expression();
  void build_call_function_expression();
  void build_index_expression();
  void build_primary_expression();
  void build_identifier();
  void build_integer_literal();
  void build_float_literal();
  void build_string_literal();
  void build_boolean_literal();
  void build_array_literal();
};