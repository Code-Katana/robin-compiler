#pragma once
#include <string>
using namespace std;

enum class TokenType
{
  // Keywords
  FUNC_KW,
  HAS_KW,
  BEGIN_KW,
  END_KW,
  PROGRAM_KW,
  IS_KW,
  VAR_KW,
  SKIP_KW,
  STOP_KW,
  READ_KW,
  WRITE_KW,
  FOR_KW,
  DO_KW,
  WHILE_KW,
  IF_KW,
  THEN_KW,
  ELSE_KW,
  RETURN_KW,
  OR_KW,
  AND_KW,
  TRUE_KW,
  FALSE_KW,
  NOT_KW,
  // type
  VOID_TY,
  INTEGER_TY,
  BOOLEAN_TY,
  STRING_TY,
  FLOAT_TY,
  //  number
  INTEGER_NUM,
  FLOAT_NUM,
  // operations
  EQUAL_OP,
  PLUS_OP,
  MINUS_OP,
  INCREMENT_OP,
  DECREMENT_OP,
  MULT_OP,
  DIVIDE_OP,
  MOD_OP,
  LESS_EQUAL_OP, 
  LESS_THAN_OP, 
  IS_EQUAL_OP,
  GREATER_THAN_OP,
  GREATER_EQUAL_OP,
  NOT_EQUAL_OP,
  // bracket
  LEFT_SQUARE_PR,
  RIGHT_SQUARE_PR,
  LEFT_CURLY_PR,
  RIGHT_CURLY_PR,
  LEFT_PR,
  RIGHT_PR,
  // symbols
  SEMI_COLON_SY,
  COLON_SY,
  COMMA_SY,
  ID_SY,
  STRING_SY,
  // others
  ERROR,
  END_OF_FILE
};

struct Token
{
  string value;
  TokenType type;
};