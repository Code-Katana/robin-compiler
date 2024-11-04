#include "scanner_base.h"

ScannerBase::ScannerBase(string src)
{
  ch = ' ';
  str = "";
  curr = 0;
  source = src;
  error_token = {"$", TokenType::END_OF_FILE};
}

Token ScannerBase::check_reserved(string val)
{
  if (Token::is_reserved(val))
  {
    return {val, Token::ReservedWords[val]};
  }

  return {val, TokenType::ID_SY};
}
