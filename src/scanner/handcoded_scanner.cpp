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
    if (expect('\n'))
    {
      update_line_count();
    }

    eat();
  }

  token_start = curr;

  if (is_eof())
  {
    return create_token("Φ", TokenType::END_OF_FILE);
  }
  // bracket
  else if (expect('['))
  {
    eat();
    return create_token("[", TokenType::LEFT_SQUARE_PR);
  }
  else if (expect(']'))
  {
    eat();
    return create_token("]", TokenType::RIGHT_SQUARE_PR);
  }
  else if (expect('{'))
  {
    eat();
    return create_token("{", TokenType::LEFT_CURLY_PR);
  }
  else if (expect('}'))
  {
    eat();
    return create_token("}", TokenType::RIGHT_CURLY_PR);
  }
  else if (expect('('))
  {
    eat();
    return create_token("(", TokenType::LEFT_PR);
  }
  else if (expect(')'))
  {
    eat();
    return create_token(")", TokenType::RIGHT_PR);
  }
  // operations
  else if (expect('='))
  {
    str += eat();

    if (expect('='))
    {
      str += eat();
      return create_token(str, TokenType::IS_EQUAL_OP);
    }

    return create_token(str, TokenType::EQUAL_OP);
  }
  else if (expect('+'))
  {
    str += eat();

    if (expect('+'))
    {
      str += eat();
      return create_token(str, TokenType::INCREMENT_OP);
    }

    return create_token(str, TokenType::PLUS_OP);
  }
  else if (expect('-'))
  {
    str += eat();

    if (expect('-'))
    {
      str += eat();
      return create_token(str, TokenType::DECREMENT_OP);
    }

    return create_token(str, TokenType::MINUS_OP);
  }
  else if (expect('$'))
  {
    eat();
    return create_token("$", TokenType::STRINGIFY_OP);
  }
  else if (expect('?'))
  {
    eat();
    return create_token("?", TokenType::BOOLEAN_OP);
  }
  else if (expect('@'))
  {
    eat();
    return create_token("@", TokenType::ROUND_OP);
  }
  else if (expect('#'))
  {
    eat();
    return create_token("#", TokenType::LENGTH_OP);
  }
  else if (expect('*'))
  {
    eat();
    return create_token("*", TokenType::MULT_OP);
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

      update_line_count();
      eat();
      return get_token();
    }
    // multi comment
    else if (expect('*'))
    {
      eat();
      while (!is_eof())
      {
        if (expect('\n'))
        {
          update_line_count();
        }
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
      return create_token(str, TokenType::DIVIDE_OP);
    }
  }
  else if (expect('%'))
  {
    eat();
    return create_token("%", TokenType::MOD_OP);
  }
  else if (expect('<'))
  {
    str += eat();

    if (expect('>'))
    {
      str += eat();
      return create_token(str, TokenType::NOT_EQUAL_OP);
    }
    else if (expect('='))
    {
      str += eat();
      return create_token(str, TokenType::LESS_EQUAL_OP);
    }

    return create_token(str, TokenType::LESS_THAN_OP);
  }
  else if (expect('>'))
  {
    str += eat();

    if (expect('='))
    {
      str += eat();
      return create_token(str, TokenType::GREATER_EQUAL_OP);
    }

    return create_token(str, TokenType::GREATER_THAN_OP);
  }
  // symbols
  else if (expect(';'))
  {
    eat();
    return create_token(";", TokenType::SEMI_COLON_SY);
  }
  else if (expect(':'))
  {
    eat();
    return create_token(":", TokenType::COLON_SY);
  }
  else if (expect(','))
  {
    eat();
    return create_token(",", TokenType::COMMA_SY);
  }
  // identifier and keywords
  else if (isalpha(peek()) || expect('_'))
  {
    str += eat();

    while ((isalnum(peek()) || expect('_')) && !is_eof())
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
      error_token = create_token("Unclosed string literal: " + str, TokenType::ERROR);
      return error_token;
    }

    eat();
    return create_token(str, TokenType::STRING_SY);
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
          error_token = create_token("Invalid floating point number " + str, TokenType::ERROR);
          return error_token;
        }
      }

      str += eat();
    }

    if (is_float)
    {
      return create_token(str, TokenType::FLOAT_NUM);
    }

    return create_token(str, TokenType::INTEGER_NUM);
  }
  else
  {
    str += eat();
    curr = source.length() + 1;
    error_token = create_token("Unrecognized token: " + str, TokenType::ERROR);
    return error_token;
  }
}

vector<Token> HandCodedScanner::get_tokens_stream(void)
{
  map<string, int> placeholders = {
      {"curr", curr},
      {"line_count", line_count},
      {"token_start", token_start},
      {"token_end", token_end},
  };

  line_count = 1;
  token_start = token_end = curr = 0;
  vector<Token> stream = {};
  Token tk = get_token();

  while (tk.type != TokenType::END_OF_FILE)
  {
    stream.push_back(tk);
    tk = get_token();
  }

  stream.push_back(tk);

  curr = placeholders["curr"];
  line_count = placeholders["line_count"];
  token_start = placeholders["token_start"];
  token_end = placeholders["token_end"];

  return stream;
}
