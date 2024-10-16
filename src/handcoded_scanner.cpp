#include "handcoded_scanner.h"

HandCodedScanner::HandCodedScanner(string src) : ScannerBase(src)
{
  // curr = 0;
}

bool HandCodedScanner::is_eof()
{
  return curr >= source.length();
}

char HandCodedScanner::peek()
{
  if (is_eof())
    return '$';

  return source.at(curr);
}

char HandCodedScanner::eat()
{
  if (is_eof())
    return '$';

  return source.at(curr++);
}

bool HandCodedScanner::expect(char expected)
{
  return peek() == expected;
}

Token HandCodedScanner::get_token()
{
  str = "";

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
    tokens.push_back({"[", TokenType::LEFT_SQUARE_PR});
    return {"[", TokenType::LEFT_SQUARE_PR};
  }
  else if (expect(']'))
  {
    eat();
    tokens.push_back({"]", TokenType::RIGHT_SQUARE_PR});
    return {"]", TokenType::RIGHT_SQUARE_PR};
  }
  else if (expect('{'))
  {
    eat();
    tokens.push_back({"{", TokenType::LEFT_CURLY_PR});
    return {"{", TokenType::LEFT_CURLY_PR};
  }
  else if (expect('}'))
  {
    eat();
    tokens.push_back({"}", TokenType::RIGHT_CURLY_PR});
    return {"}", TokenType::RIGHT_CURLY_PR};
  }
  else if (expect('('))
  {
    eat();
    tokens.push_back({"(", TokenType::LEFT_PR});
    return {"(", TokenType::LEFT_PR};
  }
  else if (expect(')'))
  {
    eat();
    tokens.push_back({")", TokenType::RIGHT_PR});
    return {")", TokenType::RIGHT_PR};
  }
  // operations
  else if (expect('='))
  {
    str += eat();

    if (expect('='))
    {
      str += eat();
      tokens.push_back({str, TokenType::IS_EQUAL_OP});
      return {str, TokenType::IS_EQUAL_OP};
    }

    tokens.push_back({str, TokenType::EQUAL_OP});
    return {str, TokenType::EQUAL_OP};
  }
  else if (expect('+'))
  {
    str += eat();

    if (expect('+'))
    {
      str += eat();
      tokens.push_back({str, TokenType::INCREMENT_OP});
      return {str, TokenType::INCREMENT_OP};
    }

    tokens.push_back({str, TokenType::PLUS_OP});
    return {str, TokenType::PLUS_OP};
  }
  else if (expect('-'))
  {
    str += eat();

    if (expect('-'))
    {
      str += eat();
      tokens.push_back({str, TokenType::DECREMENT_OP});
      return {str, TokenType::DECREMENT_OP};
    }

    tokens.push_back({str, TokenType::MINUS_OP});
    return {str, TokenType::MINUS_OP};
  }
  else if (expect('*'))
  {
    eat();
    tokens.push_back({"*", TokenType::MULT_OP});
    return {"*", TokenType::MULT_OP};
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

      return get_token();
    }
    // multi comment
    else if (expect('*'))
    {
      eat(); /**/

      while (!is_eof())
      {
        if (expect('*'))
        {
          eat();
          if (expect('/'))
          {
            eat();
            return get_token();
          }
        }

        eat();
      }
    }
    // divide op
    else
    {
      tokens.push_back({str, TokenType::DIVIDE_OP});
      return {str, TokenType::DIVIDE_OP};
    }
  }
  else if (expect('%'))
  {
    eat();
    tokens.push_back({"%", TokenType::MOD_OP});
    return {"%", TokenType::MOD_OP};
  }
  else if (expect('<'))
  {
    str += eat();

    if (expect('>'))
    {
      str += eat();
      tokens.push_back({str, TokenType::NOT_EQUAL_OP});
      return {str, TokenType::NOT_EQUAL_OP};
    }
    else if (expect('='))
    {
      str += eat();
      tokens.push_back({str, TokenType::LESS_EQUAL_OP});
      return {str, TokenType::LESS_EQUAL_OP};
    }

    tokens.push_back({str, TokenType::LESS_THAN_OP});
    return {str, TokenType::LESS_THAN_OP};
  }
  else if (expect('>'))
  {
    str += eat();

    if (expect('='))
    {
      str += eat();
      tokens.push_back({str, TokenType::GREATER_EQUAL_OP});
      return {str, TokenType::GREATER_EQUAL_OP};
    }

    tokens.push_back({str, TokenType::GREATER_THAN_OP});
    return {str, TokenType::GREATER_THAN_OP};
  }
  // symbols
  else if (expect(';'))
  {
    eat();
    tokens.push_back({";", TokenType::SEMI_COLON_SY});
    return {";", TokenType::SEMI_COLON_SY};
  }
  else if (expect(':'))
  {
    eat();
    tokens.push_back({":", TokenType::COLON_SY});
    return {":", TokenType::COLON_SY};
  }
  else if (expect(','))
  {
    eat();
    tokens.push_back({",", TokenType::COMMA_SY});
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
    str += eat();

    while (!expect('\"') && !is_eof())
    {
      str += eat();
    }

    if (is_eof())
    {
      return {"Unclosed string literal: " + str, TokenType::ERROR};
    }

    str += eat();
    tokens.push_back({str, TokenType::STRING_SY});
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
          return {"Invalid floating point number " + str, TokenType::ERROR};
        }
      }

      str += eat();
    }

    if (is_float)
    {
      tokens.push_back({str, TokenType::FLOAT_NUM});
      return {str, TokenType::FLOAT_NUM};
    }

    tokens.push_back({str, TokenType::INTEGER_NUM});
    return {str, TokenType::INTEGER_NUM};
  }
  else
  {
    str += eat();
    return {"Unrecognized token: " + str, TokenType::ERROR};
  }
}

