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
    syntax_error("expecting " + Token::get_token_name(type) +
                 " instead of " + Token::get_token_name(current_token.type));
  }

  current_token = sc->get_token();
  return placeholder;
}

void ParserBase::syntax_error(string message)
{
  current_token = Token("$", TokenType::END_OF_FILE, 0, 0, 0);
  cerr << message << endl;
  system("pause");
  throw runtime_error(message);
}

bool ParserBase::lookahead(TokenType type)
{
  return current_token.type == type;
}
