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

struct TrackingTokens
{
  Token program_start_token;
  Token program_end_token;
  Token program_kw_token;
  Token function_start_token;
  Token function_end_token;
  Token current_var_token;
  Token current_skip_token;
  Token current_stop_token;
  Token current_read_token;
  Token current_write_token;
  Token current_return_token;
  Token while_start_token;
  Token while_end_token;
  Token for_start_token;
  Token for_end_token;
  Token first_left_square_token;
  Token last_right_square_token;
  Token first_left_paren_token;
  Token last_right_paren_token;
  vector<Token> curly_stack;
  Token last_left_curly_token;
  Token last_right_curly_token;
  vector<Token> index_sq_stack;
  Token index_first_left_sq;
  Token index_last_right_sq;
  vector<Token> if_open_stack;
  vector<Token> if_close_stack;

  bool program_start_token_set = false;
  bool program_end_token_set = false;
  bool while_start_token_set = false;
  bool for_start_token_set = false;
  bool function_start_token_set = false;
  bool program_start_left_square_token = false;
  bool paren_tracked = false;
  bool arr_type_tracking = false;
};

class LL1Parser : public ParserBase
{
public:
  LL1Parser(ScannerBase *sc);
  AstNode *parse_ast();

private:
  Token peeked_token;
  TrackingTokens tracking;
  bool has_peeked;
  vector<Token> peeked_tokens;
  stack<SymbolLL1> st;
  vector<AstNode *> nodes;
  vector<FunctionDefinition *> currentFunctionList;
  vector<VariableDefinition *> currentDeclarationSeq;
  vector<Statement *> currentCommandSeq;
  static AstNode *END_OF_LIST_MARKER;
  static Statement *END_OF_LIST_ELSE;
  static Statement *START_OF_IF;
  int parseTable[(int)NonTerminal::May_be_Arg_NT + 1][(int)TokenType::END_OF_FILE + 1];

  void fill_table();
  Token peek_token();
  Token peek_token_n(int n);
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

  // helpers
  ArrayLiteral *build_array();
  void track_special_tokens(TokenType t, const Token &token);
};