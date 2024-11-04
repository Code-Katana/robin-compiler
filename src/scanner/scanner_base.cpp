#include "scanner_base.h"

ScannerBase::ScannerBase(string src)
{
  ch = ' ';
  str = "";
  curr = 0;
  source = src;
  error_token = {"$", TokenType::END_OF_FILE};
}

bool ScannerBase::is_eof()
{
  return curr >= source.length();
}

char ScannerBase::peek()
{
  if (is_eof())
  {
    return '$';
  }

  return source.at(curr);
}

char ScannerBase::eat()
{
  if (is_eof())
  {
    return '$';
  }

  return source.at(curr++);
}

bool ScannerBase::expect(char expected)
{
  return peek() == expected;
}

Token ScannerBase::check_reserved(string val)
{
  if (Token::is_reserved(val))
  {
    return {val, Token::ReservedWords[val]};
  }

  return {val, TokenType::ID_SY};
}
