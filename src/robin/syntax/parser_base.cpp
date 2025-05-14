#include "robin/syntax/parser_base.h"

namespace rbn::syntax
{
  ParserBase::ParserBase(lexical::ScannerBase *scanner)
  {
    sc = scanner;
    previous_token = core::Token();
    current_token = sc->get_token();

    if (auto error = sc->get_error())
    {
      forword_lexical_error(error, nullptr);
      return;
    }

    has_error = false;
    error_node = nullptr;
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

  ast::ErrorNode *ParserBase::forword_lexical_error(core::Token *error_token, core::Token *prev_token)
  {
    if (!prev_token)
    {
      int line = error_token->line;
      int start = error_token->start;
      int end = start + 1;
      error_node = new ast::ErrorNode(error_token->value, line, line, start, end);
      has_error = true;

      return error_node;
    }

    return syntax_error(error_token->value);
  }

  ast::ErrorNode *ParserBase::get_error_node()
  {
    return error_node;
  }

  bool ParserBase::lookahead(core::TokenType type)
  {
    return current_token.type == type;
  }
}