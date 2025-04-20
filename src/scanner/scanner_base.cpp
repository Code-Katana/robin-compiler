#include "scanner_base.h"

ScannerBase::ScannerBase(string src)
{
  ch = 0;
  str = "";
  curr = 0;
  source = src;
  line_count = 1;
  token_start = 0;
  token_end = 0;
  error_token = Token();
}

bool ScannerBase::is_eof()
{
  return curr >= source.length();
}

char ScannerBase::peek()
{
  if (is_eof())
  {
    return 'Φ';
  }

  return source.at(curr);
}

char ScannerBase::eat()
{
  if (is_eof())
  {
    return 'Φ';
  }

  return source.at(curr++);
}

bool ScannerBase::expect(char expected)
{
  return peek() == expected;
}

int ScannerBase::update_line_count()
{
  return ++line_count;
}

Token ScannerBase::check_reserved(string val)
{
  if (Token::is_reserved(val))
  {
    return create_token(val, Token::ReservedWords[val]);
  }

  return create_token(val, TokenType::ID_SY);
}

void ScannerBase::reset_scanner()
{
  ch = 0;
  str = "";
  curr = 0;
  line_count = 1;
  token_start = 0;
  token_end = 0;
  error_token = Token();
}

Token ScannerBase::create_token(string val, TokenType type)
{
  token_end = curr;
  return Token(val, type, line_count, token_start, token_end);
}

Token ScannerBase::lexical_error(string message)
{
  error_token = create_token("Lexical Error: " + message, TokenType::ERROR);
  curr = source.length() + 1;
  return error_token;
}

Token *ScannerBase::get_error()
{
  if (error_token.value.empty())
  {
    return nullptr;
  }
  return &error_token;
}
