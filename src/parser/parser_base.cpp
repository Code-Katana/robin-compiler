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

void ParserBase::syntax_error(string message)
{
  error_node.message =  message;
  current_token = Token("Φ", TokenType::END_OF_FILE, 0, 0, 0);
  
  //cerr << message << " at line " << previous_token.line << endl;
 // system("pause");
  //throw runtime_error(message);
}

bool ParserBase::lookahead(TokenType type)
{
  return current_token.type == type;
}
