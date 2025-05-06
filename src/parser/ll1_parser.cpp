#include "ll1_parser.h"

#include <cstring>

LL1Parser::LL1Parser(ScannerBase *sc) : ParserBase(sc)
{
  fill_table();
  has_peeked = false;

  currentFunctionList.clear();
  currentDeclarationSeq.clear();
  currentCommandSeq.clear();
}

AstNode *LL1Parser::END_OF_LIST_MARKER = reinterpret_cast<AstNode *>(-1);
Statement *LL1Parser::END_OF_LIST_ELSE = reinterpret_cast<Statement *>(-1);

AstNode *LL1Parser::parse_ast()
{
  st.push(SymbolLL1(TokenType::END_OF_FILE));
  st.push(SymbolLL1(NonTerminal::Source_NT));

  while (!st.empty())
  {
    SymbolLL1 top = st.top();
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
      if (rule == 73 && current_token.type == TokenType::ID_SY)
      {
        int pos = 1;
        Token tk = peek_token_n(pos);

        while (tk.type == TokenType::LEFT_SQUARE_PR)
        {
          pos += 3;
          tk = peek_token_n(pos);
        }
        if (tk.type == TokenType::EQUAL_OP)
        {
          rule = 72;
        }
      }
      if (rule == 138 && current_token.type == TokenType::ELSE_KW)
      {
        Token nextToken = peek_token();
        if (nextToken.type == TokenType::IF_KW)
        {
          rule = 66;
        }
      }
      if (rule == 0)
      {
        syntax_error("Unexpected token in prediction");
        return nullptr;
      }
      st.pop();
      push_rule(rule);
    }
    else
    {
      st.pop();
      if (!match(top.term))
      {
        return nullptr;
      }
    }
  }
  if (nodes.empty())
  {
    syntax_error("Parsing failed: no AST generated");
    return nullptr;
  }
  return dynamic_cast<AstNode *>(nodes.back());
}

Token LL1Parser::peek_token_n(int n)
{
  while ((int)peeked_tokens.size() < n)
  {
    Token next = sc->get_token();
    peeked_tokens.push_back(next);
  }
  return peeked_tokens[n - 1];
}

Token LL1Parser::peek_token()
{
  return peek_token_n(1);
}

void LL1Parser::get_token()
{
  if (!peeked_tokens.empty())
  {
    current_token = peeked_tokens.front();
    peeked_tokens.erase(peeked_tokens.begin());
  }
  else
  {
    current_token = sc->get_token();
  }
}

bool LL1Parser::match(TokenType t)
{
  if (current_token.type != t)
  {
    syntax_error("Token mismatch in match: expected " + Token::get_token_name(t) + ", got " + Token::get_token_name(current_token.type));
    return false;
  }

  previous_token = current_token;

  AstNode *leaf = nullptr;
  switch (t)
  {
  case TokenType::ID_SY:
    leaf = new Identifier(current_token.value, current_token.line, previous_token.line, current_token.start, previous_token.end);
    break;
  case TokenType::INTEGER_NUM:
    leaf = new IntegerLiteral(std::stoi(current_token.value), current_token.line, previous_token.line, current_token.start, previous_token.end);
    break;
  case TokenType::FLOAT_NUM:
    leaf = new FloatLiteral(std::stof(current_token.value), current_token.line, previous_token.line, current_token.start, previous_token.end);
    break;
  case TokenType::STRING_SY:
    leaf = new StringLiteral(current_token.value, current_token.line, previous_token.line, current_token.start, previous_token.end);
    break;
  case TokenType::TRUE_KW:
    leaf = new BooleanLiteral(true, current_token.line, previous_token.line, current_token.start, previous_token.end);
    break;
  case TokenType::FALSE_KW:
    leaf = new BooleanLiteral(false, current_token.line, previous_token.line, current_token.start, previous_token.end);
    break;
  case TokenType::INTEGER_TY:
  case TokenType::BOOLEAN_TY:
  case TokenType::STRING_TY:
  case TokenType::FLOAT_TY:
  case TokenType::VOID_TY:
    leaf = new PrimitiveType(current_token.value, current_token.line, previous_token.line, current_token.start, previous_token.end);
    break;
  case TokenType::PLUS_OP:
  case TokenType::MINUS_OP:
  case TokenType::MULT_OP:
  case TokenType::DIVIDE_OP:
  case TokenType::MOD_OP:
  case TokenType::LESS_EQUAL_OP:
  case TokenType::IS_EQUAL_OP:
  case TokenType::NOT_EQUAL_OP:
  case TokenType::LESS_THAN_OP:
  case TokenType::GREATER_THAN_OP:
  case TokenType::GREATER_EQUAL_OP:
  case TokenType::NOT_KW:
  case TokenType::INCREMENT_OP:
  case TokenType::DECREMENT_OP:
  case TokenType::STRINGIFY_OP:
  case TokenType::BOOLEAN_OP:
  case TokenType::ROUND_OP:
  case TokenType::LENGTH_OP:
    leaf = new Identifier(current_token.value, current_token.line, previous_token.line, current_token.start, previous_token.end);
  }
  if (leaf)
  {
    nodes.push_back(leaf);
  }

  get_token();
  return true;
}

