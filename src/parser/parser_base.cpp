#include "parser_base.h"

ParserBase::ParserBase(ScannerBase *scanner)
{
  sc = scanner;
  current_token = sc->get_token();
}

Token ParserBase::match(TokenType type)
{
  Token placeholder = current_token;

  if (current_token.type != type)
  {
    current_token = {"$", TokenType::END_OF_FILE};
    return placeholder;
  }

  current_token = sc->get_token();
  return placeholder;
}

bool ParserBase::expect(TokenType type)
{
  return current_token.type == type;
}
