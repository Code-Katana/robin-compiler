#include "fa_scanner.h"

FAScanner::FAScanner(string src) : ScannerBase(src) {}

Token FAScanner::get_token()
{
  str = "";
  int state = 0;
  token_start = curr;

  if (is_eof())
  {
    return create_token("Φ", TokenType::END_OF_FILE);
  }

  while (state >= START_STATE && state <= END_STATE)
  {
    switch (state)
    {
    case 0:
      ch = peek();

      if (is_eof())
      {
        state = 50;
      }
      else if (isspace(ch))
      {
        if (expect('\n'))
        {
          update_line_count();
        }

        eat();
        state = 0;
        token_start = curr;
      }

      else if (isalpha(ch) | expect('_'))
      {
        str += eat();
        state = 1;
      }
      else if (expect('['))
      {
        eat();
        state = 3;
      }
      else if (expect(']'))
      {
        eat();
        state = 4;
      }
      else if (expect(';'))
      {
        eat();
        state = 5;
      }
      else if (expect(','))
      {
        eat();
        state = 6;
      }
      else if (expect('{'))
      {
        eat();
        state = 7;
      }
      else if (expect('}'))
      {
        eat();
        state = 8;
      }
      else if (expect(':'))
      {
        eat();
        state = 9;
      }
      else if (expect('('))
      {
        eat();
        state = 10;
      }
      else if (expect(')'))
      {
        eat();
        state = 11;
      }
      else if (expect('*'))
      {
        eat();
        state = 12;
      }
      else if (expect('%'))
      {
        eat();
        state = 13;
      }
      else if (expect('='))
      {
        eat();
        state = 14;
      }
      else if (expect('+'))
      {
        eat();
        state = 17;
      }
      else if (expect('-'))
      {
        eat();
        state = 20;
      }
      else if (expect('$'))
      {
        eat();
        state = 46;
      }
      else if (expect('@'))
      {
        eat();
        state = 47;
      }
      else if (expect('?'))
      {
        eat();
        state = 48;
      }
      else if (expect('#'))
      {
        eat();
        state = 49;
      }
      else if (expect('<'))
      {
        eat();
        state = 23;
      }
      else if (expect('>'))
      {
        eat();
        state = 27;
      }
      else if (expect('/'))
      {
        eat();
        state = 30;
      }
      else if (isdigit(ch))
      {
        str += eat();
        state = 36;
      }
      else if (expect('\"'))
      {
        eat();
        state = 41;
      }
      else
      {
        state = 43;
      }
      break;
    case 1:
      ch = peek();
      if (isalnum(ch) | expect('_'))
      {
        str += eat();
        state = 1;
      }
      else
      {
        state = 2;
      }
      break;
    case 2:
      return check_reserved(str);

    case 3:
      return create_token("[", TokenType::LEFT_SQUARE_PR);

    case 4:
      return create_token("]", TokenType::RIGHT_SQUARE_PR);

    case 5:
      return create_token(";", TokenType::SEMI_COLON_SY);

    case 6:
      return create_token(",", TokenType::COMMA_SY);

    case 7:
      return create_token("{", TokenType::LEFT_CURLY_PR);

    case 8:
      return create_token("}", TokenType::RIGHT_CURLY_PR);

    case 9:
      return create_token(":", TokenType::COLON_SY);

    case 10:
      return create_token("(", TokenType::LEFT_PR);

    case 11:
      return create_token(")", TokenType::RIGHT_PR);

    case 12:
      return create_token("*", TokenType::MULT_OP);

    case 13:
      return create_token("%", TokenType::MOD_OP);

    case 14:
      ch = peek();

      if (expect('='))
      {
        eat();
        state = 15;
      }
      else
      {
        state = 16;
      }

      break;
    case 15:
      return create_token("==", TokenType::IS_EQUAL_OP);

    case 16:
      return create_token("=", TokenType::EQUAL_OP);

    case 17:
      ch = peek();

      if (expect('+'))
      {
        eat();
        state = 19;
      }
      else
      {
        state = 18;
      }

      break;
    case 18:
      return create_token("+", TokenType::PLUS_OP);

    case 19:
      return create_token("++", TokenType::INCREMENT_OP);

    case 20:
      ch = peek();

      if (expect('-'))
      {
        eat();
        state = 21;
      }
      else
      {
        state = 22;
      }

      break;
    case 21:
      return create_token("--", TokenType::DECREMENT_OP);

    case 22:
      return create_token("-", TokenType::MINUS_OP);

    case 23:
      ch = peek();

      if (expect('>'))
      {
        eat();
        state = 26;
      }
      else if (expect('='))
      {
        eat();
        state = 25;
      }
      else
      {
        state = 24;
      }

      break;
    case 24:
      return create_token("<", TokenType::LESS_THAN_OP);

    case 25:
      return create_token("<=", TokenType::LESS_EQUAL_OP);

    case 26:
      return create_token("<>", TokenType::NOT_EQUAL_OP);

    case 27:
      ch = peek();

      if (expect('='))
      {
        eat();
        state = 29;
      }
      else
      {
        state = 28;
      }

      break;
    case 28:
      return create_token(">", TokenType::GREATER_THAN_OP);

    case 29:
      return create_token(">=", TokenType::GREATER_EQUAL_OP);

    case 30:
      ch = peek();

      if (expect('/'))
      {
        eat();
        state = 32;
      }
      else if (expect('*'))
      {
        eat();
        state = 33;
      }
      else
      {
        state = 31;
      }

      break;
    case 31:
      return create_token("/", TokenType::DIVIDE_OP);

    case 32:
      ch = peek();

      if (expect('\n'))
      {
        update_line_count();
        eat();
        str = "";
        token_start = curr;
        state = 0;
      }
      else
      {
        eat();
        state = 32;
      }

      break;
    case 33:
      ch = peek();

      if (!is_eof())
      {
        if (expect('\n'))
        {
          update_line_count();
        }

        if (!expect('*'))
        {
          eat();
          state = 33;
        }
        else
        {
          eat();
          state = 34;
        }
      }
      else
      {
        eat();
        state = 50;
      }

      break;
    case 34:
      ch = peek();

      if (expect('/'))
      {
        eat();
        state = 35;
      }
      else
      {
        state = 33;
      }

      break;
    case 35:
      str = "";
      token_start = curr;
      state = 0;

      break;
    case 36:
      ch = peek();
      if (isdigit(ch))
      {
        str += eat();
        state = 36;
      }
      else if (expect('.'))
      {
        str += eat();
        state = 38;
      }
      else
      {
        state = 37;
      }
      break;
    case 37:
      return create_token(str, TokenType::INTEGER_NUM);

    case 38:
      ch = peek();

      if (isdigit(ch))
      {
        str += eat();
        state = 39;
      }
      else
      {
        state = 44;
      }

      break;
    case 39:
      ch = peek();
      if (isdigit(ch))
      {
        str += eat();
        state = 39;
      }
      else
      {
        state = 40;
      }

      break;
    case 40:
      return create_token(str, TokenType::FLOAT_NUM);

    case 41:
      ch = peek();

      if (expect('\"'))
      {
        eat();
        state = 42;
        break;
      }
      else
      {
        if (!is_eof())
        {
          str += eat();
          state = 41;
          break;
        }
      }
      state = 45;
      // ch !==
      break;
    case 42:
      return create_token(str, TokenType::STRING_SY);
      break;
    case 43:
      str += eat();

      return lexical_error("Unrecognized token: " + str);

    case 44:
      return lexical_error("Invalid floating point number " + str);

    case 45:
      return lexical_error("Unclosed string literal: " + str);

    case 46:
      return create_token("$", TokenType::STRINGIFY_OP);

    case 47:
      return create_token("@", TokenType::ROUND_OP);

    case 48:
      return create_token("?", TokenType::BOOLEAN_OP);

    case 49:
      return create_token("#", TokenType::LENGTH_OP);

    case 50:
      return create_token("Φ", TokenType::END_OF_FILE);

    default:
      return lexical_error("Unexpected end of input.");
    }
  }

  return create_token("Φ", TokenType::END_OF_FILE);
}

vector<Token> FAScanner::get_tokens_stream(void)
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