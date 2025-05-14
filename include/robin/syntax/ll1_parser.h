#pragma once

#include <stack>
#include <algorithm>

#include "robin/core/non_terminal.h"
#include "robin/syntax/parser_base.h"

using namespace std;
namespace rbn::syntax
{

  struct SymbolLL1
  {
    bool isTerm;
    core::TokenType term;
    core::NonTerminal nt;
    int reduceRule = 0;
    ast::AstNode *node = nullptr;

    SymbolLL1(core::TokenType t) : isTerm(true), term(t) {}
    SymbolLL1(core::NonTerminal n) : isTerm(false), nt(n) {}
    SymbolLL1(int rule) : isTerm(false), reduceRule(rule) {}
  };

  struct TrackingTokens
  {
    core::Token program_start_token;
    core::Token program_end_token;
    core::Token program_kw_token;
    core::Token function_start_token;
    core::Token function_end_token;
    core::Token current_var_token;
    core::Token current_skip_token;
    core::Token current_stop_token;
    core::Token current_read_token;
    core::Token current_write_token;
    core::Token current_return_token;
    core::Token while_start_token;
    core::Token while_end_token;
    core::Token for_start_token;
    core::Token for_end_token;
    core::Token first_left_square_token;
    core::Token last_right_square_token;
    core::Token first_left_paren_token;
    core::Token last_right_paren_token;
    vector<core::Token> curly_stack;
    core::Token last_left_curly_token;
    core::Token last_right_curly_token;
    vector<core::Token> index_sq_stack;
    core::Token index_first_left_sq;
    core::Token index_last_right_sq;
    vector<core::Token> if_open_stack;
    vector<core::Token> if_close_stack;

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
    LL1Parser(lexical::ScannerBase *sc);
    ast::AstNode *parse_ast();

  private:
    core::Token peeked_token;
    TrackingTokens tracking;
    bool has_peeked;
    vector<core::Token> peeked_tokens;
    stack<SymbolLL1> st;
    vector<ast::AstNode *> nodes;
    vector<ast::FunctionDefinition *> currentFunctionList;
    vector<ast::VariableDefinition *> currentDeclarationSeq;
    vector<ast::Statement *> currentCommandSeq;
    static ast::AstNode *END_OF_LIST_MARKER;
    static ast::Statement *END_OF_LIST_ELSE;
    static ast::Statement *START_OF_IF;
    int parseTable[(int)core::NonTerminal::Call_Value_NT + 1][(int)core::TokenType::END_OF_FILE + 1];

    void fill_table();
    core::Token peek_token();
    core::Token peek_token_n(int n);
    void consume();
    bool match(core::TokenType t);
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
    ast::ArrayLiteral *build_array();
    void track_special_tokens(core::TokenType t, const core::Token &token);
  };
}