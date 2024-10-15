#include "scanner_base.h"

ScannerBase::ScannerBase(string src)
{
  str = "";
  ch = ' ';
  curr = 0;
  tokens = {};
  source = src;
}

Token ScannerBase::check_reserved(string val)
{
  if (val == "func")
    return {val, TokenType::FUNC_KW};
  else if (val == "has")
    return {val, TokenType::HAS_KW};
  else if (val == "begin")
    return {val, TokenType::BEGIN_KW};
  else if (val == "end")
    return {val, TokenType::END_KW};
  else if (val == "program")
    return {val, TokenType::PROGRAM_KW};
  else if (val == "is")
    return {val, TokenType::IS_KW};
  else if (val == "var")
    return {val, TokenType::VAR_KW};
  else if (val == "skip")
    return {val, TokenType::SKIP_KW};
  else if (val == "stop")
    return {val, TokenType::STOP_KW};
  else if (val == "read")
    return {val, TokenType::READ_KW};
  else if (val == "write")
    return {val, TokenType::WRITE_KW};
  else if (val == "for")
    return {val, TokenType::FOR_KW};
  else if (val == "do")
    return {val, TokenType::DO_KW};
  else if (val == "while")
    return {val, TokenType::WHILE_KW};
  else if (val == "if")
    return {val, TokenType::IF_KW};
  else if (val == "then")
    return {val, TokenType::THEN_KW};
  else if (val == "else")
    return {val, TokenType::ELSE_KW};
  else if (val == "return")
    return {val, TokenType::RETURN_KW};
  else if (val == "or")
    return {val, TokenType::OR_KW};
  else if (val == "and")
    return {val, TokenType::AND_KW};
  else if (val == "true")
    return {val, TokenType::TRUE_KW};
  else if (val == "false")
    return {val, TokenType::FALSE_KW};
  else if (val == "not")
    return {val, TokenType::NOT_KW};
  else if (val == "void")
    return {val, TokenType::VOID_TY};
  else if (val == "integer")
    return {val, TokenType::INTEGER_TY};
  else if (val == "boolean")
    return {val, TokenType::BOOLEAN_TY};
  else if (val == "string")
    return {val, TokenType::STRING_TY};
  else if (val == "float")
    return {val, TokenType::FLOAT_TY};
  return {val, TokenType::ID_SY};
}