void LL1Parser::fill_table()
{
  memset(parseTable, 0, sizeof(parseTable));

  parseTable[(int)NonTerminal::Source_NT][(int)TokenType::FUNC_KW] = 1;
  parseTable[(int)NonTerminal::Source_NT][(int)TokenType::PROGRAM_KW] = 1;

  parseTable[(int)NonTerminal::Function_List_NT][(int)TokenType::PROGRAM_KW] = 2;
  parseTable[(int)NonTerminal::Function_List_NT][(int)TokenType::FUNC_KW] = 3;

  parseTable[(int)NonTerminal::Function_NT][(int)TokenType::FUNC_KW] = 4;

  parseTable[(int)NonTerminal::Function_Type_NT][(int)TokenType::INTEGER_TY] = 5;
  parseTable[(int)NonTerminal::Function_Type_NT][(int)TokenType::BOOLEAN_TY] = 5;
  parseTable[(int)NonTerminal::Function_Type_NT][(int)TokenType::STRING_TY] = 5;
  parseTable[(int)NonTerminal::Function_Type_NT][(int)TokenType::FLOAT_TY] = 5;
  parseTable[(int)NonTerminal::Function_Type_NT][(int)TokenType::LEFT_SQUARE_PR] = 5;
  parseTable[(int)NonTerminal::Function_Type_NT][(int)TokenType::VOID_TY] = 6;

  parseTable[(int)NonTerminal::Type_NT][(int)TokenType::LEFT_SQUARE_PR] = 7;
  parseTable[(int)NonTerminal::Type_NT][(int)TokenType::INTEGER_TY] = 8;
  parseTable[(int)NonTerminal::Type_NT][(int)TokenType::BOOLEAN_TY] = 8;
  parseTable[(int)NonTerminal::Type_NT][(int)TokenType::STRING_TY] = 8;
  parseTable[(int)NonTerminal::Type_NT][(int)TokenType::FLOAT_TY] = 8;

  parseTable[(int)NonTerminal::Array_Type_NT][(int)TokenType::LEFT_SQUARE_PR] = 9;

  parseTable[(int)NonTerminal::Array_Type_Tail_NT][(int)TokenType::INTEGER_TY] = 10;
  parseTable[(int)NonTerminal::Array_Type_Tail_NT][(int)TokenType::BOOLEAN_TY] = 10;
  parseTable[(int)NonTerminal::Array_Type_Tail_NT][(int)TokenType::STRING_TY] = 10;
  parseTable[(int)NonTerminal::Array_Type_Tail_NT][(int)TokenType::FLOAT_TY] = 10;
  parseTable[(int)NonTerminal::Array_Type_Tail_NT][(int)TokenType::LEFT_SQUARE_PR] = 11;

  parseTable[(int)NonTerminal::Primitive_NT][(int)TokenType::INTEGER_TY] = 12;
  parseTable[(int)NonTerminal::Primitive_NT][(int)TokenType::BOOLEAN_TY] = 13;
  parseTable[(int)NonTerminal::Primitive_NT][(int)TokenType::STRING_TY] = 14;
  parseTable[(int)NonTerminal::Primitive_NT][(int)TokenType::FLOAT_TY] = 15;

  parseTable[(int)NonTerminal::Program_NT][(int)TokenType::PROGRAM_KW] = 16;

  parseTable[(int)NonTerminal::Block_NT][(int)TokenType::VAR_KW] = 17;
  parseTable[(int)NonTerminal::Block_NT][(int)TokenType::BEGIN_KW] = 17;

  parseTable[(int)NonTerminal::Declaration_Seq_NT][(int)TokenType::BEGIN_KW] = 18;
  parseTable[(int)NonTerminal::Declaration_Seq_NT][(int)TokenType::VAR_KW] = 19;

  parseTable[(int)NonTerminal::Declaration_NT][(int)TokenType::VAR_KW] = 20;

  parseTable[(int)NonTerminal::Decl_Tail_NT][(int)TokenType::ID_SY] = 21;

  parseTable[(int)NonTerminal::Decl_Tail_Right_NT][(int)TokenType::SEMI_COLON_SY] = 22;
  parseTable[(int)NonTerminal::Decl_Tail_Right_NT][(int)TokenType::EQUAL_OP] = 23;

  parseTable[(int)NonTerminal::Decl_Init_NT][(int)TokenType::TRUE_KW] = 24;
  parseTable[(int)NonTerminal::Decl_Init_NT][(int)TokenType::FALSE_KW] = 24;
  parseTable[(int)NonTerminal::Decl_Init_NT][(int)TokenType::NOT_KW] = 24;
  parseTable[(int)NonTerminal::Decl_Init_NT][(int)TokenType::INTEGER_NUM] = 24;
  parseTable[(int)NonTerminal::Decl_Init_NT][(int)TokenType::FLOAT_NUM] = 24;
  parseTable[(int)NonTerminal::Decl_Init_NT][(int)TokenType::MINUS_OP] = 24;
  parseTable[(int)NonTerminal::Decl_Init_NT][(int)TokenType::INCREMENT_OP] = 24;
  parseTable[(int)NonTerminal::Decl_Init_NT][(int)TokenType::DECREMENT_OP] = 24;
  parseTable[(int)NonTerminal::Decl_Init_NT][(int)TokenType::STRINGIFY_OP] = 24;
  parseTable[(int)NonTerminal::Decl_Init_NT][(int)TokenType::BOOLEAN_OP] = 24;
  parseTable[(int)NonTerminal::Decl_Init_NT][(int)TokenType::ROUND_OP] = 24;
  parseTable[(int)NonTerminal::Decl_Init_NT][(int)TokenType::LENGTH_OP] = 24;
  parseTable[(int)NonTerminal::Decl_Init_NT][(int)TokenType::LEFT_PR] = 24;
  parseTable[(int)NonTerminal::Decl_Init_NT][(int)TokenType::ID_SY] = 24;
  parseTable[(int)NonTerminal::Decl_Init_NT][(int)TokenType::STRING_SY] = 24;
  parseTable[(int)NonTerminal::Decl_Init_NT][(int)TokenType::LEFT_CURLY_PR] = 25;

  parseTable[(int)NonTerminal::Array_Value_NT][(int)TokenType::RIGHT_CURLY_PR] = 26;
  parseTable[(int)NonTerminal::Array_Value_NT][(int)TokenType::TRUE_KW] = 27;
  parseTable[(int)NonTerminal::Array_Value_NT][(int)TokenType::FALSE_KW] = 27;
  parseTable[(int)NonTerminal::Array_Value_NT][(int)TokenType::NOT_KW] = 27;
  parseTable[(int)NonTerminal::Array_Value_NT][(int)TokenType::INTEGER_NUM] = 27;
  parseTable[(int)NonTerminal::Array_Value_NT][(int)TokenType::FLOAT_NUM] = 27;
  parseTable[(int)NonTerminal::Array_Value_NT][(int)TokenType::MINUS_OP] = 27;
  parseTable[(int)NonTerminal::Array_Value_NT][(int)TokenType::INCREMENT_OP] = 27;
  parseTable[(int)NonTerminal::Array_Value_NT][(int)TokenType::DECREMENT_OP] = 27;
  parseTable[(int)NonTerminal::Array_Value_NT][(int)TokenType::STRINGIFY_OP] = 27;
  parseTable[(int)NonTerminal::Array_Value_NT][(int)TokenType::BOOLEAN_OP] = 27;
  parseTable[(int)NonTerminal::Array_Value_NT][(int)TokenType::ROUND_OP] = 27;
  parseTable[(int)NonTerminal::Array_Value_NT][(int)TokenType::LENGTH_OP] = 27;
  parseTable[(int)NonTerminal::Array_Value_NT][(int)TokenType::LEFT_PR] = 27;
  parseTable[(int)NonTerminal::Array_Value_NT][(int)TokenType::ID_SY] = 27;
  parseTable[(int)NonTerminal::Array_Value_NT][(int)TokenType::STRING_SY] = 27;
  parseTable[(int)NonTerminal::Array_Value_NT][(int)TokenType::LEFT_CURLY_PR] = 28;

  parseTable[(int)NonTerminal::Array_Nested_Value_NT][(int)TokenType::LEFT_CURLY_PR] = 29;

  parseTable[(int)NonTerminal::More_Nested_NT][(int)TokenType::RIGHT_CURLY_PR] = 30;
  parseTable[(int)NonTerminal::More_Nested_NT][(int)TokenType::COMMA_SY] = 31;

  parseTable[(int)NonTerminal::Value_List_NT][(int)TokenType::TRUE_KW] = 32;
  parseTable[(int)NonTerminal::Value_List_NT][(int)TokenType::FALSE_KW] = 32;
  parseTable[(int)NonTerminal::Value_List_NT][(int)TokenType::NOT_KW] = 32;
  parseTable[(int)NonTerminal::Value_List_NT][(int)TokenType::INTEGER_NUM] = 32;
  parseTable[(int)NonTerminal::Value_List_NT][(int)TokenType::FLOAT_NUM] = 32;
  parseTable[(int)NonTerminal::Value_List_NT][(int)TokenType::MINUS_OP] = 32;
  parseTable[(int)NonTerminal::Value_List_NT][(int)TokenType::INCREMENT_OP] = 32;
  parseTable[(int)NonTerminal::Value_List_NT][(int)TokenType::DECREMENT_OP] = 32;
  parseTable[(int)NonTerminal::Value_List_NT][(int)TokenType::STRINGIFY_OP] = 32;
  parseTable[(int)NonTerminal::Value_List_NT][(int)TokenType::BOOLEAN_OP] = 32;
  parseTable[(int)NonTerminal::Value_List_NT][(int)TokenType::ROUND_OP] = 32;
  parseTable[(int)NonTerminal::Value_List_NT][(int)TokenType::LENGTH_OP] = 32;
  parseTable[(int)NonTerminal::Value_List_NT][(int)TokenType::LEFT_PR] = 32;
  parseTable[(int)NonTerminal::Value_List_NT][(int)TokenType::ID_SY] = 32;
  parseTable[(int)NonTerminal::Value_List_NT][(int)TokenType::STRING_SY] = 32;

  parseTable[(int)NonTerminal::More_Values_NT][(int)TokenType::RIGHT_CURLY_PR] = 33;
  parseTable[(int)NonTerminal::More_Values_NT][(int)TokenType::COMMA_SY] = 34;

  parseTable[(int)NonTerminal::Variable_List_NT][(int)TokenType::ID_SY] = 35;

  parseTable[(int)NonTerminal::More_Variables_NT][(int)TokenType::COLON_SY] = 36;
  parseTable[(int)NonTerminal::More_Variables_NT][(int)TokenType::COMMA_SY] = 37;

  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::SKIP_KW] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::STOP_KW] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::READ_KW] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::WRITE_KW] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::FOR_KW] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::WHILE_KW] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::IF_KW] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::RETURN_KW] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::VAR_KW] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::MINUS_OP] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::STRINGIFY_OP] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::BOOLEAN_OP] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::ROUND_OP] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::LENGTH_OP] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::INCREMENT_OP] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::DECREMENT_OP] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::NOT_KW] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::LEFT_PR] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::INTEGER_NUM] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::FLOAT_NUM] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::STRING_TY] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::TRUE_KW] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::FALSE_KW] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::ID_SY] = 38;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::END_KW] = 39;
  parseTable[(int)NonTerminal::Command_Seq_NT][(int)TokenType::ELSE_KW] = 39;

  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::SKIP_KW] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::STOP_KW] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::READ_KW] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::WRITE_KW] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::FOR_KW] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::WHILE_KW] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::IF_KW] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::RETURN_KW] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::VAR_KW] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::MINUS_OP] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::STRINGIFY_OP] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::BOOLEAN_OP] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::ROUND_OP] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::LENGTH_OP] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::INCREMENT_OP] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::DECREMENT_OP] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::NOT_KW] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::LEFT_PR] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::INTEGER_NUM] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::FLOAT_NUM] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::STRING_TY] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::TRUE_KW] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::FALSE_KW] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::ID_SY] = 40;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::END_KW] = 41;
  parseTable[(int)NonTerminal::Command_Seq_Tail_NT][(int)TokenType::ELSE_KW] = 41;

  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::SKIP_KW] = 42;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::STOP_KW] = 43;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::READ_KW] = 44;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::WRITE_KW] = 45;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::FOR_KW] = 46;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::WHILE_KW] = 46;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::IF_KW] = 47;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::RETURN_KW] = 48;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::VAR_KW] = 50;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::MINUS_OP] = 49;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::STRINGIFY_OP] = 49;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::BOOLEAN_OP] = 49;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::ROUND_OP] = 49;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::LENGTH_OP] = 49;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::INCREMENT_OP] = 49;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::DECREMENT_OP] = 49;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::NOT_KW] = 49;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::LEFT_PR] = 49;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::INTEGER_NUM] = 49;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::FLOAT_NUM] = 49;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::STRING_TY] = 49;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::TRUE_KW] = 49;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::FALSE_KW] = 49;
  parseTable[(int)NonTerminal::Command_NT][(int)TokenType::ID_SY] = 49;

  parseTable[(int)NonTerminal::Read_Statement_NT][(int)TokenType::READ_KW] = 51;

  parseTable[(int)NonTerminal::Write_Statement_NT][(int)TokenType::WRITE_KW] = 52;

  parseTable[(int)NonTerminal::Variable_List_read_NT][(int)TokenType::ID_SY] = 53;

  parseTable[(int)NonTerminal::More_Variables_read_NT][(int)TokenType::SEMI_COLON_SY] = 54;
  parseTable[(int)NonTerminal::More_Variables_read_NT][(int)TokenType::COMMA_SY] = 55;

  parseTable[(int)NonTerminal::Expr_List_NT][(int)TokenType::TRUE_KW] = 56;
  parseTable[(int)NonTerminal::Expr_List_NT][(int)TokenType::FALSE_KW] = 56;
  parseTable[(int)NonTerminal::Expr_List_NT][(int)TokenType::NOT_KW] = 56;
  parseTable[(int)NonTerminal::Expr_List_NT][(int)TokenType::INTEGER_NUM] = 56;
  parseTable[(int)NonTerminal::Expr_List_NT][(int)TokenType::FLOAT_NUM] = 56;
  parseTable[(int)NonTerminal::Expr_List_NT][(int)TokenType::MINUS_OP] = 56;
  parseTable[(int)NonTerminal::Expr_List_NT][(int)TokenType::INCREMENT_OP] = 56;
  parseTable[(int)NonTerminal::Expr_List_NT][(int)TokenType::DECREMENT_OP] = 56;
  parseTable[(int)NonTerminal::Expr_List_NT][(int)TokenType::STRINGIFY_OP] = 56;
  parseTable[(int)NonTerminal::Expr_List_NT][(int)TokenType::BOOLEAN_OP] = 56;
  parseTable[(int)NonTerminal::Expr_List_NT][(int)TokenType::ROUND_OP] = 56;
  parseTable[(int)NonTerminal::Expr_List_NT][(int)TokenType::LENGTH_OP] = 56;
  parseTable[(int)NonTerminal::Expr_List_NT][(int)TokenType::LEFT_PR] = 56;
  parseTable[(int)NonTerminal::Expr_List_NT][(int)TokenType::ID_SY] = 56;
  parseTable[(int)NonTerminal::Expr_List_NT][(int)TokenType::STRING_SY] = 56;

  parseTable[(int)NonTerminal::More_Exprs_NT][(int)TokenType::SEMI_COLON_SY] = 57;
  parseTable[(int)NonTerminal::More_Exprs_NT][(int)TokenType::COMMA_SY] = 58;

  parseTable[(int)NonTerminal::Loops_NT][(int)TokenType::FOR_KW] = 59;
  parseTable[(int)NonTerminal::Loops_NT][(int)TokenType::WHILE_KW] = 60;

  parseTable[(int)NonTerminal::For_Loop_NT][(int)TokenType::FOR_KW] = 61;

  parseTable[(int)NonTerminal::Integer_Assign_NT][(int)TokenType::ID_SY] = 62;

  parseTable[(int)NonTerminal::While_Loop_NT][(int)TokenType::WHILE_KW] = 63;

  parseTable[(int)NonTerminal::IF_Statement_NT][(int)TokenType::IF_KW] = 64;

  parseTable[(int)NonTerminal::Else_Part_NT][(int)TokenType::END_KW] = 65;
  parseTable[(int)NonTerminal::Else_Part_NT][(int)TokenType::ELSE_KW] = 138;

  parseTable[(int)NonTerminal::Return_NT][(int)TokenType::RETURN_KW] = 67;

  parseTable[(int)NonTerminal::Return_Value_NT][(int)TokenType::SEMI_COLON_SY] = 68;
  parseTable[(int)NonTerminal::Return_Value_NT][(int)TokenType::TRUE_KW] = 69;
  parseTable[(int)NonTerminal::Return_Value_NT][(int)TokenType::FALSE_KW] = 69;
  parseTable[(int)NonTerminal::Return_Value_NT][(int)TokenType::NOT_KW] = 69;
  parseTable[(int)NonTerminal::Return_Value_NT][(int)TokenType::INTEGER_NUM] = 69;
  parseTable[(int)NonTerminal::Return_Value_NT][(int)TokenType::FLOAT_NUM] = 69;
  parseTable[(int)NonTerminal::Return_Value_NT][(int)TokenType::MINUS_OP] = 69;
  parseTable[(int)NonTerminal::Return_Value_NT][(int)TokenType::INCREMENT_OP] = 69;
  parseTable[(int)NonTerminal::Return_Value_NT][(int)TokenType::DECREMENT_OP] = 69;
  parseTable[(int)NonTerminal::Return_Value_NT][(int)TokenType::STRINGIFY_OP] = 69;
  parseTable[(int)NonTerminal::Return_Value_NT][(int)TokenType::BOOLEAN_OP] = 69;
  parseTable[(int)NonTerminal::Return_Value_NT][(int)TokenType::ROUND_OP] = 69;
  parseTable[(int)NonTerminal::Return_Value_NT][(int)TokenType::LENGTH_OP] = 69;
  parseTable[(int)NonTerminal::Return_Value_NT][(int)TokenType::LEFT_PR] = 69;
  parseTable[(int)NonTerminal::Return_Value_NT][(int)TokenType::ID_SY] = 69;
  parseTable[(int)NonTerminal::Return_Value_NT][(int)TokenType::STRING_SY] = 69;

  parseTable[(int)NonTerminal::Expr_Statement_NT][(int)TokenType::TRUE_KW] = 70;
  parseTable[(int)NonTerminal::Expr_Statement_NT][(int)TokenType::FALSE_KW] = 70;
  parseTable[(int)NonTerminal::Expr_Statement_NT][(int)TokenType::NOT_KW] = 70;
  parseTable[(int)NonTerminal::Expr_Statement_NT][(int)TokenType::INTEGER_NUM] = 70;
  parseTable[(int)NonTerminal::Expr_Statement_NT][(int)TokenType::FLOAT_NUM] = 70;
  parseTable[(int)NonTerminal::Expr_Statement_NT][(int)TokenType::MINUS_OP] = 70;
  parseTable[(int)NonTerminal::Expr_Statement_NT][(int)TokenType::INCREMENT_OP] = 70;
  parseTable[(int)NonTerminal::Expr_Statement_NT][(int)TokenType::DECREMENT_OP] = 70;
  parseTable[(int)NonTerminal::Expr_Statement_NT][(int)TokenType::STRINGIFY_OP] = 70;
  parseTable[(int)NonTerminal::Expr_Statement_NT][(int)TokenType::BOOLEAN_OP] = 70;
  parseTable[(int)NonTerminal::Expr_Statement_NT][(int)TokenType::ROUND_OP] = 70;
  parseTable[(int)NonTerminal::Expr_Statement_NT][(int)TokenType::LENGTH_OP] = 70;
  parseTable[(int)NonTerminal::Expr_Statement_NT][(int)TokenType::LEFT_PR] = 70;
  parseTable[(int)NonTerminal::Expr_Statement_NT][(int)TokenType::ID_SY] = 70;
  parseTable[(int)NonTerminal::Expr_Statement_NT][(int)TokenType::STRING_SY] = 70;

  parseTable[(int)NonTerminal::Expr_NT][(int)TokenType::ID_SY] = 71;
  parseTable[(int)NonTerminal::Expr_NT][(int)TokenType::TRUE_KW] = 71;
  parseTable[(int)NonTerminal::Expr_NT][(int)TokenType::FALSE_KW] = 71;
  parseTable[(int)NonTerminal::Expr_NT][(int)TokenType::NOT_KW] = 71;
  parseTable[(int)NonTerminal::Expr_NT][(int)TokenType::INTEGER_NUM] = 71;
  parseTable[(int)NonTerminal::Expr_NT][(int)TokenType::FLOAT_NUM] = 71;
  parseTable[(int)NonTerminal::Expr_NT][(int)TokenType::MINUS_OP] = 71;
  parseTable[(int)NonTerminal::Expr_NT][(int)TokenType::INCREMENT_OP] = 71;
  parseTable[(int)NonTerminal::Expr_NT][(int)TokenType::DECREMENT_OP] = 71;
  parseTable[(int)NonTerminal::Expr_NT][(int)TokenType::STRINGIFY_OP] = 71;
  parseTable[(int)NonTerminal::Expr_NT][(int)TokenType::BOOLEAN_OP] = 71;
  parseTable[(int)NonTerminal::Expr_NT][(int)TokenType::ROUND_OP] = 71;
  parseTable[(int)NonTerminal::Expr_NT][(int)TokenType::LENGTH_OP] = 71;
  parseTable[(int)NonTerminal::Expr_NT][(int)TokenType::LEFT_PR] = 71;
  parseTable[(int)NonTerminal::Expr_NT][(int)TokenType::STRING_SY] = 71;

  parseTable[(int)NonTerminal::Assign_Expr_NT][(int)TokenType::TRUE_KW] = 73;
  parseTable[(int)NonTerminal::Assign_Expr_NT][(int)TokenType::FALSE_KW] = 73;
  parseTable[(int)NonTerminal::Assign_Expr_NT][(int)TokenType::NOT_KW] = 73;
  parseTable[(int)NonTerminal::Assign_Expr_NT][(int)TokenType::INTEGER_NUM] = 73;
  parseTable[(int)NonTerminal::Assign_Expr_NT][(int)TokenType::FLOAT_NUM] = 73;
  parseTable[(int)NonTerminal::Assign_Expr_NT][(int)TokenType::MINUS_OP] = 73;
  parseTable[(int)NonTerminal::Assign_Expr_NT][(int)TokenType::INCREMENT_OP] = 73;
  parseTable[(int)NonTerminal::Assign_Expr_NT][(int)TokenType::DECREMENT_OP] = 73;
  parseTable[(int)NonTerminal::Assign_Expr_NT][(int)TokenType::STRINGIFY_OP] = 73;
  parseTable[(int)NonTerminal::Assign_Expr_NT][(int)TokenType::BOOLEAN_OP] = 73;
  parseTable[(int)NonTerminal::Assign_Expr_NT][(int)TokenType::ROUND_OP] = 73;
  parseTable[(int)NonTerminal::Assign_Expr_NT][(int)TokenType::LENGTH_OP] = 73;
  parseTable[(int)NonTerminal::Assign_Expr_NT][(int)TokenType::LEFT_PR] = 73;
  parseTable[(int)NonTerminal::Assign_Expr_NT][(int)TokenType::ID_SY] = 73;
  parseTable[(int)NonTerminal::Assign_Expr_NT][(int)TokenType::STRING_SY] = 73;

  parseTable[(int)NonTerminal::Assign_Expr_Value_NT][(int)TokenType::TRUE_KW] = 74;
  parseTable[(int)NonTerminal::Assign_Expr_Value_NT][(int)TokenType::FALSE_KW] = 74;
  parseTable[(int)NonTerminal::Assign_Expr_Value_NT][(int)TokenType::NOT_KW] = 74;
  parseTable[(int)NonTerminal::Assign_Expr_Value_NT][(int)TokenType::INTEGER_NUM] = 74;
  parseTable[(int)NonTerminal::Assign_Expr_Value_NT][(int)TokenType::FLOAT_NUM] = 74;
  parseTable[(int)NonTerminal::Assign_Expr_Value_NT][(int)TokenType::MINUS_OP] = 74;
  parseTable[(int)NonTerminal::Assign_Expr_Value_NT][(int)TokenType::INCREMENT_OP] = 74;
  parseTable[(int)NonTerminal::Assign_Expr_Value_NT][(int)TokenType::DECREMENT_OP] = 74;
  parseTable[(int)NonTerminal::Assign_Expr_Value_NT][(int)TokenType::STRINGIFY_OP] = 74;
  parseTable[(int)NonTerminal::Assign_Expr_Value_NT][(int)TokenType::BOOLEAN_OP] = 74;
  parseTable[(int)NonTerminal::Assign_Expr_Value_NT][(int)TokenType::ROUND_OP] = 74;
  parseTable[(int)NonTerminal::Assign_Expr_Value_NT][(int)TokenType::LENGTH_OP] = 74;
  parseTable[(int)NonTerminal::Assign_Expr_Value_NT][(int)TokenType::LEFT_PR] = 74;
  parseTable[(int)NonTerminal::Assign_Expr_Value_NT][(int)TokenType::ID_SY] = 74;
  parseTable[(int)NonTerminal::Assign_Expr_Value_NT][(int)TokenType::STRING_SY] = 74;
  parseTable[(int)NonTerminal::Assign_Expr_Value_NT][(int)TokenType::LEFT_CURLY_PR] = 75;

  parseTable[(int)NonTerminal::Assignable_Expr_NT][(int)TokenType::ID_SY] = 76;

  parseTable[(int)NonTerminal::Or_Expr_NT][(int)TokenType::TRUE_KW] = 77;
  parseTable[(int)NonTerminal::Or_Expr_NT][(int)TokenType::FALSE_KW] = 77;
  parseTable[(int)NonTerminal::Or_Expr_NT][(int)TokenType::NOT_KW] = 77;
  parseTable[(int)NonTerminal::Or_Expr_NT][(int)TokenType::INTEGER_NUM] = 77;
  parseTable[(int)NonTerminal::Or_Expr_NT][(int)TokenType::FLOAT_NUM] = 77;
  parseTable[(int)NonTerminal::Or_Expr_NT][(int)TokenType::MINUS_OP] = 77;
  parseTable[(int)NonTerminal::Or_Expr_NT][(int)TokenType::INCREMENT_OP] = 77;
  parseTable[(int)NonTerminal::Or_Expr_NT][(int)TokenType::DECREMENT_OP] = 77;
  parseTable[(int)NonTerminal::Or_Expr_NT][(int)TokenType::STRINGIFY_OP] = 77;
  parseTable[(int)NonTerminal::Or_Expr_NT][(int)TokenType::BOOLEAN_OP] = 77;
  parseTable[(int)NonTerminal::Or_Expr_NT][(int)TokenType::ROUND_OP] = 77;
  parseTable[(int)NonTerminal::Or_Expr_NT][(int)TokenType::LENGTH_OP] = 77;
  parseTable[(int)NonTerminal::Or_Expr_NT][(int)TokenType::LEFT_PR] = 77;
  parseTable[(int)NonTerminal::Or_Expr_NT][(int)TokenType::ID_SY] = 77;
  parseTable[(int)NonTerminal::Or_Expr_NT][(int)TokenType::STRING_SY] = 77;

  parseTable[(int)NonTerminal::Or_Tail_NT][(int)TokenType::SEMI_COLON_SY] = 78;
  parseTable[(int)NonTerminal::Or_Tail_NT][(int)TokenType::COMMA_SY] = 78;
  parseTable[(int)NonTerminal::Or_Tail_NT][(int)TokenType::RIGHT_CURLY_PR] = 78;
  parseTable[(int)NonTerminal::Or_Tail_NT][(int)TokenType::DO_KW] = 78;
  parseTable[(int)NonTerminal::Or_Tail_NT][(int)TokenType::THEN_KW] = 78;
  parseTable[(int)NonTerminal::Or_Tail_NT][(int)TokenType::RIGHT_PR] = 78;
  parseTable[(int)NonTerminal::Or_Tail_NT][(int)TokenType::RIGHT_SQUARE_PR] = 78;
  parseTable[(int)NonTerminal::Or_Tail_NT][(int)TokenType::OR_KW] = 79;

  parseTable[(int)NonTerminal::And_Expr_NT][(int)TokenType::TRUE_KW] = 80;
  parseTable[(int)NonTerminal::And_Expr_NT][(int)TokenType::FALSE_KW] = 80;
  parseTable[(int)NonTerminal::And_Expr_NT][(int)TokenType::NOT_KW] = 80;
  parseTable[(int)NonTerminal::And_Expr_NT][(int)TokenType::INTEGER_NUM] = 80;
  parseTable[(int)NonTerminal::And_Expr_NT][(int)TokenType::FLOAT_NUM] = 80;
  parseTable[(int)NonTerminal::And_Expr_NT][(int)TokenType::MINUS_OP] = 80;
  parseTable[(int)NonTerminal::And_Expr_NT][(int)TokenType::INCREMENT_OP] = 80;
  parseTable[(int)NonTerminal::And_Expr_NT][(int)TokenType::DECREMENT_OP] = 80;
  parseTable[(int)NonTerminal::And_Expr_NT][(int)TokenType::STRINGIFY_OP] = 80;
  parseTable[(int)NonTerminal::And_Expr_NT][(int)TokenType::BOOLEAN_OP] = 80;
  parseTable[(int)NonTerminal::And_Expr_NT][(int)TokenType::ROUND_OP] = 80;
  parseTable[(int)NonTerminal::And_Expr_NT][(int)TokenType::LENGTH_OP] = 80;
  parseTable[(int)NonTerminal::And_Expr_NT][(int)TokenType::LEFT_PR] = 80;
  parseTable[(int)NonTerminal::And_Expr_NT][(int)TokenType::ID_SY] = 80;
  parseTable[(int)NonTerminal::And_Expr_NT][(int)TokenType::STRING_SY] = 80;

  parseTable[(int)NonTerminal::And_Tail_NT][(int)TokenType::SEMI_COLON_SY] = 81;
  parseTable[(int)NonTerminal::And_Tail_NT][(int)TokenType::COMMA_SY] = 81;
  parseTable[(int)NonTerminal::And_Tail_NT][(int)TokenType::RIGHT_CURLY_PR] = 81;
  parseTable[(int)NonTerminal::And_Tail_NT][(int)TokenType::DO_KW] = 81;
  parseTable[(int)NonTerminal::And_Tail_NT][(int)TokenType::OR_KW] = 81;
  parseTable[(int)NonTerminal::And_Tail_NT][(int)TokenType::THEN_KW] = 81;
  parseTable[(int)NonTerminal::And_Tail_NT][(int)TokenType::RIGHT_PR] = 81;
  parseTable[(int)NonTerminal::And_Tail_NT][(int)TokenType::RIGHT_SQUARE_PR] = 81;
  parseTable[(int)NonTerminal::And_Tail_NT][(int)TokenType::AND_KW] = 82;

  parseTable[(int)NonTerminal::Equality_Expr_NT][(int)TokenType::TRUE_KW] = 83;
  parseTable[(int)NonTerminal::Equality_Expr_NT][(int)TokenType::FALSE_KW] = 83;
  parseTable[(int)NonTerminal::Equality_Expr_NT][(int)TokenType::NOT_KW] = 83;
  parseTable[(int)NonTerminal::Equality_Expr_NT][(int)TokenType::INTEGER_NUM] = 83;
  parseTable[(int)NonTerminal::Equality_Expr_NT][(int)TokenType::FLOAT_NUM] = 83;
  parseTable[(int)NonTerminal::Equality_Expr_NT][(int)TokenType::MINUS_OP] = 83;
  parseTable[(int)NonTerminal::Equality_Expr_NT][(int)TokenType::INCREMENT_OP] = 83;
  parseTable[(int)NonTerminal::Equality_Expr_NT][(int)TokenType::DECREMENT_OP] = 83;
  parseTable[(int)NonTerminal::Equality_Expr_NT][(int)TokenType::STRINGIFY_OP] = 83;
  parseTable[(int)NonTerminal::Equality_Expr_NT][(int)TokenType::BOOLEAN_OP] = 83;
  parseTable[(int)NonTerminal::Equality_Expr_NT][(int)TokenType::ROUND_OP] = 83;
  parseTable[(int)NonTerminal::Equality_Expr_NT][(int)TokenType::LENGTH_OP] = 83;
  parseTable[(int)NonTerminal::Equality_Expr_NT][(int)TokenType::LEFT_PR] = 83;
  parseTable[(int)NonTerminal::Equality_Expr_NT][(int)TokenType::ID_SY] = 83;
  parseTable[(int)NonTerminal::Equality_Expr_NT][(int)TokenType::STRING_SY] = 83;

  parseTable[(int)NonTerminal::Equality_Tail_NT][(int)TokenType::SEMI_COLON_SY] = 84;
  parseTable[(int)NonTerminal::Equality_Tail_NT][(int)TokenType::COMMA_SY] = 84;
  parseTable[(int)NonTerminal::Equality_Tail_NT][(int)TokenType::RIGHT_CURLY_PR] = 84;
  parseTable[(int)NonTerminal::Equality_Tail_NT][(int)TokenType::DO_KW] = 84;
  parseTable[(int)NonTerminal::Equality_Tail_NT][(int)TokenType::OR_KW] = 84;
  parseTable[(int)NonTerminal::Equality_Tail_NT][(int)TokenType::AND_KW] = 84;
  parseTable[(int)NonTerminal::Equality_Tail_NT][(int)TokenType::THEN_KW] = 84;
  parseTable[(int)NonTerminal::Equality_Tail_NT][(int)TokenType::RIGHT_PR] = 84;
  parseTable[(int)NonTerminal::Equality_Tail_NT][(int)TokenType::RIGHT_SQUARE_PR] = 84;
  parseTable[(int)NonTerminal::Equality_Tail_NT][(int)TokenType::IS_EQUAL_OP] = 85;
  parseTable[(int)NonTerminal::Equality_Tail_NT][(int)TokenType::NOT_EQUAL_OP] = 85;

  parseTable[(int)NonTerminal::OP_Equal_NT][(int)TokenType::IS_EQUAL_OP] = 86;
  parseTable[(int)NonTerminal::OP_Equal_NT][(int)TokenType::NOT_EQUAL_OP] = 87;

  parseTable[(int)NonTerminal::Relational_Expr_NT][(int)TokenType::TRUE_KW] = 88;
  parseTable[(int)NonTerminal::Relational_Expr_NT][(int)TokenType::FALSE_KW] = 88;
  parseTable[(int)NonTerminal::Relational_Expr_NT][(int)TokenType::NOT_KW] = 88;
  parseTable[(int)NonTerminal::Relational_Expr_NT][(int)TokenType::INTEGER_NUM] = 88;
  parseTable[(int)NonTerminal::Relational_Expr_NT][(int)TokenType::FLOAT_NUM] = 88;
  parseTable[(int)NonTerminal::Relational_Expr_NT][(int)TokenType::MINUS_OP] = 88;
  parseTable[(int)NonTerminal::Relational_Expr_NT][(int)TokenType::INCREMENT_OP] = 88;
  parseTable[(int)NonTerminal::Relational_Expr_NT][(int)TokenType::DECREMENT_OP] = 88;
  parseTable[(int)NonTerminal::Relational_Expr_NT][(int)TokenType::STRINGIFY_OP] = 88;
  parseTable[(int)NonTerminal::Relational_Expr_NT][(int)TokenType::BOOLEAN_OP] = 88;
  parseTable[(int)NonTerminal::Relational_Expr_NT][(int)TokenType::ROUND_OP] = 88;
  parseTable[(int)NonTerminal::Relational_Expr_NT][(int)TokenType::LENGTH_OP] = 88;
  parseTable[(int)NonTerminal::Relational_Expr_NT][(int)TokenType::LEFT_PR] = 88;
  parseTable[(int)NonTerminal::Relational_Expr_NT][(int)TokenType::ID_SY] = 88;
  parseTable[(int)NonTerminal::Relational_Expr_NT][(int)TokenType::STRING_SY] = 88;

  parseTable[(int)NonTerminal::Relational_Tail_NT][(int)TokenType::SEMI_COLON_SY] = 89;
  parseTable[(int)NonTerminal::Relational_Tail_NT][(int)TokenType::COMMA_SY] = 89;
  parseTable[(int)NonTerminal::Relational_Tail_NT][(int)TokenType::RIGHT_CURLY_PR] = 89;
  parseTable[(int)NonTerminal::Relational_Tail_NT][(int)TokenType::DO_KW] = 89;
  parseTable[(int)NonTerminal::Relational_Tail_NT][(int)TokenType::OR_KW] = 89;
  parseTable[(int)NonTerminal::Relational_Tail_NT][(int)TokenType::AND_KW] = 89;
  parseTable[(int)NonTerminal::Relational_Tail_NT][(int)TokenType::IS_EQUAL_OP] = 89;
  parseTable[(int)NonTerminal::Relational_Tail_NT][(int)TokenType::NOT_EQUAL_OP] = 89;
  parseTable[(int)NonTerminal::Relational_Tail_NT][(int)TokenType::THEN_KW] = 89;
  parseTable[(int)NonTerminal::Relational_Tail_NT][(int)TokenType::RIGHT_PR] = 89;
  parseTable[(int)NonTerminal::Relational_Tail_NT][(int)TokenType::RIGHT_SQUARE_PR] = 89;
  parseTable[(int)NonTerminal::Relational_Tail_NT][(int)TokenType::LESS_EQUAL_OP] = 90;
  parseTable[(int)NonTerminal::Relational_Tail_NT][(int)TokenType::LESS_THAN_OP] = 90;
  parseTable[(int)NonTerminal::Relational_Tail_NT][(int)TokenType::GREATER_THAN_OP] = 90;
  parseTable[(int)NonTerminal::Relational_Tail_NT][(int)TokenType::GREATER_EQUAL_OP] = 90;

  parseTable[(int)NonTerminal::Relation_NT][(int)TokenType::LESS_EQUAL_OP] = 91;
  parseTable[(int)NonTerminal::Relation_NT][(int)TokenType::LESS_THAN_OP] = 92;
  parseTable[(int)NonTerminal::Relation_NT][(int)TokenType::GREATER_THAN_OP] = 93;
  parseTable[(int)NonTerminal::Relation_NT][(int)TokenType::GREATER_EQUAL_OP] = 94;

  parseTable[(int)NonTerminal::Additive_Expr_NT][(int)TokenType::TRUE_KW] = 95;
  parseTable[(int)NonTerminal::Additive_Expr_NT][(int)TokenType::FALSE_KW] = 95;
  parseTable[(int)NonTerminal::Additive_Expr_NT][(int)TokenType::NOT_KW] = 95;
  parseTable[(int)NonTerminal::Additive_Expr_NT][(int)TokenType::INTEGER_NUM] = 95;
  parseTable[(int)NonTerminal::Additive_Expr_NT][(int)TokenType::FLOAT_NUM] = 95;
  parseTable[(int)NonTerminal::Additive_Expr_NT][(int)TokenType::MINUS_OP] = 95;
  parseTable[(int)NonTerminal::Additive_Expr_NT][(int)TokenType::INCREMENT_OP] = 95;
  parseTable[(int)NonTerminal::Additive_Expr_NT][(int)TokenType::DECREMENT_OP] = 95;
  parseTable[(int)NonTerminal::Additive_Expr_NT][(int)TokenType::STRINGIFY_OP] = 95;
  parseTable[(int)NonTerminal::Additive_Expr_NT][(int)TokenType::BOOLEAN_OP] = 95;
  parseTable[(int)NonTerminal::Additive_Expr_NT][(int)TokenType::ROUND_OP] = 95;
  parseTable[(int)NonTerminal::Additive_Expr_NT][(int)TokenType::LENGTH_OP] = 95;
  parseTable[(int)NonTerminal::Additive_Expr_NT][(int)TokenType::LEFT_PR] = 95;
  parseTable[(int)NonTerminal::Additive_Expr_NT][(int)TokenType::ID_SY] = 95;
  parseTable[(int)NonTerminal::Additive_Expr_NT][(int)TokenType::STRING_SY] = 95;

  parseTable[(int)NonTerminal::Additive_Tail_NT][(int)TokenType::SEMI_COLON_SY] = 96;
  parseTable[(int)NonTerminal::Additive_Tail_NT][(int)TokenType::COMMA_SY] = 96;
  parseTable[(int)NonTerminal::Additive_Tail_NT][(int)TokenType::RIGHT_CURLY_PR] = 96;
  parseTable[(int)NonTerminal::Additive_Tail_NT][(int)TokenType::DO_KW] = 96;
  parseTable[(int)NonTerminal::Additive_Tail_NT][(int)TokenType::THEN_KW] = 96;
  parseTable[(int)NonTerminal::Additive_Tail_NT][(int)TokenType::RIGHT_PR] = 96;
  parseTable[(int)NonTerminal::Additive_Tail_NT][(int)TokenType::RIGHT_SQUARE_PR] = 96;
  parseTable[(int)NonTerminal::Additive_Tail_NT][(int)TokenType::OR_KW] = 96;
  parseTable[(int)NonTerminal::Additive_Tail_NT][(int)TokenType::AND_KW] = 96;
  parseTable[(int)NonTerminal::Additive_Tail_NT][(int)TokenType::IS_EQUAL_OP] = 96;
  parseTable[(int)NonTerminal::Additive_Tail_NT][(int)TokenType::NOT_EQUAL_OP] = 96;
  parseTable[(int)NonTerminal::Additive_Tail_NT][(int)TokenType::LESS_EQUAL_OP] = 96;
  parseTable[(int)NonTerminal::Additive_Tail_NT][(int)TokenType::LESS_THAN_OP] = 96;
  parseTable[(int)NonTerminal::Additive_Tail_NT][(int)TokenType::GREATER_THAN_OP] = 96;
  parseTable[(int)NonTerminal::Additive_Tail_NT][(int)TokenType::GREATER_EQUAL_OP] = 96;
  parseTable[(int)NonTerminal::Additive_Tail_NT][(int)TokenType::PLUS_OP] = 97;
  parseTable[(int)NonTerminal::Additive_Tail_NT][(int)TokenType::MINUS_OP] = 97;

  parseTable[(int)NonTerminal::Weak_OP_NT][(int)TokenType::PLUS_OP] = 98;
  parseTable[(int)NonTerminal::Weak_OP_NT][(int)TokenType::MINUS_OP] = 99;

  parseTable[(int)NonTerminal::Multiplicative_Expr_NT][(int)TokenType::TRUE_KW] = 100;
  parseTable[(int)NonTerminal::Multiplicative_Expr_NT][(int)TokenType::FALSE_KW] = 100;
  parseTable[(int)NonTerminal::Multiplicative_Expr_NT][(int)TokenType::NOT_KW] = 100;
  parseTable[(int)NonTerminal::Multiplicative_Expr_NT][(int)TokenType::INTEGER_NUM] = 100;
  parseTable[(int)NonTerminal::Multiplicative_Expr_NT][(int)TokenType::FLOAT_NUM] = 100;
  parseTable[(int)NonTerminal::Multiplicative_Expr_NT][(int)TokenType::MINUS_OP] = 100;
  parseTable[(int)NonTerminal::Multiplicative_Expr_NT][(int)TokenType::INCREMENT_OP] = 100;
  parseTable[(int)NonTerminal::Multiplicative_Expr_NT][(int)TokenType::DECREMENT_OP] = 100;
  parseTable[(int)NonTerminal::Multiplicative_Expr_NT][(int)TokenType::STRINGIFY_OP] = 100;
  parseTable[(int)NonTerminal::Multiplicative_Expr_NT][(int)TokenType::BOOLEAN_OP] = 100;
  parseTable[(int)NonTerminal::Multiplicative_Expr_NT][(int)TokenType::ROUND_OP] = 100;
  parseTable[(int)NonTerminal::Multiplicative_Expr_NT][(int)TokenType::LENGTH_OP] = 100;
  parseTable[(int)NonTerminal::Multiplicative_Expr_NT][(int)TokenType::LEFT_PR] = 100;
  parseTable[(int)NonTerminal::Multiplicative_Expr_NT][(int)TokenType::ID_SY] = 100;
  parseTable[(int)NonTerminal::Multiplicative_Expr_NT][(int)TokenType::STRING_SY] = 100;

  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::SEMI_COLON_SY] = 101;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::COMMA_SY] = 101;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::RIGHT_CURLY_PR] = 101;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::DO_KW] = 101;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::THEN_KW] = 101;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::RIGHT_PR] = 101;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::RIGHT_SQUARE_PR] = 101;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::OR_KW] = 101;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::AND_KW] = 101;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::IS_EQUAL_OP] = 101;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::NOT_EQUAL_OP] = 101;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::LESS_EQUAL_OP] = 101;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::LESS_THAN_OP] = 101;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::GREATER_THAN_OP] = 101;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::GREATER_EQUAL_OP] = 101;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::PLUS_OP] = 101;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::MINUS_OP] = 101;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::MULT_OP] = 102;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::DIVIDE_OP] = 102;
  parseTable[(int)NonTerminal::Multiplicative_Tail_NT][(int)TokenType::MOD_OP] = 102;

  parseTable[(int)NonTerminal::Strong_OP_NT][(int)TokenType::MULT_OP] = 103;
  parseTable[(int)NonTerminal::Strong_OP_NT][(int)TokenType::DIVIDE_OP] = 104;
  parseTable[(int)NonTerminal::Strong_OP_NT][(int)TokenType::MOD_OP] = 105;

  parseTable[(int)NonTerminal::Unary_Expr_NT][(int)TokenType::MINUS_OP] = 106;
  parseTable[(int)NonTerminal::Unary_Expr_NT][(int)TokenType::STRINGIFY_OP] = 107;
  parseTable[(int)NonTerminal::Unary_Expr_NT][(int)TokenType::BOOLEAN_OP] = 108;
  parseTable[(int)NonTerminal::Unary_Expr_NT][(int)TokenType::ROUND_OP] = 109;
  parseTable[(int)NonTerminal::Unary_Expr_NT][(int)TokenType::LENGTH_OP] = 110;
  parseTable[(int)NonTerminal::Unary_Expr_NT][(int)TokenType::INCREMENT_OP] = 111;
  parseTable[(int)NonTerminal::Unary_Expr_NT][(int)TokenType::DECREMENT_OP] = 111;
  parseTable[(int)NonTerminal::Unary_Expr_NT][(int)TokenType::NOT_KW] = 112;
  parseTable[(int)NonTerminal::Unary_Expr_NT][(int)TokenType::TRUE_KW] = 113;
  parseTable[(int)NonTerminal::Unary_Expr_NT][(int)TokenType::FALSE_KW] = 113;
  parseTable[(int)NonTerminal::Unary_Expr_NT][(int)TokenType::INTEGER_NUM] = 113;
  parseTable[(int)NonTerminal::Unary_Expr_NT][(int)TokenType::FLOAT_NUM] = 113;
  parseTable[(int)NonTerminal::Unary_Expr_NT][(int)TokenType::LEFT_PR] = 113;
  parseTable[(int)NonTerminal::Unary_Expr_NT][(int)TokenType::ID_SY] = 113;
  parseTable[(int)NonTerminal::Unary_Expr_NT][(int)TokenType::STRING_SY] = 113;

  parseTable[(int)NonTerminal::Prefix_NT][(int)TokenType::INCREMENT_OP] = 114;
  parseTable[(int)NonTerminal::Prefix_NT][(int)TokenType::DECREMENT_OP] = 115;

  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::SEMI_COLON_SY] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::COMMA_SY] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::RIGHT_CURLY_PR] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::DO_KW] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::THEN_KW] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::RIGHT_PR] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::RIGHT_SQUARE_PR] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::OR_KW] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::AND_KW] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::IS_EQUAL_OP] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::NOT_EQUAL_OP] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::LESS_EQUAL_OP] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::LESS_THAN_OP] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::GREATER_THAN_OP] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::GREATER_EQUAL_OP] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::PLUS_OP] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::MINUS_OP] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::MULT_OP] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::DIVIDE_OP] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::MOD_OP] = 116;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::INCREMENT_OP] = 117;
  parseTable[(int)NonTerminal::maybe_Postfix_NT][(int)TokenType::DECREMENT_OP] = 118;

  parseTable[(int)NonTerminal::Index_Expr_NT][(int)TokenType::ID_SY] = 119;
  parseTable[(int)NonTerminal::Index_Expr_NT][(int)TokenType::LEFT_PR] = 120;
  parseTable[(int)NonTerminal::Index_Expr_NT][(int)TokenType::INTEGER_NUM] = 120;
  parseTable[(int)NonTerminal::Index_Expr_NT][(int)TokenType::FLOAT_NUM] = 120;
  parseTable[(int)NonTerminal::Index_Expr_NT][(int)TokenType::STRING_SY] = 120;
  parseTable[(int)NonTerminal::Index_Expr_NT][(int)TokenType::TRUE_KW] = 120;
  parseTable[(int)NonTerminal::Index_Expr_NT][(int)TokenType::FALSE_KW] = 120;

  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::SEMI_COLON_SY] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::COMMA_SY] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::RIGHT_CURLY_PR] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::DO_KW] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::THEN_KW] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::RIGHT_PR] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::RIGHT_SQUARE_PR] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::OR_KW] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::AND_KW] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::IS_EQUAL_OP] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::NOT_EQUAL_OP] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::LESS_EQUAL_OP] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::LESS_THAN_OP] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::GREATER_THAN_OP] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::GREATER_EQUAL_OP] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::PLUS_OP] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::MINUS_OP] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::MULT_OP] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::DIVIDE_OP] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::MOD_OP] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::EQUAL_OP] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::INCREMENT_OP] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::DECREMENT_OP] = 121;
  parseTable[(int)NonTerminal::Index_Chain_NT][(int)TokenType::LEFT_SQUARE_PR] = 122;

  parseTable[(int)NonTerminal::Primary_Expr_NT][(int)TokenType::LEFT_PR] = 123;
  parseTable[(int)NonTerminal::Primary_Expr_NT][(int)TokenType::INTEGER_NUM] = 124;
  parseTable[(int)NonTerminal::Primary_Expr_NT][(int)TokenType::FLOAT_NUM] = 124;
  parseTable[(int)NonTerminal::Primary_Expr_NT][(int)TokenType::STRING_SY] = 124;
  parseTable[(int)NonTerminal::Primary_Expr_NT][(int)TokenType::TRUE_KW] = 124;
  parseTable[(int)NonTerminal::Primary_Expr_NT][(int)TokenType::FALSE_KW] = 124;

  parseTable[(int)NonTerminal::Literal_NT][(int)TokenType::INTEGER_NUM] = 125;
  parseTable[(int)NonTerminal::Literal_NT][(int)TokenType::FLOAT_NUM] = 126;
  parseTable[(int)NonTerminal::Literal_NT][(int)TokenType::STRING_SY] = 127;
  parseTable[(int)NonTerminal::Literal_NT][(int)TokenType::TRUE_KW] = 128;
  parseTable[(int)NonTerminal::Literal_NT][(int)TokenType::FALSE_KW] = 129;

  parseTable[(int)NonTerminal::CallOrVariable_NT][(int)TokenType::ID_SY] = 130;

  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::SEMI_COLON_SY] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::COMMA_SY] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::RIGHT_CURLY_PR] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::DO_KW] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::THEN_KW] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::RIGHT_PR] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::RIGHT_SQUARE_PR] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::OR_KW] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::AND_KW] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::INCREMENT_OP] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::DECREMENT_OP] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::IS_EQUAL_OP] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::NOT_EQUAL_OP] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::LESS_EQUAL_OP] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::LESS_THAN_OP] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::GREATER_THAN_OP] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::GREATER_EQUAL_OP] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::PLUS_OP] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::MINUS_OP] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::MULT_OP] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::DIVIDE_OP] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::MOD_OP] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::EQUAL_OP] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::LEFT_SQUARE_PR] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::LEFT_PR] = 132;

  parseTable[(int)NonTerminal::Call_Expr_NT][(int)TokenType::LEFT_PR] = 133;

  parseTable[(int)NonTerminal::Call_Expr_Tail_NT][(int)TokenType::RIGHT_PR] = 134;
  parseTable[(int)NonTerminal::Call_Expr_Tail_NT][(int)TokenType::COMMA_SY] = 135;

  parseTable[(int)NonTerminal::May_be_Arg_NT][(int)TokenType::TRUE_KW] = 136;
  parseTable[(int)NonTerminal::May_be_Arg_NT][(int)TokenType::FALSE_KW] = 136;
  parseTable[(int)NonTerminal::May_be_Arg_NT][(int)TokenType::NOT_KW] = 136;
  parseTable[(int)NonTerminal::May_be_Arg_NT][(int)TokenType::INTEGER_NUM] = 136;
  parseTable[(int)NonTerminal::May_be_Arg_NT][(int)TokenType::FLOAT_NUM] = 136;
  parseTable[(int)NonTerminal::May_be_Arg_NT][(int)TokenType::MINUS_OP] = 136;
  parseTable[(int)NonTerminal::May_be_Arg_NT][(int)TokenType::INCREMENT_OP] = 136;
  parseTable[(int)NonTerminal::May_be_Arg_NT][(int)TokenType::DECREMENT_OP] = 136;
  parseTable[(int)NonTerminal::May_be_Arg_NT][(int)TokenType::STRINGIFY_OP] = 136;
  parseTable[(int)NonTerminal::May_be_Arg_NT][(int)TokenType::BOOLEAN_OP] = 136;
  parseTable[(int)NonTerminal::May_be_Arg_NT][(int)TokenType::ROUND_OP] = 136;
  parseTable[(int)NonTerminal::May_be_Arg_NT][(int)TokenType::LENGTH_OP] = 136;
  parseTable[(int)NonTerminal::May_be_Arg_NT][(int)TokenType::LEFT_PR] = 136;
  parseTable[(int)NonTerminal::May_be_Arg_NT][(int)TokenType::ID_SY] = 136;
  parseTable[(int)NonTerminal::May_be_Arg_NT][(int)TokenType::STRING_SY] = 136;
  parseTable[(int)NonTerminal::May_be_Arg_NT][(int)TokenType::RIGHT_PR] = 137;
}

