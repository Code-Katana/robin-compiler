#include "ll1_parser.h"

#include <cstring>

LL1Parser::LL1Parser(ScannerBase *sc) : ParserBase(sc)
{
  fill_table();
  get_token();
}

AstNode *LL1Parser::parse_ast()
{
  st.push(Symbol(TokenType::END_OF_FILE));
  st.push(Symbol(NonTerminal::Source_NT));

  while (!st.empty())
  {
    Symbol top = st.top();
    if (top.reduceRule > 0)
    {
      st.pop();
      builder(top.reduceRule);
    }
    else if (!top.isTerm)
    {
      int rule = parseTable[(int)top.nt][(int)current_token.type];
      // if(rule == 73 && current_token.type == TokenType::ID_SY)
      // {
      //   get_token();
      //   if(current_token.type == TokenType::EQUAL_OP)
      //   {
      //     rule = 72;
      //   }
      //   else
      //   {

      //   }
      // }
      if (rule == 0)
      {
        syntax_error("Unexpected token in prediction");
      }
      st.pop();
      push_rule(rule);
    }
    else
    {
      st.pop();
      match(top.term);
    }
  }
  return dynamic_cast<AstNode *>(nodes.back());
}

void LL1Parser::get_token()
{
  current_token = sc->get_token();
}

void LL1Parser::match(TokenType t)
{
  if (current_token.type != t)
  {
    syntax_error("Token mismatch in match");
  }

  AstNode *leaf = nullptr;
  switch (t)
  {
  case TokenType::ID_SY:
    leaf = new Identifier(current_token.value,
                          current_token.line, current_token.line,
                          current_token.start, current_token.end);
    break;
  case TokenType::INTEGER_NUM:
    leaf = new IntegerLiteral(std::stoi(current_token.value),
                              current_token.line, current_token.line,
                              current_token.start, current_token.end);
    break;
  case TokenType::FLOAT_NUM:
    leaf = new FloatLiteral(std::stof(current_token.value),
                            current_token.line, current_token.line,
                            current_token.start, current_token.end);
    break;
  case TokenType::STRING_SY:
    leaf = new StringLiteral(current_token.value,
                             current_token.line, current_token.line,
                             current_token.start, current_token.end);
    break;
  case TokenType::TRUE_KW:
    leaf = new BooleanLiteral(true,
                              current_token.line, current_token.line,
                              current_token.start, current_token.end);
    break;
  case TokenType::FALSE_KW:
    leaf = new BooleanLiteral(false,
                              current_token.line, current_token.line,
                              current_token.start, current_token.end);
    break;
  }
  if (leaf)
  {
    nodes.push_back(leaf);
  }

  get_token();
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

  parseTable[(int)NonTerminal::Variable_List_NT][(int)TokenType::ID_SY] = 53;

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

  parseTable[(int)NonTerminal::While_Loop_NT][(int)TokenType::WHILE_KW] = 62;

  parseTable[(int)NonTerminal::IF_Statement_NT][(int)TokenType::IF_KW] = 63;

  parseTable[(int)NonTerminal::Else_Part_NT][(int)TokenType::END_KW] = 65;
  parseTable[(int)NonTerminal::Else_Part_NT][(int)TokenType::ELSE_KW] = 66;

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

  // parseTable[(int)NonTerminal::Assign_Expr_NT][(int)TokenType::ID_SY] = 72;
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
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::OR_KW] = 131;
  parseTable[(int)NonTerminal::MaybeCall_NT][(int)TokenType::AND_KW] = 131;
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
}