void HandCodedScanner::display_tokens(void)
{
  Token T = get_token();

  while (T.type != TokenType::END_OF_FILE)
  {
    switch (T.type)
    {
    case TokenType::FUNC_KW:
      cout << "Function Key Word : token" << endl;
      break;
    case TokenType::HAS_KW:
      cout << "Has Key Word : token" << endl;
      break;
    case TokenType::BEGIN_KW:
      cout << "Begin Key Word : token" << endl;
      break;
    case TokenType::END_KW:
      cout << "End Key Word : token" << endl;
      break;
    case TokenType::VOID_TY:
      cout << "Void Key Word : token" << endl;
      break;
    case TokenType::LEFT_SQUARE_PR:
      cout << "Left Square bracket [ Symbol : token" << endl;
      break;
    case TokenType::RIGHT_SQUARE_PR:
      cout << "Right Square bracket ] Symbol : token" << endl;
      break;
    case TokenType::INTEGER_TY:
      cout << "Integer Key Word : token" << endl;
      break;
    case TokenType::BOOLEAN_TY:
      cout << "Boolean Key Word : token" << endl;
      break;
    case TokenType::STRING_TY:
      cout << "String Key Word : token" << endl;
      break;
    case TokenType::FLOAT_TY:
      cout << "Float Key Word : token" << endl;
      break;
    case TokenType::PROGRAM_KW:
      cout << "Program Key Word : token" << endl;
      break;
    case TokenType::IS_KW:
      cout << "Is Key Word : token" << endl;
      break;
    case TokenType::SEMI_COLON_SY:
      cout << "Semi Colon ; Symbol : token" << endl;
      break;
    case TokenType::VAR_KW:
      cout << "Var Key Word : token" << endl;
      break;
    case TokenType::COLON_SY:
      cout << "Colon : Symbol : token" << endl;
      break;
    case TokenType::LEFT_CURLY_PR:
      cout << "Left Curly bracket { Symbol : token" << endl;
      break;
    case TokenType::RIGHT_CURLY_PR:
      cout << "Right Curly bracket } Symbol : token" << endl;
      break;
    case TokenType::COMMA_SY:
      cout << "Comma , Symbol : token" << endl;
      break;
    case TokenType::SKIP_KW:
      cout << "Skip Key Word : token" << endl;
      break;
    case TokenType::STOP_KW:
      cout << "Stop Key Word : token" << endl;
      break;
    case TokenType::EQUAL_OP:
      cout << "Equal = Operator : token" << endl;
      break;
    case TokenType::READ_KW:
      cout << "Read Key Word : token" << endl;
      break;
    case TokenType::WRITE_KW:
      cout << "Write Key Word : token" << endl;
      break;
    case TokenType::WHILE_KW:
      cout << "While Key Word : token" << endl;
      break;
    case TokenType::DO_KW:
      cout << "Do Key Word : token" << endl;
      break;
    case TokenType::FOR_KW:
      cout << "For Key Word : token" << endl;
      break;
    case TokenType::IF_KW:
      cout << "IF Key Word : token" << endl;
      break;
    case TokenType::THEN_KW:
      cout << "Then Key Word : token" << endl;
      break;
    case TokenType::ELSE_KW:
      cout << "Else Key Word : token" << endl;
      break;
    case TokenType::RETURN_KW:
      cout << "Return Key Word : token" << endl;
      break;
    case TokenType::LEFT_PR:
      cout << "Left parentheses ( Symbol : token" << endl;
      break;
    case TokenType::RIGHT_PR:
      cout << "Right parentheses ) Symbol : token" << endl;
      break;
    case TokenType::OR_KW:
      cout << "OR Key Word : token" << endl;
      break;
    case TokenType::AND_KW:
      cout << "AND Key Word : token" << endl;
      break;
    case TokenType::TRUE_KW:
      cout << "True Key Word : token" << endl;
      break;
    case TokenType::FALSE_KW:
      cout << "False Key Word : token" << endl;
      break;
    case TokenType::NOT_KW:
      cout << "Not Key Word : token" << endl;
      break;
    case TokenType::PLUS_OP:
      cout << "Plus + Operator : token" << endl;
      break;
    case TokenType::MINUS_OP:
      cout << "Minus - Operator : token" << endl;
      break;
    case TokenType::INCREMENT_OP:
      cout << "Increment ++ Operator : token" << endl;
      break;
    case TokenType::DECREMENT_OP:
      cout << "Decrement -- Operator : token" << endl;
      break;
    case TokenType::MULT_OP:
      cout << "MUlti * Operator : token" << endl;
      break;
    case TokenType::DIVIDE_OP:
      cout << "Divide / Operator : token" << endl;
      break;
    case TokenType::MOD_OP:
      cout << "Module Operator : token" << endl;
      break;
    case TokenType::LESS_THAN_OP:
      cout << "LessThan < Operator : token" << endl;
      break;
    case TokenType::LESS_EQUAL_OP:
      cout << "LessEqual <= Operator : token" << endl;
      break;
    case TokenType::NOT_EQUAL_OP:
      cout << "NotEqual <> Operator : token" << endl;
      break;
    case TokenType::IS_EQUAL_OP:
      cout << "IsEqual == Operator : token" << endl;
      break;
    case TokenType::GREATER_THAN_OP:
      cout << "GreaterThan > Operator : token" << endl;
      break;
    case TokenType::GREATER_EQUAL_OP:
      cout << "GreaterEqual >= Operator : token" << endl;
      break;
    case TokenType::ID_SY:
      cout << "Identifier " << T.value << "  token" << endl;
      break;
    case TokenType::INTEGER_NUM:
      cout << "Integer " << T.value << " : token" << endl;
      break;
    case TokenType::FLOAT_NUM:
      cout << "Float " << T.value << " : token" << endl;
      break;
    case TokenType::STRING_SY:
      cout << "String " << T.value << " : token" << endl;
      break;

    case TokenType::ERROR:
      cout << T.value << endl;
      break;

    default:
      cout << "Lexical Error : illegal Token." << endl;
      break;
    }
    T = get_token();
  }
  cout << "End of File encountered." << endl;
}
