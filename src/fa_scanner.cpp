#include "fa_scanner.h"

using namespace std;
FAScanner::FAScanner(string src) : ScannerBase(src)
{
}

Token FAScanner::get_token()
{
  str = "";
  int state = 0;

  while ((state >= 0 && state <= 43) && (curr < source.length()))
  {
    switch (state)
    {
    case 0:
      ch = source.at(curr++);
      if (isspace(ch))
        state = 0;
      else if (isalpha(ch) || ch == '_')
      {
        str += ch;
        state = 1;
      }
      else if (ch == '[')
        state = 3;
      else if (ch == ']')
        state = 4;
      else if (ch == ';')
        state = 5;
      else if (ch == ',')
        state = 6;
      else if (ch == '{')
        state = 7;
      else if (ch == '}')
        state = 8;
      else if (ch == ':')
        state = 9;
      else if (ch == '(')
        state = 10;
      else if (ch == ')')
        state = 11;
      else if (ch == '*')
        state = 12;
      else if (ch == '%')
        state = 13;
      else if (ch == '=')
        state = 14;
      else if (ch == '+')
        state = 17;
      else if (ch == '-')
        state = 20;
      else if (ch == '<')
        state = 23;
      else if (ch == '>')
        state = 27;
      else if (ch == '/')
        state = 30;
      else if (isdigit(ch))
      {
        str += ch;
        state = 36;
      }
      else if (ch == '"')
      {
        state = 41;
        str += ch;
      }
      else
        state = 43;
      break;
    case 1:
      ch = source.at(curr++);
      if (isalnum(ch) || ch == '_')
      {
        str += ch;
        state = 1;
      }
      else
      {
        state = 2;
        curr--;
      }
      break;
    case 2:
      tokens.push_back(check_reserved(str));
      return check_reserved(str);
      break;
    case 3:
      tokens.push_back({"[", TokenType::LEFT_SQUARE_PR});
      return {"[", TokenType::LEFT_SQUARE_PR};
      break;
    case 4:
      tokens.push_back({"]", TokenType::RIGHT_SQUARE_PR});
      return {"]", TokenType::RIGHT_SQUARE_PR};
      break;
    case 5:
      tokens.push_back({";", TokenType::SEMI_COLON_SY});
      return {";", TokenType::SEMI_COLON_SY};
      break;
    case 6:
      tokens.push_back({",", TokenType::COMMA_SY});
      return {",", TokenType::COMMA_SY};
      break;
    case 7:
      tokens.push_back({"{", TokenType::LEFT_CURLY_PR});
      return {"{", TokenType::LEFT_CURLY_PR};
      break;
    case 8:
      tokens.push_back({"}", TokenType::RIGHT_CURLY_PR});
      return {"}", TokenType::RIGHT_CURLY_PR};
      break;
    case 9:
      tokens.push_back({":", TokenType::COLON_SY});
      return {":", TokenType::COLON_SY};
      break;
    case 10:
      tokens.push_back({"(", TokenType::LEFT_PR});
      return {"(", TokenType::LEFT_PR};
      break;
    case 11:
      tokens.push_back({")", TokenType::RIGHT_PR});
      return {")", TokenType::RIGHT_PR};
      break;
    case 12:
      tokens.push_back({"*", TokenType::MULT_OP});
      return {"*", TokenType::MULT_OP};
      break;
    case 13:
      tokens.push_back({"%", TokenType::MOD_OP});
      return {"%", TokenType::MOD_OP};
      break;
    case 14:
      ch = source.at(curr++);
      if (ch == '=')
      {
        state = 15;
      }
      else
      {
        curr--;
        state = 16;
      }
      break;
    case 15:
      tokens.push_back({"==", TokenType::IS_EQUAL_OP});
      return {"==", TokenType::IS_EQUAL_OP};
      break;
    case 16:
      tokens.push_back({"=", TokenType::EQUAL_OP});
      return {"=", TokenType::EQUAL_OP};
      break;
    case 17:
      ch = source.at(curr++);
      if (ch == '+')
      {
        state = 19;
      }
      else
      {
        curr--;
        state = 18;
      }
      break;
    case 18:
      tokens.push_back({"+", TokenType::PLUS_OP});
      return {"+", TokenType::PLUS_OP};
      break;
    case 19:
      tokens.push_back({"++", TokenType::INCREMENT_OP});
      return {"++", TokenType::INCREMENT_OP};
      break;
    case 20:
      ch = source.at(curr++);
      if (ch == '-')
      {
        state = 21;
      }
      else
      {
        curr--;
        state = 22;
      }
      break;
    case 21:
      tokens.push_back({"--", TokenType::DECREMENT_OP});
      return {"--", TokenType::DECREMENT_OP};
      break;
    case 22:
      tokens.push_back({"-", TokenType::MINUS_OP});
      return {"-", TokenType::MINUS_OP};
      break;
    case 23:
      ch = source.at(curr++);
      if (ch == '>')
      {
        state = 26;
      }
      else if (ch == '=')
      {
        state = 25;
      }
      else
      {
        curr--;
        state = 24;
      }
      break;
    case 24:
      tokens.push_back({"<", TokenType::LESS_THAN_OP});
      return {"<", TokenType::LESS_THAN_OP};
      break;
    case 25:
      tokens.push_back({"<=", TokenType::LESS_EQUAL_OP});
      return {"<=", TokenType::LESS_EQUAL_OP};
      break;
    case 26:
      tokens.push_back({"<>", TokenType::NOT_EQUAL_OP});
      return {"<>", TokenType::NOT_EQUAL_OP};
      break;
    case 27:
      ch = source.at(curr++);
      if (ch == '=')
      {
        state = 29;
      }
      else
      {
        curr--;
        state = 28;
      }
      break;
    case 28:
      tokens.push_back({">", TokenType::GREATER_THAN_OP});
      return {">", TokenType::GREATER_THAN_OP};
      break;
    case 29:
      tokens.push_back({">=", TokenType::GREATER_EQUAL_OP});
      return {">=", TokenType::GREATER_EQUAL_OP};
      break;
    case 30:
      ch = source.at(curr++);
      if (ch == '/')
      {
        state = 32;
      }
      else if (ch == '*')
      {
        state = 33;
      }
      else
      {
        curr--;
        state = 31;
      }
      break;
    case 31:
      tokens.push_back({"/", TokenType::DIVIDE_OP});
      return {"/", TokenType::DIVIDE_OP};
      break;
    case 32:
      ch = source.at(curr++);
      if (ch == '\n')
      {
        str = "";
        state = 0;
      }
      else
      {
        state = 32;
      }
      break;
    case 33:
      ch = source.at(curr++);
      if (curr == source.length() - 1)
      {
        state = 43;
      }
      else if (ch != '*')
      {
        state = 33;
      }
      else
      {
        state = 34;
      }
      break;
    case 34:
      ch = source.at(curr++);
      if (ch == '/')
      {
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
      ch = source.at(curr++);
      if (isdigit(ch))
      {
        str += ch;
        state = 36;
      }
      else if (ch == '.')
      {
        str += ch;
        state = 38;
      }
      else
      {
        state = 37;
        curr--;
      }
      break;
    case 37:
      tokens.push_back({str, TokenType::INTEGER_NUM});
      return {str, TokenType::INTEGER_NUM};
      break;
    case 38:
      ch = source.at(curr++);
      if (isdigit(ch))
      {
        str += ch;
        state = 39;
      }
      else
      {
        state = 43;
        curr--;
      }
      break;
    case 39:
      ch = source.at(curr++);
      if (isdigit(ch))
      {
        str += ch;
        state = 39;
      }
      else
      {
        curr--;
        state = 40;
      }
      break;
    case 40:
      tokens.push_back({str, TokenType::FLOAT_NUM});
      return {str, TokenType::FLOAT_NUM};
      break;
    case 41:
      ch = source.at(curr++);
      if (curr == source.length() - 1)
      {
        state = 43;
      }
      else if (ch != '"')
      {
        str += ch;
        state = 41;
      }
      else if (ch == '"')
      {
        str += ch;
        state = 42;
      }
      break;
    case 42:
      tokens.push_back({str, TokenType::STRING_SY});
      return {str, TokenType::STRING_SY};
      break;
    case 43:
      tokens.push_back({"ERROR", TokenType::ERROR});
      return {"ERROR", TokenType::STRING_SY};
      break;
    default:
      tokens.push_back({"ERROR", TokenType::ERROR});
      return {"ERROR", TokenType::STRING_SY};
      break;
    }
  }
  return {"$", TokenType::END_OF_FILE};
}

void FAScanner::display_tokens(void)
{
  Token T;
  T = get_token();
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

    default:
      cout << "Lexical Error : illegal Token." << endl;
      break;
    }
    T = get_token();
  }
  cout << "End of File encountered." << endl;
}