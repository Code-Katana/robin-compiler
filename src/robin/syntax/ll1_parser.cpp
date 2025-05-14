#include "robin/syntax/ll1_parser.h"

#include <cstring>
namespace rbn::syntax
{
  LL1Parser::LL1Parser(lexical::ScannerBase *sc) : ParserBase(sc)
  {
    fill_table();
    has_peeked = false;
    currentFunctionList.clear();
    currentDeclarationSeq.clear();
    currentCommandSeq.clear();
  }

  ast::AstNode *LL1Parser::END_OF_LIST_MARKER = reinterpret_cast<ast::AstNode *>(-1);
  ast::Statement *LL1Parser::END_OF_LIST_ELSE = reinterpret_cast<ast::Statement *>(-1);
  ast::Statement *LL1Parser::START_OF_IF = reinterpret_cast<ast::Statement *>(-2);

  ast::AstNode *LL1Parser::parse_ast()
  {
    st.push(SymbolLL1(core::NonTerminal::Source_NT));

    while (!st.empty())
    {
      SymbolLL1 top = st.top();
      if (has_error)
      {
        return error_node;
      }
      if (top.reduceRule > 0)
      {
        st.pop();
        builder(top.reduceRule);
      }
      else if (!top.isTerm)
      {
        int x = (int)top.nt;
        int y = (int)current_token.type;
        int rule = parseTable[x][y];
        if (rule == 73 && current_token.type == core::TokenType::ID_SY)
        {
          int pos = 1;
          core::Token tk = peek_token_n(pos);

          while (tk.type == core::TokenType::LEFT_SQUARE_PR)
          {
            pos += 3;
            tk = peek_token_n(pos);
          }
          if (tk.type == core::TokenType::EQUAL_OP)
          {
            rule = 72;
          }
        }
        if (rule == 138 && current_token.type == core::TokenType::ELSE_KW)
        {
          core::Token nextToken = peek_token();
          if (nextToken.type == core::TokenType::IF_KW)
          {
            rule = 66;
          }
        }
        if (rule == 0)
        {
          return syntax_error("Unexpected core::token in prediction");
        }
        st.pop();
        push_rule(rule);
      }
      else
      {
        st.pop();
        if (!match(top.term))
        {
          return error_node;
        }
      }
    }

    reset_parser();

    if (has_error)
    {
      return error_node;
    }

    if (nodes.empty())
    {
      return syntax_error("Parsing failed: no AST generated");
    }

    return dynamic_cast<ast::AstNode *>(nodes.back());
  }

  core::Token LL1Parser::peek_token_n(int n)
  {
    while ((int)peeked_tokens.size() < n)
    {
      core::Token next = sc->get_token();
      peeked_tokens.push_back(next);
    }
    return peeked_tokens[n - 1];
  }

  core::Token LL1Parser::peek_token()
  {
    return peek_token_n(1);
  }

  void LL1Parser::consume()
  {
    if (!peeked_tokens.empty())
    {
      current_token = peeked_tokens.front();
      peeked_tokens.erase(peeked_tokens.begin());
    }
    else
    {
      current_token = sc->get_token();

      if (current_token.type == core::TokenType::ERROR)
      {
        forword_lexical_error(&current_token, &previous_token);
      }
    }
  }

  bool LL1Parser::match(core::TokenType t)
  {
    if (current_token.type != t)
    {
      ast::AstNode *error = syntax_error("core::Token mismatch in match: expected " + core::Token::get_token_name(t) + ", got " + core::Token::get_token_name(current_token.type));
      nodes.push_back(error);
      return false;
    }

    track_special_tokens(t, current_token);

    previous_token = current_token;

    ast::AstNode *leaf = nullptr;
    switch (t)
    {
    case core::TokenType::ID_SY:
      leaf = new ast::Identifier(current_token.value, current_token.line, current_token.line, current_token.start, current_token.end);
      break;
    case core::TokenType::INTEGER_NUM:
      leaf = new ast::IntegerLiteral(std::stoi(current_token.value), current_token.line, current_token.line, current_token.start, current_token.end);
      break;
    case core::TokenType::FLOAT_NUM:
      leaf = new ast::FloatLiteral(std::stof(current_token.value), current_token.line, current_token.line, current_token.start, current_token.end);
      break;
    case core::TokenType::STRING_SY:
      leaf = new ast::StringLiteral(current_token.value, current_token.line, current_token.line, current_token.start, current_token.end);
      break;
    case core::TokenType::TRUE_KW:
      leaf = new ast::BooleanLiteral(true, current_token.line, current_token.line, current_token.start, current_token.end);
      break;
    case core::TokenType::FALSE_KW:
      leaf = new ast::BooleanLiteral(false, current_token.line, current_token.line, current_token.start, current_token.end);
      break;
    case core::TokenType::INTEGER_TY:
    case core::TokenType::BOOLEAN_TY:
    case core::TokenType::STRING_TY:
    case core::TokenType::FLOAT_TY:
    case core::TokenType::VOID_TY:
      leaf = new ast::PrimitiveDataType(current_token.value, current_token.line, current_token.line, current_token.start, current_token.end);
      break;
    case core::TokenType::PLUS_OP:
    case core::TokenType::MINUS_OP:
    case core::TokenType::MULT_OP:
    case core::TokenType::DIVIDE_OP:
    case core::TokenType::MOD_OP:
    case core::TokenType::LESS_EQUAL_OP:
    case core::TokenType::IS_EQUAL_OP:
    case core::TokenType::NOT_EQUAL_OP:
    case core::TokenType::LESS_THAN_OP:
    case core::TokenType::GREATER_THAN_OP:
    case core::TokenType::GREATER_EQUAL_OP:
    case core::TokenType::NOT_KW:
    case core::TokenType::INCREMENT_OP:
    case core::TokenType::DECREMENT_OP:
    case core::TokenType::STRINGIFY_OP:
    case core::TokenType::BOOLEAN_OP:
    case core::TokenType::ROUND_OP:
    case core::TokenType::LENGTH_OP:
      leaf = new ast::Identifier(current_token.value, current_token.line, current_token.line, current_token.start, current_token.end);
      break;
    case core::TokenType::RIGHT_CURLY_PR:
      leaf = build_array();
      break;
    }
    if (leaf)
    {
      nodes.push_back(leaf);
    }

    consume();
    return true;
  }