void LL1Parser::push_rule(int rule)
{
  switch (rule)
  {
  case 1:
    st.push(Symbol(1));
    st.push(Symbol(TokenType::END_OF_FILE));
    st.push(Symbol(NonTerminal::Program_NT));
    st.push(Symbol(NonTerminal::Function_List_NT));
    break;
  case 2:
    st.push(Symbol(2));
    break;
  case 3:
    st.push(Symbol(3));
    st.push(Symbol(NonTerminal::Function_List_NT));
    st.push(Symbol(NonTerminal::Function_NT));
    break;
  case 4:
    st.push(Symbol(4));
    st.push(Symbol(TokenType::FUNC_KW));
    st.push(Symbol(TokenType::END_KW));
    st.push(Symbol(NonTerminal::Command_Seq_NT));
    st.push(Symbol(TokenType::BEGIN_KW));
    st.push(Symbol(NonTerminal::Declaration_Seq_NT));
    st.push(Symbol(TokenType::HAS_KW));
    st.push(Symbol(TokenType::ID_SY));
    st.push(Symbol(NonTerminal::Function_Type_NT));
    st.push(Symbol(TokenType::FUNC_KW));
    break;
  case 5:
    st.push(Symbol(5));
    st.push(Symbol(NonTerminal::Type_NT));
    break;
  case 6:
    st.push(Symbol(6));
    st.push(Symbol(TokenType::VOID_TY));
    break;
  case 7:
    st.push(Symbol(7));
    st.push(Symbol(NonTerminal::Array_Type_NT));

    break;
  case 8:
    st.push(Symbol(8));
    st.push(Symbol(NonTerminal::Primitive_NT));
    break;
  case 9:
    st.push(Symbol(9));
    st.push(Symbol(NonTerminal::Array_Type_Tail_NT));
    st.push(Symbol(TokenType::LEFT_SQUARE_PR));
    break;
  case 10:
    st.push(Symbol(10));
    st.push(Symbol(TokenType::RIGHT_SQUARE_PR));
    st.push(Symbol(NonTerminal::Primitive_NT));
    break;
  case 11:
    st.push(Symbol(11));
    st.push(Symbol(TokenType::RIGHT_SQUARE_PR));
    st.push(Symbol(NonTerminal::Array_Type_NT));
    break;
  case 12:
    st.push(Symbol(12));
    st.push(Symbol(TokenType::INTEGER_TY));
    break;
  case 13:
    st.push(Symbol(13));
    st.push(Symbol(TokenType::BOOLEAN_TY));
    break;
  case 14:
    st.push(Symbol(14));
    st.push(Symbol(TokenType::STRING_TY));
    break;
  case 15:
    st.push(Symbol(15));
    st.push(Symbol(TokenType::FLOAT_TY));
    break;
  case 16:
    st.push(Symbol(16));
    st.push(Symbol(NonTerminal::Block_NT));
    st.push(Symbol(TokenType::IS_KW));
    st.push(Symbol(TokenType::ID_SY));
    st.push(Symbol(TokenType::PROGRAM_KW));
    break;
  case 17:
    st.push(Symbol(17));
    st.push(Symbol(TokenType::END_KW));
    st.push(Symbol(NonTerminal::Command_Seq_NT));
    st.push(Symbol(TokenType::BEGIN_KW));
    st.push(Symbol(NonTerminal::Declaration_Seq_NT));
    break;
  case 18:
    st.push(Symbol(18));
    break;
  case 19:
    st.push(Symbol(19));
    st.push(Symbol(NonTerminal::Declaration_Seq_NT));
    st.push(Symbol(NonTerminal::Declaration_NT));
    break;
  case 20:
    st.push(Symbol(20));
    st.push(Symbol(NonTerminal::Decl_Tail_NT));
    st.push(Symbol(TokenType::VAR_KW));
    break;
  case 21:
    st.push(Symbol(21));
    st.push(Symbol(NonTerminal::Decl_Tail_Right_NT));
    st.push(Symbol(NonTerminal::Type_NT));
    st.push(Symbol(TokenType::COLON_SY));
    st.push(Symbol(NonTerminal::Variable_List_NT));
    break;
  case 22:
    st.push(Symbol(22));
    st.push(Symbol(TokenType::SEMI_COLON_SY));
    break;
  case 23:
    st.push(Symbol(23));
    st.push(Symbol(TokenType::SEMI_COLON_SY));
    st.push(Symbol(NonTerminal::Decl_Init_NT));
    st.push(Symbol(TokenType::EQUAL_OP));
    break;
  case 24:
    st.push(Symbol(24));
    st.push(Symbol(NonTerminal::Or_Expr_NT));
    break;
  case 25:
    st.push(Symbol(25));
    st.push(Symbol(TokenType::RIGHT_CURLY_PR));
    st.push(Symbol(NonTerminal::Array_Value_NT));
    st.push(Symbol(TokenType::LEFT_CURLY_PR));
    break;
  case 26:
    st.push(Symbol(26));
    break;
  case 27:
    st.push(Symbol(27));
    st.push(Symbol(NonTerminal::Value_List_NT));
    break;
  case 28:
    st.push(Symbol(28));
    st.push(Symbol(NonTerminal::Array_Nested_Value_NT));
    break;
  case 29:
    st.push(Symbol(29));
    st.push(Symbol(NonTerminal::More_Nested_NT));
    st.push(Symbol(TokenType::RIGHT_CURLY_PR));
    st.push(Symbol(NonTerminal::Value_List_NT));
    st.push(Symbol(TokenType::LEFT_CURLY_PR));
    break;
  case 30:
    st.push(Symbol(30));
    break;
  case 31:
    st.push(Symbol(31));
    st.push(Symbol(NonTerminal::Array_Nested_Value_NT));
    st.push(Symbol(TokenType::COMMA_SY));
    break;
  case 32:
    st.push(Symbol(32));
    st.push(Symbol(NonTerminal::More_Values_NT));
    st.push(Symbol(NonTerminal::Or_Expr_NT));
    break;
  case 33:
    st.push(Symbol(33));
    break;
  case 34:
    st.push(Symbol(34));
    st.push(Symbol(NonTerminal::Value_List_NT));
    st.push(Symbol(TokenType::COMMA_SY));
    break;
  case 35:
    st.push(Symbol(35));
    st.push(Symbol(NonTerminal::More_Variables_NT));
    st.push(Symbol(TokenType::ID_SY));

    break;
  case 36:
    st.push(Symbol(36));
    break;
  case 37:
    st.push(Symbol(37));
    st.push(Symbol(NonTerminal::Variable_List_NT));
    st.push(Symbol(TokenType::COMMA_SY));
    break;
  case 38:
    st.push(Symbol(38));
    st.push(Symbol(NonTerminal::Command_Seq_Tail_NT));
    st.push(Symbol(NonTerminal::Command_NT));
    break;
  case 39:
    st.push(Symbol(39));
    break;
  case 40:
    st.push(Symbol(40));
    st.push(Symbol(NonTerminal::Command_Seq_Tail_NT));
    st.push(Symbol(NonTerminal::Command_NT));
    break;
  case 41:
    st.push(Symbol(41));
    break;
  case 42:
    st.push(Symbol(42));
    st.push(Symbol(TokenType::SEMI_COLON_SY));
    st.push(Symbol(TokenType::SKIP_KW));
    break;
  case 43:
    st.push(Symbol(43));
    st.push(Symbol(TokenType::SEMI_COLON_SY));
    st.push(Symbol(TokenType::STOP_KW));
    break;
  case 44:
    st.push(Symbol(44));
    st.push(Symbol(NonTerminal::Read_Statement_NT));
    break;
  case 45:
    st.push(Symbol(45));
    st.push(Symbol(NonTerminal::Write_Statement_NT));
    break;
  case 46:
    st.push(Symbol(46));
    st.push(Symbol(NonTerminal::Loops_NT));
    break;
  case 47:
    st.push(Symbol(47));
    st.push(Symbol(NonTerminal::IF_Statement_NT));
    break;
  case 48:
    st.push(Symbol(48));
    st.push(Symbol(NonTerminal::Return_NT));
    break;
  case 49:
    st.push(Symbol(49));
    st.push(Symbol(NonTerminal::Expr_Statement_NT));
    break;
  case 50:
    st.push(Symbol(50));
    st.push(Symbol(NonTerminal::Declaration_NT));
    break;
  case 51:
    st.push(Symbol(51));
    st.push(Symbol(TokenType::SEMI_COLON_SY));
    st.push(Symbol(NonTerminal::Variable_List_read_NT));
    st.push(Symbol(TokenType::READ_KW));
    break;
  case 52:
    st.push(Symbol(52));
    st.push(Symbol(TokenType::SEMI_COLON_SY));
    st.push(Symbol(NonTerminal::Expr_List_NT));
    st.push(Symbol(TokenType::WRITE_KW));
    break;
  case 53:
    st.push(Symbol(53));
    st.push(Symbol(NonTerminal::More_Variables_read_NT));
    st.push(Symbol(NonTerminal::Index_Chain_NT));
    st.push(Symbol(TokenType::ID_SY));
    break;
  case 54:
    st.push(Symbol(54));
    break;
  case 55:
    st.push(Symbol(55));
    st.push(Symbol(NonTerminal::Variable_List_read_NT));
    st.push(Symbol(TokenType::COMMA_SY));
    break;
  case 56:
    st.push(Symbol(56));
    st.push(Symbol(NonTerminal::More_Exprs_NT));
    st.push(Symbol(NonTerminal::Expr_NT));
    break;
  case 57:
    st.push(Symbol(57));
    break;
  case 58:
    st.push(Symbol(58));
    st.push(Symbol(NonTerminal::Expr_List_NT));
    st.push(Symbol(TokenType::COMMA_SY));
    break;
  case 59:
    st.push(Symbol(59));
    st.push(Symbol(NonTerminal::For_Loop_NT));
    break;
  case 60:
    st.push(Symbol(60));
    st.push(Symbol(NonTerminal::While_Loop_NT));
    break;
  case 61:
    st.push(Symbol(61));
    st.push(Symbol(TokenType::FOR_KW));
    st.push(Symbol(TokenType::END_KW));
    st.push(Symbol(NonTerminal::Command_Seq_NT));
    st.push(Symbol(TokenType::DO_KW));
    st.push(Symbol(NonTerminal::Expr_NT));
    st.push(Symbol(TokenType::SEMI_COLON_SY));
    st.push(Symbol(NonTerminal::Or_Expr_NT));
    st.push(Symbol(TokenType::SEMI_COLON_SY));
    st.push(Symbol(NonTerminal::Integer_Assign_NT));
    st.push(Symbol(TokenType::FOR_KW));
    break;
  case 62:
    st.push(Symbol(62));
    st.push(Symbol(NonTerminal::Expr_NT));
    st.push(Symbol(TokenType::EQUAL_OP));
    st.push(Symbol(TokenType::ID_SY));
    break;
  case 63:
    st.push(Symbol(63));
    st.push(Symbol(TokenType::WHILE_KW));
    st.push(Symbol(TokenType::END_KW));
    st.push(Symbol(NonTerminal::Command_Seq_NT));
    st.push(Symbol(TokenType::DO_KW));
    st.push(Symbol(NonTerminal::Or_Expr_NT));
    st.push(Symbol(TokenType::WHILE_KW));
    break;
  case 64:
    st.push(Symbol(64));
    st.push(Symbol(TokenType::IF_KW));
    st.push(Symbol(TokenType::END_KW));
    st.push(Symbol(NonTerminal::Else_Part_NT));
    st.push(Symbol(NonTerminal::Command_Seq_NT));
    st.push(Symbol(TokenType::THEN_KW));
    st.push(Symbol(NonTerminal::Expr_NT));
    st.push(Symbol(TokenType::IF_KW));
    break;
  case 65:
    st.push(Symbol(65));
    break;
  case 66:
    st.push(Symbol(66));
    st.push(Symbol(NonTerminal::Command_Seq_NT));
    st.push(Symbol(TokenType::ELSE_KW));
    break;
  case 67:
    st.push(Symbol(67));
    st.push(Symbol(TokenType::SEMI_COLON_SY));
    st.push(Symbol(NonTerminal::Return_Value_NT));
    st.push(Symbol(TokenType::RETURN_KW));
    break;
  case 68:
    st.push(Symbol(68));
    break;
  case 69:
    st.push(Symbol(69));
    st.push(Symbol(NonTerminal::Or_Expr_NT));
    break;
  case 70:
    st.push(Symbol(70));
    st.push(Symbol(TokenType::SEMI_COLON_SY));
    st.push(Symbol(NonTerminal::Expr_NT));
    break;
  case 71:
    st.push(Symbol(71));
    st.push(Symbol(NonTerminal::Assign_Expr_NT));
    break;
  case 72:
    st.push(Symbol(72));
    st.push(Symbol(NonTerminal::Assign_Expr_Value_NT));
    st.push(Symbol(TokenType::EQUAL_OP));
    st.push(Symbol(NonTerminal::Assignable_Expr_NT));
    break;
  case 73:
    st.push(Symbol(73));
    st.push(Symbol(NonTerminal::Or_Expr_NT));
    break;
  case 74:
    st.push(Symbol(74));
    st.push(Symbol(NonTerminal::Expr_NT));
    break;
  case 75:
    st.push(Symbol(75));
    st.push(Symbol(TokenType::RIGHT_CURLY_PR));
    st.push(Symbol(NonTerminal::Array_Value_NT));
    st.push(Symbol(TokenType::LEFT_CURLY_PR));
    break;
  case 76:
    st.push(Symbol(76));
    st.push(Symbol(NonTerminal::Index_Chain_NT));
    st.push(Symbol(TokenType::ID_SY));
    break;
  case 77:
    st.push(Symbol(77));
    st.push(Symbol(NonTerminal::Or_Tail_NT));
    st.push(Symbol(NonTerminal::And_Expr_NT));
    break;
  case 78:
    st.push(Symbol(78));
    break;
  case 79:
    st.push(Symbol(79));
    st.push(Symbol(NonTerminal::Or_Tail_NT));
    st.push(Symbol(NonTerminal::And_Expr_NT));
    st.push(Symbol(TokenType::OR_KW));
    break;
  case 80:
    st.push(Symbol(80));
    st.push(Symbol(NonTerminal::And_Tail_NT));
    st.push(Symbol(NonTerminal::Equality_Expr_NT));
    break;
  case 81:
    st.push(Symbol(81));
    break;
  case 82:
    st.push(Symbol(82));
    st.push(Symbol(NonTerminal::And_Tail_NT));
    st.push(Symbol(NonTerminal::Equality_Expr_NT));
    st.push(Symbol(TokenType::AND_KW));
    break;
  case 83:
    st.push(Symbol(83));
    st.push(Symbol(NonTerminal::Equality_Tail_NT));
    st.push(Symbol(NonTerminal::Relational_Expr_NT));
    break;
  case 84:
    st.push(Symbol(84));
    break;
  case 85:
    st.push(Symbol(85));
    st.push(Symbol(NonTerminal::Equality_Tail_NT));
    st.push(Symbol(NonTerminal::Relational_Expr_NT));
    st.push(Symbol(NonTerminal::OP_Equal_NT));
    break;
  case 86:
    st.push(Symbol(86));
    st.push(Symbol(TokenType::IS_EQUAL_OP));
    break;
  case 87:
    st.push(Symbol(87));
    st.push(Symbol(TokenType::NOT_EQUAL_OP));
    break;
  case 88:
    st.push(Symbol(88));
    st.push(Symbol(NonTerminal::Relational_Tail_NT));
    st.push(Symbol(NonTerminal::Additive_Expr_NT));
    break;
  case 89:
    st.push(Symbol(89));
    break;
  case 90:
    st.push(Symbol(90));
    st.push(Symbol(NonTerminal::Relational_Tail_NT));
    st.push(Symbol(NonTerminal::Additive_Expr_NT));
    st.push(Symbol(NonTerminal::Relation_NT));
    break;
  case 91:
    st.push(Symbol(91));
    st.push(Symbol(TokenType::LESS_EQUAL_OP));
    break;
  case 92:
    st.push(Symbol(92));
    st.push(Symbol(TokenType::LESS_THAN_OP));
    break;
  case 93:
    st.push(Symbol(93));
    st.push(Symbol(TokenType::GREATER_THAN_OP));
    break;
  case 94:
    st.push(Symbol(94));
    st.push(Symbol(TokenType::GREATER_EQUAL_OP));
    break;
  case 95:
    st.push(Symbol(95));
    st.push(Symbol(NonTerminal::Additive_Tail_NT));
    st.push(Symbol(NonTerminal::Multiplicative_Expr_NT));
    break;
  case 96:
    st.push(Symbol(96));
    break;
  case 97:
    st.push(Symbol(97));
    st.push(Symbol(NonTerminal::Additive_Tail_NT));
    st.push(Symbol(NonTerminal::Multiplicative_Expr_NT));
    st.push(Symbol(NonTerminal::Weak_OP_NT));
    break;
  case 98:
    st.push(Symbol(98));
    st.push(Symbol(TokenType::PLUS_OP));
    break;
  case 99:
    st.push(Symbol(99));
    st.push(Symbol(TokenType::MINUS_OP));
    break;
  case 100:
    st.push(Symbol(100));
    st.push(Symbol(NonTerminal::Multiplicative_Tail_NT));
    st.push(Symbol(NonTerminal::Unary_Expr_NT));
    break;
  case 101:
    st.push(Symbol(101));
    break;
  case 102:
    st.push(Symbol(102));
    st.push(Symbol(NonTerminal::Multiplicative_Tail_NT));
    st.push(Symbol(NonTerminal::Unary_Expr_NT));
    st.push(Symbol(NonTerminal::Strong_OP_NT));
    break;
  case 103:
    st.push(Symbol(103));
    st.push(Symbol(TokenType::MULT_OP));
    break;
  case 104:
    st.push(Symbol(104));
    st.push(Symbol(TokenType::DIVIDE_OP));
    break;
  case 105:
    st.push(Symbol(105));
    st.push(Symbol(TokenType::MOD_OP));
    break;
  case 106:
    st.push(Symbol(106));
    st.push(Symbol(NonTerminal::Expr_NT));
    st.push(Symbol(TokenType::MINUS_OP));
    break;
  case 107:
    st.push(Symbol(107));
    st.push(Symbol(NonTerminal::Expr_NT));
    st.push(Symbol(TokenType::STRINGIFY_OP));
    break;
  case 108:
    st.push(Symbol(108));
    st.push(Symbol(NonTerminal::Expr_NT));
    st.push(Symbol(TokenType::BOOLEAN_OP));
    break;
  case 109:
    st.push(Symbol(109));
    st.push(Symbol(NonTerminal::Expr_NT));
    st.push(Symbol(TokenType::ROUND_OP));
    break;
  case 110:
    st.push(Symbol(110));
    st.push(Symbol(NonTerminal::Expr_NT));
    st.push(Symbol(TokenType::LENGTH_OP));
    break;
  case 111:
    st.push(Symbol(111));
    st.push(Symbol(NonTerminal::Assignable_Expr_NT));
    st.push(Symbol(NonTerminal::Prefix_NT));
    break;
  case 112:
    st.push(Symbol(112));
    st.push(Symbol(NonTerminal::Index_Expr_NT));
    st.push(Symbol(TokenType::NOT_KW));
    break;
  case 113:
    st.push(Symbol(113));
    st.push(Symbol(NonTerminal::maybe_Postfix_NT));
    st.push(Symbol(NonTerminal::Index_Expr_NT));
    break;
  case 114:
    st.push(Symbol(114));
    st.push(Symbol(TokenType::INCREMENT_OP));
    break;
  case 115:
    st.push(Symbol(115));
    st.push(Symbol(TokenType::DECREMENT_OP));
    break;
  case 116:
    st.push(Symbol(116));
    break;
  case 117:
    st.push(Symbol(117));
    st.push(Symbol(TokenType::INCREMENT_OP));
    break;
  case 118:
    st.push(Symbol(118));
    st.push(Symbol(TokenType::DECREMENT_OP));
    break;
  case 119:
    st.push(Symbol(119));
    st.push(Symbol(NonTerminal::Index_Chain_NT));
    st.push(Symbol(NonTerminal::CallOrVariable_NT));
    break;
  case 120:
    st.push(Symbol(120));
    st.push(Symbol(NonTerminal::Primary_Expr_NT));
    break;
  case 121:
    st.push(Symbol(121));
    break;
  case 122:
    st.push(Symbol(122));
    st.push(Symbol(NonTerminal::Index_Chain_NT));
    st.push(Symbol(TokenType::RIGHT_CURLY_PR));
    st.push(Symbol(NonTerminal::Expr_NT));
    st.push(Symbol(TokenType::LEFT_CURLY_PR));
    break;
  case 123:
    st.push(Symbol(123));
    st.push(Symbol(TokenType::RIGHT_PR));
    st.push(Symbol(NonTerminal::Expr_NT));
    st.push(Symbol(TokenType::LEFT_PR));
    break;
  case 124:
    st.push(Symbol(124));
    st.push(Symbol(NonTerminal::Literal_NT));
    break;
  case 125:
    st.push(Symbol(125));
    st.push(Symbol(TokenType::INTEGER_NUM));
    break;
  case 126:
    st.push(Symbol(126));
    st.push(Symbol(TokenType::FLOAT_NUM));
    break;
  case 127:
    st.push(Symbol(127));
    st.push(Symbol(TokenType::STRING_SY));
    break;
  case 128:
    st.push(Symbol(128));
    st.push(Symbol(TokenType::TRUE_KW));
    break;
  case 129:
    st.push(Symbol(129));
    st.push(Symbol(TokenType::FALSE_KW));
    break;
  case 130:
    st.push(Symbol(130));
    st.push(Symbol(NonTerminal::MaybeCall_NT));
    st.push(Symbol(TokenType::ID_SY));
    break;
  case 131:
    st.push(Symbol(131));
    break;
  case 132:
    st.push(Symbol(132));
    st.push(Symbol(NonTerminal::Call_Expr_NT));
    break;
  case 133:
    st.push(Symbol(133));
    st.push(Symbol(TokenType::RIGHT_PR));
    st.push(Symbol(NonTerminal::Call_Expr_Tail_NT));
    st.push(Symbol(NonTerminal::Expr_NT));
    st.push(Symbol(TokenType::LEFT_PR));
    break;
  case 134:
    st.push(Symbol(134));
    break;
  case 135:
    st.push(Symbol(135));
    st.push(Symbol(NonTerminal::Call_Expr_Tail_NT));
    st.push(Symbol(NonTerminal::Expr_NT));
    st.push(Symbol(TokenType::COMMA_SY));
    break;
  }
}
