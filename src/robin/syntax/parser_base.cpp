#include "robin/syntax/parser_base.h"

namespace rbn::syntax
{
  ParserBase::ParserBase(lexical::ScannerBase *scanner)
  {
    sc = scanner;
    current_token = sc->get_token();
    previous_token = core::Token();
    has_error = false;
  }

  void ParserBase::reset_parser()
  {
    sc->reset_scanner();
    current_token = sc->get_token();
    previous_token = core::Token();
  }

  ast::ErrorNode *ParserBase::syntax_error(string message)
  {
    if (!has_error)
    {
      error_node = new ast::ErrorNode(message, current_token.line, previous_token.line, current_token.start, previous_token.end);
      has_error = true;
    }

    return error_node;
  }

  bool ParserBase::lookahead(core::TokenType type)
  {
    return current_token.type == type;
  }
}