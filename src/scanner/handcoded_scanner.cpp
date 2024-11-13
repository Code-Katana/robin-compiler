#include "handcoded_scanner.h"

HandCodedScanner::HandCodedScanner(string src) : ScannerBase(src) {}

Token HandCodedScanner::get_token()
{
  str = "";

  if (error_token.type == TokenType::ERROR && !is_eof())
  {
    curr = source.length() + 1;
    return error_token;
  }

  while (isspace(peek()) && !is_eof())
  {
    eat();
  }

  if (is_eof())
  {
    return {"$", TokenType::END_OF_FILE};
  }
  // bracket
  else if (expect('['))
  {
    eat();
    return {"[", TokenType::LEFT_SQUARE_PR};
  }
  else if (expect(']'))
  {
    eat();
    return {"]", TokenType::RIGHT_SQUARE_PR};
  }
  else if (expect('{'))
  {
    eat();
    return {"{", TokenType::LEFT_CURLY_PR};
  }
  else if (expect('}'))
  {
    eat();
    return {"}", TokenType::RIGHT_CURLY_PR};
  }
  else if (expect('('))
  {
    eat();
    return {"(", TokenType::LEFT_PR};
  }
  else if (expect(')'))
  {
    eat();
    return {")", TokenType::RIGHT_PR};
  }
  // operations
  else if (expect('='))
  {
    str += eat();

    if (expect('='))
    {
      str += eat();
      return {str, TokenType::IS_EQUAL_OP};
    }

    return {str, TokenType::EQUAL_OP};
  }
  else if (expect('+'))
  {
    str += eat();

    if (expect('+'))
    {
      str += eat();
      return {str, TokenType::INCREMENT_OP};
    }

    return {str, TokenType::PLUS_OP};
  }
  else if (expect('-'))
  {
    str += eat();

    if (expect('-'))
    {
      str += eat();
      return {str, TokenType::DECREMENT_OP};
    }

    return {str, TokenType::MINUS_OP};
  }
  else if (expect('*'))
  {
    eat();
    return {"*", TokenType::MULT_OP}; // DIVIDE_OP
  }
  else if (expect('/'))
  {
    str += eat();
    // single comment
    if (expect('/'))
    {
      eat();

      while (!expect('\n') && !is_eof())
      {
        eat();
      }

      eat();
      return get_token();
    }
    // multi comment
    else if (expect('*'))
    {
      eat();
      while (!is_eof())
      {
        if (!expect('*'))
        {
          eat();
        }
        else
        {
          eat();
          if (expect('/'))
          {
            eat();
            return get_token();
          }
        }
      }
      eat();
      return get_token();
    }
    // divide op
    else
    {
      return {str, TokenType::DIVIDE_OP};
    }
  }
  else if (expect('%'))
  {
    eat();
    return {"%", TokenType::MOD_OP};
  }
  else if (expect('<'))
  {
    str += eat();

    if (expect('>'))
    {
      str += eat();
      return {str, TokenType::NOT_EQUAL_OP};
    }
    else if (expect('='))
    {
      str += eat();
      return {str, TokenType::LESS_EQUAL_OP};
    }

    return {str, TokenType::LESS_THAN_OP};
  }
  else if (expect('>'))
  {
    str += eat();

    if (expect('='))
    {
      str += eat();
      return {str, TokenType::GREATER_EQUAL_OP};
    }

    return {str, TokenType::GREATER_THAN_OP};
  }
  // symbols
  else if (expect(';'))
  {
    eat();
    return {";", TokenType::SEMI_COLON_SY};
  }
  else if (expect(':'))
  {
    eat();
    return {":", TokenType::COLON_SY};
  }
  else if (expect(','))
  {
    eat();
    return {",", TokenType::COMMA_SY};
  }
  // identifier and keywords
  else if (isalpha(peek()))
  {
    str += eat();

    while (isalnum(peek()) && !is_eof())
    {
      str += eat();
    }

    return check_reserved(str);
  }
  // string literal
  else if (expect('\"'))
  {
    eat();

    while (!expect('\"') && !is_eof())
    {
      str += eat();
    }

    if (is_eof())
    {
      curr = source.length() + 1;
      error_token = {"Unclosed string literal: " + str, TokenType::ERROR};
      return error_token;
    }

    eat();
    return {str, TokenType::STRING_SY};
  }
  // numbers
  else if (isdigit(peek()))
  {
    str += eat();
    bool is_float = false;

    while ((isdigit(peek()) || expect('.')) && !is_eof())
    {
      if (expect('.') && !is_float)
      {
        is_float = true;
        str += eat();

        if (!isdigit(peek()))
        {
          curr = source.length() + 1;
          error_token = {"Invalid floating point number " + str, TokenType::ERROR};
          return error_token;
        }
      }

      str += eat();
    }

    if (is_float)
    {
      return {str, TokenType::FLOAT_NUM};
    }

    return {str, TokenType::INTEGER_NUM};
  }
  else
  {
    str += eat();
    curr = source.length() + 1;
    error_token = {"Unrecognized token: " + str, TokenType::ERROR};
    return error_token;
  }
}

vector<Token> HandCodedScanner::get_tokens_stream(void)
{
  int placeholder = curr;
  curr = 0;
  vector<Token> stream = {};
  Token tk = get_token();

  while (tk.type != TokenType::END_OF_FILE)
  {
    stream.push_back(tk);
    tk = get_token();
  }

  curr = placeholder;
  return stream;
}
