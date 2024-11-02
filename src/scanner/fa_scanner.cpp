#include "fa_scanner.h"

bool FAScanner::is_eof()
{
  return curr >= source.length();
}

char FAScanner::peek()
{
  if (is_eof())
  {
    return '$';
  }

  return source.at(curr);
}

char FAScanner::eat()
{
  if (is_eof())
  {
    return '$';
  }

  return source.at(curr++);
}

bool FAScanner::expect(char expected)
{
  return peek() == expected;
}

using namespace std;
FAScanner::FAScanner(string src) : ScannerBase(src) {}

Token FAScanner::get_token()
{
  str = "";
  int state = 0;

  while ((state >= 0 && state <= 46) && !is_eof())
  {
    switch (state)
    {
    case 0:
      ch = eat();
      if (isspace(ch))
        state = 0;
      else if (isalpha(ch) | expect('_'))
      {
        str += eat();
        state = 1;
      }
      else if (expect('['))
        state = 3;
      else if (expect(']'))
        state = 4;
      else if (expect(';'))
        state = 5;
      else if (expect(','))
        state = 6;
      else if (expect('{'))
        state = 7;
      else if (expect('}'))
        state = 8;
      else if (expect(':'))
        state = 9;
      else if (expect('('))
        state = 10;
      else if (expect(')'))
        state = 11;
      else if (expect('*'))
        state = 12;
      else if (expect('%'))
        state = 13;
      else if (expect('='))
        state = 14;
      else if (expect('+'))
        state = 17;
      else if (expect('-'))
        state = 20;
      else if (expect('<'))
        state = 23;
      else if (expect('>'))
        state = 27;
      else if (expect('/'))
        state = 30;
      else if (isdigit(ch))
      {
        str += ch;
        state = 36;
      }
      else if (expect('\"'))
      {
        state = 41;
        str += ch;
      }
      else
        state = 43;
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
        // curr--;
      }
      break;
    case 2:
      // tokens.push_back(check_reserved(str));
      return check_reserved(str);
      break;
    case 3:
      // tokens.push_back({"[", TokenType::LEFT_SQUARE_PR});
      return {"[", TokenType::LEFT_SQUARE_PR};
      break;
    case 4:
      // tokens.push_back({"]", TokenType::RIGHT_SQUARE_PR});
      return {"]", TokenType::RIGHT_SQUARE_PR};
      break;
    case 5:
      // tokens.push_back({";", TokenType::SEMI_COLON_SY});
      return {";", TokenType::SEMI_COLON_SY};
      break;
    case 6:
      // tokens.push_back({",", TokenType::COMMA_SY});
      return {",", TokenType::COMMA_SY};
      break;
    case 7:
      // tokens.push_back({"{", TokenType::LEFT_CURLY_PR});
      return {"{", TokenType::LEFT_CURLY_PR};
      break;
    case 8:
      // tokens.push_back({"}", TokenType::RIGHT_CURLY_PR});
      return {"}", TokenType::RIGHT_CURLY_PR};
      break;
    case 9:
      // tokens.push_back({":", TokenType::COLON_SY});
      return {":", TokenType::COLON_SY};
      break;
    case 10:
      // tokens.push_back({"(", TokenType::LEFT_PR});
      return {"(", TokenType::LEFT_PR};
      break;
    case 11:
      // tokens.push_back({")", TokenType::RIGHT_PR});
      return {")", TokenType::RIGHT_PR};
      break;
    case 12:
      // tokens.push_back({"*", TokenType::MULT_OP});
      return {"*", TokenType::MULT_OP};
      break;
    case 13:
      // tokens.push_back({"%", TokenType::MOD_OP});
      return {"%", TokenType::MOD_OP};
      break;
    case 14:
      ch = peek();
      if (expect('='))
      {
        eat();
        state = 15;
      }
      else
      {
        // curr--;
        state = 16;
      }
      break;
    case 15:
      // tokens.push_back({"==", TokenType::IS_EQUAL_OP});
      return {"==", TokenType::IS_EQUAL_OP};
      break;
    case 16:
      // tokens.push_back({"=", TokenType::EQUAL_OP});
      return {"=", TokenType::EQUAL_OP};
      break;
    case 17:
      ch = peek();
      if (expect('+'))
      {
        eat();
        state = 19;
      }
      else
      {
        // curr--;
        state = 18;
      }
      break;
    case 18:
      // tokens.push_back({"+", TokenType::PLUS_OP});
      return {"+", TokenType::PLUS_OP};
      break;
    case 19:
      // tokens.push_back({"++", TokenType::INCREMENT_OP});
      return {"++", TokenType::INCREMENT_OP};
      break;
    case 20:
      ch = peek();
      if (expect('-'))
      {
        eat();
        state = 21;
      }
      else
      {
        // curr--;
        state = 22;
      }
      break;
    case 21:
      // tokens.push_back({"--", TokenType::DECREMENT_OP});
      return {"--", TokenType::DECREMENT_OP};
      break;
    case 22:
      // tokens.push_back({"-", TokenType::MINUS_OP});
      return {"-", TokenType::MINUS_OP};
      break;
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
        // curr--;
        state = 24;
      }
      break;
    case 24:
      // tokens.push_back({"<", TokenType::LESS_THAN_OP});
      return {"<", TokenType::LESS_THAN_OP};
      break;
    case 25:
      // tokens.push_back({"<=", TokenType::LESS_EQUAL_OP});
      return {"<=", TokenType::LESS_EQUAL_OP};
      break;
    case 26:
      // tokens.push_back({"<>", TokenType::NOT_EQUAL_OP});
      return {"<>", TokenType::NOT_EQUAL_OP};
      break;
    case 27:
      ch = peek();
      if (expect('='))
      {
        eat();
        state = 29;
      }
      else
      {
        // curr--;
        state = 28;
      }
      break;
    case 28:
      // tokens.push_back({">", TokenType::GREATER_THAN_OP});
      return {">", TokenType::GREATER_THAN_OP};
      break;
    case 29:
      // tokens.push_back({">=", TokenType::GREATER_EQUAL_OP});
      return {">=", TokenType::GREATER_EQUAL_OP};
      break;
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
        // curr--;
        state = 31;
      }
      break;
    case 31:
      // tokens.push_back({"/", TokenType::DIVIDE_OP});
      return {"/", TokenType::DIVIDE_OP};
      break;
    case 32:
      ch = peek();
      if (expect('\n'))
      {
        eat();
        str = "";
        state = 0;
      }
      else
      {
        state = 32;
      }
      break;
    case 33:
      ch = peek();
      if (ch != '*')
      {
        eat();
        state = 33;
      }
      else
      {
        state = 34;
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
        // curr--;
      }
      break;
    case 37:
      // tokens.push_back({str, TokenType::INTEGER_NUM});
      return {str, TokenType::INTEGER_NUM};
      break;
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
        // curr--;
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
        // curr--;
        state = 40;
      }
      break;
    case 40:
      // tokens.push_back({str, TokenType::FLOAT_NUM});
      return {str, TokenType::FLOAT_NUM};
      break;
    case 41:
      ch = peek();
      if (curr >= source.length() - 1)
      {
        eat();
        state = 45;
      }
      else if (ch != '\"')
      {
        str += eat();
        state = 41;
      }
      else if (expect('\"'))
      {
        str += eat();
        state = 42;
      }
      break;
    case 42:
      // tokens.push_back({str, TokenType::STRING_SY});
      return {str, TokenType::STRING_SY};
      break;
    case 43:
      // tokens.push_back({"Unrecognized token: " + source.at(curr), TokenType::ERROR});
      return {"Unrecognized token: " + peek(), TokenType::ERROR};
      break;
    case 44:
      // tokens.push_back({"Invalid floating point number " + str, TokenType::ERROR});
      return {"Invalid floating point number " + str, TokenType::ERROR};
      break;
    case 45:
      // tokens.push_back({"Unclosed string literal: " + str, TokenType::ERROR});
      return {"Unclosed string literal: " + str, TokenType::ERROR};
      break;
    default:
      // tokens.push_back({"ERROR", TokenType::ERROR});
      return {"ERROR", TokenType::STRING_SY};
      break;
    }
  }
  return {"$", TokenType::END_OF_FILE};
}

vector<Token> FAScanner::get_tokens_stream(void)
{
  int curr_placeholder = curr;
  curr = 0;
  vector<Token> stream = {};
  Token tk = get_token();

  while (tk.type != TokenType::END_OF_FILE)
  {
    stream.push_back(tk);
    tk = get_token();
  }

  curr = curr_placeholder;
  return stream;
}
