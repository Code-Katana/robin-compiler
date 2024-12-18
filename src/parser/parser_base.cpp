#include "parser_base.h"

ParserBase::ParserBase(ScannerBase *scanner)
{
  sc = scanner;
  current_token = sc->get_token();
  previous_token = Token();
}

void ParserBase::reset_parser()
{
  sc->reset_scanner();
  current_token = sc->get_token();
  previous_token = Token();
}

Token ParserBase::match(TokenType type)
{
  previous_token = current_token;

  if (current_token.type != type)
  {
    syntax_error("expecting " + Token::get_token_name(type) +
                 " instead of " + Token::get_token_name(current_token.type));
  }

  current_token = sc->get_token();
  return previous_token;
}

void ParserBase::syntax_error(string message)
{
  current_token = Token("$", TokenType::END_OF_FILE, 0, 0, 0);
  cerr << message << " at line " << previous_token.line << endl;
  system("pause");
  throw runtime_error(message);
}

bool ParserBase::lookahead(TokenType type)
{
  return current_token.type == type;
}