void LL1Parser::push_rule(int rule)
{
  switch (rule)
  {
  case 1:
    st.push(SymbolLL1(1));
    st.push(SymbolLL1(NonTerminal::Program_NT));
    st.push(SymbolLL1(NonTerminal::Function_List_NT));
    break;
  case 2:
    break;
  case 3:
    st.push(SymbolLL1(3));
    st.push(SymbolLL1(NonTerminal::Function_List_NT));
    st.push(SymbolLL1(NonTerminal::Function_NT));
    break;
  case 4:
    st.push(SymbolLL1(4));
    st.push(SymbolLL1(TokenType::FUNC_KW));
    st.push(SymbolLL1(TokenType::END_KW));
    st.push(SymbolLL1(NonTerminal::Command_Seq_NT));
    st.push(SymbolLL1(TokenType::BEGIN_KW));
    st.push(SymbolLL1(NonTerminal::Declaration_Seq_NT));
    st.push(SymbolLL1(TokenType::HAS_KW));
    st.push(SymbolLL1(TokenType::ID_SY));
    st.push(SymbolLL1(NonTerminal::Function_Type_NT));
    st.push(SymbolLL1(TokenType::FUNC_KW));
    break;
  case 5:
    st.push(SymbolLL1(5));
    st.push(SymbolLL1(NonTerminal::Type_NT));
    break;
  case 6:
    st.push(SymbolLL1(6));
    st.push(SymbolLL1(TokenType::VOID_TY));
    break;
  case 7:
    st.push(SymbolLL1(7));
    st.push(SymbolLL1(NonTerminal::Array_Type_NT));

    break;
  case 8:
    st.push(SymbolLL1(NonTerminal::Primitive_NT));
    break;
  case 9:
    st.push(SymbolLL1(NonTerminal::Array_Type_Tail_NT));
    st.push(SymbolLL1(TokenType::LEFT_SQUARE_PR));
    break;
  case 10:
    st.push(SymbolLL1(TokenType::RIGHT_SQUARE_PR));
    st.push(SymbolLL1(NonTerminal::Primitive_NT));
    break;
  case 11:
    st.push(SymbolLL1(TokenType::RIGHT_SQUARE_PR));
    st.push(SymbolLL1(NonTerminal::Array_Type_NT));
    break;
  case 12:
    st.push(SymbolLL1(12));
    st.push(SymbolLL1(TokenType::INTEGER_TY));
    break;
  case 13:
    st.push(SymbolLL1(13));
    st.push(SymbolLL1(TokenType::BOOLEAN_TY));
    break;
  case 14:
    st.push(SymbolLL1(14));
    st.push(SymbolLL1(TokenType::STRING_TY));
    break;
  case 15:
    st.push(SymbolLL1(15));
    st.push(SymbolLL1(TokenType::FLOAT_TY));
    break;
  case 16:
    st.push(SymbolLL1(16));
    st.push(SymbolLL1(NonTerminal::Block_NT));
    st.push(SymbolLL1(TokenType::IS_KW));
    st.push(SymbolLL1(TokenType::ID_SY));
    st.push(SymbolLL1(TokenType::PROGRAM_KW));
    break;
  case 17:
    st.push(SymbolLL1(TokenType::END_KW));
    st.push(SymbolLL1(NonTerminal::Command_Seq_NT));
    st.push(SymbolLL1(TokenType::BEGIN_KW));
    st.push(SymbolLL1(NonTerminal::Declaration_Seq_NT));
    break;
  case 18:
    break;
  case 19:
    st.push(SymbolLL1(19));
    st.push(SymbolLL1(NonTerminal::Declaration_Seq_NT));
    st.push(SymbolLL1(NonTerminal::Declaration_NT));
    break;
  case 20:
    st.push(SymbolLL1(20));
    st.push(SymbolLL1(NonTerminal::Decl_Tail_NT));
    nodes.push_back(END_OF_LIST_MARKER);
    st.push(SymbolLL1(TokenType::VAR_KW));
    break;
  case 21:
    st.push(SymbolLL1(21));
    st.push(SymbolLL1(NonTerminal::Decl_Tail_Right_NT));
    st.push(SymbolLL1(NonTerminal::Type_NT));
    st.push(SymbolLL1(TokenType::COLON_SY));
    st.push(SymbolLL1(NonTerminal::Variable_List_NT));
    break;
  case 22:
    st.push(SymbolLL1(TokenType::SEMI_COLON_SY));
    break;
  case 23:
    st.push(SymbolLL1(23));
    st.push(SymbolLL1(TokenType::SEMI_COLON_SY));
    st.push(SymbolLL1(NonTerminal::Decl_Init_NT));
    st.push(SymbolLL1(TokenType::EQUAL_OP));
    break;
  case 24:
    st.push(SymbolLL1(NonTerminal::Or_Expr_NT));
    break;
  case 25:
    st.push(SymbolLL1(25));
    st.push(SymbolLL1(TokenType::RIGHT_CURLY_PR));
    st.push(SymbolLL1(NonTerminal::Array_Value_NT));
    st.push(SymbolLL1(TokenType::LEFT_CURLY_PR));
    break;
  case 26:
    break;
  case 27:
    st.push(SymbolLL1(NonTerminal::Value_List_NT));
    break;
  case 28:
    st.push(SymbolLL1(NonTerminal::Array_Nested_Value_NT));
    break;
  case 29:
    st.push(SymbolLL1(NonTerminal::More_Nested_NT));
    st.push(SymbolLL1(TokenType::RIGHT_CURLY_PR));
    st.push(SymbolLL1(NonTerminal::Value_List_NT));
    st.push(SymbolLL1(TokenType::LEFT_CURLY_PR));
    break;
  case 30:
    break;
  case 31:
    st.push(SymbolLL1(NonTerminal::Array_Nested_Value_NT));
    st.push(SymbolLL1(TokenType::COMMA_SY));
    break;
  case 32:
    st.push(SymbolLL1(NonTerminal::More_Values_NT));
    st.push(SymbolLL1(NonTerminal::Or_Expr_NT));
    break;
  case 33:
    break;
  case 34:
    st.push(SymbolLL1(NonTerminal::Value_List_NT));
    st.push(SymbolLL1(TokenType::COMMA_SY));
    break;
  case 35:
    st.push(SymbolLL1(NonTerminal::More_Variables_NT));
    st.push(SymbolLL1(TokenType::ID_SY));

    break;
  case 36:
    break;
  case 37:
    st.push(SymbolLL1(NonTerminal::Variable_List_NT));
    st.push(SymbolLL1(TokenType::COMMA_SY));
    break;
  case 38:
    st.push(SymbolLL1(38));
    st.push(SymbolLL1(NonTerminal::Command_Seq_Tail_NT));
    st.push(SymbolLL1(NonTerminal::Command_NT));
    break;
  case 39:
    break;
  case 40:
    st.push(SymbolLL1(40));
    st.push(SymbolLL1(NonTerminal::Command_Seq_Tail_NT));
    st.push(SymbolLL1(NonTerminal::Command_NT));
    break;
  case 41:
    break;
  case 42:
    st.push(SymbolLL1(42));
    st.push(SymbolLL1(TokenType::SEMI_COLON_SY));
    st.push(SymbolLL1(TokenType::SKIP_KW));
    break;
  case 43:
    st.push(SymbolLL1(43));
    st.push(SymbolLL1(TokenType::SEMI_COLON_SY));
    st.push(SymbolLL1(TokenType::STOP_KW));
    break;
  case 44:
    st.push(SymbolLL1(44));
    st.push(SymbolLL1(NonTerminal::Read_Statement_NT));
    break;
  case 45:
    st.push(SymbolLL1(45));
    st.push(SymbolLL1(NonTerminal::Write_Statement_NT));
    break;
  case 46:
    st.push(SymbolLL1(NonTerminal::Loops_NT));
    break;
  case 47:
    st.push(SymbolLL1(47));
    st.push(SymbolLL1(NonTerminal::IF_Statement_NT));
    break;
  case 48:
    st.push(SymbolLL1(48));
    st.push(SymbolLL1(NonTerminal::Return_NT));
    break;
  case 49:
    st.push(SymbolLL1(NonTerminal::Expr_Statement_NT));
    break;
  case 50:
    st.push(SymbolLL1(NonTerminal::Declaration_NT));
    break;
  case 51:
    st.push(SymbolLL1(TokenType::SEMI_COLON_SY));
    st.push(SymbolLL1(NonTerminal::Variable_List_read_NT));
    nodes.push_back(END_OF_LIST_MARKER);
    st.push(SymbolLL1(TokenType::READ_KW));
    break;
  case 52:
    st.push(SymbolLL1(TokenType::SEMI_COLON_SY));
    st.push(SymbolLL1(NonTerminal::Expr_List_NT));
    nodes.push_back(END_OF_LIST_MARKER);
    st.push(SymbolLL1(TokenType::WRITE_KW));
    break;
  case 53:
    st.push(SymbolLL1(NonTerminal::More_Variables_read_NT));
    st.push(SymbolLL1(NonTerminal::Index_Chain_NT));
    st.push(SymbolLL1(TokenType::ID_SY));
    break;
  case 54:
    break;
  case 55:
    st.push(SymbolLL1(NonTerminal::Variable_List_read_NT));
    st.push(SymbolLL1(TokenType::COMMA_SY));
    break;
  case 56:
    st.push(SymbolLL1(NonTerminal::More_Exprs_NT));
    st.push(SymbolLL1(NonTerminal::Expr_NT));
    break;
  case 57:
    break;
  case 58:
    st.push(SymbolLL1(NonTerminal::Expr_List_NT));
    st.push(SymbolLL1(TokenType::COMMA_SY));
    break;
  case 59:
    st.push(SymbolLL1(NonTerminal::For_Loop_NT));
    break;
  case 60:
    st.push(SymbolLL1(NonTerminal::While_Loop_NT));
    break;
  case 61:
    st.push(SymbolLL1(61));
    st.push(SymbolLL1(TokenType::FOR_KW));
    st.push(SymbolLL1(TokenType::END_KW));
    st.push(SymbolLL1(NonTerminal::Command_Seq_NT));
    st.push(SymbolLL1(TokenType::DO_KW));
    st.push(SymbolLL1(NonTerminal::Expr_NT));
    st.push(SymbolLL1(TokenType::SEMI_COLON_SY));
    st.push(SymbolLL1(NonTerminal::Or_Expr_NT));
    st.push(SymbolLL1(TokenType::SEMI_COLON_SY));
    st.push(SymbolLL1(NonTerminal::Integer_Assign_NT));
    st.push(SymbolLL1(TokenType::FOR_KW));
    break;
  case 62:
    st.push(SymbolLL1(62));
    st.push(SymbolLL1(NonTerminal::Expr_NT));
    st.push(SymbolLL1(TokenType::EQUAL_OP));
    st.push(SymbolLL1(TokenType::ID_SY));
    break;
  case 63:
    st.push(SymbolLL1(63));
    st.push(SymbolLL1(TokenType::WHILE_KW));
    st.push(SymbolLL1(TokenType::END_KW));
    st.push(SymbolLL1(NonTerminal::Command_Seq_NT));
    st.push(SymbolLL1(TokenType::DO_KW));
    st.push(SymbolLL1(NonTerminal::Or_Expr_NT));
    st.push(SymbolLL1(TokenType::WHILE_KW));
    break;
  case 64:
    st.push(SymbolLL1(TokenType::IF_KW));
    st.push(SymbolLL1(TokenType::END_KW));
    st.push(SymbolLL1(NonTerminal::Else_Part_NT));
    st.push(SymbolLL1(NonTerminal::Command_Seq_NT));
    st.push(SymbolLL1(TokenType::THEN_KW));
    st.push(SymbolLL1(NonTerminal::Expr_NT));
    st.push(SymbolLL1(TokenType::IF_KW));
    break;
  case 65:
    currentCommandSeq.insert(currentCommandSeq.begin(), nullptr);
    break;
  case 66:
    st.push(SymbolLL1(47));
    st.push(SymbolLL1(NonTerminal::Else_Part_NT));
    st.push(SymbolLL1(NonTerminal::Command_Seq_NT));
    st.push(SymbolLL1(TokenType::THEN_KW));
    st.push(SymbolLL1(NonTerminal::Or_Expr_NT));
    st.push(SymbolLL1(TokenType::IF_KW));
    st.push(SymbolLL1(TokenType::ELSE_KW));
    currentCommandSeq.insert(currentCommandSeq.begin(), nullptr);
    currentCommandSeq.insert(currentCommandSeq.begin(), END_OF_LIST_ELSE);
    break;
  case 67:
    st.push(SymbolLL1(TokenType::SEMI_COLON_SY));
    st.push(SymbolLL1(NonTerminal::Return_Value_NT));
    st.push(SymbolLL1(TokenType::RETURN_KW));
    break;
  case 68:
    nodes.push_back(nullptr);
    break;
  case 69:
    st.push(SymbolLL1(NonTerminal::Or_Expr_NT));
    break;
  case 70:
    st.push(SymbolLL1(TokenType::SEMI_COLON_SY));
    st.push(SymbolLL1(NonTerminal::Expr_NT));
    break;
  case 71:
    st.push(SymbolLL1(NonTerminal::Assign_Expr_NT));
    break;
  case 72:
    st.push(SymbolLL1(72));
    st.push(SymbolLL1(NonTerminal::Assign_Expr_Value_NT));
    st.push(SymbolLL1(TokenType::EQUAL_OP));
    st.push(SymbolLL1(NonTerminal::Assignable_Expr_NT));
    break;
  case 73:
    st.push(SymbolLL1(NonTerminal::Or_Expr_NT));
    break;
  case 74:
    st.push(SymbolLL1(NonTerminal::Expr_NT));
    break;
  case 75:
    st.push(SymbolLL1(75));
    st.push(SymbolLL1(TokenType::RIGHT_CURLY_PR));
    st.push(SymbolLL1(NonTerminal::Array_Value_NT));
    st.push(SymbolLL1(TokenType::LEFT_CURLY_PR));
    break;
  case 76:
    st.push(SymbolLL1(NonTerminal::Index_Chain_NT));
    st.push(SymbolLL1(TokenType::ID_SY));
    break;
  case 77:
    st.push(SymbolLL1(NonTerminal::Or_Tail_NT));
    st.push(SymbolLL1(NonTerminal::And_Expr_NT));
    break;
  case 78:
    break;
  case 79:
    st.push(SymbolLL1(79));
    st.push(SymbolLL1(NonTerminal::Or_Tail_NT));
    st.push(SymbolLL1(NonTerminal::And_Expr_NT));
    st.push(SymbolLL1(TokenType::OR_KW));
    break;
  case 80:
    st.push(SymbolLL1(NonTerminal::And_Tail_NT));
    st.push(SymbolLL1(NonTerminal::Equality_Expr_NT));
    break;
  case 81:
    break;
  case 82:
    st.push(SymbolLL1(82));
    st.push(SymbolLL1(NonTerminal::And_Tail_NT));
    st.push(SymbolLL1(NonTerminal::Equality_Expr_NT));
    st.push(SymbolLL1(TokenType::AND_KW));
    break;
  case 83:
    st.push(SymbolLL1(NonTerminal::Equality_Tail_NT));
    st.push(SymbolLL1(NonTerminal::Relational_Expr_NT));
    break;
  case 84:
    break;
  case 85:
    st.push(SymbolLL1(85));
    st.push(SymbolLL1(NonTerminal::Equality_Tail_NT));
    st.push(SymbolLL1(NonTerminal::Relational_Expr_NT));
    st.push(SymbolLL1(NonTerminal::OP_Equal_NT));
    break;
  case 86:
    st.push(SymbolLL1(TokenType::IS_EQUAL_OP));
    break;
  case 87:
    st.push(SymbolLL1(TokenType::NOT_EQUAL_OP));
    break;
  case 88:
    st.push(SymbolLL1(NonTerminal::Relational_Tail_NT));
    st.push(SymbolLL1(NonTerminal::Additive_Expr_NT));
    break;
  case 89:
    break;
  case 90:
    st.push(SymbolLL1(90));
    st.push(SymbolLL1(NonTerminal::Relational_Tail_NT));
    st.push(SymbolLL1(NonTerminal::Additive_Expr_NT));
    st.push(SymbolLL1(NonTerminal::Relation_NT));
    break;
  case 91:
    st.push(SymbolLL1(TokenType::LESS_EQUAL_OP));
    break;
  case 92:
    st.push(SymbolLL1(TokenType::LESS_THAN_OP));
    break;
  case 93:
    st.push(SymbolLL1(TokenType::GREATER_THAN_OP));
    break;
  case 94:
    st.push(SymbolLL1(TokenType::GREATER_EQUAL_OP));
    break;
  case 95:
    st.push(SymbolLL1(NonTerminal::Additive_Tail_NT));
    st.push(SymbolLL1(NonTerminal::Multiplicative_Expr_NT));
    break;
  case 96:
    break;
  case 97:
    st.push(SymbolLL1(97));
    st.push(SymbolLL1(NonTerminal::Additive_Tail_NT));
    st.push(SymbolLL1(NonTerminal::Multiplicative_Expr_NT));
    st.push(SymbolLL1(NonTerminal::Weak_OP_NT));
    break;
  case 98:
    st.push(SymbolLL1(TokenType::PLUS_OP));
    break;
  case 99:
    st.push(SymbolLL1(TokenType::MINUS_OP));
    break;
  case 100:
    st.push(SymbolLL1(NonTerminal::Multiplicative_Tail_NT));
    st.push(SymbolLL1(NonTerminal::Unary_Expr_NT));
    break;
  case 101:
    break;
  case 102:
    st.push(SymbolLL1(102));
    st.push(SymbolLL1(NonTerminal::Multiplicative_Tail_NT));
    st.push(SymbolLL1(NonTerminal::Unary_Expr_NT));
    st.push(SymbolLL1(NonTerminal::Strong_OP_NT));
    break;
  case 103:
    st.push(SymbolLL1(TokenType::MULT_OP));
    break;
  case 104:
    st.push(SymbolLL1(TokenType::DIVIDE_OP));
    break;
  case 105:
    st.push(SymbolLL1(TokenType::MOD_OP));
    break;
  case 106:
    st.push(SymbolLL1(106));
    st.push(SymbolLL1(NonTerminal::Index_Expr_NT));
    st.push(SymbolLL1(TokenType::MINUS_OP));
    break;
  case 107:
    st.push(SymbolLL1(107));
    st.push(SymbolLL1(NonTerminal::Index_Expr_NT));
    st.push(SymbolLL1(TokenType::STRINGIFY_OP));
    break;
  case 108:
    st.push(SymbolLL1(108));
    st.push(SymbolLL1(NonTerminal::Index_Expr_NT));
    st.push(SymbolLL1(TokenType::BOOLEAN_OP));
    break;
  case 109:
    st.push(SymbolLL1(109));
    st.push(SymbolLL1(NonTerminal::Index_Expr_NT));
    st.push(SymbolLL1(TokenType::ROUND_OP));
    break;
  case 110:
    st.push(SymbolLL1(110));
    st.push(SymbolLL1(NonTerminal::Index_Expr_NT));
    st.push(SymbolLL1(TokenType::LENGTH_OP));
    break;
  case 111:
    st.push(SymbolLL1(111));
    st.push(SymbolLL1(NonTerminal::Assignable_Expr_NT));
    st.push(SymbolLL1(NonTerminal::Prefix_NT));
    break;
  case 112:
    st.push(SymbolLL1(112));
    st.push(SymbolLL1(NonTerminal::Index_Expr_NT));
    st.push(SymbolLL1(TokenType::NOT_KW));
    break;
  case 113:
    st.push(SymbolLL1(NonTerminal::maybe_Postfix_NT));
    st.push(SymbolLL1(NonTerminal::Index_Expr_NT));
    break;
  case 114:
    st.push(SymbolLL1(TokenType::INCREMENT_OP));
    break;
  case 115:
    st.push(SymbolLL1(TokenType::DECREMENT_OP));
    break;
  case 116:
    break;
  case 117:
    st.push(SymbolLL1(117));
    st.push(SymbolLL1(TokenType::INCREMENT_OP));
    break;
  case 118:
    st.push(SymbolLL1(118));
    st.push(SymbolLL1(TokenType::DECREMENT_OP));
    break;
  case 119:
    st.push(SymbolLL1(NonTerminal::Index_Chain_NT));
    st.push(SymbolLL1(NonTerminal::CallOrVariable_NT));
    break;
  case 120:
    st.push(SymbolLL1(NonTerminal::Primary_Expr_NT));
    break;
  case 121:
    break;
  case 122:
    st.push(SymbolLL1(NonTerminal::Index_Chain_NT));
    st.push(SymbolLL1(122));
    st.push(SymbolLL1(TokenType::RIGHT_SQUARE_PR));
    st.push(SymbolLL1(NonTerminal::Expr_NT));
    st.push(SymbolLL1(TokenType::LEFT_SQUARE_PR));
    break;
  case 123:
    st.push(SymbolLL1(TokenType::RIGHT_PR));
    st.push(SymbolLL1(NonTerminal::Expr_NT));
    st.push(SymbolLL1(TokenType::LEFT_PR));
    break;
  case 124:
    st.push(SymbolLL1(NonTerminal::Literal_NT));
    break;
  case 125:
    st.push(SymbolLL1(125));
    st.push(SymbolLL1(TokenType::INTEGER_NUM));
    break;
  case 126:
    st.push(SymbolLL1(126));
    st.push(SymbolLL1(TokenType::FLOAT_NUM));
    break;
  case 127:
    st.push(SymbolLL1(127));
    st.push(SymbolLL1(TokenType::STRING_SY));
    break;
  case 128:
    st.push(SymbolLL1(128));
    st.push(SymbolLL1(TokenType::TRUE_KW));
    break;
  case 129:
    st.push(SymbolLL1(129));
    st.push(SymbolLL1(TokenType::FALSE_KW));
    break;
  case 130:
    st.push(SymbolLL1(NonTerminal::MaybeCall_NT));
    st.push(SymbolLL1(TokenType::ID_SY));
    break;
  case 131:
    break;
  case 132:
    st.push(SymbolLL1(132));
    st.push(SymbolLL1(NonTerminal::Call_Expr_NT));
    break;
  case 133:
    st.push(SymbolLL1(TokenType::RIGHT_PR));
    st.push(SymbolLL1(NonTerminal::May_be_Arg_NT));
    st.push(SymbolLL1(TokenType::LEFT_PR));
    break;
  case 134:
    break;
  case 135:
    st.push(SymbolLL1(NonTerminal::Call_Expr_Tail_NT));
    st.push(SymbolLL1(NonTerminal::Or_Expr_NT));
    st.push(SymbolLL1(TokenType::COMMA_SY));
    break;
  case 136:
    st.push(SymbolLL1(NonTerminal::Call_Expr_Tail_NT));
    st.push(SymbolLL1(NonTerminal::Or_Expr_NT));
    break;
  case 137:
    break;
  case 138:
    st.push(SymbolLL1(NonTerminal::Command_Seq_NT));
    currentCommandSeq.insert(currentCommandSeq.begin(), nullptr);
    st.push(SymbolLL1(TokenType::ELSE_KW));
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
  AstNode *programNode = nodes.back();
  nodes.pop_back();

  Program *program = dynamic_cast<Program *>(programNode);
  if (!program)
  {
    syntax_error("Invalid program node");
    return;
  }

  int start_line = program->start_line;
  int end_line = program->end_line;
  int node_start = program->node_start;
  int node_end = program->node_end;

  if (!currentFunctionList.empty())
  {
    start_line = currentFunctionList.front()->start_line;
    node_start = currentFunctionList.front()->node_start;
  }

  Source *sourceNode = new Source(program, currentFunctionList, start_line, end_line, node_start, node_end);

  currentFunctionList.clear();

  nodes.push_back(sourceNode);
}

void LL1Parser::build_program()
{
  AstNode *idNode = nodes.back();
  nodes.pop_back();

  Identifier *id = dynamic_cast<Identifier *>(idNode);
  if (!id)
  {
    syntax_error("Invalid identifier node");
    return;
  }

  int start_line = id->start_line;
  int end_line = previous_token.line;
  ;
  int node_start = id->node_start;
  int node_end = previous_token.end;
  ;

  Program *programNode = new Program(id, currentDeclarationSeq, currentCommandSeq, start_line, end_line, node_start, node_end);

  currentDeclarationSeq.clear();
  currentCommandSeq.clear();

  nodes.push_back(programNode);
}

void LL1Parser::build_function()
{
  AstNode *idNode = nodes.back();
  nodes.pop_back();
  AstNode *returnNode = nodes.back();
  nodes.pop_back();

  Identifier *id = dynamic_cast<Identifier *>(idNode);
  if (!id)
  {
    syntax_error("Error building Function node: invalid Identifier");
    return;
  }

  ReturnType *rtn = dynamic_cast<ReturnType *>(returnNode);
  if (!rtn)
  {
    syntax_error("Error building Function node: invalid Return type");
    return;
  }

  auto &declSeq = currentDeclarationSeq;
  auto &commandSeq = currentCommandSeq;

  int start_line = id->start_line;
  int end_line = commandSeq.empty() ? id->end_line : commandSeq.back()->end_line;
  int node_start = id->node_start;
  int node_end = commandSeq.empty() ? id->node_end : commandSeq.back()->node_end;

  Function *funcNode = new Function(id, rtn, declSeq, commandSeq, start_line, end_line, node_start, node_end);

  currentDeclarationSeq.clear();
  currentCommandSeq.clear();

  nodes.push_back(funcNode);
}

void LL1Parser::build_variable_definition()
{
  AstNode *defNode = nodes.back();
  nodes.pop_back();

  Statement *defStmt = dynamic_cast<Statement *>(defNode);
  if (!defStmt)
  {
    syntax_error("Error building VariableDefinition node: invalid child node");
    return;
  }

  VariableDefinition *varDef = new VariableDefinition(defStmt, defStmt->start_line, defStmt->end_line, defStmt->node_start, defStmt->node_end);

  nodes.push_back(varDef);
}

void LL1Parser::build_variable_declaration()
{
  AstNode *typeNode = nodes.back();
  if (dynamic_cast<VariableInitialization *>(typeNode))
  {
    return;
  }
  nodes.pop_back();
  DataType *dt = dynamic_cast<DataType *>(typeNode);

  vector<Identifier *> varList;

  while (!nodes.empty())
  {
    AstNode *node = nodes.back();
    if (node == END_OF_LIST_MARKER)
    {
      nodes.pop_back();
      break;
    }

    Identifier *id = dynamic_cast<Identifier *>(node);
    if (!id)
    {
      syntax_error("Error building VariableDeclaration node: invalid child nodes");
      return;
    }

    varList.push_back(id);
    nodes.pop_back();
  }

  reverse(varList.begin(), varList.end());

  if (varList.empty() || !dt)
  {
    syntax_error("Error building VariableDeclaration node: invalid child nodes");
    return;
  }

  int start_line = previous_token.line;
  int end_line = previous_token.line;
  int node_start = previous_token.start;
  int node_end = previous_token.end;

  VariableDeclaration *varDecl = new VariableDeclaration(varList, dt, start_line, end_line, node_start, node_end);

  nodes.push_back(varDecl);
}

void LL1Parser::build_variable_initialization()
{
  AstNode *initializerNode = nodes.back();
  nodes.pop_back();
  AstNode *datatypeNode = nodes.back();
  nodes.pop_back();
  AstNode *idNode = nodes.back();
  nodes.pop_back();
  nodes.pop_back();

  Identifier *id = dynamic_cast<Identifier *>(idNode);
  DataType *dt = dynamic_cast<DataType *>(datatypeNode);
  Expression *initializer = dynamic_cast<Expression *>(initializerNode);

  if (!id || !dt || !initializer)
  {
    syntax_error("Error building VariableInitialization node: invalid child nodes");
    return;
  }

  VariableInitialization *varInit = new VariableInitialization(id, dt, initializer, id->start_line, initializer->end_line, id->node_start, initializer->node_end);

  nodes.push_back(varInit);
}

void LL1Parser::build_return_type()
{
  AstNode *typeNode = nodes.back();
  nodes.pop_back();

  if (auto prim = dynamic_cast<PrimitiveType *>(typeNode))
  {
    ReturnType *retType = new ReturnType(prim, prim->start_line, prim->end_line, prim->node_start, prim->node_end);
    nodes.push_back(retType);
  }
  else
  {
    syntax_error("Error building ReturnType node: invalid child node");
  }
}

void LL1Parser::build_primitive_type()
{
  AstNode *tokenNode = nodes.back();
  nodes.pop_back();

  PrimitiveType *primType = dynamic_cast<PrimitiveType *>(tokenNode);
  if (!primType)
  {
    syntax_error("Error building PrimitiveType node");
    return;
  }

  nodes.push_back(primType);
}

void LL1Parser::build_array_type()
{
  AstNode *tailNode = nodes.back();
  nodes.pop_back();

  if (auto prim = dynamic_cast<PrimitiveType *>(tailNode))
  {
    ArrayType *arrType = new ArrayType(prim->datatype, 1, prim->start_line, prim->end_line, prim->node_start, prim->node_end);
    delete prim;
    nodes.push_back(arrType);
  }
  else if (auto arr = dynamic_cast<ArrayType *>(tailNode))
  {
    ArrayType *arrType = new ArrayType(arr->datatype, arr->dimension + 1, arr->start_line, arr->end_line, arr->node_start, arr->node_end);
    delete arr;
    nodes.push_back(arrType);
  }
  else
  {
    syntax_error("Error building ArrayType node: invalid child node");
  }
}

void LL1Parser::build_if_statement()
{
  AstNode *conditionNode = nodes.back();
  nodes.pop_back();

  vector<Statement *> *consequent = new vector<Statement *>();
  vector<Statement *> *alternate = new vector<Statement *>();

  bool in_alternate = true;

  while (!currentCommandSeq.empty())
  {
    Statement *stmt = currentCommandSeq.front();
    currentCommandSeq.erase(currentCommandSeq.begin());

    if (stmt == nullptr)
    {
      in_alternate = false;
      continue;
    }

    if (stmt == END_OF_LIST_ELSE)
    {
      Expression *condition = dynamic_cast<Expression *>(conditionNode);

      int start_line = condition->start_line;
      int end_line = alternate->empty() ? consequent->back()->end_line : alternate->back()->end_line;
      int node_start = condition->node_start;
      int node_end = alternate->empty() ? consequent->back()->node_end : alternate->back()->node_end;

      reverse(consequent->begin(), consequent->end());
      reverse(alternate->begin(), alternate->end());

      IfStatement *ifStmt = new IfStatement(condition, *consequent, *alternate, start_line, end_line, node_start, node_end);

      delete consequent;
      delete alternate;

      currentCommandSeq.insert(currentCommandSeq.begin(), ifStmt);
      return;
    }

    if (!stmt)
    {
      syntax_error("Expected statement inside if/else block");
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

  Expression *condition = dynamic_cast<Expression *>(conditionNode);

  if (!condition || !consequent)
  {
    syntax_error("Error building IfStatement node: invalid child nodes");
    return;
  }

  int start_line = condition->start_line;
  int end_line = alternate->empty() ? consequent->back()->end_line : alternate->back()->end_line;
  int node_start = condition->node_start;
  int node_end = alternate->empty() ? consequent->back()->node_end : alternate->back()->node_end;

  reverse(consequent->begin(), consequent->end());
  reverse(alternate->begin(), alternate->end());

  IfStatement *ifStmt = new IfStatement(condition, *consequent, *alternate, start_line, end_line, node_start, node_end);

  delete consequent;
  delete alternate;

  nodes.push_back(ifStmt);
}

void LL1Parser::build_return_statement()
{
  AstNode *returnValueNode = nodes.back();
  nodes.pop_back();

  Expression *returnValue = dynamic_cast<Expression *>(returnValueNode);

  int start_line = previous_token.line;
  int end_line = previous_token.line;
  int node_start = previous_token.start;
  int node_end = previous_token.end;

  if (returnValue)
  {
    end_line = returnValue->end_line;
    node_end = returnValue->node_end;
  }

  ReturnStatement *retStmt = new ReturnStatement(returnValue, start_line, end_line, node_start, node_end);

  nodes.push_back(retStmt);
}

void LL1Parser::build_skip_statement()
{
  int start_line = previous_token.line;
  int end_line = previous_token.line;
  int node_start = previous_token.start;
  int node_end = previous_token.end;

  SkipStatement *skipStmt = new SkipStatement(start_line, end_line, node_start, node_end);

  nodes.push_back(skipStmt);
}

void LL1Parser::build_stop_statement()
{
  int start_line = previous_token.line;
  int end_line = previous_token.line;
  int node_start = previous_token.start;
  int node_end = previous_token.end;

  StopStatement *stopStmt = new StopStatement(start_line, end_line, node_start, node_end);

  nodes.push_back(stopStmt);
}

void LL1Parser::build_read_statement()
{
  vector<AssignableExpression *> *variables = new vector<AssignableExpression *>();

  while (!nodes.empty())
  {
    AstNode *node = nodes.back();
    nodes.pop_back();

    if (node == END_OF_LIST_MARKER)
      break;

    AssignableExpression *var = dynamic_cast<AssignableExpression *>(node);
    if (!var)
    {
      syntax_error("Expected Assignable in Read statement");
      delete variables;
      return;
    }

    variables->insert(variables->begin(), var);
  }

  int start_line = previous_token.line;
  int end_line = previous_token.line;
  int node_start = previous_token.start;
  int node_end = previous_token.end;

  ReadStatement *readStmt = new ReadStatement(*variables, start_line, end_line, node_start, node_end);

  delete variables;

  nodes.push_back(readStmt);
}

void LL1Parser::build_write_statement()
{
  vector<Expression *> *exprList = new vector<Expression *>();
  while (!nodes.empty())
  {
    AstNode *node = nodes.back();
    nodes.pop_back();

    if (node == END_OF_LIST_MARKER)
      break;

    Expression *expr = dynamic_cast<Expression *>(node);
    if (!expr)
    {
      syntax_error("Expected expression in write statement");
      delete exprList;
      return;
    }

    exprList->insert(exprList->begin(), expr);
  }

  int start_line = previous_token.line;
  int end_line = previous_token.line;
  int node_start = previous_token.start;
  int node_end = previous_token.end;

  if (exprList->empty())
  {
    syntax_error("Expected expression in write statement");
    delete exprList;
    return;
  }

  WriteStatement *writeStmt = new WriteStatement(*exprList, start_line, end_line, node_start, node_end);

  delete exprList;

  nodes.push_back(writeStmt);
}

void LL1Parser::build_for_loop()
{
  vector<Statement *> *body = new vector<Statement *>();
  while (!currentCommandSeq.empty())
  {
    Statement *stmt = currentCommandSeq.back();
    currentCommandSeq.pop_back();

    if (!stmt)
    {
      syntax_error("Expected statement inside while block");
      delete body;
      return;
    }
    body->insert(body->begin(), stmt);
  }

  AstNode *updateExprNode = nodes.back();
  nodes.pop_back();
  AstNode *conditionNode = nodes.back();
  nodes.pop_back();
  AstNode *initAssignNode = nodes.back();
  nodes.pop_back();

  AssignmentExpression *init = dynamic_cast<AssignmentExpression *>(initAssignNode);
  Expression *condition = dynamic_cast<Expression *>(conditionNode);
  Expression *update = dynamic_cast<Expression *>(updateExprNode);

  if (!init || !condition || !update)
  {
    syntax_error("Error building ForLoop node: invalid child nodes");
    return;
  }

  int start_line = init->start_line;
  int end_line = body->empty() ? init->end_line : body->back()->end_line;
  int node_start = init->node_start;
  int node_end = body->empty() ? init->node_end : body->back()->node_end;

  ForLoop *forLoop = new ForLoop(init, condition, update, *body, start_line, end_line, node_start, node_end);

  delete body;

  nodes.push_back(forLoop);
}

void LL1Parser::build_int_assign()
{
  AstNode *valueNode = nodes.back();
  nodes.pop_back();
  AstNode *idNode = nodes.back();
  nodes.pop_back();

  Identifier *id = dynamic_cast<Identifier *>(idNode);
  Expression *value = dynamic_cast<Expression *>(valueNode);

  if (!id || !value)
  {
    syntax_error("Error building int Assignment Expression node: invalid child nodes");
    return;
  }

  int start_line = id->start_line;
  int end_line = value->end_line;
  int node_start = id->node_start;
  int node_end = value->node_end;

  AssignmentExpression *assignExpr = new AssignmentExpression(id, value, start_line, end_line, node_start, node_end);

  nodes.push_back(assignExpr);
}

void LL1Parser::build_while_loop()
{
  vector<Statement *> *body = new vector<Statement *>();
  while (!currentCommandSeq.empty())
  {
    Statement *stmt = currentCommandSeq.back();
    currentCommandSeq.pop_back();

    if (!stmt)
    {
      syntax_error("Expected statement inside while block");
      delete body;
      return;
    }
    body->insert(body->begin(), stmt);
  }

  AstNode *conditionNode = nodes.back();
  nodes.pop_back();

  Expression *condition = dynamic_cast<Expression *>(conditionNode);

  if (!condition)
  {
    syntax_error("Error building WhileLoop node: invalid child nodes");
    return;
  }

  int start_line = condition->start_line;
  int end_line = body->empty() ? condition->end_line : body->back()->end_line;
  int node_start = condition->node_start;
  int node_end = body->empty() ? condition->node_end : body->back()->node_end;

  WhileLoop *whileLoop = new WhileLoop(condition, *body, start_line, end_line, node_start, node_end);

  delete body;

  nodes.push_back(whileLoop);
}

void LL1Parser::build_assignment_expression()
{
  AstNode *valueNode = nodes.back();
  nodes.pop_back();
  AstNode *assigneeNode = nodes.back();
  nodes.pop_back();

  AssignableExpression *assignee = dynamic_cast<AssignableExpression *>(assigneeNode);
  Expression *value = dynamic_cast<Expression *>(valueNode);

  if (!assignee || !value)
  {
    syntax_error("Error building AssignmentExpression node: invalid child nodes");
    return;
  }

  int start_line = assignee->start_line;
  int end_line = value->end_line;
  int node_start = assignee->node_start;
  int node_end = value->node_end;

  AssignmentExpression *assignExpr = new AssignmentExpression(assignee, value, start_line, end_line, node_start, node_end);

  nodes.push_back(assignExpr);
}

void LL1Parser::build_or_expression()
{
  AstNode *orTailNode = nodes.back();
  nodes.pop_back();
  AstNode *andExprNode = nodes.back();
  nodes.pop_back();

  Expression *left = dynamic_cast<Expression *>(andExprNode);
  Expression *right = dynamic_cast<Expression *>(orTailNode);

  if (!left || !right)
  {
    syntax_error("Error building OrExpression node: invalid left or Right child");
    return;
  }

  int start_line = left->start_line;
  int end_line = right->end_line;
  int node_start = left->node_start;
  int node_end = right->node_end;

  OrExpression *orExpr = new OrExpression(left, right, start_line, end_line, node_start, node_end);

  nodes.push_back(orExpr);
}

void LL1Parser::build_and_expression()
{
  AstNode *andTailNode = nodes.back();
  nodes.pop_back();
  AstNode *equalityExprNode = nodes.back();
  nodes.pop_back();

  Expression *left = dynamic_cast<Expression *>(equalityExprNode);
  Expression *right = dynamic_cast<Expression *>(andTailNode);

  if (!left || !right)
  {
    syntax_error("Error building AndExpression node: invalid left or right child");
    return;
  }

  int start_line = left->start_line;
  int end_line = right->end_line;
  int node_start = left->node_start;
  int node_end = right->node_end;

  AndExpression *andExpr = new AndExpression(left, right, start_line, end_line, node_start, node_end);

  nodes.push_back(andExpr);
}

void LL1Parser::build_equality_expression()
{
  AstNode *equalityTailNode = nodes.back();
  nodes.pop_back();
  AstNode *optNode = nodes.back();
  nodes.pop_back();
  AstNode *relationalExprNode = nodes.back();
  nodes.pop_back();

  Expression *left = dynamic_cast<Expression *>(relationalExprNode);
  Expression *right = dynamic_cast<Expression *>(equalityTailNode);
  Identifier *opt = dynamic_cast<Identifier *>(optNode);
  string op = opt->name;

  if (!left || !right || !opt)
  {
    syntax_error("Error building EqualityExpression node: invalid left or right or opreator child");
    return;
  }

  int start_line = left->start_line;
  int end_line = right->end_line;
  int node_start = left->node_start;
  int node_end = right->node_end;

  EqualityExpression *eqExpr = new EqualityExpression(left, right, op, start_line, end_line, node_start, node_end);

  nodes.push_back(eqExpr);
}

void LL1Parser::build_relational_expression()
{
  AstNode *relationalTailNode = nodes.back();
  nodes.pop_back();
  AstNode *optNode = nodes.back();
  nodes.pop_back();
  AstNode *additiveExprNode = nodes.back();
  nodes.pop_back();

  Expression *left = dynamic_cast<Expression *>(additiveExprNode);
  Expression *right = dynamic_cast<Expression *>(relationalTailNode);
  Identifier *opt = dynamic_cast<Identifier *>(optNode);
  string op = opt->name;

  if (!left || !right || !opt)
  {
    syntax_error("Error building RelationalExpression node: invalid left or right or opreator child");
    return;
  }

  int start_line = left->start_line;
  int end_line = right->end_line;
  int node_start = left->node_start;
  int node_end = right->node_end;

  RelationalExpression *relExpr = new RelationalExpression(left, right, op, start_line, end_line, node_start, node_end);

  nodes.push_back(relExpr);
}

void LL1Parser::build_additive_expression()
{
  AstNode *additiveTailNode = nodes.back();
  nodes.pop_back();
  AstNode *optNode = nodes.back();
  nodes.pop_back();
  AstNode *multiplicativeExprNode = nodes.back();
  nodes.pop_back();

  Expression *left = dynamic_cast<Expression *>(multiplicativeExprNode);
  Expression *right = dynamic_cast<Expression *>(additiveTailNode);
  Identifier *opt = dynamic_cast<Identifier *>(optNode);
  string op = opt->name;

  if (!left || !right || !opt)
  {
    syntax_error("Error building AdditiveExpression node: invalid left or right or opreator child");
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

  AdditiveExpression *addExpr = new AdditiveExpression(left, right, op, start_line, end_line, node_start, node_end);

  nodes.push_back(addExpr);
}

void LL1Parser::build_multiplicative_expression()
{
  AstNode *multiplicativeTailNode = nodes.back();
  nodes.pop_back();
  AstNode *optNode = nodes.back();
  nodes.pop_back();
  AstNode *unaryExprNode = nodes.back();
  nodes.pop_back();

  Expression *left = dynamic_cast<Expression *>(unaryExprNode);
  Expression *right = dynamic_cast<Expression *>(multiplicativeTailNode);
  Identifier *opt = dynamic_cast<Identifier *>(optNode);
  string op = opt->name;

  if (!left || !right || !opt)
  {
    syntax_error("Error building MultiplicativeExpression node: invalid left or right or opreator child");
    return;
  }

  int start_line = left->start_line;
  int end_line = right->end_line;
  int node_start = left->node_start;
  int node_end = right->node_end;

  MultiplicativeExpression *multExpr = new MultiplicativeExpression(left, right, op, start_line, end_line, node_start, node_end);

  nodes.push_back(multExpr);
}

void LL1Parser::build_unary_expression()
{
  Expression *operand;
  Identifier *opt;
  string op;
  bool postfix = false;

  AstNode *firstNode = nodes.back();
  nodes.pop_back();
  AstNode *secondNode;

  if (dynamic_cast<Identifier *>(firstNode))
  {
    opt = dynamic_cast<Identifier *>(firstNode);
    op = opt->name;
    if (op == "++" || op == "--")
    {
      postfix = true;
      secondNode = nodes.back();
      nodes.pop_back();
      operand = dynamic_cast<Expression *>(secondNode);
      if (!dynamic_cast<AssignableExpression *>(secondNode))
      {
        syntax_error("Error building UnaryExpression: invalid operand");
        return;
      }
      if (!operand)
      {
        syntax_error("Error building UnaryExpression: invalid operand");
        return;
      }
      int start_line = operand->start_line;
      int end_line = operand->end_line;
      int node_start = operand->node_start;
      int node_end = operand->node_end;
      UnaryExpression *unaryExpr = new UnaryExpression(operand, op, postfix, start_line, end_line, node_start, node_end);

      nodes.push_back(unaryExpr);
      return;
    }
  }
  operand = dynamic_cast<Expression *>(firstNode);
  secondNode = nodes.back();
  nodes.pop_back();
  opt = dynamic_cast<Identifier *>(secondNode);
  op = opt->name;
  if (!dynamic_cast<AssignableExpression *>(firstNode) && (op == "++" || op == "--"))
  {
    syntax_error("Error building UnaryExpression: invalid operand");
    return;
  }
  if (!operand)
  {
    syntax_error("Error building UnaryExpression: invalid operand");
    return;
  }

  int start_line = operand->start_line;
  int end_line = operand->end_line;
  int node_start = operand->node_start;
  int node_end = operand->node_end;

  UnaryExpression *unaryExpr = new UnaryExpression(operand, op, postfix, start_line, end_line, node_start, node_end);

  nodes.push_back(unaryExpr);
}

void LL1Parser::build_call_function_expression()
{
  vector<Expression *> *exprList = new vector<Expression *>();
  while (!nodes.empty())
  {
    AstNode *node = nodes.back();

    if (node == dynamic_cast<Identifier *>(node))
      break;
    nodes.pop_back();

    Expression *expr = dynamic_cast<Expression *>(node);
    if (!expr)
    {
      syntax_error("Expected expression in write statement");
      delete exprList;
      return;
    }

    exprList->insert(exprList->begin(), expr);
  }

  AstNode *idNode = nodes.back();
  nodes.pop_back();

  Identifier *id = dynamic_cast<Identifier *>(idNode);

  if (!id)
  {
    syntax_error("Error building CallFunctionExpression: invalid Identifier");
    return;
  }

  int start_line = id->start_line;
  int end_line = exprList->empty() ? id->end_line : exprList->back()->end_line;
  int node_start = id->node_start;
  int node_end = exprList->empty() ? id->node_end : exprList->back()->node_end;

  CallFunctionExpression *callFuncExpr = new CallFunctionExpression(id, *exprList, start_line, end_line, node_start, node_end);

  nodes.push_back(callFuncExpr);
}

void LL1Parser::build_index_expression()
{
  AstNode *indexChainNode = nodes.back();
  nodes.pop_back();

  AstNode *baseNode = nodes.back();
  nodes.pop_back();

  Expression *base = dynamic_cast<Expression *>(baseNode);
  Expression *indexChain = dynamic_cast<Expression *>(indexChainNode);

  if (!base || !indexChain)
  {
    syntax_error("Error building IndexExpression: invalid child nodes");
    return;
  }

  int start_line = base->start_line;
  int end_line = indexChain->end_line;
  int node_start = base->node_start;
  int node_end = indexChain->node_end;

  IndexExpression *current = new IndexExpression(base, indexChain, start_line, end_line, node_start, node_end);

  nodes.push_back(current);
}

void LL1Parser::build_identifier()
{
  AstNode *idNode = nodes.back();
  nodes.pop_back();

  Identifier *id = dynamic_cast<Identifier *>(idNode);
  if (!id)
  {
    syntax_error("Error building Identifier node");
    return;
  }

  nodes.push_back(id);
}

void LL1Parser::build_integer_literal()
{
  AstNode *intNode = nodes.back();
  nodes.pop_back();

  IntegerLiteral *intLit = dynamic_cast<IntegerLiteral *>(intNode);
  if (!intLit)
  {
    syntax_error("Error building IntegerLiteral node");
    return;
  }

  nodes.push_back(intLit);
}

void LL1Parser::build_float_literal()
{
  AstNode *floatNode = nodes.back();
  nodes.pop_back();

  FloatLiteral *floatLit = dynamic_cast<FloatLiteral *>(floatNode);
  if (!floatLit)
  {
    syntax_error("Error building FloatLiteral node");
    return;
  }

  nodes.push_back(floatLit);
}

void LL1Parser::build_string_literal()
{
  AstNode *strNode = nodes.back();
  nodes.pop_back();

  StringLiteral *strLit = dynamic_cast<StringLiteral *>(strNode);
  if (!strLit)
  {
    syntax_error("Error building StringLiteral node");
    return;
  }

  nodes.push_back(strLit);
}

void LL1Parser::build_boolean_literal()
{
  AstNode *boolNode = nodes.back();
  nodes.pop_back();

  BooleanLiteral *boolLit = dynamic_cast<BooleanLiteral *>(boolNode);
  if (!boolLit)
  {
    syntax_error("Error building BooleanLiteral node");
    return;
  }

  nodes.push_back(boolLit);
}

void LL1Parser::build_array_literal()
{
  AstNode *arrayNode = nodes.back();
  nodes.pop_back();

  ArrayLiteral *arrLit = dynamic_cast<ArrayLiteral *>(arrayNode);
  if (!arrLit)
  {
    syntax_error("Error building ArrayLiteral node");
    return;
  }

  nodes.push_back(arrLit);
}

void LL1Parser::build_function_list()
{
  AstNode *funcNode = nodes.back();
  nodes.pop_back();

  Function *func = dynamic_cast<Function *>(funcNode);
  if (!func)
  {
    syntax_error("Invalid function node");
    return;
  }

  currentFunctionList.insert(currentFunctionList.begin(), func);
}

void LL1Parser::build_declaration_seq()
{
  AstNode *declNode = nodes.back();
  nodes.pop_back();

  VariableDefinition *decl = dynamic_cast<VariableDefinition *>(declNode);
  if (!decl)
  {
    syntax_error("Invalid declaration node");
    return;
  }

  currentDeclarationSeq.insert(currentDeclarationSeq.begin(), decl);
}

void LL1Parser::build_command_seq()
{
  AstNode *cmdNode = nodes.back();
  nodes.pop_back();

  Statement *cmd = dynamic_cast<Statement *>(cmdNode);
  if (!cmd)
  {
    syntax_error("Invalid command node");
    return;
  }

  currentCommandSeq.insert(currentCommandSeq.begin(), cmd);
}
