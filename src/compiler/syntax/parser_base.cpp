#include "compiler/syntax/parser_base.h"

ParserBase::ParserBase(ScannerBase *scanner)
{
  sc = scanner;
  current_token = sc->get_token();
  previous_token = Token();
  has_error = false;
}

void ParserBase::reset_parser()
{
  sc->reset_scanner();
  current_token = sc->get_token();
  previous_token = Token();
}

ErrorNode *ParserBase::syntax_error(string message)
{
  if (!has_error)
  {
    error_node = new ErrorNode(message, current_token.line, previous_token.line, current_token.start, previous_token.end);
    has_error = true;
  }

  return error_node;
}

bool ParserBase::lookahead(TokenType type)
{
  return current_token.type == type;
}
