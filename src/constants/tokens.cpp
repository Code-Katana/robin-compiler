#include "token.h"

map<string, TokenType> Token::ReservedWords = {
    {"func", TokenType::FUNC_KW},
    {"has", TokenType::HAS_KW},
    {"begin", TokenType::BEGIN_KW},
    {"end", TokenType::END_KW},
    {"program", TokenType::PROGRAM_KW},
    {"is", TokenType::IS_KW},
    {"var", TokenType::VAR_KW},
    {"skip", TokenType::SKIP_KW},
    {"stop", TokenType::STOP_KW},
    {"read", TokenType::READ_KW},
    {"write", TokenType::WRITE_KW},
    {"for", TokenType::FOR_KW},
    {"do", TokenType::DO_KW},
    {"while", TokenType::WHILE_KW},
    {"if", TokenType::IF_KW},
    {"then", TokenType::THEN_KW},
    {"else", TokenType::ELSE_KW},
    {"return", TokenType::RETURN_KW},
    {"or", TokenType::OR_KW},
    {"and", TokenType::AND_KW},
    {"true", TokenType::TRUE_KW},
    {"false", TokenType::FALSE_KW},
    {"not", TokenType::NOT_KW},
    {"void", TokenType::VOID_TY},
    {"integer", TokenType::INTEGER_TY},
    {"boolean", TokenType::BOOLEAN_TY},
    {"string", TokenType::STRING_TY},
    {"float", TokenType::FLOAT_TY},
};

map<TokenType, string> Token::TokenNames = {
    // Keywords
    {TokenType::FUNC_KW, "FUNC_KW"},
    {TokenType::HAS_KW, "HAS_KW"},
    {TokenType::BEGIN_KW, "BEGIN_KW"},
    {TokenType::END_KW, "END_KW"},
    {TokenType::PROGRAM_KW, "PROGRAM_KW"},
    {TokenType::IS_KW, "IS_KW"},
    {TokenType::VAR_KW, "VAR_KW"},
    {TokenType::SKIP_KW, "SKIP_KW"},
    {TokenType::STOP_KW, "STOP_KW"},
    {TokenType::READ_KW, "READ_KW"},
    {TokenType::WRITE_KW, "WRITE_KW"},
    {TokenType::FOR_KW, "FOR_KW"},
    {TokenType::DO_KW, "DO_KW"},
    {TokenType::WHILE_KW, "WHILE_KW"},
    {TokenType::IF_KW, "IF_KW"},
    {TokenType::THEN_KW, "THEN_KW"},
    {TokenType::ELSE_KW, "ELSE_KW"},
    {TokenType::RETURN_KW, "RETURN_KW"},
    {TokenType::OR_KW, "OR_KW"},
    {TokenType::AND_KW, "AND_KW"},
    {TokenType::TRUE_KW, "TRUE_KW"},
    {TokenType::FALSE_KW, "FALSE_KW"},
    {TokenType::NOT_KW, "NOT_KW"},
    // type
    {TokenType::VOID_TY, "VOID_TY"},
    {TokenType::INTEGER_TY, "INTEGER_TY"},
    {TokenType::BOOLEAN_TY, "BOOLEAN_TY"},
    {TokenType::STRING_TY, "STRING_TY"},
    {TokenType::FLOAT_TY, "FLOAT_TY"},
    //  number
    {TokenType::INTEGER_NUM, "INTEGER_NUM"},
    {TokenType::FLOAT_NUM, "FLOAT_NUM"},
    // operations
    {TokenType::EQUAL_OP, "EQUAL_OP"},
    {TokenType::PLUS_OP, "PLUS_OP"},
    {TokenType::MINUS_OP, "MINUS_OP"},
    {TokenType::INCREMENT_OP, "INCREMENT_OP"},
    {TokenType::DECREMENT_OP, "DECREMENT_OP"},
    {TokenType::MULT_OP, "MULT_OP"},
    {TokenType::DIVIDE_OP, "DIVIDE_OP"},
    {TokenType::MOD_OP, "MOD_OP"},
    {TokenType::LESS_EQUAL_OP, "LESS_EQUAL_OP"},
    {TokenType::LESS_THAN_OP, "LESS_THAN_OP"},
    {TokenType::IS_EQUAL_OP, "IS_EQUAL_OP"},
    {TokenType::GREATER_THAN_OP, "GREATER_THAN_OP"},
    {TokenType::GREATER_EQUAL_OP, "GREATER_EQUAL_OP"},
    {TokenType::NOT_EQUAL_OP, "NOT_EQUAL_OP"},
    // bracket
    {TokenType::LEFT_SQUARE_PR, "LEFT_SQUARE_PR"},
    {TokenType::RIGHT_SQUARE_PR, "RIGHT_SQUARE_PR"},
    {TokenType::LEFT_CURLY_PR, "LEFT_CURLY_PR"},
    {TokenType::RIGHT_CURLY_PR, "RIGHT_CURLY_PR"},
    {TokenType::LEFT_PR, "LEFT_PR"},
    {TokenType::RIGHT_PR, "RIGHT_PR"},
    // symbols
    {TokenType::SEMI_COLON_SY, "SEMI_COLON_SY"},
    {TokenType::COLON_SY, "COLON_SY"},
    {TokenType::COMMA_SY, "COMMA_SY"},
    {TokenType::ID_SY, "ID_SY"},
    {TokenType::STRING_SY, "STRING_SY"},
    // others
    {TokenType::ERROR, "UNRECOGNISED_TOKEN"},
    {TokenType::END_OF_FILE, "$"},
};

Token::Token() {}

Token::Token(string val, TokenType ty) : value(val), type(ty)
{
  line = start = end = 0;
}

Token::Token(string val, TokenType ty, int l, int s, int e) : value(val), type(ty), line(l), start(s), end(e) {}

bool Token::is_reserved(string val)
{
  return Token::ReservedWords.find(val) != Token::ReservedWords.end();
}

string Token::get_token_name(TokenType ty)
{
  if (Token::TokenNames.find(ty) == Token::TokenNames.end())
  {
    return Token::TokenNames[TokenType::ERROR];
  }

  return Token::TokenNames[ty];
}
