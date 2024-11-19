#include "scanner_base.h"

ScannerBase::ScannerBase(string src)
{
  ch = 0;
  str = "";
  curr = 0;
  source = src;
  error_token = Token("$", TokenType::END_OF_FILE);
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
    return Token(val, Token::ReservedWords[val]);
  }

  return Token(val, TokenType::ID_SY);
}

Token ScannerBase::create_token(string val, TokenType type, int l, int s, int e)
{
  return Token(val, type, l, s, e);
}
