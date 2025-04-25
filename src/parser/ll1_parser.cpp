#include "ll1_parser.h"

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