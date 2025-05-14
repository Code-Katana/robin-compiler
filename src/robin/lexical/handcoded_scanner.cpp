#include "robin/lexical/handcoded_scanner.h"

namespace rbn::lexical
{
  HandCodedScanner::HandCodedScanner(string src) : ScannerBase(src) {}

  core::Token HandCodedScanner::get_token()
  {
    str = "";

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
      return create_token("Φ", core::TokenType::END_OF_FILE);
    }
    // bracket
    else if (expect('['))
    {
      eat();
      return create_token("[", core::TokenType::LEFT_SQUARE_PR);
    }
    else if (expect(']'))
    {
      eat();
      return create_token("]", core::TokenType::RIGHT_SQUARE_PR);
    }
    else if (expect('{'))
    {
      eat();
      return create_token("{", core::TokenType::LEFT_CURLY_PR);
    }
    else if (expect('}'))
    {
      eat();
      return create_token("}", core::TokenType::RIGHT_CURLY_PR);
    }
    else if (expect('('))
    {
      eat();
      return create_token("(", core::TokenType::LEFT_PR);
    }
    else if (expect(')'))
    {
      eat();
      return create_token(")", core::TokenType::RIGHT_PR);
    }
    // operations
    else if (expect('='))
    {
      str += eat();

      if (expect('='))
      {
        str += eat();
        return create_token(str, core::TokenType::IS_EQUAL_OP);
      }

      return create_token(str, core::TokenType::EQUAL_OP);
    }
    else if (expect('+'))
    {
      str += eat();

      if (expect('+'))
      {
        str += eat();
        return create_token(str, core::TokenType::INCREMENT_OP);
      }

      return create_token(str, core::TokenType::PLUS_OP);
    }
    else if (expect('-'))
    {
      str += eat();

      if (expect('-'))
      {
        str += eat();
        return create_token(str, core::TokenType::DECREMENT_OP);
      }

      return create_token(str, core::TokenType::MINUS_OP);
    }
    else if (expect('$'))
    {
      eat();
      return create_token("$", core::TokenType::STRINGIFY_OP);
    }
    else if (expect('?'))
    {
      eat();
      return create_token("?", core::TokenType::BOOLEAN_OP);
    }
    else if (expect('@'))
    {
      eat();
      return create_token("@", core::TokenType::ROUND_OP);
    }
    else if (expect('#'))
    {
      eat();
      return create_token("#", core::TokenType::LENGTH_OP);
    }
    else if (expect('*'))
    {
      eat();
      return create_token("*", core::TokenType::MULT_OP);
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
        return create_token(str, core::TokenType::DIVIDE_OP);
      }
    }
    else if (expect('%'))
    {
      eat();
      return create_token("%", core::TokenType::MOD_OP);
    }
    else if (expect('<'))
    {
      str += eat();

      if (expect('>'))
      {
        str += eat();
        return create_token(str, core::TokenType::NOT_EQUAL_OP);
      }
      else if (expect('='))
      {
        str += eat();
        return create_token(str, core::TokenType::LESS_EQUAL_OP);
      }

      return create_token(str, core::TokenType::LESS_THAN_OP);
    }
    else if (expect('>'))
    {
      str += eat();

      if (expect('='))
      {
        str += eat();
        return create_token(str, core::TokenType::GREATER_EQUAL_OP);
      }

      return create_token(str, core::TokenType::GREATER_THAN_OP);
    }
    // symbols
    else if (expect(';'))
    {
      eat();
      return create_token(";", core::TokenType::SEMI_COLON_SY);
    }
    else if (expect(':'))
    {
      eat();
      return create_token(":", core::TokenType::COLON_SY);
    }
    else if (expect(','))
    {
      eat();
      return create_token(",", core::TokenType::COMMA_SY);
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
        return lexical_error("Unclosed string literal: " + str);
      }

      eat();
      return create_token(str, core::TokenType::STRING_SY);
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
            return lexical_error("Invalid floating point number " + str);
          }
        }

        str += eat();
      }

      if (is_float)
      {
        return create_token(str, core::TokenType::FLOAT_NUM);
      }

      return create_token(str, core::TokenType::INTEGER_NUM);
    }
    else
    {
      str += eat();
      return lexical_error("Unrecognized token: " + str);
    }
  }

  vector<core::Token> HandCodedScanner::get_tokens_stream(void)
  {
    map<string, int> placeholders = {
        {"curr", curr},
        {"line_count", line_count},
        {"token_start", token_start},
        {"token_end", token_end},
    };

    core::Token error_placeholder = core::Token();

    line_count = 1;
    token_start = token_end = curr = 0;
    vector<core::Token> stream = {};
    core::Token tk = get_token();

    while (tk.type != core::TokenType::END_OF_FILE)
    {
      stream.push_back(tk);
      if (tk.type == core::TokenType::ERROR && error_placeholder.value.empty())
      {
        error_placeholder = tk;
        error_token = core::Token();
        curr = token_end;
      }
      else if (error_placeholder.value != error_token.value)
      {
        curr = token_end;
      }
      tk = get_token();
    }

    stream.push_back(tk);

    curr = placeholders["curr"];
    line_count = placeholders["line_count"];
    token_start = placeholders["token_start"];
    token_end = placeholders["token_end"];
    error_token = error_placeholder;

    return stream;
  }
}