  void LL1Parser::fill_table()
  {
    memset(parseTable, 0, sizeof(parseTable));

    parseTable[(int)core::NonTerminal::Source_NT][(int)core::TokenType::FUNC_KW] = 1;
    parseTable[(int)core::NonTerminal::Source_NT][(int)core::TokenType::PROGRAM_KW] = 1;

    parseTable[(int)core::NonTerminal::Function_List_NT][(int)core::TokenType::PROGRAM_KW] = 2;
    parseTable[(int)core::NonTerminal::Function_List_NT][(int)core::TokenType::FUNC_KW] = 3;

    parseTable[(int)core::NonTerminal::Function_NT][(int)core::TokenType::FUNC_KW] = 4;

    parseTable[(int)core::NonTerminal::Function_Type_NT][(int)core::TokenType::INTEGER_TY] = 5;
    parseTable[(int)core::NonTerminal::Function_Type_NT][(int)core::TokenType::BOOLEAN_TY] = 5;
    parseTable[(int)core::NonTerminal::Function_Type_NT][(int)core::TokenType::STRING_TY] = 5;
    parseTable[(int)core::NonTerminal::Function_Type_NT][(int)core::TokenType::FLOAT_TY] = 5;
    parseTable[(int)core::NonTerminal::Function_Type_NT][(int)core::TokenType::LEFT_SQUARE_PR] = 5;
    parseTable[(int)core::NonTerminal::Function_Type_NT][(int)core::TokenType::VOID_TY] = 6;

    parseTable[(int)core::NonTerminal::Type_NT][(int)core::TokenType::LEFT_SQUARE_PR] = 7;
    parseTable[(int)core::NonTerminal::Type_NT][(int)core::TokenType::INTEGER_TY] = 8;
    parseTable[(int)core::NonTerminal::Type_NT][(int)core::TokenType::BOOLEAN_TY] = 8;
    parseTable[(int)core::NonTerminal::Type_NT][(int)core::TokenType::STRING_TY] = 8;
    parseTable[(int)core::NonTerminal::Type_NT][(int)core::TokenType::FLOAT_TY] = 8;

    parseTable[(int)core::NonTerminal::Array_Type_NT][(int)core::TokenType::LEFT_SQUARE_PR] = 9;

    parseTable[(int)core::NonTerminal::Array_Type_Tail_NT][(int)core::TokenType::INTEGER_TY] = 10;
    parseTable[(int)core::NonTerminal::Array_Type_Tail_NT][(int)core::TokenType::BOOLEAN_TY] = 10;
    parseTable[(int)core::NonTerminal::Array_Type_Tail_NT][(int)core::TokenType::STRING_TY] = 10;
    parseTable[(int)core::NonTerminal::Array_Type_Tail_NT][(int)core::TokenType::FLOAT_TY] = 10;
    parseTable[(int)core::NonTerminal::Array_Type_Tail_NT][(int)core::TokenType::LEFT_SQUARE_PR] = 11;

    parseTable[(int)core::NonTerminal::Primitive_NT][(int)core::TokenType::INTEGER_TY] = 12;
    parseTable[(int)core::NonTerminal::Primitive_NT][(int)core::TokenType::BOOLEAN_TY] = 13;
    parseTable[(int)core::NonTerminal::Primitive_NT][(int)core::TokenType::STRING_TY] = 14;
    parseTable[(int)core::NonTerminal::Primitive_NT][(int)core::TokenType::FLOAT_TY] = 15;

    parseTable[(int)core::NonTerminal::Program_NT][(int)core::TokenType::PROGRAM_KW] = 16;

    parseTable[(int)core::NonTerminal::Block_NT][(int)core::TokenType::VAR_KW] = 17;
    parseTable[(int)core::NonTerminal::Block_NT][(int)core::TokenType::BEGIN_KW] = 17;

    parseTable[(int)core::NonTerminal::Declaration_Seq_NT][(int)core::TokenType::BEGIN_KW] = 18;
    parseTable[(int)core::NonTerminal::Declaration_Seq_NT][(int)core::TokenType::VAR_KW] = 19;

    parseTable[(int)core::NonTerminal::Declaration_NT][(int)core::TokenType::VAR_KW] = 20;

    parseTable[(int)core::NonTerminal::Decl_Tail_NT][(int)core::TokenType::ID_SY] = 21;

    parseTable[(int)core::NonTerminal::Decl_Tail_Right_NT][(int)core::TokenType::SEMI_COLON_SY] = 22;
    parseTable[(int)core::NonTerminal::Decl_Tail_Right_NT][(int)core::TokenType::EQUAL_OP] = 23;

    parseTable[(int)core::NonTerminal::Decl_Init_NT][(int)core::TokenType::TRUE_KW] = 24;
    parseTable[(int)core::NonTerminal::Decl_Init_NT][(int)core::TokenType::FALSE_KW] = 24;
    parseTable[(int)core::NonTerminal::Decl_Init_NT][(int)core::TokenType::NOT_KW] = 24;
    parseTable[(int)core::NonTerminal::Decl_Init_NT][(int)core::TokenType::INTEGER_NUM] = 24;
    parseTable[(int)core::NonTerminal::Decl_Init_NT][(int)core::TokenType::FLOAT_NUM] = 24;
    parseTable[(int)core::NonTerminal::Decl_Init_NT][(int)core::TokenType::MINUS_OP] = 24;
    parseTable[(int)core::NonTerminal::Decl_Init_NT][(int)core::TokenType::INCREMENT_OP] = 24;
    parseTable[(int)core::NonTerminal::Decl_Init_NT][(int)core::TokenType::DECREMENT_OP] = 24;
    parseTable[(int)core::NonTerminal::Decl_Init_NT][(int)core::TokenType::STRINGIFY_OP] = 24;
    parseTable[(int)core::NonTerminal::Decl_Init_NT][(int)core::TokenType::BOOLEAN_OP] = 24;
    parseTable[(int)core::NonTerminal::Decl_Init_NT][(int)core::TokenType::ROUND_OP] = 24;
    parseTable[(int)core::NonTerminal::Decl_Init_NT][(int)core::TokenType::LENGTH_OP] = 24;
    parseTable[(int)core::NonTerminal::Decl_Init_NT][(int)core::TokenType::LEFT_PR] = 24;
    parseTable[(int)core::NonTerminal::Decl_Init_NT][(int)core::TokenType::ID_SY] = 24;
    parseTable[(int)core::NonTerminal::Decl_Init_NT][(int)core::TokenType::STRING_SY] = 24;
    parseTable[(int)core::NonTerminal::Decl_Init_NT][(int)core::TokenType::LEFT_CURLY_PR] = 25;

    parseTable[(int)core::NonTerminal::Array_Value_NT][(int)core::TokenType::RIGHT_CURLY_PR] = 26;
    parseTable[(int)core::NonTerminal::Array_Value_NT][(int)core::TokenType::TRUE_KW] = 27;
    parseTable[(int)core::NonTerminal::Array_Value_NT][(int)core::TokenType::FALSE_KW] = 27;
    parseTable[(int)core::NonTerminal::Array_Value_NT][(int)core::TokenType::NOT_KW] = 27;
    parseTable[(int)core::NonTerminal::Array_Value_NT][(int)core::TokenType::INTEGER_NUM] = 27;
    parseTable[(int)core::NonTerminal::Array_Value_NT][(int)core::TokenType::FLOAT_NUM] = 27;
    parseTable[(int)core::NonTerminal::Array_Value_NT][(int)core::TokenType::MINUS_OP] = 27;
    parseTable[(int)core::NonTerminal::Array_Value_NT][(int)core::TokenType::INCREMENT_OP] = 27;
    parseTable[(int)core::NonTerminal::Array_Value_NT][(int)core::TokenType::DECREMENT_OP] = 27;
    parseTable[(int)core::NonTerminal::Array_Value_NT][(int)core::TokenType::STRINGIFY_OP] = 27;
    parseTable[(int)core::NonTerminal::Array_Value_NT][(int)core::TokenType::BOOLEAN_OP] = 27;
    parseTable[(int)core::NonTerminal::Array_Value_NT][(int)core::TokenType::ROUND_OP] = 27;
    parseTable[(int)core::NonTerminal::Array_Value_NT][(int)core::TokenType::LENGTH_OP] = 27;
    parseTable[(int)core::NonTerminal::Array_Value_NT][(int)core::TokenType::LEFT_PR] = 27;
    parseTable[(int)core::NonTerminal::Array_Value_NT][(int)core::TokenType::ID_SY] = 27;
    parseTable[(int)core::NonTerminal::Array_Value_NT][(int)core::TokenType::STRING_SY] = 27;
    parseTable[(int)core::NonTerminal::Array_Value_NT][(int)core::TokenType::LEFT_CURLY_PR] = 28;

    parseTable[(int)core::NonTerminal::Array_Nested_Value_NT][(int)core::TokenType::LEFT_CURLY_PR] = 29;

    parseTable[(int)core::NonTerminal::More_Nested_NT][(int)core::TokenType::RIGHT_CURLY_PR] = 30;
    parseTable[(int)core::NonTerminal::More_Nested_NT][(int)core::TokenType::COMMA_SY] = 31;

    parseTable[(int)core::NonTerminal::Value_List_NT][(int)core::TokenType::TRUE_KW] = 32;
    parseTable[(int)core::NonTerminal::Value_List_NT][(int)core::TokenType::FALSE_KW] = 32;
    parseTable[(int)core::NonTerminal::Value_List_NT][(int)core::TokenType::NOT_KW] = 32;
    parseTable[(int)core::NonTerminal::Value_List_NT][(int)core::TokenType::INTEGER_NUM] = 32;
    parseTable[(int)core::NonTerminal::Value_List_NT][(int)core::TokenType::FLOAT_NUM] = 32;
    parseTable[(int)core::NonTerminal::Value_List_NT][(int)core::TokenType::MINUS_OP] = 32;
    parseTable[(int)core::NonTerminal::Value_List_NT][(int)core::TokenType::INCREMENT_OP] = 32;
    parseTable[(int)core::NonTerminal::Value_List_NT][(int)core::TokenType::DECREMENT_OP] = 32;
    parseTable[(int)core::NonTerminal::Value_List_NT][(int)core::TokenType::STRINGIFY_OP] = 32;
    parseTable[(int)core::NonTerminal::Value_List_NT][(int)core::TokenType::BOOLEAN_OP] = 32;
    parseTable[(int)core::NonTerminal::Value_List_NT][(int)core::TokenType::ROUND_OP] = 32;
    parseTable[(int)core::NonTerminal::Value_List_NT][(int)core::TokenType::LENGTH_OP] = 32;
    parseTable[(int)core::NonTerminal::Value_List_NT][(int)core::TokenType::LEFT_PR] = 32;
    parseTable[(int)core::NonTerminal::Value_List_NT][(int)core::TokenType::ID_SY] = 32;
    parseTable[(int)core::NonTerminal::Value_List_NT][(int)core::TokenType::STRING_SY] = 32;

    parseTable[(int)core::NonTerminal::More_Values_NT][(int)core::TokenType::RIGHT_CURLY_PR] = 33;
    parseTable[(int)core::NonTerminal::More_Values_NT][(int)core::TokenType::COMMA_SY] = 34;

    parseTable[(int)core::NonTerminal::Variable_List_NT][(int)core::TokenType::ID_SY] = 35;

    parseTable[(int)core::NonTerminal::More_Variables_NT][(int)core::TokenType::COLON_SY] = 36;
    parseTable[(int)core::NonTerminal::More_Variables_NT][(int)core::TokenType::COMMA_SY] = 37;

    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::SKIP_KW] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::STOP_KW] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::READ_KW] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::WRITE_KW] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::FOR_KW] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::WHILE_KW] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::IF_KW] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::RETURN_KW] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::VAR_KW] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::MINUS_OP] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::STRINGIFY_OP] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::BOOLEAN_OP] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::ROUND_OP] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::LENGTH_OP] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::INCREMENT_OP] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::DECREMENT_OP] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::NOT_KW] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::LEFT_PR] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::INTEGER_NUM] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::FLOAT_NUM] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::STRING_TY] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::TRUE_KW] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::FALSE_KW] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::ID_SY] = 38;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::END_KW] = 39;
    parseTable[(int)core::NonTerminal::Command_Seq_NT][(int)core::TokenType::ELSE_KW] = 39;

    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::SKIP_KW] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::STOP_KW] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::READ_KW] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::WRITE_KW] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::FOR_KW] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::WHILE_KW] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::IF_KW] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::RETURN_KW] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::VAR_KW] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::MINUS_OP] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::STRINGIFY_OP] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::BOOLEAN_OP] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::ROUND_OP] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::LENGTH_OP] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::INCREMENT_OP] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::DECREMENT_OP] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::NOT_KW] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::LEFT_PR] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::INTEGER_NUM] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::FLOAT_NUM] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::STRING_TY] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::TRUE_KW] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::FALSE_KW] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::ID_SY] = 40;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::END_KW] = 41;
    parseTable[(int)core::NonTerminal::Command_Seq_Tail_NT][(int)core::TokenType::ELSE_KW] = 41;

    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::SKIP_KW] = 42;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::STOP_KW] = 43;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::READ_KW] = 44;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::WRITE_KW] = 45;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::FOR_KW] = 46;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::WHILE_KW] = 46;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::IF_KW] = 47;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::RETURN_KW] = 48;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::VAR_KW] = 50;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::MINUS_OP] = 49;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::STRINGIFY_OP] = 49;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::BOOLEAN_OP] = 49;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::ROUND_OP] = 49;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::LENGTH_OP] = 49;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::INCREMENT_OP] = 49;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::DECREMENT_OP] = 49;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::NOT_KW] = 49;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::LEFT_PR] = 49;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::INTEGER_NUM] = 49;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::FLOAT_NUM] = 49;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::STRING_TY] = 49;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::TRUE_KW] = 49;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::FALSE_KW] = 49;
    parseTable[(int)core::NonTerminal::Command_NT][(int)core::TokenType::ID_SY] = 49;

    parseTable[(int)core::NonTerminal::Read_Statement_NT][(int)core::TokenType::READ_KW] = 51;

    parseTable[(int)core::NonTerminal::Write_Statement_NT][(int)core::TokenType::WRITE_KW] = 52;

    parseTable[(int)core::NonTerminal::Variable_List_read_NT][(int)core::TokenType::ID_SY] = 53;

    parseTable[(int)core::NonTerminal::More_Variables_read_NT][(int)core::TokenType::SEMI_COLON_SY] = 54;
    parseTable[(int)core::NonTerminal::More_Variables_read_NT][(int)core::TokenType::COMMA_SY] = 55;

    parseTable[(int)core::NonTerminal::Expr_List_NT][(int)core::TokenType::TRUE_KW] = 56;
    parseTable[(int)core::NonTerminal::Expr_List_NT][(int)core::TokenType::FALSE_KW] = 56;
    parseTable[(int)core::NonTerminal::Expr_List_NT][(int)core::TokenType::NOT_KW] = 56;
    parseTable[(int)core::NonTerminal::Expr_List_NT][(int)core::TokenType::INTEGER_NUM] = 56;
    parseTable[(int)core::NonTerminal::Expr_List_NT][(int)core::TokenType::FLOAT_NUM] = 56;
    parseTable[(int)core::NonTerminal::Expr_List_NT][(int)core::TokenType::MINUS_OP] = 56;
    parseTable[(int)core::NonTerminal::Expr_List_NT][(int)core::TokenType::INCREMENT_OP] = 56;
    parseTable[(int)core::NonTerminal::Expr_List_NT][(int)core::TokenType::DECREMENT_OP] = 56;
    parseTable[(int)core::NonTerminal::Expr_List_NT][(int)core::TokenType::STRINGIFY_OP] = 56;
    parseTable[(int)core::NonTerminal::Expr_List_NT][(int)core::TokenType::BOOLEAN_OP] = 56;
    parseTable[(int)core::NonTerminal::Expr_List_NT][(int)core::TokenType::ROUND_OP] = 56;
    parseTable[(int)core::NonTerminal::Expr_List_NT][(int)core::TokenType::LENGTH_OP] = 56;
    parseTable[(int)core::NonTerminal::Expr_List_NT][(int)core::TokenType::LEFT_PR] = 56;
    parseTable[(int)core::NonTerminal::Expr_List_NT][(int)core::TokenType::ID_SY] = 56;
    parseTable[(int)core::NonTerminal::Expr_List_NT][(int)core::TokenType::STRING_SY] = 56;

    parseTable[(int)core::NonTerminal::More_Exprs_NT][(int)core::TokenType::SEMI_COLON_SY] = 57;
    parseTable[(int)core::NonTerminal::More_Exprs_NT][(int)core::TokenType::COMMA_SY] = 58;

    parseTable[(int)core::NonTerminal::Loops_NT][(int)core::TokenType::FOR_KW] = 59;
    parseTable[(int)core::NonTerminal::Loops_NT][(int)core::TokenType::WHILE_KW] = 60;

    parseTable[(int)core::NonTerminal::For_Loop_NT][(int)core::TokenType::FOR_KW] = 61;

    parseTable[(int)core::NonTerminal::Integer_Assign_NT][(int)core::TokenType::ID_SY] = 62;

    parseTable[(int)core::NonTerminal::While_Loop_NT][(int)core::TokenType::WHILE_KW] = 63;

    parseTable[(int)core::NonTerminal::IF_Statement_NT][(int)core::TokenType::IF_KW] = 64;

    parseTable[(int)core::NonTerminal::Else_Part_NT][(int)core::TokenType::END_KW] = 65;
    parseTable[(int)core::NonTerminal::Else_Part_NT][(int)core::TokenType::ELSE_KW] = 138;

    parseTable[(int)core::NonTerminal::Return_NT][(int)core::TokenType::RETURN_KW] = 67;

    parseTable[(int)core::NonTerminal::Return_Value_NT][(int)core::TokenType::SEMI_COLON_SY] = 68;
    parseTable[(int)core::NonTerminal::Return_Value_NT][(int)core::TokenType::TRUE_KW] = 69;
    parseTable[(int)core::NonTerminal::Return_Value_NT][(int)core::TokenType::FALSE_KW] = 69;
    parseTable[(int)core::NonTerminal::Return_Value_NT][(int)core::TokenType::NOT_KW] = 69;
    parseTable[(int)core::NonTerminal::Return_Value_NT][(int)core::TokenType::INTEGER_NUM] = 69;
    parseTable[(int)core::NonTerminal::Return_Value_NT][(int)core::TokenType::FLOAT_NUM] = 69;
    parseTable[(int)core::NonTerminal::Return_Value_NT][(int)core::TokenType::MINUS_OP] = 69;
    parseTable[(int)core::NonTerminal::Return_Value_NT][(int)core::TokenType::INCREMENT_OP] = 69;
    parseTable[(int)core::NonTerminal::Return_Value_NT][(int)core::TokenType::DECREMENT_OP] = 69;
    parseTable[(int)core::NonTerminal::Return_Value_NT][(int)core::TokenType::STRINGIFY_OP] = 69;
    parseTable[(int)core::NonTerminal::Return_Value_NT][(int)core::TokenType::BOOLEAN_OP] = 69;
    parseTable[(int)core::NonTerminal::Return_Value_NT][(int)core::TokenType::ROUND_OP] = 69;
    parseTable[(int)core::NonTerminal::Return_Value_NT][(int)core::TokenType::LENGTH_OP] = 69;
    parseTable[(int)core::NonTerminal::Return_Value_NT][(int)core::TokenType::LEFT_PR] = 69;
    parseTable[(int)core::NonTerminal::Return_Value_NT][(int)core::TokenType::ID_SY] = 69;
    parseTable[(int)core::NonTerminal::Return_Value_NT][(int)core::TokenType::STRING_SY] = 69;

    parseTable[(int)core::NonTerminal::Expr_Statement_NT][(int)core::TokenType::TRUE_KW] = 70;
    parseTable[(int)core::NonTerminal::Expr_Statement_NT][(int)core::TokenType::FALSE_KW] = 70;
    parseTable[(int)core::NonTerminal::Expr_Statement_NT][(int)core::TokenType::NOT_KW] = 70;
    parseTable[(int)core::NonTerminal::Expr_Statement_NT][(int)core::TokenType::INTEGER_NUM] = 70;
    parseTable[(int)core::NonTerminal::Expr_Statement_NT][(int)core::TokenType::FLOAT_NUM] = 70;
    parseTable[(int)core::NonTerminal::Expr_Statement_NT][(int)core::TokenType::MINUS_OP] = 70;
    parseTable[(int)core::NonTerminal::Expr_Statement_NT][(int)core::TokenType::INCREMENT_OP] = 70;
    parseTable[(int)core::NonTerminal::Expr_Statement_NT][(int)core::TokenType::DECREMENT_OP] = 70;
    parseTable[(int)core::NonTerminal::Expr_Statement_NT][(int)core::TokenType::STRINGIFY_OP] = 70;
    parseTable[(int)core::NonTerminal::Expr_Statement_NT][(int)core::TokenType::BOOLEAN_OP] = 70;
    parseTable[(int)core::NonTerminal::Expr_Statement_NT][(int)core::TokenType::ROUND_OP] = 70;
    parseTable[(int)core::NonTerminal::Expr_Statement_NT][(int)core::TokenType::LENGTH_OP] = 70;
    parseTable[(int)core::NonTerminal::Expr_Statement_NT][(int)core::TokenType::LEFT_PR] = 70;
    parseTable[(int)core::NonTerminal::Expr_Statement_NT][(int)core::TokenType::ID_SY] = 70;
    parseTable[(int)core::NonTerminal::Expr_Statement_NT][(int)core::TokenType::STRING_SY] = 70;

    parseTable[(int)core::NonTerminal::Expr_NT][(int)core::TokenType::ID_SY] = 71;
    parseTable[(int)core::NonTerminal::Expr_NT][(int)core::TokenType::TRUE_KW] = 71;
    parseTable[(int)core::NonTerminal::Expr_NT][(int)core::TokenType::FALSE_KW] = 71;
    parseTable[(int)core::NonTerminal::Expr_NT][(int)core::TokenType::NOT_KW] = 71;
    parseTable[(int)core::NonTerminal::Expr_NT][(int)core::TokenType::INTEGER_NUM] = 71;
    parseTable[(int)core::NonTerminal::Expr_NT][(int)core::TokenType::FLOAT_NUM] = 71;
    parseTable[(int)core::NonTerminal::Expr_NT][(int)core::TokenType::MINUS_OP] = 71;
    parseTable[(int)core::NonTerminal::Expr_NT][(int)core::TokenType::INCREMENT_OP] = 71;
    parseTable[(int)core::NonTerminal::Expr_NT][(int)core::TokenType::DECREMENT_OP] = 71;
    parseTable[(int)core::NonTerminal::Expr_NT][(int)core::TokenType::STRINGIFY_OP] = 71;
    parseTable[(int)core::NonTerminal::Expr_NT][(int)core::TokenType::BOOLEAN_OP] = 71;
    parseTable[(int)core::NonTerminal::Expr_NT][(int)core::TokenType::ROUND_OP] = 71;
    parseTable[(int)core::NonTerminal::Expr_NT][(int)core::TokenType::LENGTH_OP] = 71;
    parseTable[(int)core::NonTerminal::Expr_NT][(int)core::TokenType::LEFT_PR] = 71;
    parseTable[(int)core::NonTerminal::Expr_NT][(int)core::TokenType::STRING_SY] = 71;

    parseTable[(int)core::NonTerminal::Assign_Expr_NT][(int)core::TokenType::TRUE_KW] = 73;
    parseTable[(int)core::NonTerminal::Assign_Expr_NT][(int)core::TokenType::FALSE_KW] = 73;
    parseTable[(int)core::NonTerminal::Assign_Expr_NT][(int)core::TokenType::NOT_KW] = 73;
    parseTable[(int)core::NonTerminal::Assign_Expr_NT][(int)core::TokenType::INTEGER_NUM] = 73;
    parseTable[(int)core::NonTerminal::Assign_Expr_NT][(int)core::TokenType::FLOAT_NUM] = 73;
    parseTable[(int)core::NonTerminal::Assign_Expr_NT][(int)core::TokenType::MINUS_OP] = 73;
    parseTable[(int)core::NonTerminal::Assign_Expr_NT][(int)core::TokenType::INCREMENT_OP] = 73;
    parseTable[(int)core::NonTerminal::Assign_Expr_NT][(int)core::TokenType::DECREMENT_OP] = 73;
    parseTable[(int)core::NonTerminal::Assign_Expr_NT][(int)core::TokenType::STRINGIFY_OP] = 73;
    parseTable[(int)core::NonTerminal::Assign_Expr_NT][(int)core::TokenType::BOOLEAN_OP] = 73;
    parseTable[(int)core::NonTerminal::Assign_Expr_NT][(int)core::TokenType::ROUND_OP] = 73;
    parseTable[(int)core::NonTerminal::Assign_Expr_NT][(int)core::TokenType::LENGTH_OP] = 73;
    parseTable[(int)core::NonTerminal::Assign_Expr_NT][(int)core::TokenType::LEFT_PR] = 73;
    parseTable[(int)core::NonTerminal::Assign_Expr_NT][(int)core::TokenType::ID_SY] = 73;
    parseTable[(int)core::NonTerminal::Assign_Expr_NT][(int)core::TokenType::STRING_SY] = 73;

    parseTable[(int)core::NonTerminal::Assign_Expr_Value_NT][(int)core::TokenType::TRUE_KW] = 74;
    parseTable[(int)core::NonTerminal::Assign_Expr_Value_NT][(int)core::TokenType::FALSE_KW] = 74;
    parseTable[(int)core::NonTerminal::Assign_Expr_Value_NT][(int)core::TokenType::NOT_KW] = 74;
    parseTable[(int)core::NonTerminal::Assign_Expr_Value_NT][(int)core::TokenType::INTEGER_NUM] = 74;
    parseTable[(int)core::NonTerminal::Assign_Expr_Value_NT][(int)core::TokenType::FLOAT_NUM] = 74;
    parseTable[(int)core::NonTerminal::Assign_Expr_Value_NT][(int)core::TokenType::MINUS_OP] = 74;
    parseTable[(int)core::NonTerminal::Assign_Expr_Value_NT][(int)core::TokenType::INCREMENT_OP] = 74;
    parseTable[(int)core::NonTerminal::Assign_Expr_Value_NT][(int)core::TokenType::DECREMENT_OP] = 74;
    parseTable[(int)core::NonTerminal::Assign_Expr_Value_NT][(int)core::TokenType::STRINGIFY_OP] = 74;
    parseTable[(int)core::NonTerminal::Assign_Expr_Value_NT][(int)core::TokenType::BOOLEAN_OP] = 74;
    parseTable[(int)core::NonTerminal::Assign_Expr_Value_NT][(int)core::TokenType::ROUND_OP] = 74;
    parseTable[(int)core::NonTerminal::Assign_Expr_Value_NT][(int)core::TokenType::LENGTH_OP] = 74;
    parseTable[(int)core::NonTerminal::Assign_Expr_Value_NT][(int)core::TokenType::LEFT_PR] = 74;
    parseTable[(int)core::NonTerminal::Assign_Expr_Value_NT][(int)core::TokenType::ID_SY] = 74;
    parseTable[(int)core::NonTerminal::Assign_Expr_Value_NT][(int)core::TokenType::STRING_SY] = 74;
    parseTable[(int)core::NonTerminal::Assign_Expr_Value_NT][(int)core::TokenType::LEFT_CURLY_PR] = 75;

    parseTable[(int)core::NonTerminal::Assignable_Expr_NT][(int)core::TokenType::ID_SY] = 76;

    parseTable[(int)core::NonTerminal::Or_Expr_NT][(int)core::TokenType::TRUE_KW] = 77;
    parseTable[(int)core::NonTerminal::Or_Expr_NT][(int)core::TokenType::FALSE_KW] = 77;
    parseTable[(int)core::NonTerminal::Or_Expr_NT][(int)core::TokenType::NOT_KW] = 77;
    parseTable[(int)core::NonTerminal::Or_Expr_NT][(int)core::TokenType::INTEGER_NUM] = 77;
    parseTable[(int)core::NonTerminal::Or_Expr_NT][(int)core::TokenType::FLOAT_NUM] = 77;
    parseTable[(int)core::NonTerminal::Or_Expr_NT][(int)core::TokenType::MINUS_OP] = 77;
    parseTable[(int)core::NonTerminal::Or_Expr_NT][(int)core::TokenType::INCREMENT_OP] = 77;
    parseTable[(int)core::NonTerminal::Or_Expr_NT][(int)core::TokenType::DECREMENT_OP] = 77;
    parseTable[(int)core::NonTerminal::Or_Expr_NT][(int)core::TokenType::STRINGIFY_OP] = 77;
    parseTable[(int)core::NonTerminal::Or_Expr_NT][(int)core::TokenType::BOOLEAN_OP] = 77;
    parseTable[(int)core::NonTerminal::Or_Expr_NT][(int)core::TokenType::ROUND_OP] = 77;
    parseTable[(int)core::NonTerminal::Or_Expr_NT][(int)core::TokenType::LENGTH_OP] = 77;
    parseTable[(int)core::NonTerminal::Or_Expr_NT][(int)core::TokenType::LEFT_PR] = 77;
    parseTable[(int)core::NonTerminal::Or_Expr_NT][(int)core::TokenType::ID_SY] = 77;
    parseTable[(int)core::NonTerminal::Or_Expr_NT][(int)core::TokenType::STRING_SY] = 77;

    parseTable[(int)core::NonTerminal::Or_Tail_NT][(int)core::TokenType::SEMI_COLON_SY] = 78;
    parseTable[(int)core::NonTerminal::Or_Tail_NT][(int)core::TokenType::COMMA_SY] = 78;
    parseTable[(int)core::NonTerminal::Or_Tail_NT][(int)core::TokenType::RIGHT_CURLY_PR] = 78;
    parseTable[(int)core::NonTerminal::Or_Tail_NT][(int)core::TokenType::DO_KW] = 78;
    parseTable[(int)core::NonTerminal::Or_Tail_NT][(int)core::TokenType::THEN_KW] = 78;
    parseTable[(int)core::NonTerminal::Or_Tail_NT][(int)core::TokenType::RIGHT_PR] = 78;
    parseTable[(int)core::NonTerminal::Or_Tail_NT][(int)core::TokenType::RIGHT_SQUARE_PR] = 78;
    parseTable[(int)core::NonTerminal::Or_Tail_NT][(int)core::TokenType::OR_KW] = 79;

    parseTable[(int)core::NonTerminal::And_Expr_NT][(int)core::TokenType::TRUE_KW] = 80;
    parseTable[(int)core::NonTerminal::And_Expr_NT][(int)core::TokenType::FALSE_KW] = 80;
    parseTable[(int)core::NonTerminal::And_Expr_NT][(int)core::TokenType::NOT_KW] = 80;
    parseTable[(int)core::NonTerminal::And_Expr_NT][(int)core::TokenType::INTEGER_NUM] = 80;
    parseTable[(int)core::NonTerminal::And_Expr_NT][(int)core::TokenType::FLOAT_NUM] = 80;
    parseTable[(int)core::NonTerminal::And_Expr_NT][(int)core::TokenType::MINUS_OP] = 80;
    parseTable[(int)core::NonTerminal::And_Expr_NT][(int)core::TokenType::INCREMENT_OP] = 80;
    parseTable[(int)core::NonTerminal::And_Expr_NT][(int)core::TokenType::DECREMENT_OP] = 80;
    parseTable[(int)core::NonTerminal::And_Expr_NT][(int)core::TokenType::STRINGIFY_OP] = 80;
    parseTable[(int)core::NonTerminal::And_Expr_NT][(int)core::TokenType::BOOLEAN_OP] = 80;
    parseTable[(int)core::NonTerminal::And_Expr_NT][(int)core::TokenType::ROUND_OP] = 80;
    parseTable[(int)core::NonTerminal::And_Expr_NT][(int)core::TokenType::LENGTH_OP] = 80;
    parseTable[(int)core::NonTerminal::And_Expr_NT][(int)core::TokenType::LEFT_PR] = 80;
    parseTable[(int)core::NonTerminal::And_Expr_NT][(int)core::TokenType::ID_SY] = 80;
    parseTable[(int)core::NonTerminal::And_Expr_NT][(int)core::TokenType::STRING_SY] = 80;

    parseTable[(int)core::NonTerminal::And_Tail_NT][(int)core::TokenType::SEMI_COLON_SY] = 81;
    parseTable[(int)core::NonTerminal::And_Tail_NT][(int)core::TokenType::COMMA_SY] = 81;
    parseTable[(int)core::NonTerminal::And_Tail_NT][(int)core::TokenType::RIGHT_CURLY_PR] = 81;
    parseTable[(int)core::NonTerminal::And_Tail_NT][(int)core::TokenType::DO_KW] = 81;
    parseTable[(int)core::NonTerminal::And_Tail_NT][(int)core::TokenType::OR_KW] = 81;
    parseTable[(int)core::NonTerminal::And_Tail_NT][(int)core::TokenType::THEN_KW] = 81;
    parseTable[(int)core::NonTerminal::And_Tail_NT][(int)core::TokenType::RIGHT_PR] = 81;
    parseTable[(int)core::NonTerminal::And_Tail_NT][(int)core::TokenType::RIGHT_SQUARE_PR] = 81;
    parseTable[(int)core::NonTerminal::And_Tail_NT][(int)core::TokenType::AND_KW] = 82;

    parseTable[(int)core::NonTerminal::Equality_Expr_NT][(int)core::TokenType::TRUE_KW] = 83;
    parseTable[(int)core::NonTerminal::Equality_Expr_NT][(int)core::TokenType::FALSE_KW] = 83;
    parseTable[(int)core::NonTerminal::Equality_Expr_NT][(int)core::TokenType::NOT_KW] = 83;
    parseTable[(int)core::NonTerminal::Equality_Expr_NT][(int)core::TokenType::INTEGER_NUM] = 83;
    parseTable[(int)core::NonTerminal::Equality_Expr_NT][(int)core::TokenType::FLOAT_NUM] = 83;
    parseTable[(int)core::NonTerminal::Equality_Expr_NT][(int)core::TokenType::MINUS_OP] = 83;
    parseTable[(int)core::NonTerminal::Equality_Expr_NT][(int)core::TokenType::INCREMENT_OP] = 83;
    parseTable[(int)core::NonTerminal::Equality_Expr_NT][(int)core::TokenType::DECREMENT_OP] = 83;
    parseTable[(int)core::NonTerminal::Equality_Expr_NT][(int)core::TokenType::STRINGIFY_OP] = 83;
    parseTable[(int)core::NonTerminal::Equality_Expr_NT][(int)core::TokenType::BOOLEAN_OP] = 83;
    parseTable[(int)core::NonTerminal::Equality_Expr_NT][(int)core::TokenType::ROUND_OP] = 83;
    parseTable[(int)core::NonTerminal::Equality_Expr_NT][(int)core::TokenType::LENGTH_OP] = 83;
    parseTable[(int)core::NonTerminal::Equality_Expr_NT][(int)core::TokenType::LEFT_PR] = 83;
    parseTable[(int)core::NonTerminal::Equality_Expr_NT][(int)core::TokenType::ID_SY] = 83;
    parseTable[(int)core::NonTerminal::Equality_Expr_NT][(int)core::TokenType::STRING_SY] = 83;

    parseTable[(int)core::NonTerminal::Equality_Tail_NT][(int)core::TokenType::SEMI_COLON_SY] = 84;
    parseTable[(int)core::NonTerminal::Equality_Tail_NT][(int)core::TokenType::COMMA_SY] = 84;
    parseTable[(int)core::NonTerminal::Equality_Tail_NT][(int)core::TokenType::RIGHT_CURLY_PR] = 84;
    parseTable[(int)core::NonTerminal::Equality_Tail_NT][(int)core::TokenType::DO_KW] = 84;
    parseTable[(int)core::NonTerminal::Equality_Tail_NT][(int)core::TokenType::OR_KW] = 84;
    parseTable[(int)core::NonTerminal::Equality_Tail_NT][(int)core::TokenType::AND_KW] = 84;
    parseTable[(int)core::NonTerminal::Equality_Tail_NT][(int)core::TokenType::THEN_KW] = 84;
    parseTable[(int)core::NonTerminal::Equality_Tail_NT][(int)core::TokenType::RIGHT_PR] = 84;
    parseTable[(int)core::NonTerminal::Equality_Tail_NT][(int)core::TokenType::RIGHT_SQUARE_PR] = 84;
    parseTable[(int)core::NonTerminal::Equality_Tail_NT][(int)core::TokenType::IS_EQUAL_OP] = 85;
    parseTable[(int)core::NonTerminal::Equality_Tail_NT][(int)core::TokenType::NOT_EQUAL_OP] = 85;

    parseTable[(int)core::NonTerminal::OP_Equal_NT][(int)core::TokenType::IS_EQUAL_OP] = 86;
    parseTable[(int)core::NonTerminal::OP_Equal_NT][(int)core::TokenType::NOT_EQUAL_OP] = 87;

    parseTable[(int)core::NonTerminal::Relational_Expr_NT][(int)core::TokenType::TRUE_KW] = 88;
    parseTable[(int)core::NonTerminal::Relational_Expr_NT][(int)core::TokenType::FALSE_KW] = 88;
    parseTable[(int)core::NonTerminal::Relational_Expr_NT][(int)core::TokenType::NOT_KW] = 88;
    parseTable[(int)core::NonTerminal::Relational_Expr_NT][(int)core::TokenType::INTEGER_NUM] = 88;
    parseTable[(int)core::NonTerminal::Relational_Expr_NT][(int)core::TokenType::FLOAT_NUM] = 88;
    parseTable[(int)core::NonTerminal::Relational_Expr_NT][(int)core::TokenType::MINUS_OP] = 88;
    parseTable[(int)core::NonTerminal::Relational_Expr_NT][(int)core::TokenType::INCREMENT_OP] = 88;
    parseTable[(int)core::NonTerminal::Relational_Expr_NT][(int)core::TokenType::DECREMENT_OP] = 88;
    parseTable[(int)core::NonTerminal::Relational_Expr_NT][(int)core::TokenType::STRINGIFY_OP] = 88;
    parseTable[(int)core::NonTerminal::Relational_Expr_NT][(int)core::TokenType::BOOLEAN_OP] = 88;
    parseTable[(int)core::NonTerminal::Relational_Expr_NT][(int)core::TokenType::ROUND_OP] = 88;
    parseTable[(int)core::NonTerminal::Relational_Expr_NT][(int)core::TokenType::LENGTH_OP] = 88;
    parseTable[(int)core::NonTerminal::Relational_Expr_NT][(int)core::TokenType::LEFT_PR] = 88;
    parseTable[(int)core::NonTerminal::Relational_Expr_NT][(int)core::TokenType::ID_SY] = 88;
    parseTable[(int)core::NonTerminal::Relational_Expr_NT][(int)core::TokenType::STRING_SY] = 88;

    parseTable[(int)core::NonTerminal::Relational_Tail_NT][(int)core::TokenType::SEMI_COLON_SY] = 89;
    parseTable[(int)core::NonTerminal::Relational_Tail_NT][(int)core::TokenType::COMMA_SY] = 89;
    parseTable[(int)core::NonTerminal::Relational_Tail_NT][(int)core::TokenType::RIGHT_CURLY_PR] = 89;
    parseTable[(int)core::NonTerminal::Relational_Tail_NT][(int)core::TokenType::DO_KW] = 89;
    parseTable[(int)core::NonTerminal::Relational_Tail_NT][(int)core::TokenType::OR_KW] = 89;
    parseTable[(int)core::NonTerminal::Relational_Tail_NT][(int)core::TokenType::AND_KW] = 89;
    parseTable[(int)core::NonTerminal::Relational_Tail_NT][(int)core::TokenType::IS_EQUAL_OP] = 89;
    parseTable[(int)core::NonTerminal::Relational_Tail_NT][(int)core::TokenType::NOT_EQUAL_OP] = 89;
    parseTable[(int)core::NonTerminal::Relational_Tail_NT][(int)core::TokenType::THEN_KW] = 89;
    parseTable[(int)core::NonTerminal::Relational_Tail_NT][(int)core::TokenType::RIGHT_PR] = 89;
    parseTable[(int)core::NonTerminal::Relational_Tail_NT][(int)core::TokenType::RIGHT_SQUARE_PR] = 89;
    parseTable[(int)core::NonTerminal::Relational_Tail_NT][(int)core::TokenType::LESS_EQUAL_OP] = 90;
    parseTable[(int)core::NonTerminal::Relational_Tail_NT][(int)core::TokenType::LESS_THAN_OP] = 90;
    parseTable[(int)core::NonTerminal::Relational_Tail_NT][(int)core::TokenType::GREATER_THAN_OP] = 90;
    parseTable[(int)core::NonTerminal::Relational_Tail_NT][(int)core::TokenType::GREATER_EQUAL_OP] = 90;

    parseTable[(int)core::NonTerminal::Relation_NT][(int)core::TokenType::LESS_EQUAL_OP] = 91;
    parseTable[(int)core::NonTerminal::Relation_NT][(int)core::TokenType::LESS_THAN_OP] = 92;
    parseTable[(int)core::NonTerminal::Relation_NT][(int)core::TokenType::GREATER_THAN_OP] = 93;
    parseTable[(int)core::NonTerminal::Relation_NT][(int)core::TokenType::GREATER_EQUAL_OP] = 94;

    parseTable[(int)core::NonTerminal::Additive_Expr_NT][(int)core::TokenType::TRUE_KW] = 95;
    parseTable[(int)core::NonTerminal::Additive_Expr_NT][(int)core::TokenType::FALSE_KW] = 95;
    parseTable[(int)core::NonTerminal::Additive_Expr_NT][(int)core::TokenType::NOT_KW] = 95;
    parseTable[(int)core::NonTerminal::Additive_Expr_NT][(int)core::TokenType::INTEGER_NUM] = 95;
    parseTable[(int)core::NonTerminal::Additive_Expr_NT][(int)core::TokenType::FLOAT_NUM] = 95;
    parseTable[(int)core::NonTerminal::Additive_Expr_NT][(int)core::TokenType::MINUS_OP] = 95;
    parseTable[(int)core::NonTerminal::Additive_Expr_NT][(int)core::TokenType::INCREMENT_OP] = 95;
    parseTable[(int)core::NonTerminal::Additive_Expr_NT][(int)core::TokenType::DECREMENT_OP] = 95;
    parseTable[(int)core::NonTerminal::Additive_Expr_NT][(int)core::TokenType::STRINGIFY_OP] = 95;
    parseTable[(int)core::NonTerminal::Additive_Expr_NT][(int)core::TokenType::BOOLEAN_OP] = 95;
    parseTable[(int)core::NonTerminal::Additive_Expr_NT][(int)core::TokenType::ROUND_OP] = 95;
    parseTable[(int)core::NonTerminal::Additive_Expr_NT][(int)core::TokenType::LENGTH_OP] = 95;
    parseTable[(int)core::NonTerminal::Additive_Expr_NT][(int)core::TokenType::LEFT_PR] = 95;
    parseTable[(int)core::NonTerminal::Additive_Expr_NT][(int)core::TokenType::ID_SY] = 95;
    parseTable[(int)core::NonTerminal::Additive_Expr_NT][(int)core::TokenType::STRING_SY] = 95;

    parseTable[(int)core::NonTerminal::Additive_Tail_NT][(int)core::TokenType::SEMI_COLON_SY] = 96;
    parseTable[(int)core::NonTerminal::Additive_Tail_NT][(int)core::TokenType::COMMA_SY] = 96;
    parseTable[(int)core::NonTerminal::Additive_Tail_NT][(int)core::TokenType::RIGHT_CURLY_PR] = 96;
    parseTable[(int)core::NonTerminal::Additive_Tail_NT][(int)core::TokenType::DO_KW] = 96;
    parseTable[(int)core::NonTerminal::Additive_Tail_NT][(int)core::TokenType::THEN_KW] = 96;
    parseTable[(int)core::NonTerminal::Additive_Tail_NT][(int)core::TokenType::RIGHT_PR] = 96;
    parseTable[(int)core::NonTerminal::Additive_Tail_NT][(int)core::TokenType::RIGHT_SQUARE_PR] = 96;
    parseTable[(int)core::NonTerminal::Additive_Tail_NT][(int)core::TokenType::OR_KW] = 96;
    parseTable[(int)core::NonTerminal::Additive_Tail_NT][(int)core::TokenType::AND_KW] = 96;
    parseTable[(int)core::NonTerminal::Additive_Tail_NT][(int)core::TokenType::IS_EQUAL_OP] = 96;
    parseTable[(int)core::NonTerminal::Additive_Tail_NT][(int)core::TokenType::NOT_EQUAL_OP] = 96;
    parseTable[(int)core::NonTerminal::Additive_Tail_NT][(int)core::TokenType::LESS_EQUAL_OP] = 96;
    parseTable[(int)core::NonTerminal::Additive_Tail_NT][(int)core::TokenType::LESS_THAN_OP] = 96;
    parseTable[(int)core::NonTerminal::Additive_Tail_NT][(int)core::TokenType::GREATER_THAN_OP] = 96;
    parseTable[(int)core::NonTerminal::Additive_Tail_NT][(int)core::TokenType::GREATER_EQUAL_OP] = 96;
    parseTable[(int)core::NonTerminal::Additive_Tail_NT][(int)core::TokenType::PLUS_OP] = 97;
    parseTable[(int)core::NonTerminal::Additive_Tail_NT][(int)core::TokenType::MINUS_OP] = 97;

    parseTable[(int)core::NonTerminal::Weak_OP_NT][(int)core::TokenType::PLUS_OP] = 98;
    parseTable[(int)core::NonTerminal::Weak_OP_NT][(int)core::TokenType::MINUS_OP] = 99;

    parseTable[(int)core::NonTerminal::Multiplicative_Expr_NT][(int)core::TokenType::TRUE_KW] = 100;
    parseTable[(int)core::NonTerminal::Multiplicative_Expr_NT][(int)core::TokenType::FALSE_KW] = 100;
    parseTable[(int)core::NonTerminal::Multiplicative_Expr_NT][(int)core::TokenType::NOT_KW] = 100;
    parseTable[(int)core::NonTerminal::Multiplicative_Expr_NT][(int)core::TokenType::INTEGER_NUM] = 100;
    parseTable[(int)core::NonTerminal::Multiplicative_Expr_NT][(int)core::TokenType::FLOAT_NUM] = 100;
    parseTable[(int)core::NonTerminal::Multiplicative_Expr_NT][(int)core::TokenType::MINUS_OP] = 100;
    parseTable[(int)core::NonTerminal::Multiplicative_Expr_NT][(int)core::TokenType::INCREMENT_OP] = 100;
    parseTable[(int)core::NonTerminal::Multiplicative_Expr_NT][(int)core::TokenType::DECREMENT_OP] = 100;
    parseTable[(int)core::NonTerminal::Multiplicative_Expr_NT][(int)core::TokenType::STRINGIFY_OP] = 100;
    parseTable[(int)core::NonTerminal::Multiplicative_Expr_NT][(int)core::TokenType::BOOLEAN_OP] = 100;
    parseTable[(int)core::NonTerminal::Multiplicative_Expr_NT][(int)core::TokenType::ROUND_OP] = 100;
    parseTable[(int)core::NonTerminal::Multiplicative_Expr_NT][(int)core::TokenType::LENGTH_OP] = 100;
    parseTable[(int)core::NonTerminal::Multiplicative_Expr_NT][(int)core::TokenType::LEFT_PR] = 100;
    parseTable[(int)core::NonTerminal::Multiplicative_Expr_NT][(int)core::TokenType::ID_SY] = 100;
    parseTable[(int)core::NonTerminal::Multiplicative_Expr_NT][(int)core::TokenType::STRING_SY] = 100;

    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::SEMI_COLON_SY] = 101;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::COMMA_SY] = 101;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::RIGHT_CURLY_PR] = 101;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::DO_KW] = 101;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::THEN_KW] = 101;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::RIGHT_PR] = 101;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::RIGHT_SQUARE_PR] = 101;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::OR_KW] = 101;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::AND_KW] = 101;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::IS_EQUAL_OP] = 101;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::NOT_EQUAL_OP] = 101;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::LESS_EQUAL_OP] = 101;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::LESS_THAN_OP] = 101;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::GREATER_THAN_OP] = 101;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::GREATER_EQUAL_OP] = 101;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::PLUS_OP] = 101;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::MINUS_OP] = 101;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::MULT_OP] = 102;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::DIVIDE_OP] = 102;
    parseTable[(int)core::NonTerminal::Multiplicative_Tail_NT][(int)core::TokenType::MOD_OP] = 102;

    parseTable[(int)core::NonTerminal::Strong_OP_NT][(int)core::TokenType::MULT_OP] = 103;
    parseTable[(int)core::NonTerminal::Strong_OP_NT][(int)core::TokenType::DIVIDE_OP] = 104;
    parseTable[(int)core::NonTerminal::Strong_OP_NT][(int)core::TokenType::MOD_OP] = 105;

    parseTable[(int)core::NonTerminal::Unary_Expr_NT][(int)core::TokenType::MINUS_OP] = 106;
    parseTable[(int)core::NonTerminal::Unary_Expr_NT][(int)core::TokenType::STRINGIFY_OP] = 107;
    parseTable[(int)core::NonTerminal::Unary_Expr_NT][(int)core::TokenType::BOOLEAN_OP] = 108;
    parseTable[(int)core::NonTerminal::Unary_Expr_NT][(int)core::TokenType::ROUND_OP] = 109;
    parseTable[(int)core::NonTerminal::Unary_Expr_NT][(int)core::TokenType::LENGTH_OP] = 110;
    parseTable[(int)core::NonTerminal::Unary_Expr_NT][(int)core::TokenType::INCREMENT_OP] = 111;
    parseTable[(int)core::NonTerminal::Unary_Expr_NT][(int)core::TokenType::DECREMENT_OP] = 111;
    parseTable[(int)core::NonTerminal::Unary_Expr_NT][(int)core::TokenType::NOT_KW] = 112;
    parseTable[(int)core::NonTerminal::Unary_Expr_NT][(int)core::TokenType::TRUE_KW] = 113;
    parseTable[(int)core::NonTerminal::Unary_Expr_NT][(int)core::TokenType::FALSE_KW] = 113;
    parseTable[(int)core::NonTerminal::Unary_Expr_NT][(int)core::TokenType::INTEGER_NUM] = 113;
    parseTable[(int)core::NonTerminal::Unary_Expr_NT][(int)core::TokenType::FLOAT_NUM] = 113;
    parseTable[(int)core::NonTerminal::Unary_Expr_NT][(int)core::TokenType::LEFT_PR] = 113;
    parseTable[(int)core::NonTerminal::Unary_Expr_NT][(int)core::TokenType::ID_SY] = 113;
    parseTable[(int)core::NonTerminal::Unary_Expr_NT][(int)core::TokenType::STRING_SY] = 113;

    parseTable[(int)core::NonTerminal::Prefix_NT][(int)core::TokenType::INCREMENT_OP] = 114;
    parseTable[(int)core::NonTerminal::Prefix_NT][(int)core::TokenType::DECREMENT_OP] = 115;

    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::SEMI_COLON_SY] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::COMMA_SY] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::RIGHT_CURLY_PR] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::DO_KW] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::THEN_KW] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::RIGHT_PR] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::RIGHT_SQUARE_PR] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::OR_KW] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::AND_KW] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::IS_EQUAL_OP] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::NOT_EQUAL_OP] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::LESS_EQUAL_OP] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::LESS_THAN_OP] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::GREATER_THAN_OP] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::GREATER_EQUAL_OP] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::PLUS_OP] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::MINUS_OP] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::MULT_OP] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::DIVIDE_OP] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::MOD_OP] = 116;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::INCREMENT_OP] = 117;
    parseTable[(int)core::NonTerminal::maybe_Postfix_NT][(int)core::TokenType::DECREMENT_OP] = 118;

    parseTable[(int)core::NonTerminal::Index_Expr_NT][(int)core::TokenType::ID_SY] = 119;
    parseTable[(int)core::NonTerminal::Index_Expr_NT][(int)core::TokenType::LEFT_PR] = 120;
    parseTable[(int)core::NonTerminal::Index_Expr_NT][(int)core::TokenType::INTEGER_NUM] = 120;
    parseTable[(int)core::NonTerminal::Index_Expr_NT][(int)core::TokenType::FLOAT_NUM] = 120;
    parseTable[(int)core::NonTerminal::Index_Expr_NT][(int)core::TokenType::STRING_SY] = 120;
    parseTable[(int)core::NonTerminal::Index_Expr_NT][(int)core::TokenType::TRUE_KW] = 120;
    parseTable[(int)core::NonTerminal::Index_Expr_NT][(int)core::TokenType::FALSE_KW] = 120;

    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::SEMI_COLON_SY] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::COMMA_SY] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::RIGHT_CURLY_PR] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::DO_KW] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::THEN_KW] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::RIGHT_PR] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::RIGHT_SQUARE_PR] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::OR_KW] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::AND_KW] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::IS_EQUAL_OP] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::NOT_EQUAL_OP] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::LESS_EQUAL_OP] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::LESS_THAN_OP] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::GREATER_THAN_OP] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::GREATER_EQUAL_OP] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::PLUS_OP] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::MINUS_OP] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::MULT_OP] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::DIVIDE_OP] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::MOD_OP] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::EQUAL_OP] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::INCREMENT_OP] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::DECREMENT_OP] = 121;
    parseTable[(int)core::NonTerminal::Index_Chain_NT][(int)core::TokenType::LEFT_SQUARE_PR] = 122;

    parseTable[(int)core::NonTerminal::Primary_Expr_NT][(int)core::TokenType::LEFT_PR] = 123;
    parseTable[(int)core::NonTerminal::Primary_Expr_NT][(int)core::TokenType::INTEGER_NUM] = 124;
    parseTable[(int)core::NonTerminal::Primary_Expr_NT][(int)core::TokenType::FLOAT_NUM] = 124;
    parseTable[(int)core::NonTerminal::Primary_Expr_NT][(int)core::TokenType::STRING_SY] = 124;
    parseTable[(int)core::NonTerminal::Primary_Expr_NT][(int)core::TokenType::TRUE_KW] = 124;
    parseTable[(int)core::NonTerminal::Primary_Expr_NT][(int)core::TokenType::FALSE_KW] = 124;

    parseTable[(int)core::NonTerminal::Literal_NT][(int)core::TokenType::INTEGER_NUM] = 125;
    parseTable[(int)core::NonTerminal::Literal_NT][(int)core::TokenType::FLOAT_NUM] = 126;
    parseTable[(int)core::NonTerminal::Literal_NT][(int)core::TokenType::STRING_SY] = 127;
    parseTable[(int)core::NonTerminal::Literal_NT][(int)core::TokenType::TRUE_KW] = 128;
    parseTable[(int)core::NonTerminal::Literal_NT][(int)core::TokenType::FALSE_KW] = 129;

    parseTable[(int)core::NonTerminal::CallOrVariable_NT][(int)core::TokenType::ID_SY] = 130;

    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::SEMI_COLON_SY] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::COMMA_SY] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::RIGHT_CURLY_PR] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::DO_KW] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::THEN_KW] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::RIGHT_PR] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::RIGHT_SQUARE_PR] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::OR_KW] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::AND_KW] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::INCREMENT_OP] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::DECREMENT_OP] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::IS_EQUAL_OP] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::NOT_EQUAL_OP] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::LESS_EQUAL_OP] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::LESS_THAN_OP] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::GREATER_THAN_OP] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::GREATER_EQUAL_OP] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::PLUS_OP] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::MINUS_OP] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::MULT_OP] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::DIVIDE_OP] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::MOD_OP] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::EQUAL_OP] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::LEFT_SQUARE_PR] = 131;
    parseTable[(int)core::NonTerminal::MaybeCall_NT][(int)core::TokenType::LEFT_PR] = 132;

    parseTable[(int)core::NonTerminal::Call_Expr_NT][(int)core::TokenType::LEFT_PR] = 133;

    parseTable[(int)core::NonTerminal::Call_Expr_Tail_NT][(int)core::TokenType::RIGHT_PR] = 134;
    parseTable[(int)core::NonTerminal::Call_Expr_Tail_NT][(int)core::TokenType::COMMA_SY] = 135;

    parseTable[(int)core::NonTerminal::May_be_Arg_NT][(int)core::TokenType::TRUE_KW] = 136;
    parseTable[(int)core::NonTerminal::May_be_Arg_NT][(int)core::TokenType::FALSE_KW] = 136;
    parseTable[(int)core::NonTerminal::May_be_Arg_NT][(int)core::TokenType::NOT_KW] = 136;
    parseTable[(int)core::NonTerminal::May_be_Arg_NT][(int)core::TokenType::INTEGER_NUM] = 136;
    parseTable[(int)core::NonTerminal::May_be_Arg_NT][(int)core::TokenType::FLOAT_NUM] = 136;
    parseTable[(int)core::NonTerminal::May_be_Arg_NT][(int)core::TokenType::MINUS_OP] = 136;
    parseTable[(int)core::NonTerminal::May_be_Arg_NT][(int)core::TokenType::INCREMENT_OP] = 136;
    parseTable[(int)core::NonTerminal::May_be_Arg_NT][(int)core::TokenType::DECREMENT_OP] = 136;
    parseTable[(int)core::NonTerminal::May_be_Arg_NT][(int)core::TokenType::STRINGIFY_OP] = 136;
    parseTable[(int)core::NonTerminal::May_be_Arg_NT][(int)core::TokenType::BOOLEAN_OP] = 136;
    parseTable[(int)core::NonTerminal::May_be_Arg_NT][(int)core::TokenType::ROUND_OP] = 136;
    parseTable[(int)core::NonTerminal::May_be_Arg_NT][(int)core::TokenType::LENGTH_OP] = 136;
    parseTable[(int)core::NonTerminal::May_be_Arg_NT][(int)core::TokenType::LEFT_PR] = 136;
    parseTable[(int)core::NonTerminal::May_be_Arg_NT][(int)core::TokenType::ID_SY] = 136;
    parseTable[(int)core::NonTerminal::May_be_Arg_NT][(int)core::TokenType::STRING_SY] = 136;
    parseTable[(int)core::NonTerminal::May_be_Arg_NT][(int)core::TokenType::RIGHT_PR] = 137;
  }

  void LL1Parser::push_rule(int rule)
  {
    switch (rule)
    {
    case 1:
      st.push(SymbolLL1(1));
      st.push(SymbolLL1(core::TokenType::END_OF_FILE));
      st.push(SymbolLL1(core::NonTerminal::Program_NT));
      st.push(SymbolLL1(core::NonTerminal::Function_List_NT));
      break;
    case 2:
      break;
    case 3:
      st.push(SymbolLL1(3));
      st.push(SymbolLL1(core::NonTerminal::Function_List_NT));
      st.push(SymbolLL1(core::NonTerminal::Function_NT));
      break;
    case 4:
      st.push(SymbolLL1(4));
      st.push(SymbolLL1(core::TokenType::FUNC_KW));
      st.push(SymbolLL1(core::TokenType::END_KW));
      st.push(SymbolLL1(core::NonTerminal::Command_Seq_NT));
      st.push(SymbolLL1(core::TokenType::BEGIN_KW));
      st.push(SymbolLL1(core::NonTerminal::Declaration_Seq_NT));
      st.push(SymbolLL1(core::TokenType::HAS_KW));
      st.push(SymbolLL1(core::TokenType::ID_SY));
      st.push(SymbolLL1(core::NonTerminal::Function_Type_NT));
      st.push(SymbolLL1(core::TokenType::FUNC_KW));
      break;
    case 5:
      st.push(SymbolLL1(5));
      st.push(SymbolLL1(core::NonTerminal::Type_NT));
      break;
    case 6:
      st.push(SymbolLL1(6));
      st.push(SymbolLL1(core::TokenType::VOID_TY));
      break;
    case 7:
      st.push(SymbolLL1(7));
      st.push(SymbolLL1(core::NonTerminal::Array_Type_NT));

      break;
    case 8:
      st.push(SymbolLL1(core::NonTerminal::Primitive_NT));
      break;
    case 9:
      tracking.arr_type_tracking = true;
      st.push(SymbolLL1(core::NonTerminal::Array_Type_Tail_NT));
      st.push(SymbolLL1(core::TokenType::LEFT_SQUARE_PR));
      break;
    case 10:
      st.push(SymbolLL1(core::TokenType::RIGHT_SQUARE_PR));
      st.push(SymbolLL1(core::NonTerminal::Primitive_NT));
      break;
    case 11:
      st.push(SymbolLL1(11));
      st.push(SymbolLL1(core::TokenType::RIGHT_SQUARE_PR));
      st.push(SymbolLL1(core::NonTerminal::Array_Type_NT));
      break;
    case 12:
      st.push(SymbolLL1(12));
      st.push(SymbolLL1(core::TokenType::INTEGER_TY));
      break;
    case 13:
      st.push(SymbolLL1(13));
      st.push(SymbolLL1(core::TokenType::BOOLEAN_TY));
      break;
    case 14:
      st.push(SymbolLL1(14));
      st.push(SymbolLL1(core::TokenType::STRING_TY));
      break;
    case 15:
      st.push(SymbolLL1(15));
      st.push(SymbolLL1(core::TokenType::FLOAT_TY));
      break;
    case 16:
      st.push(SymbolLL1(16));
      st.push(SymbolLL1(core::NonTerminal::Block_NT));
      st.push(SymbolLL1(core::TokenType::IS_KW));
      st.push(SymbolLL1(core::TokenType::ID_SY));
      st.push(SymbolLL1(core::TokenType::PROGRAM_KW));
      break;
    case 17:
      st.push(SymbolLL1(core::TokenType::END_KW));
      st.push(SymbolLL1(core::NonTerminal::Command_Seq_NT));
      st.push(SymbolLL1(core::TokenType::BEGIN_KW));
      st.push(SymbolLL1(core::NonTerminal::Declaration_Seq_NT));
      break;
    case 18:
      break;
    case 19:
      st.push(SymbolLL1(19));
      st.push(SymbolLL1(core::NonTerminal::Declaration_Seq_NT));
      st.push(SymbolLL1(core::NonTerminal::Declaration_NT));
      break;
    case 20:
      st.push(SymbolLL1(20));
      st.push(SymbolLL1(core::NonTerminal::Decl_Tail_NT));
      nodes.push_back(END_OF_LIST_MARKER);
      st.push(SymbolLL1(core::TokenType::VAR_KW));
      break;
    case 21:
      st.push(SymbolLL1(21));
      st.push(SymbolLL1(core::NonTerminal::Decl_Tail_Right_NT));
      st.push(SymbolLL1(core::NonTerminal::Type_NT));
      st.push(SymbolLL1(core::TokenType::COLON_SY));
      st.push(SymbolLL1(core::NonTerminal::Variable_List_NT));
      break;
    case 22:
      st.push(SymbolLL1(core::TokenType::SEMI_COLON_SY));
      break;
    case 23:
      st.push(SymbolLL1(23));
      st.push(SymbolLL1(core::TokenType::SEMI_COLON_SY));
      st.push(SymbolLL1(core::NonTerminal::Decl_Init_NT));
      st.push(SymbolLL1(core::TokenType::EQUAL_OP));
      break;
    case 24:
      st.push(SymbolLL1(core::NonTerminal::Or_Expr_NT));
      break;
    case 25:
      st.push(SymbolLL1(25));
      st.push(SymbolLL1(core::TokenType::RIGHT_CURLY_PR));
      st.push(SymbolLL1(core::NonTerminal::Array_Value_NT));
      nodes.push_back(END_OF_LIST_MARKER);
      st.push(SymbolLL1(core::TokenType::LEFT_CURLY_PR));
      break;
    case 26:
      break;
    case 27:
      st.push(SymbolLL1(core::NonTerminal::Value_List_NT));
      break;
    case 28:
      st.push(SymbolLL1(core::NonTerminal::Array_Nested_Value_NT));
      break;
    case 29:
      st.push(SymbolLL1(core::NonTerminal::More_Nested_NT));
      st.push(SymbolLL1(core::TokenType::RIGHT_CURLY_PR));
      st.push(SymbolLL1(core::NonTerminal::Array_Value_NT));
      nodes.push_back(END_OF_LIST_MARKER);
      st.push(SymbolLL1(core::TokenType::LEFT_CURLY_PR));
      break;
    case 30:
      break;
    case 31:
      st.push(SymbolLL1(core::NonTerminal::Array_Nested_Value_NT));
      st.push(SymbolLL1(core::TokenType::COMMA_SY));
      break;
    case 32:
      st.push(SymbolLL1(core::NonTerminal::More_Values_NT));
      st.push(SymbolLL1(core::NonTerminal::Or_Expr_NT));
      break;
    case 33:
      break;
    case 34:
      st.push(SymbolLL1(core::NonTerminal::Value_List_NT));
      st.push(SymbolLL1(core::TokenType::COMMA_SY));
      break;
    case 35:
      st.push(SymbolLL1(core::NonTerminal::More_Variables_NT));
      st.push(SymbolLL1(core::TokenType::ID_SY));

      break;
    case 36:
      break;
    case 37:
      st.push(SymbolLL1(core::NonTerminal::Variable_List_NT));
      st.push(SymbolLL1(core::TokenType::COMMA_SY));
      break;
    case 38:
      st.push(SymbolLL1(38));
      st.push(SymbolLL1(core::NonTerminal::Command_Seq_Tail_NT));
      st.push(SymbolLL1(core::NonTerminal::Command_NT));
      break;
    case 39:
      break;
    case 40:
      st.push(SymbolLL1(40));
      st.push(SymbolLL1(core::NonTerminal::Command_Seq_Tail_NT));
      st.push(SymbolLL1(core::NonTerminal::Command_NT));
      break;
    case 41:
      break;
    case 42:
      st.push(SymbolLL1(42));
      st.push(SymbolLL1(core::TokenType::SEMI_COLON_SY));
      st.push(SymbolLL1(core::TokenType::SKIP_KW));
      break;
    case 43:
      st.push(SymbolLL1(43));
      st.push(SymbolLL1(core::TokenType::SEMI_COLON_SY));
      st.push(SymbolLL1(core::TokenType::STOP_KW));
      break;
    case 44:
      st.push(SymbolLL1(44));
      st.push(SymbolLL1(core::NonTerminal::Read_Statement_NT));
      break;
    case 45:
      st.push(SymbolLL1(45));
      st.push(SymbolLL1(core::NonTerminal::Write_Statement_NT));
      break;
    case 46:
      st.push(SymbolLL1(core::NonTerminal::Loops_NT));
      break;
    case 47:
      st.push(SymbolLL1(47));
      st.push(SymbolLL1(core::NonTerminal::IF_Statement_NT));
      break;
    case 48:
      st.push(SymbolLL1(48));
      st.push(SymbolLL1(core::NonTerminal::Return_NT));
      break;
    case 49:
      st.push(SymbolLL1(core::NonTerminal::Expr_Statement_NT));
      break;
    case 50:
      st.push(SymbolLL1(core::NonTerminal::Declaration_NT));
      break;
    case 51:
      st.push(SymbolLL1(core::TokenType::SEMI_COLON_SY));
      st.push(SymbolLL1(core::NonTerminal::Variable_List_read_NT));
      nodes.push_back(END_OF_LIST_MARKER);
      st.push(SymbolLL1(core::TokenType::READ_KW));
      break;
    case 52:
      st.push(SymbolLL1(core::TokenType::SEMI_COLON_SY));
      st.push(SymbolLL1(core::NonTerminal::Expr_List_NT));
      nodes.push_back(END_OF_LIST_MARKER);
      st.push(SymbolLL1(core::TokenType::WRITE_KW));
      break;
    case 53:
      st.push(SymbolLL1(core::NonTerminal::More_Variables_read_NT));
      st.push(SymbolLL1(core::NonTerminal::Index_Chain_NT));
      st.push(SymbolLL1(core::TokenType::ID_SY));
      break;
    case 54:
      break;
    case 55:
      st.push(SymbolLL1(core::NonTerminal::Variable_List_read_NT));
      st.push(SymbolLL1(core::TokenType::COMMA_SY));
      break;
    case 56:
      st.push(SymbolLL1(core::NonTerminal::More_Exprs_NT));
      st.push(SymbolLL1(core::NonTerminal::Or_Expr_NT));
      break;
    case 57:
      break;
    case 58:
      st.push(SymbolLL1(core::NonTerminal::Expr_List_NT));
      st.push(SymbolLL1(core::TokenType::COMMA_SY));
      break;
    case 59:
      st.push(SymbolLL1(core::NonTerminal::For_Loop_NT));
      break;
    case 60:
      st.push(SymbolLL1(core::NonTerminal::While_Loop_NT));
      break;
    case 61:
      st.push(SymbolLL1(61));
      st.push(SymbolLL1(core::TokenType::FOR_KW));
      st.push(SymbolLL1(core::TokenType::END_KW));
      st.push(SymbolLL1(core::NonTerminal::Command_Seq_NT));
      st.push(SymbolLL1(core::TokenType::DO_KW));
      st.push(SymbolLL1(core::NonTerminal::Expr_NT));
      st.push(SymbolLL1(core::TokenType::SEMI_COLON_SY));
      st.push(SymbolLL1(core::NonTerminal::Or_Expr_NT));
      st.push(SymbolLL1(core::TokenType::SEMI_COLON_SY));
      st.push(SymbolLL1(core::NonTerminal::Integer_Assign_NT));
      st.push(SymbolLL1(core::TokenType::FOR_KW));
      break;
    case 62:
      st.push(SymbolLL1(62));
      st.push(SymbolLL1(core::NonTerminal::Expr_NT));
      st.push(SymbolLL1(core::TokenType::EQUAL_OP));
      st.push(SymbolLL1(core::TokenType::ID_SY));
      break;
    case 63:
      st.push(SymbolLL1(63));
      st.push(SymbolLL1(core::TokenType::WHILE_KW));
      st.push(SymbolLL1(core::TokenType::END_KW));
      st.push(SymbolLL1(core::NonTerminal::Command_Seq_NT));
      st.push(SymbolLL1(core::TokenType::DO_KW));
      st.push(SymbolLL1(core::NonTerminal::Or_Expr_NT));
      st.push(SymbolLL1(core::TokenType::WHILE_KW));
      break;
    case 64:
      st.push(SymbolLL1(core::TokenType::IF_KW));
      st.push(SymbolLL1(core::TokenType::END_KW));
      st.push(SymbolLL1(core::NonTerminal::Else_Part_NT));
      st.push(SymbolLL1(65));
      st.push(SymbolLL1(core::NonTerminal::Command_Seq_NT));
      st.push(SymbolLL1(core::TokenType::THEN_KW));
      st.push(SymbolLL1(core::NonTerminal::Or_Expr_NT));
      st.push(SymbolLL1(core::TokenType::IF_KW));
      st.push(SymbolLL1(64));
      break;
    case 65:
      break;
    case 66:
      st.push(SymbolLL1(47));
      st.push(SymbolLL1(core::NonTerminal::Else_Part_NT));
      st.push(SymbolLL1(65));
      st.push(SymbolLL1(core::NonTerminal::Command_Seq_NT));
      st.push(SymbolLL1(core::TokenType::THEN_KW));
      st.push(SymbolLL1(core::NonTerminal::Or_Expr_NT));
      st.push(SymbolLL1(core::TokenType::IF_KW));
      st.push(SymbolLL1(core::TokenType::ELSE_KW));
      st.push(SymbolLL1(66));
      break;
    case 67:
      st.push(SymbolLL1(core::TokenType::SEMI_COLON_SY));
      st.push(SymbolLL1(core::NonTerminal::Return_Value_NT));
      st.push(SymbolLL1(core::TokenType::RETURN_KW));
      break;
    case 68:
      nodes.push_back(nullptr);
      break;
    case 69:
      st.push(SymbolLL1(core::NonTerminal::Or_Expr_NT));
      break;
    case 70:
      st.push(SymbolLL1(core::TokenType::SEMI_COLON_SY));
      st.push(SymbolLL1(core::NonTerminal::Expr_NT));
      break;
    case 71:
      st.push(SymbolLL1(core::NonTerminal::Assign_Expr_NT));
      break;
    case 72:
      st.push(SymbolLL1(72));
      st.push(SymbolLL1(core::NonTerminal::Assign_Expr_Value_NT));
      st.push(SymbolLL1(core::TokenType::EQUAL_OP));
      st.push(SymbolLL1(core::NonTerminal::Assignable_Expr_NT));
      break;
    case 73:
      st.push(SymbolLL1(core::NonTerminal::Or_Expr_NT));
      break;
    case 74:
      st.push(SymbolLL1(core::NonTerminal::Expr_NT));
      break;
    case 75:
      st.push(SymbolLL1(75));
      st.push(SymbolLL1(core::TokenType::RIGHT_CURLY_PR));
      st.push(SymbolLL1(core::NonTerminal::Array_Value_NT));
      nodes.push_back(END_OF_LIST_MARKER);
      st.push(SymbolLL1(core::TokenType::LEFT_CURLY_PR));
      break;
    case 76:
      st.push(SymbolLL1(core::NonTerminal::Index_Chain_NT));
      st.push(SymbolLL1(core::TokenType::ID_SY));
      break;
    case 77:
      st.push(SymbolLL1(core::NonTerminal::Or_Tail_NT));
      st.push(SymbolLL1(core::NonTerminal::And_Expr_NT));
      break;
    case 78:
      break;
    case 79:
      st.push(SymbolLL1(79));
      st.push(SymbolLL1(core::NonTerminal::Or_Tail_NT));
      st.push(SymbolLL1(core::NonTerminal::And_Expr_NT));
      st.push(SymbolLL1(core::TokenType::OR_KW));
      break;
    case 80:
      st.push(SymbolLL1(core::NonTerminal::And_Tail_NT));
      st.push(SymbolLL1(core::NonTerminal::Equality_Expr_NT));
      break;
    case 81:
      break;
    case 82:
      st.push(SymbolLL1(82));
      st.push(SymbolLL1(core::NonTerminal::And_Tail_NT));
      st.push(SymbolLL1(core::NonTerminal::Equality_Expr_NT));
      st.push(SymbolLL1(core::TokenType::AND_KW));
      break;
    case 83:
      st.push(SymbolLL1(core::NonTerminal::Equality_Tail_NT));
      st.push(SymbolLL1(core::NonTerminal::Relational_Expr_NT));
      break;
    case 84:
      break;
    case 85:
      st.push(SymbolLL1(85));
      st.push(SymbolLL1(core::NonTerminal::Equality_Tail_NT));
      st.push(SymbolLL1(core::NonTerminal::Relational_Expr_NT));
      st.push(SymbolLL1(core::NonTerminal::OP_Equal_NT));
      break;
    case 86:
      st.push(SymbolLL1(core::TokenType::IS_EQUAL_OP));
      break;
    case 87:
      st.push(SymbolLL1(core::TokenType::NOT_EQUAL_OP));
      break;
    case 88:
      st.push(SymbolLL1(core::NonTerminal::Relational_Tail_NT));
      st.push(SymbolLL1(core::NonTerminal::Additive_Expr_NT));
      break;
    case 89:
      break;
    case 90:
      st.push(SymbolLL1(90));
      st.push(SymbolLL1(core::NonTerminal::Relational_Tail_NT));
      st.push(SymbolLL1(core::NonTerminal::Additive_Expr_NT));
      st.push(SymbolLL1(core::NonTerminal::Relation_NT));
      break;
    case 91:
      st.push(SymbolLL1(core::TokenType::LESS_EQUAL_OP));
      break;
    case 92:
      st.push(SymbolLL1(core::TokenType::LESS_THAN_OP));
      break;
    case 93:
      st.push(SymbolLL1(core::TokenType::GREATER_THAN_OP));
      break;
    case 94:
      st.push(SymbolLL1(core::TokenType::GREATER_EQUAL_OP));
      break;
    case 95:
      st.push(SymbolLL1(core::NonTerminal::Additive_Tail_NT));
      st.push(SymbolLL1(core::NonTerminal::Multiplicative_Expr_NT));
      break;
    case 96:
      break;
    case 97:
      st.push(SymbolLL1(97));
      st.push(SymbolLL1(core::NonTerminal::Additive_Tail_NT));
      st.push(SymbolLL1(core::NonTerminal::Multiplicative_Expr_NT));
      st.push(SymbolLL1(core::NonTerminal::Weak_OP_NT));
      break;
    case 98:
      st.push(SymbolLL1(core::TokenType::PLUS_OP));
      break;
    case 99:
      st.push(SymbolLL1(core::TokenType::MINUS_OP));
      break;
    case 100:
      st.push(SymbolLL1(core::NonTerminal::Multiplicative_Tail_NT));
      st.push(SymbolLL1(core::NonTerminal::Unary_Expr_NT));
      break;
    case 101:
      break;
    case 102:
      st.push(SymbolLL1(102));
      st.push(SymbolLL1(core::NonTerminal::Multiplicative_Tail_NT));
      st.push(SymbolLL1(core::NonTerminal::Unary_Expr_NT));
      st.push(SymbolLL1(core::NonTerminal::Strong_OP_NT));
      break;
    case 103:
      st.push(SymbolLL1(core::TokenType::MULT_OP));
      break;
    case 104:
      st.push(SymbolLL1(core::TokenType::DIVIDE_OP));
      break;
    case 105:
      st.push(SymbolLL1(core::TokenType::MOD_OP));
      break;
    case 106:
      st.push(SymbolLL1(106));
      st.push(SymbolLL1(core::NonTerminal::Index_Expr_NT));
      st.push(SymbolLL1(core::TokenType::MINUS_OP));
      break;
    case 107:
      st.push(SymbolLL1(107));
      st.push(SymbolLL1(core::NonTerminal::Index_Expr_NT));
      st.push(SymbolLL1(core::TokenType::STRINGIFY_OP));
      break;
    case 108:
      st.push(SymbolLL1(108));
      st.push(SymbolLL1(core::NonTerminal::Index_Expr_NT));
      st.push(SymbolLL1(core::TokenType::BOOLEAN_OP));
      break;
    case 109:
      st.push(SymbolLL1(109));
      st.push(SymbolLL1(core::NonTerminal::Index_Expr_NT));
      st.push(SymbolLL1(core::TokenType::ROUND_OP));
      break;
    case 110:
      st.push(SymbolLL1(110));
      st.push(SymbolLL1(core::NonTerminal::Index_Expr_NT));
      st.push(SymbolLL1(core::TokenType::LENGTH_OP));
      break;
    case 111:
      st.push(SymbolLL1(111));
      st.push(SymbolLL1(core::NonTerminal::Assignable_Expr_NT));
      st.push(SymbolLL1(core::NonTerminal::Prefix_NT));
      break;
    case 112:
      st.push(SymbolLL1(112));
      st.push(SymbolLL1(core::NonTerminal::Index_Expr_NT));
      st.push(SymbolLL1(core::TokenType::NOT_KW));
      break;
    case 113:
      st.push(SymbolLL1(core::NonTerminal::maybe_Postfix_NT));
      st.push(SymbolLL1(core::NonTerminal::Index_Expr_NT));
      break;
    case 114:
      st.push(SymbolLL1(core::TokenType::INCREMENT_OP));
      break;
    case 115:
      st.push(SymbolLL1(core::TokenType::DECREMENT_OP));
      break;
    case 116:
      break;
    case 117:
      st.push(SymbolLL1(117));
      st.push(SymbolLL1(core::TokenType::INCREMENT_OP));
      break;
    case 118:
      st.push(SymbolLL1(118));
      st.push(SymbolLL1(core::TokenType::DECREMENT_OP));
      break;
    case 119:
      st.push(SymbolLL1(core::NonTerminal::Index_Chain_NT));
      st.push(SymbolLL1(core::NonTerminal::CallOrVariable_NT));
      break;
    case 120:
      st.push(SymbolLL1(core::NonTerminal::Primary_Expr_NT));
      break;
    case 121:
      break;
    case 122:
      st.push(SymbolLL1(core::NonTerminal::Index_Chain_NT));
      st.push(SymbolLL1(122));
      st.push(SymbolLL1(core::TokenType::RIGHT_SQUARE_PR));
      st.push(SymbolLL1(core::NonTerminal::Or_Expr_NT));
      st.push(SymbolLL1(core::TokenType::LEFT_SQUARE_PR));
      break;
    case 123:
      st.push(SymbolLL1(core::TokenType::RIGHT_PR));
      st.push(SymbolLL1(core::NonTerminal::Expr_NT));
      st.push(SymbolLL1(core::TokenType::LEFT_PR));
      break;
    case 124:
      st.push(SymbolLL1(core::NonTerminal::Literal_NT));
      break;
    case 125:
      st.push(SymbolLL1(125));
      st.push(SymbolLL1(core::TokenType::INTEGER_NUM));
      break;
    case 126:
      st.push(SymbolLL1(126));
      st.push(SymbolLL1(core::TokenType::FLOAT_NUM));
      break;
    case 127:
      st.push(SymbolLL1(127));
      st.push(SymbolLL1(core::TokenType::STRING_SY));
      break;
    case 128:
      st.push(SymbolLL1(128));
      st.push(SymbolLL1(core::TokenType::TRUE_KW));
      break;
    case 129:
      st.push(SymbolLL1(129));
      st.push(SymbolLL1(core::TokenType::FALSE_KW));
      break;
    case 130:
      st.push(SymbolLL1(core::NonTerminal::MaybeCall_NT));
      st.push(SymbolLL1(core::TokenType::ID_SY));
      break;
    case 131:
      break;
    case 132:
      st.push(SymbolLL1(132));
      st.push(SymbolLL1(core::NonTerminal::Call_Expr_NT));
      break;
    case 133:
      st.push(SymbolLL1(core::TokenType::RIGHT_PR));
      st.push(SymbolLL1(core::NonTerminal::May_be_Arg_NT));
      st.push(SymbolLL1(core::TokenType::LEFT_PR));
      break;
    case 134:
      break;
    case 135:
      st.push(SymbolLL1(core::NonTerminal::Call_Expr_Tail_NT));
      st.push(SymbolLL1(core::NonTerminal::Or_Expr_NT));
      st.push(SymbolLL1(core::TokenType::COMMA_SY));
      break;
    case 136:
      st.push(SymbolLL1(core::NonTerminal::Call_Expr_Tail_NT));
      st.push(SymbolLL1(core::NonTerminal::Or_Expr_NT));
      break;
    case 137:
      break;
    case 138:
      st.push(SymbolLL1(core::NonTerminal::Command_Seq_NT));
      st.push(SymbolLL1(65));
      st.push(SymbolLL1(core::TokenType::ELSE_KW));
      break;
    }
  }

  void LL1Parser::builder(int rule)
  {
    switch (rule)
    {
    case 1:
      build_source();
      break;
    case 2:
      currentFunctionList.clear();
      break;
    case 3:
      build_function_list();
      break;
    case 4:
      build_function();
      break;
    case 5:
    case 6:
      build_return_type();
      break;
    case 7:
      build_array_type();
      break;
    case 12:
    case 13:
    case 14:
    case 15:
      build_primitive_type();
      break;
    case 11:
      build_array_type();
      break;
    case 16:
      build_program();
      break;
    case 19:
      build_declaration_seq();
      break;
    case 20:
      build_variable_definition();
      break;
    case 21:
      build_variable_declaration();
      break;
    case 23:
      build_variable_initialization();
      break;
    case 25:
      build_array_literal();
      break;
    case 38:
    case 40:
      build_command_seq();
      break;
    case 42:
      build_skip_statement();
      break;
    case 43:
      build_stop_statement();
      break;
    case 44:
      build_read_statement();
      break;
    case 45:
      build_write_statement();
      break;
    case 47:
      build_if_statement();
      break;
    case 48:
      build_return_statement();
      break;
    case 61:
      build_for_loop();
      break;
    case 62:
      build_int_assign();
      break;
    case 63:
      build_while_loop();
      break;
    case 64:
      currentCommandSeq.insert(currentCommandSeq.begin(), START_OF_IF);
      break;
    case 65:
      currentCommandSeq.insert(currentCommandSeq.begin(), nullptr);
      break;
    case 66:
      currentCommandSeq.insert(currentCommandSeq.begin(), END_OF_LIST_ELSE);
      break;
    case 72:
      build_assignment_expression();
      break;
    case 75:
      build_array_literal();
      break;
    case 79:
      build_or_expression();
      break;
    case 82:
      build_and_expression();
      break;
    case 85:
      build_equality_expression();
      break;
    case 90:
      build_relational_expression();
      break;
    case 97:
      build_additive_expression();
      break;
    case 102:
      build_multiplicative_expression();
      break;
    case 106:
    case 107:
    case 108:
    case 109:
    case 110:
    case 111:
    case 112:
      build_unary_expression();
      break;
    case 117:
    case 118:
      build_unary_expression();
      break;
    case 122:
      build_index_expression();
      break;
    case 125:
      build_integer_literal();
      break;
    case 126:
      build_float_literal();
      break;
    case 127:
      build_string_literal();
      break;
    case 128:
    case 129:
      build_boolean_literal();
      break;
    case 132:
      build_call_function_expression();
      break;
    }
  }

  void LL1Parser::build_source()
  {
    ast::AstNode *programNode = nodes.back();
    nodes.pop_back();

    ast::ProgramDefinition *program = dynamic_cast<ast::ProgramDefinition *>(programNode);
    if (!program)
    {
      nodes.push_back(syntax_error("Error building Source node: invalid program node"));
      return;
    }

    int start_line = tracking.program_start_token.line;
    int node_start = tracking.program_start_token.start;
    int end_line = tracking.program_end_token.line;
    int node_end = tracking.program_end_token.end;

    ast::Source *sourceNode = new ast::Source(program, currentFunctionList, start_line, end_line, node_start, node_end);

    currentFunctionList.clear();

    nodes.push_back(sourceNode);
  }

  void LL1Parser::build_program()
  {
    ast::AstNode *idNode = nodes.back();
    nodes.pop_back();

    ast::Identifier *id = dynamic_cast<ast::Identifier *>(idNode);
    if (!id)
    {
      nodes.push_back(syntax_error("Invalid identifier node"));
      return;
    }

    int start_line = tracking.program_kw_token.line;
    int node_start = tracking.program_kw_token.start;
    int end_line = currentCommandSeq.empty() ? id->end_line : currentCommandSeq.back()->end_line;
    int node_end = currentCommandSeq.empty() ? id->node_end : currentCommandSeq.back()->node_end;

    if (previous_token.type == core::TokenType::END_KW)
    {
      end_line = previous_token.line;
      node_end = previous_token.end;
    }

    ast::ProgramDefinition *programNode = new ast::ProgramDefinition(id, currentDeclarationSeq, currentCommandSeq, start_line, end_line, node_start, node_end);

    currentDeclarationSeq.clear();
    currentCommandSeq.clear();

    nodes.push_back(programNode);
  }

  void LL1Parser::build_function()
  {
    ast::AstNode *idNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *returnNode = nodes.back();
    nodes.pop_back();

    ast::Identifier *id = dynamic_cast<ast::Identifier *>(idNode);
    if (!id)
    {
      nodes.push_back(syntax_error("Error building Function node: invalid Identifier"));
      return;
    }

    ast::ReturnType *rtn = dynamic_cast<ast::ReturnType *>(returnNode);
    if (!rtn)
    {
      nodes.push_back(syntax_error("Error building Function node: invalid Return type"));
      return;
    }

    auto &declSeq = currentDeclarationSeq;
    auto &commandSeq = currentCommandSeq;

    int start_line = tracking.function_start_token.line;
    int node_start = tracking.function_start_token.start;
    int end_line = tracking.function_end_token.line;
    int node_end = tracking.function_end_token.end;

    ast::FunctionDefinition *funcNode = new ast::FunctionDefinition(id, rtn, declSeq, commandSeq, start_line, end_line, node_start, node_end);

    currentDeclarationSeq.clear();
    currentCommandSeq.clear();

    nodes.push_back(funcNode);
  }

  void LL1Parser::build_variable_definition()
  {
    ast::AstNode *defNode = nodes.back();
    nodes.pop_back();

    ast::Statement *defStmt = dynamic_cast<ast::Statement *>(defNode);
    if (!defStmt)
    {
      nodes.push_back(syntax_error("Error building VariableDefinition node: invalid child node"));
      return;
    }

    ast::VariableDefinition *varDef = new ast::VariableDefinition(defStmt, defStmt->start_line, defStmt->end_line, defStmt->node_start, defStmt->node_end);
    nodes.push_back(varDef);
  }

  void LL1Parser::build_variable_declaration()
  {
    ast::AstNode *typeNode = nodes.back();
    if (dynamic_cast<ast::VariableInitialization *>(typeNode))
    {
      return;
    }
    nodes.pop_back();
    ast::DataType *dt = dynamic_cast<ast::DataType *>(typeNode);

    vector<ast::Identifier *> varList;

    while (!nodes.empty())
    {
      ast::AstNode *node = nodes.back();
      if (node == END_OF_LIST_MARKER)
      {
        nodes.pop_back();
        break;
      }

      ast::Identifier *id = dynamic_cast<ast::Identifier *>(node);
      if (!id)
      {
        nodes.push_back(syntax_error("Error building VariableDeclaration node: invalid child nodes"));
        return;
      }

      varList.push_back(id);
      nodes.pop_back();
    }

    reverse(varList.begin(), varList.end());

    if (varList.empty() || !dt)
    {
      nodes.push_back(syntax_error("Error building VariableDeclaration node: invalid child nodes"));
      return;
    }

    int start_line = tracking.current_var_token.line;
    int node_start = tracking.current_var_token.start;
    int end_line = previous_token.line;
    int node_end = previous_token.end;

    ast::VariableDeclaration *varDecl = new ast::VariableDeclaration(varList, dt, start_line, end_line, node_start, node_end);

    nodes.push_back(varDecl);
  }

  void LL1Parser::build_variable_initialization()
  {
    ast::AstNode *initializerNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *datatypeNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *idNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *check = nodes.back();
    nodes.pop_back();

    if (check != END_OF_LIST_MARKER)
    {
      nodes.push_back(syntax_error("Error building VariableInitialization node: The Variable must be one"));
      return;
    }

    ast::Identifier *id = dynamic_cast<ast::Identifier *>(idNode);
    ast::DataType *dt = dynamic_cast<ast::DataType *>(datatypeNode);
    ast::Expression *initializer = dynamic_cast<ast::Expression *>(initializerNode);

    if (!id || !dt || !initializer)
    {
      nodes.push_back(syntax_error("Error building VariableInitialization node: invalid child nodes"));
      return;
    }

    int start_line = tracking.current_var_token.line;
    int node_start = tracking.current_var_token.start;
    int end_line = previous_token.line;
    int node_end = previous_token.end;

    ast::VariableInitialization *varInit = new ast::VariableInitialization(id, dt, initializer, start_line, end_line, node_start, node_end);

    nodes.push_back(varInit);
  }

  void LL1Parser::build_return_type()
  {
    ast::AstNode *typeNode = nodes.back();
    nodes.pop_back();

    if (auto prim = dynamic_cast<ast::PrimitiveDataType *>(typeNode))
    {
      ast::ReturnType *retType = new ast::ReturnType(prim, prim->start_line, prim->end_line, prim->node_start, prim->node_end);
      nodes.push_back(retType);
    }
    else if (auto arr = dynamic_cast<ast::ArrayDataType *>(typeNode))
    {
      ast::ReturnType *retType = new ast::ReturnType(arr, arr->start_line, arr->end_line, arr->node_start, arr->node_end);
      nodes.push_back(retType);
    }
    else
    {
      nodes.push_back(syntax_error("Error building ReturnType node: invalid child node"));
    }
  }

  void LL1Parser::build_primitive_type()
  {
    ast::AstNode *tokenNode = nodes.back();
    nodes.pop_back();

    ast::PrimitiveDataType *primType = dynamic_cast<ast::PrimitiveDataType *>(tokenNode);
    if (!primType)
    {
      nodes.push_back(syntax_error("Error building PrimitiveType node"));
      return;
    }

    nodes.push_back(primType);
  }

  void LL1Parser::build_array_type()
  {
    int start_line = tracking.first_left_square_token.line;
    int node_start = tracking.first_left_square_token.start;
    int end_line;
    int node_end;
    string type;
    int dim;

    ast::AstNode *tailNode = nodes.back();
    nodes.pop_back();

    if (auto prim = dynamic_cast<ast::PrimitiveDataType *>(tailNode))
    {
      end_line = tracking.last_right_square_token.line;
      node_end = tracking.last_right_square_token.end;
      type = prim->datatype;
      dim = 1;

      delete prim;
    }
    else if (auto arr = dynamic_cast<ast::ArrayDataType *>(tailNode))
    {
      end_line = tracking.last_right_square_token.line;
      node_end = tracking.last_right_square_token.end;
      type = arr->datatype;
      dim = arr->dimension + 1;

      delete arr;
    }
    else
    {
      nodes.push_back(syntax_error("Error building ArrayType node: invalid child node"));
      return;
    }

    ast::ArrayDataType *arrType = new ast::ArrayDataType(type, dim, start_line, end_line, node_start, node_end);
    nodes.push_back(arrType);
    tracking.arr_type_tracking = false;
  }

  void LL1Parser::build_if_statement()
  {
    ast::AstNode *conditionNode = nodes.back();
    nodes.pop_back();

    vector<ast::Statement *> *consequent = new vector<ast::Statement *>();
    vector<ast::Statement *> *alternate = new vector<ast::Statement *>();

    bool in_alternate = true;

    while (!currentCommandSeq.empty())
    {
      ast::Statement *stmt = currentCommandSeq.front();
      currentCommandSeq.erase(currentCommandSeq.begin());

      if (stmt == nullptr)
      {
        in_alternate = false;
        continue;
      }

      if (stmt == START_OF_IF)
      {
        ast::Statement *stmt = currentCommandSeq.front();

        ast::Expression *condition = dynamic_cast<ast::Expression *>(conditionNode);

        int start_line, node_start;
        if (!tracking.if_open_stack.empty())
        {
          core::Token t = tracking.if_open_stack.back();
          tracking.if_open_stack.pop_back();
          start_line = t.line;
          node_start = t.start;
        }
        int end_line, node_end;
        if (!tracking.if_close_stack.empty())
        {
          core::Token t = tracking.if_close_stack.back();
          tracking.if_close_stack.pop_back();
          end_line = t.line;
          node_end = t.end;
        }
        else
        {
          core::Token next_end = peek_token();
          core::Token next_if = peek_token();

          end_line = next_if.line;
          node_end = next_if.end;
        }

        reverse(consequent->begin(), consequent->end());
        reverse(alternate->begin(), alternate->end());

        ast::IfStatement *ifStmt = new ast::IfStatement(condition, *consequent, *alternate, start_line, end_line, node_start, node_end);

        nodes.push_back(ifStmt);

        delete consequent;
        delete alternate;

        return;
      }

      if (stmt == END_OF_LIST_ELSE)
      {
        ast::Expression *condition = dynamic_cast<ast::Expression *>(conditionNode);

        int start_line, node_start;
        if (!tracking.if_open_stack.empty())
        {
          core::Token t = tracking.if_open_stack.back();
          tracking.if_open_stack.pop_back();
          start_line = t.line;
          node_start = t.start;
        }
        int end_line, node_end;
        if (!tracking.if_close_stack.empty())
        {
          core::Token t = tracking.if_close_stack.back();
          tracking.if_close_stack.pop_back();
          end_line = t.line;
          node_end = t.end;
        }
        else
        {
          core::Token next_end = peek_token();
          core::Token next_if = peek_token();

          end_line = next_if.line;
          node_end = next_if.end;
        }

        reverse(consequent->begin(), consequent->end());
        reverse(alternate->begin(), alternate->end());

        ast::IfStatement *ifStmt = new ast::IfStatement(condition, *consequent, *alternate, start_line, end_line, node_start, node_end);

        delete consequent;
        delete alternate;

        currentCommandSeq.insert(currentCommandSeq.begin(), ifStmt);

        return;
      }

      if (!stmt)
      {
        nodes.push_back(syntax_error("Expected statement inside if/else block"));
        delete consequent;
        delete alternate;
        return;
      }

      if (in_alternate)
      {
        alternate->insert(alternate->begin(), stmt);
      }
      else
      {
        consequent->insert(consequent->begin(), stmt);
      }
    }

    ast::Expression *condition = dynamic_cast<ast::Expression *>(conditionNode);

    if (!condition || !consequent)
    {
      nodes.push_back(syntax_error("Error building IfStatement node: invalid child nodes"));
      return;
    }

    int start_line, node_start;
    if (!tracking.if_open_stack.empty())
    {
      core::Token t = tracking.if_open_stack.back();
      tracking.if_open_stack.pop_back();
      start_line = t.line;
      node_start = t.start;
    }
    int end_line, node_end;
    if (!tracking.if_close_stack.empty())
    {
      core::Token t = tracking.if_close_stack.back();
      tracking.if_close_stack.pop_back();
      end_line = t.line;
      node_end = t.end;
    }
    else
    {
      core::Token next_end = peek_token();
      core::Token next_if = peek_token();

      end_line = next_if.line;
      node_end = next_if.end;
    }

    reverse(consequent->begin(), consequent->end());
    reverse(alternate->begin(), alternate->end());

    ast::IfStatement *ifStmt = new ast::IfStatement(condition, *consequent, *alternate, start_line, end_line, node_start, node_end);

    delete consequent;
    delete alternate;

    if (currentCommandSeq.front() != START_OF_IF)
    {
      currentCommandSeq.insert(currentCommandSeq.begin(), ifStmt);
    }
    else
    {
      currentCommandSeq.erase(currentCommandSeq.begin());
      nodes.push_back(ifStmt);
    }
  }

  void LL1Parser::build_return_statement()
  {
    ast::AstNode *returnValueNode = nodes.back();
    nodes.pop_back();

    ast::Expression *returnValue = dynamic_cast<ast::Expression *>(returnValueNode);

    int start_line = tracking.current_return_token.line;
    int node_start = tracking.current_return_token.start;
    int end_line = previous_token.line;
    int node_end = previous_token.end;

    ast::ReturnStatement *retStmt = new ast::ReturnStatement(returnValue, start_line, end_line, node_start, node_end);

    nodes.push_back(retStmt);
  }

  void LL1Parser::build_skip_statement()
  {
    int start_line = tracking.current_skip_token.line;
    int node_start = tracking.current_skip_token.start;
    int end_line = previous_token.line;
    int node_end = previous_token.end;

    ast::SkipStatement *skipStmt = new ast::SkipStatement(start_line, end_line, node_start, node_end);

    nodes.push_back(skipStmt);
  }

  void LL1Parser::build_stop_statement()
  {
    int start_line = tracking.current_stop_token.line;
    int node_start = tracking.current_stop_token.start;
    int end_line = previous_token.line;
    int node_end = previous_token.end;

    ast::StopStatement *stopStmt = new ast::StopStatement(start_line, end_line, node_start, node_end);

    nodes.push_back(stopStmt);
  }

  void LL1Parser::build_read_statement()
  {
    vector<ast::AssignableExpression *> *variables = new vector<ast::AssignableExpression *>();

    while (!nodes.empty())
    {
      ast::AstNode *node = nodes.back();
      nodes.pop_back();

      if (node == END_OF_LIST_MARKER)
        break;

      ast::AssignableExpression *var = dynamic_cast<ast::AssignableExpression *>(node);
      if (!var)
      {
        nodes.push_back(syntax_error("Expected Assignable in Read statement"));
        delete variables;
        return;
      }

      variables->insert(variables->begin(), var);
    }

    int start_line = tracking.current_read_token.line;
    int node_start = tracking.current_read_token.start;
    int end_line = previous_token.line;
    int node_end = previous_token.end;

    ast::ReadStatement *readStmt = new ast::ReadStatement(*variables, start_line, end_line, node_start, node_end);

    delete variables;

    nodes.push_back(readStmt);
  }

  void LL1Parser::build_write_statement()
  {
    vector<ast::Expression *> *exprList = new vector<ast::Expression *>();
    while (!nodes.empty())
    {
      ast::AstNode *node = nodes.back();
      nodes.pop_back();

      if (node == END_OF_LIST_MARKER)
        break;

      ast::Expression *expr = dynamic_cast<ast::Expression *>(node);
      if (!expr)
      {
        nodes.push_back(syntax_error("Expected expression in write statement"));
        delete exprList;
        return;
      }

      exprList->insert(exprList->begin(), expr);
    }

    int start_line = tracking.current_write_token.line;
    int node_start = tracking.current_write_token.start;
    int end_line = previous_token.line;
    int node_end = previous_token.end;

    if (exprList->empty())
    {
      nodes.push_back(syntax_error("Expected expression in write statement"));
      delete exprList;
      return;
    }

    ast::WriteStatement *writeStmt = new ast::WriteStatement(*exprList, start_line, end_line, node_start, node_end);

    delete exprList;

    nodes.push_back(writeStmt);
  }

  void LL1Parser::build_for_loop()
  {
    vector<ast::Statement *> *body = new vector<ast::Statement *>();
    while (!currentCommandSeq.empty())
    {
      ast::Statement *stmt = currentCommandSeq.back();
      currentCommandSeq.pop_back();

      if (!stmt)
      {
        nodes.push_back(syntax_error("Expected statement inside while block"));
        delete body;
        return;
      }
      body->insert(body->begin(), stmt);
    }

    ast::AstNode *updateExprNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *conditionNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *initAssignNode = nodes.back();
    nodes.pop_back();

    ast::AssignmentExpression *init = dynamic_cast<ast::AssignmentExpression *>(initAssignNode);
    ast::Expression *condition = dynamic_cast<ast::Expression *>(conditionNode);
    ast::Expression *update = dynamic_cast<ast::Expression *>(updateExprNode);

    if (!init || !condition || !update)
    {
      nodes.push_back(syntax_error("Error building ForLoop node: invalid child nodes"));
      return;
    }

    int start_line = tracking.for_start_token.line;
    int node_start = tracking.for_start_token.start;
    int end_line = tracking.for_end_token.line;
    int node_end = tracking.for_end_token.end;

    ast::ForLoop *forLoop = new ast::ForLoop(init, condition, update, *body, start_line, end_line, node_start, node_end);

    delete body;

    nodes.push_back(forLoop);
  }

  void LL1Parser::build_int_assign()
  {
    ast::AstNode *valueNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *idNode = nodes.back();
    nodes.pop_back();

    ast::Identifier *id = dynamic_cast<ast::Identifier *>(idNode);
    ast::Expression *value = dynamic_cast<ast::Expression *>(valueNode);

    if (!id || !value)
    {
      nodes.push_back(syntax_error("Error building int Assignment Expression node: invalid child nodes"));
      return;
    }

    int start_line = id->start_line;
    int end_line = value->end_line;
    int node_start = id->node_start;
    int node_end = value->node_end;

    ast::AssignmentExpression *assignExpr = new ast::AssignmentExpression(id, value, start_line, end_line, node_start, node_end);

    nodes.push_back(assignExpr);
  }

  void LL1Parser::build_while_loop()
  {
    vector<ast::Statement *> *body = new vector<ast::Statement *>();
    while (!currentCommandSeq.empty())
    {
      ast::Statement *stmt = currentCommandSeq.back();
      currentCommandSeq.pop_back();

      if (!stmt)
      {
        nodes.push_back(syntax_error("Expected statement inside while block"));
        delete body;
        return;
      }
      body->insert(body->begin(), stmt);
    }

    ast::AstNode *conditionNode = nodes.back();
    nodes.pop_back();

    ast::Expression *condition = dynamic_cast<ast::Expression *>(conditionNode);

    if (!condition)
    {
      nodes.push_back(syntax_error("Error building WhileLoop node: invalid child nodes"));
      return;
    }

    int start_line = tracking.while_start_token.line;
    int node_start = tracking.while_start_token.start;
    int end_line = tracking.while_end_token.line;
    int node_end = tracking.while_end_token.end;

    ast::WhileLoop *whileLoop = new ast::WhileLoop(condition, *body, start_line, end_line, node_start, node_end);

    delete body;

    nodes.push_back(whileLoop);
  }

  void LL1Parser::build_assignment_expression()
  {
    ast::AstNode *valueNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *assigneeNode = nodes.back();
    nodes.pop_back();

    ast::AssignableExpression *assignee = dynamic_cast<ast::AssignableExpression *>(assigneeNode);
    ast::Expression *value = dynamic_cast<ast::Expression *>(valueNode);

    if (!assignee || !value)
    {
      nodes.push_back(syntax_error("Error building AssignmentExpression node: invalid child nodes"));
      return;
    }

    int start_line = assignee->start_line;
    int node_start = assignee->node_start;
    int end_line = value->end_line;
    int node_end = value->node_end;

    ast::AssignmentExpression *assignExpr = new ast::AssignmentExpression(assignee, value, start_line, end_line, node_start, node_end);

    nodes.push_back(assignExpr);
  }

  void LL1Parser::build_or_expression()
  {
    ast::AstNode *orTailNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *andExprNode = nodes.back();
    nodes.pop_back();

    ast::Expression *left = dynamic_cast<ast::Expression *>(andExprNode);
    ast::Expression *right = dynamic_cast<ast::Expression *>(orTailNode);

    if (!left || !right)
    {
      nodes.push_back(syntax_error("Error building OrExpression node: invalid left or Right child"));
      return;
    }

    int start_line = left->start_line;
    int end_line = right->end_line;
    int node_start = left->node_start;
    int node_end = right->node_end;

    ast::OrExpression *orExpr = new ast::OrExpression(left, right, start_line, end_line, node_start, node_end);

    nodes.push_back(orExpr);
  }

  void LL1Parser::build_and_expression()
  {
    ast::AstNode *andTailNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *equalityExprNode = nodes.back();
    nodes.pop_back();

    ast::Expression *left = dynamic_cast<ast::Expression *>(equalityExprNode);
    ast::Expression *right = dynamic_cast<ast::Expression *>(andTailNode);

    if (!left || !right)
    {
      nodes.push_back(syntax_error("Error building AndExpression node: invalid left or right child"));
      return;
    }

    int start_line = left->start_line;
    int end_line = right->end_line;
    int node_start = left->node_start;
    int node_end = right->node_end;

    ast::AndExpression *andExpr = new ast::AndExpression(left, right, start_line, end_line, node_start, node_end);

    nodes.push_back(andExpr);
  }

  void LL1Parser::build_equality_expression()
  {
    ast::AstNode *equalityTailNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *optNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *relationalExprNode = nodes.back();
    nodes.pop_back();

    ast::Expression *left = dynamic_cast<ast::Expression *>(relationalExprNode);
    ast::Expression *right = dynamic_cast<ast::Expression *>(equalityTailNode);
    ast::Identifier *opt = dynamic_cast<ast::Identifier *>(optNode);
    string op = opt->name;

    if (!left || !right || !opt)
    {
      nodes.push_back(syntax_error("Error building EqualityExpression node: invalid left or right or opreator child"));
      return;
    }

    int start_line = left->start_line;
    int end_line = right->end_line;
    int node_start = left->node_start;
    int node_end = right->node_end;

    ast::EqualityExpression *eqExpr = new ast::EqualityExpression(left, right, op, start_line, end_line, node_start, node_end);

    nodes.push_back(eqExpr);
  }

  void LL1Parser::build_relational_expression()
  {
    ast::AstNode *relationalTailNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *optNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *additiveExprNode = nodes.back();
    nodes.pop_back();

    ast::Expression *left = dynamic_cast<ast::Expression *>(additiveExprNode);
    ast::Expression *right = dynamic_cast<ast::Expression *>(relationalTailNode);
    ast::Identifier *opt = dynamic_cast<ast::Identifier *>(optNode);
    string op = opt->name;

    if (!left || !right || !opt)
    {
      nodes.push_back(syntax_error("Error building RelationalExpression node: invalid left or right or opreator child"));
      return;
    }

    int start_line = left->start_line;
    int end_line = right->end_line;
    int node_start = left->node_start;
    int node_end = right->node_end;

    ast::RelationalExpression *relExpr = new ast::RelationalExpression(left, right, op, start_line, end_line, node_start, node_end);

    nodes.push_back(relExpr);
  }

  void LL1Parser::build_additive_expression()
  {
    ast::AstNode *additiveTailNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *optNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *multiplicativeExprNode = nodes.back();
    nodes.pop_back();

    ast::Expression *left = dynamic_cast<ast::Expression *>(multiplicativeExprNode);
    ast::Expression *right = dynamic_cast<ast::Expression *>(additiveTailNode);
    ast::Identifier *opt = dynamic_cast<ast::Identifier *>(optNode);
    string op = opt->name;

    if (!left || !right || !opt)
    {
      nodes.push_back(syntax_error("Error building AdditiveExpression node: invalid left or right or opreator child"));
      return;
    }

    if (!right)
    {
      nodes.push_back(left);
      return;
    }

    int start_line = left->start_line;
    int end_line = right->end_line;
    int node_start = left->node_start;
    int node_end = right->node_end;

    ast::AdditiveExpression *addExpr = new ast::AdditiveExpression(left, right, op, start_line, end_line, node_start, node_end);

    nodes.push_back(addExpr);
  }

  void LL1Parser::build_multiplicative_expression()
  {
    ast::AstNode *multiplicativeTailNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *optNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *unaryExprNode = nodes.back();
    nodes.pop_back();

    ast::Expression *left = dynamic_cast<ast::Expression *>(unaryExprNode);
    ast::Expression *right = dynamic_cast<ast::Expression *>(multiplicativeTailNode);
    ast::Identifier *opt = dynamic_cast<ast::Identifier *>(optNode);
    string op = opt->name;

    if (!left || !right || !opt)
    {
      nodes.push_back(syntax_error("Error building MultiplicativeExpression node: invalid left or right or opreator child"));
      return;
    }

    int start_line = left->start_line;
    int end_line = right->end_line;
    int node_start = left->node_start;
    int node_end = right->node_end;

    ast::MultiplicativeExpression *multExpr = new ast::MultiplicativeExpression(left, right, op, start_line, end_line, node_start, node_end);

    nodes.push_back(multExpr);
  }

  void LL1Parser::build_unary_expression()
  {
    ast::Expression *operand;
    ast::Identifier *opt;
    string op;
    bool postfix = false;

    ast::AstNode *firstNode = nodes.back();
    nodes.pop_back();
    ast::AstNode *secondNode;

    if (dynamic_cast<ast::Identifier *>(firstNode))
    {
      opt = dynamic_cast<ast::Identifier *>(firstNode);
      op = opt->name;
      if (op == "++" || op == "--")
      {
        postfix = true;
        secondNode = nodes.back();
        nodes.pop_back();
        operand = dynamic_cast<ast::Expression *>(secondNode);
        if (!dynamic_cast<ast::AssignableExpression *>(secondNode))
        {
          nodes.push_back(syntax_error("Error building UnaryExpression: invalid operand"));
          return;
        }
        if (!operand)
        {
          nodes.push_back(syntax_error("Error building UnaryExpression: invalid operand"));
          return;
        }

        int start_line = operand->start_line;
        int end_line = opt->end_line;
        int node_start = operand->node_start;
        int node_end = opt->node_end;

        ast::UnaryExpression *unaryExpr = new ast::UnaryExpression(operand, op, postfix, start_line, end_line, node_start, node_end);

        nodes.push_back(unaryExpr);
        return;
      }
    }
    operand = dynamic_cast<ast::Expression *>(firstNode);
    secondNode = nodes.back();
    nodes.pop_back();
    opt = dynamic_cast<ast::Identifier *>(secondNode);
    op = opt->name;
    if (!dynamic_cast<ast::AssignableExpression *>(firstNode) && (op == "++" || op == "--"))
    {
      nodes.push_back(syntax_error("Error building UnaryExpression: invalid operand"));
      return;
    }
    if (!operand)
    {
      nodes.push_back(syntax_error("Error building UnaryExpression: invalid operand"));
      return;
    }

    int start_line = opt->start_line;
    int end_line = operand->end_line;
    int node_start = opt->node_start;
    int node_end = operand->node_end;

    ast::UnaryExpression *unaryExpr = new ast::UnaryExpression(operand, op, postfix, start_line, end_line, node_start, node_end);

    nodes.push_back(unaryExpr);
  }

  void LL1Parser::build_call_function_expression()
  {
    vector<ast::Expression *> *exprList = new vector<ast::Expression *>();
    while (!nodes.empty())
    {
      ast::AstNode *node = nodes.back();

      if (node == dynamic_cast<ast::Identifier *>(node))
        break;
      nodes.pop_back();

      ast::Expression *expr = dynamic_cast<ast::Expression *>(node);
      if (!expr)
      {
        nodes.push_back(syntax_error("Expected expression in write statement"));
        delete exprList;
        return;
      }

      exprList->insert(exprList->begin(), expr);
    }

    ast::AstNode *idNode = nodes.back();
    nodes.pop_back();

    ast::Identifier *id = dynamic_cast<ast::Identifier *>(idNode);

    if (!id)
    {
      nodes.push_back(syntax_error("Error building CallFunctionExpression: invalid Identifier"));
      return;
    }

    int start_line = tracking.first_left_paren_token.line;
    int node_start = tracking.first_left_paren_token.start;
    int end_line = tracking.last_right_paren_token.line;
    int node_end = tracking.last_right_paren_token.end;

    ast::CallFunctionExpression *callFuncExpr = new ast::CallFunctionExpression(id, *exprList, start_line, end_line, node_start, node_end);

    nodes.push_back(callFuncExpr);
  }

  void LL1Parser::build_index_expression()
  {
    ast::AstNode *indexChainNode = nodes.back();
    nodes.pop_back();

    ast::AstNode *baseNode = nodes.back();
    nodes.pop_back();

    ast::Expression *base = dynamic_cast<ast::Expression *>(baseNode);
    ast::Expression *indexChain = dynamic_cast<ast::Expression *>(indexChainNode);

    if (!base || !indexChain)
    {
      nodes.push_back(syntax_error("Error building IndexExpression: invalid child nodes"));
      return;
    }

    int start_line = base->start_line;
    int node_start = base->node_start;

    core::Token lastRsq = tracking.index_last_right_sq;
    int end_line = lastRsq.line;
    int node_end = lastRsq.end;

    ast::IndexExpression *current = new ast::IndexExpression(base, indexChain, start_line, end_line, node_start, node_end);

    nodes.push_back(current);
  }

  void LL1Parser::build_identifier()
  {
    ast::AstNode *idNode = nodes.back();
    nodes.pop_back();

    ast::Identifier *id = dynamic_cast<ast::Identifier *>(idNode);
    if (!id)
    {
      nodes.push_back(syntax_error("Error building Identifier node"));
      return;
    }

    nodes.push_back(id);
  }

  void LL1Parser::build_integer_literal()
  {
    ast::AstNode *intNode = nodes.back();
    nodes.pop_back();

    ast::IntegerLiteral *intLit = dynamic_cast<ast::IntegerLiteral *>(intNode);
    if (!intLit)
    {
      nodes.push_back(syntax_error("Error building IntegerLiteral node"));
      return;
    }

    nodes.push_back(intLit);
  }

  void LL1Parser::build_float_literal()
  {
    ast::AstNode *floatNode = nodes.back();
    nodes.pop_back();

    ast::FloatLiteral *floatLit = dynamic_cast<ast::FloatLiteral *>(floatNode);
    if (!floatLit)
    {
      nodes.push_back(syntax_error("Error building FloatLiteral node"));
      return;
    }

    nodes.push_back(floatLit);
  }

  void LL1Parser::build_string_literal()
  {
    ast::AstNode *strNode = nodes.back();
    nodes.pop_back();

    ast::StringLiteral *strLit = dynamic_cast<ast::StringLiteral *>(strNode);
    if (!strLit)
    {
      nodes.push_back(syntax_error("Error building StringLiteral node"));
      return;
    }

    nodes.push_back(strLit);
  }

  void LL1Parser::build_boolean_literal()
  {
    ast::AstNode *boolNode = nodes.back();
    nodes.pop_back();

    ast::BooleanLiteral *boolLit = dynamic_cast<ast::BooleanLiteral *>(boolNode);
    if (!boolLit)
    {
      nodes.push_back(syntax_error("Error building BooleanLiteral node"));
      return;
    }

    nodes.push_back(boolLit);
  }

  void LL1Parser::build_array_literal()
  {
    ast::AstNode *arrayNode = nodes.back();
    nodes.pop_back();

    ast::ArrayLiteral *arrLit = dynamic_cast<ast::ArrayLiteral *>(arrayNode);
    if (!arrLit)
    {
      nodes.push_back(syntax_error("Error building ArrayLiteral node"));
      return;
    }

    nodes.push_back(arrLit);
  }

  void LL1Parser::build_function_list()
  {
    ast::AstNode *funcNode = nodes.back();
    nodes.pop_back();

    ast::FunctionDefinition *func = dynamic_cast<ast::FunctionDefinition *>(funcNode);
    if (!func)
    {
      nodes.push_back(syntax_error("Invalid function node"));
      return;
    }

    currentFunctionList.insert(currentFunctionList.begin(), func);
  }

  void LL1Parser::build_declaration_seq()
  {
    ast::AstNode *declNode = nodes.back();
    nodes.pop_back();

    ast::VariableDefinition *decl = dynamic_cast<ast::VariableDefinition *>(declNode);
    if (!decl)
    {
      nodes.push_back(syntax_error("Invalid declaration node"));
      return;
    }

    currentDeclarationSeq.insert(currentDeclarationSeq.begin(), decl);
  }

  void LL1Parser::build_command_seq()
  {
    ast::AstNode *cmdNode = nodes.back();
    nodes.pop_back();

    ast::Statement *cmd = dynamic_cast<ast::Statement *>(cmdNode);
    if (!cmd)
    {
      nodes.push_back(syntax_error("Invalid command node"));
      return;
    }

    currentCommandSeq.insert(currentCommandSeq.begin(), cmd);
  }

  ast::ArrayLiteral *LL1Parser::build_array()
  {
    vector<ast::Expression *> *elements = new vector<ast::Expression *>();
    while (!nodes.empty())
    {
      ast::AstNode *node = nodes.back();
      nodes.pop_back();

      if (node == END_OF_LIST_MARKER)
        break;

      ast::Expression *expr = dynamic_cast<ast::Expression *>(node);
      if (!expr)
      {
        nodes.push_back(syntax_error("Expected expression in array literal"));
        delete elements;
        return nullptr;
      }
      elements->insert(elements->begin(), expr);
    }

    int start_line = tracking.last_left_curly_token.line;
    int node_start = tracking.last_left_curly_token.start;
    int end_line = tracking.last_right_curly_token.line;
    int node_end = tracking.last_right_curly_token.end;

    ast::ArrayLiteral *arrLit = new ast::ArrayLiteral(*elements, start_line, end_line, node_start, node_end);

    delete elements;

    return arrLit;
  }

  void LL1Parser::track_special_tokens(core::TokenType type, const core::Token &token)
  {
    if (!tracking.program_start_token_set)
    {
      tracking.program_start_token = token;
      tracking.program_start_token_set = true;
    }

    switch (type)
    {
    case core::TokenType::PROGRAM_KW:
      tracking.program_kw_token = token;
      break;
    case core::TokenType::FUNC_KW:
      if (!tracking.function_start_token_set)
      {
        tracking.function_start_token = token;
        tracking.function_start_token_set = true;
      }
      else
      {
        tracking.function_end_token = token;
        tracking.function_start_token_set = false;
      }
      break;
    case core::TokenType::WHILE_KW:
      if (!tracking.while_start_token_set)
      {
        tracking.while_start_token = token;
        tracking.while_start_token_set = true;
      }
      else
      {
        tracking.while_end_token = token;
        tracking.while_start_token_set = false;
      }
      break;
    case core::TokenType::FOR_KW:
      if (!tracking.for_start_token_set)
      {
        tracking.for_start_token = token;
        tracking.for_start_token_set = true;
      }
      else
      {
        tracking.for_end_token = token;
        tracking.for_start_token_set = false;
      }
      break;
    case core::TokenType::IF_KW:
      if (previous_token.type == core::TokenType::END_KW)
      {
        tracking.if_close_stack.push_back(token);
      }
      else
      {
        tracking.if_open_stack.push_back(token);
      }
      break;
    case core::TokenType::VAR_KW:
      tracking.current_var_token = token;
      break;
    case core::TokenType::SKIP_KW:
      tracking.current_skip_token = token;
      break;
    case core::TokenType::STOP_KW:
      tracking.current_stop_token = token;
      break;
    case core::TokenType::WRITE_KW:
      tracking.current_write_token = token;
      break;
    case core::TokenType::READ_KW:
      tracking.current_read_token = token;
      break;
    case core::TokenType::RETURN_KW:
      tracking.current_return_token = token;
      break;
    case core::TokenType::LEFT_SQUARE_PR:
      if (tracking.arr_type_tracking)
      {
        if (!tracking.program_start_left_square_token)
        {
          tracking.first_left_square_token = token;
          tracking.program_start_left_square_token = true;
        }
      }
      else
      {
        tracking.index_sq_stack.push_back(token);
      }
      break;
    case core::TokenType::RIGHT_SQUARE_PR:
      if (tracking.arr_type_tracking)
      {
        tracking.last_right_square_token = token;
        tracking.program_start_left_square_token = false;
      }
      else
      {
        tracking.index_last_right_sq = token;
        if (!tracking.index_sq_stack.empty())
        {
          tracking.index_first_left_sq = tracking.index_sq_stack.back();
          tracking.index_sq_stack.pop_back();
        }
      }
      break;
    case core::TokenType::LEFT_PR:
      if (!tracking.paren_tracked)
      {
        tracking.first_left_paren_token = token;
        tracking.paren_tracked = true;
      }
      break;
    case core::TokenType::RIGHT_PR:
      tracking.last_right_paren_token = token;
      tracking.paren_tracked = false;
      break;
    case core::TokenType::LEFT_CURLY_PR:
      tracking.curly_stack.push_back(token);
      break;
    case core::TokenType::RIGHT_CURLY_PR:
      tracking.last_right_curly_token = token;
      if (!tracking.curly_stack.empty())
      {
        tracking.last_left_curly_token = tracking.curly_stack.back();
        tracking.curly_stack.pop_back();
      }
      break;
    case core::TokenType::END_OF_FILE:
      if (!tracking.program_end_token_set)
      {
        tracking.program_end_token = token;
        tracking.program_end_token_set = true;
      }
      break;
    }
  }
}