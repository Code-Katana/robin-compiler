#include "recursive_decent_parser.h"

RecursiveDecentParser::RecursiveDecentParser(ScannerBase *sc) : ParserBase(sc) {}

AstNode *RecursiveDecentParser::parse_ast()
{
  return parse_source();
}

AstNode *RecursiveDecentParser::parse_source()
{
  return parse_return_stmt();
}

ReturnStatement *RecursiveDecentParser::parse_return_stmt()
{
  match(TokenType::RETURN_KW);
  Expression *val;

  if (!expect(TokenType::SEMI_COLON_SY))
  {
    val = parse_expr();
    match(TokenType::SEMI_COLON_SY);
  }
  else
  {
    val = nullptr;
  }

  return new ReturnStatement(val);
}

Expression *RecursiveDecentParser::parse_expr()
{
  return parse_primary_expr();
}

PrimaryExpression *RecursiveDecentParser::parse_primary_expr()
{
  Literal *val = parse_int_literal();
  return new PrimaryExpression(val);
}

IntegerLiteral *RecursiveDecentParser::parse_int_literal()
{
  string val = current_token.value;
  match(TokenType::INTEGER_NUM);
  return new IntegerLiteral(stoi(val));
}
