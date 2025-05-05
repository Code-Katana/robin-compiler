#pragma once

#include <stack>
#include <algorithm>

#include "parser_base.h"
#include "non_terminal.h"

using namespace std;

struct SymbolLL1
{
  bool isTerm;
  TokenType term;
  NonTerminal nt;
  int reduceRule = 0;
  AstNode *node = nullptr;

  SymbolLL1(TokenType t) : isTerm(true), term(t) {}
  SymbolLL1(NonTerminal n) : isTerm(false), nt(n) {}
  SymbolLL1(int rule) : isTerm(false), reduceRule(rule) {}
};

class LL1Parser : public ParserBase
{
public:
  LL1Parser(ScannerBase *sc);
  AstNode *parse_ast();

private:
  Token peeked_token;
  bool has_peeked;
  stack<SymbolLL1> st;
  vector<AstNode *> nodes;
  vector<Function *> currentFunctionList;
  vector<VariableDefinition *> currentDeclarationSeq;
  vector<Statement *> currentCommandSeq;
  static AstNode *END_OF_LIST_MARKER;
  static Statement *END_OF_LIST_ELSE;
  int parseTable[(int)NonTerminal::May_be_Arg_NT + 1][(int)TokenType::END_OF_FILE + 1];

  void fill_table();
  Token peek_token();
  void get_token();
  bool match(TokenType t);
  void push_rule(int rule);
  void builder(int rule);

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
  void build_int_assign();
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
  void build_identifier();
  void build_integer_literal();
  void build_float_literal();
  void build_string_literal();
  void build_boolean_literal();
  void build_array_literal();
  void build_function_list();
  void build_declaration_seq();
  void build_command_seq();
};