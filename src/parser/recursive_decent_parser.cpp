#include "recursive_decent_parser.h"

RecursiveDecentParser::RecursiveDecentParser(ScannerBase *sc) : ParserBase(sc) {}

AstNode *RecursiveDecentParser::parse_ast()
{
  return parse_source();
}

Source *RecursiveDecentParser::parse_source()
{
  set_start();

  vector<Function *> funcs = {};

  while (lookahead(TokenType::FUNC_KW))
  {
    funcs.push_back(parse_function());
  }

  Program *program = parse_program();

  set_end();
  return new Source(program, funcs, start_line, end_line, node_start, node_end);
}

Function *RecursiveDecentParser::parse_function()
{
  set_start();

  match(TokenType::FUNC_KW);
  ReturnType *return_type;
  Identifier *id;
  vector<VariableDefinition *> params = {};
  vector<Statement *> body = {};

  return_type = parse_return_type();

  id = parse_identifier();

  match(TokenType::HAS_KW);

  while (!lookahead(TokenType::BEGIN_KW))
  {
    params.push_back(parse_var_def());
  }

  match(TokenType::BEGIN_KW);

  while (!lookahead(TokenType::END_KW))
  {
    body.push_back(parse_command());
  }

  match(TokenType::END_KW);
  match(TokenType::FUNC_KW);

  set_end();
  return new Function(id, return_type, params, body, start_line, end_line, node_start, node_end);
}

Program *RecursiveDecentParser::parse_program()
{
  set_start();

  match(TokenType::PROGRAM_KW);
  Identifier *id = parse_identifier();
  vector<VariableDefinition *> globals = {};
  vector<Statement *> body = {};
  match(TokenType::IS_KW);

  while (!lookahead(TokenType::BEGIN_KW))
  {
    globals.push_back(parse_var_def());
  }

  match(TokenType::BEGIN_KW);

  while (!lookahead(TokenType::END_KW))
  {
    body.push_back(parse_command());
  }

  match(TokenType::END_KW);
  match(TokenType::END_OF_FILE);

  set_end();

  return new Program(id, globals, body, start_line, end_line, node_start, node_end);
}

DataType *RecursiveDecentParser::parse_data_type()
{
  set_start();

  string datatype;
  int dim = 0;

  if (!lookahead(TokenType::LEFT_SQUARE_PR) && AstNode::is_data_type(current_token.type))
  {
    datatype = current_token.value;
    match(current_token.type);

    set_end();

    return new PrimitiveType(datatype, start_line, end_line, node_start, node_end);
  }

  match(TokenType::LEFT_SQUARE_PR);
  ++dim;

  while (lookahead(TokenType::LEFT_SQUARE_PR))
  {
    match(TokenType::LEFT_SQUARE_PR);
    ++dim;
  }

  if (AstNode::is_data_type(current_token.type))
  {
    datatype = current_token.value;
    match(current_token.type);
  }

  for (int i = 0; i < dim; ++i)
  {
    match(TokenType::RIGHT_SQUARE_PR);
  }

  set_end();

  return new ArrayType(datatype, dim, start_line, end_line, node_start, node_end);
}

ReturnType *RecursiveDecentParser::parse_return_type()
{
  set_start();

  DataType *return_type;

  string datatype;
  int dim = 0;

  if (!lookahead(TokenType::LEFT_SQUARE_PR) && AstNode::is_return_type(current_token.type))
  {
    datatype = current_token.value;
    match(current_token.type);

    set_end();

    return_type = new PrimitiveType(datatype, start_line, end_line, node_start, node_end);
    return new ReturnType(return_type, start_line, end_line, node_start, node_end);
  }

  match(TokenType::LEFT_SQUARE_PR);
  ++dim;

  while (lookahead(TokenType::LEFT_SQUARE_PR))
  {
    match(TokenType::LEFT_SQUARE_PR);
    ++dim;
  }

  if (AstNode::is_data_type(current_token.type))
  {
    datatype = current_token.value;
    match(current_token.type);
  }

  for (int i = 0; i < dim; ++i)
  {
    match(TokenType::RIGHT_SQUARE_PR);
  }

  set_end();

  return_type = new ArrayType(datatype, dim, start_line, end_line, node_start, node_end);
  return new ReturnType(return_type, start_line, end_line, node_start, node_end);
}

VariableDefinition *RecursiveDecentParser::parse_var_def()
{
  set_start();

  vector<Identifier *> variables;
  DataType *datatype;

  match(TokenType::VAR_KW);

  variables.push_back(parse_identifier());

  while (lookahead(TokenType::COMMA_SY))
  {
    match(TokenType::COMMA_SY);
    variables.push_back(parse_identifier());
  }

  if (variables.size() == 1)
  {
    match(TokenType::COLON_SY);

    datatype = parse_data_type();

    if (lookahead(TokenType::SEMI_COLON_SY))
    {
      match(TokenType::SEMI_COLON_SY);

      set_end();

      Statement *declaration = new VariableDeclaration(variables, datatype, start_line, end_line, node_start, node_end);

      return new VariableDefinition(declaration, start_line, end_line, node_start, node_end);
    }
    else if (lookahead(TokenType::EQUAL_OP))
    {
      match(TokenType::EQUAL_OP);

      Expression *initializer = parse_expr();

      match(TokenType::SEMI_COLON_SY);

      set_end();

      Statement *initialization = new VariableInitialization(variables[0], datatype, initializer, start_line, end_line, node_start, node_end);

      return new VariableDefinition(initialization, start_line, end_line, node_start, node_end);
    }
  }

  match(TokenType::COLON_SY);

  datatype = parse_data_type();

  match(TokenType::SEMI_COLON_SY);

  set_end();

  Statement *declaration = new VariableDeclaration(variables, datatype, start_line, end_line, node_start, node_end);

  return new VariableDefinition(declaration, start_line, end_line, node_start, node_end);
}

Statement *RecursiveDecentParser::parse_command()
{
  // skip
  if (lookahead(TokenType::SKIP_KW))
  {
    return parse_skip_expr();
  }
  // stop
  else if (lookahead(TokenType::STOP_KW))
  {
    return parse_stop_expr();
  }

  // io stream
  else if (lookahead(TokenType::WRITE_KW))
  {
    return parse_write();
  }
  else if (lookahead(TokenType::READ_KW))
  {
    return parse_read();
  }
  // if
  else if (lookahead(TokenType::IF_KW))
  {
    return parse_if();
  }
  // loop
  else if (lookahead(TokenType::FOR_KW))
  {
    return parse_for();
  }
  else if (lookahead(TokenType::WHILE_KW))
  {
    return parse_while();
  }
  // return
  else if (lookahead(TokenType::RETURN_KW))
  {
    return parse_return_stmt();
  }
  // def
  else if (lookahead(TokenType::VAR_KW))
  {
    return parse_var_def();
  }

  return parse_expr_stmt();
}

SkipStatement *RecursiveDecentParser::parse_skip_expr()
{
  set_start();

  match(TokenType::SKIP_KW);
  match(TokenType::SEMI_COLON_SY);

  set_end();

  return new SkipStatement(start_line, end_line, node_start, node_end);
}

StopStatement *RecursiveDecentParser::parse_stop_expr()
{
  set_start();

  match(TokenType::STOP_KW);
  match(TokenType::SEMI_COLON_SY);

  set_end();

  return new StopStatement(start_line, end_line, node_start, node_end);
}

ReadStatement *RecursiveDecentParser::parse_read()
{
  set_start();

  vector<Identifier *> variables;

  match(TokenType::READ_KW);
  variables.push_back(parse_identifier());
  while (!lookahead(TokenType::SEMI_COLON_SY))
  {
    match(TokenType::COMMA_SY);
    variables.push_back(parse_identifier());
  }
  match(TokenType::SEMI_COLON_SY);

  set_end();

  return new ReadStatement(variables, start_line, end_line, node_start, node_end);
}

WriteStatement *RecursiveDecentParser::parse_write()
{
  set_start();

  vector<Expression *> args;
  match(TokenType::WRITE_KW);

  args.push_back(parse_or_expr());

  while (!lookahead(TokenType::SEMI_COLON_SY))
  {
    match(TokenType::COMMA_SY);
    args.push_back(parse_or_expr());
  }
  match(TokenType::SEMI_COLON_SY);

  set_end();

  return new WriteStatement(args, start_line, end_line, node_start, node_end);
}

ReturnStatement *RecursiveDecentParser::parse_return_stmt()
{
  set_start();

  Expression *val = nullptr;
  match(TokenType::RETURN_KW);

  if (!lookahead(TokenType::SEMI_COLON_SY))
  {
    val = parse_or_expr();
  }

  match(TokenType::SEMI_COLON_SY);

  set_end();
  return new ReturnStatement(val, start_line, end_line, node_start, node_end);
}

IfStatement *RecursiveDecentParser::parse_if()
{
  set_start();

  Expression *condition;
  vector<Statement *> consequent;
  vector<Statement *> alternate;

  match(TokenType::IF_KW);

  condition = parse_bool_expr();

  match(TokenType::THEN_KW);

  while (!lookahead(TokenType::ELSE_KW) && !lookahead(TokenType::END_KW))
  {
    consequent.push_back(parse_command());
  }

  if (lookahead(TokenType::ELSE_KW))
  {
    set_end();

    match(TokenType::ELSE_KW);

    if (lookahead(TokenType::IF_KW))
    {
      alternate.push_back(parse_if());
      return new IfStatement(condition, consequent, alternate, start_line, end_line, node_start, node_end);
    }

    while (!lookahead(TokenType::END_KW))
    {
      alternate.push_back(parse_command());
    }
  }

  match(TokenType::END_KW);
  match(TokenType::IF_KW);

  set_end();

  return new IfStatement(condition, consequent, alternate, start_line, end_line, node_start, node_end);
}

ForLoop *RecursiveDecentParser::parse_for()
{
  set_start();

  AssignmentExpression *init;
  BooleanExpression *condition;
  Expression *update;
  vector<Statement *> body;

  match(TokenType::FOR_KW);
  init = parse_int_assign();
  match(TokenType::SEMI_COLON_SY);
  condition = parse_bool_expr();
  match(TokenType::SEMI_COLON_SY);
  update = parse_expr();
  match(TokenType::DO_KW);
  while (!lookahead(TokenType::END_KW))
  {
    body.push_back(parse_command());
  }
  match(TokenType::END_KW);
  match(TokenType::FOR_KW);

  set_end();

  return new ForLoop(init, condition, update, body, start_line, end_line, node_start, node_end);
}

AssignmentExpression *RecursiveDecentParser::parse_int_assign()
{
  set_start();

  Identifier *id = parse_identifier();

  match(TokenType::EQUAL_OP);

  Expression *val = parse_expr();

  set_end();
  return new AssignmentExpression(id, val, start_line, end_line, node_start, node_end);
}

WhileLoop *RecursiveDecentParser::parse_while()
{
  set_start();

  Expression *condition;
  vector<Statement *> body;

  match(TokenType::WHILE_KW);
  condition = parse_bool_expr();
  match(TokenType::DO_KW);
  while (!lookahead(TokenType::END_KW))
  {
    body.push_back(parse_command());
  }
  match(TokenType::END_KW);
  match(TokenType::WHILE_KW);

  set_end();
  return new WhileLoop(condition, body, start_line, end_line, node_start, node_end);
}

BooleanExpression *RecursiveDecentParser::parse_bool_expr()
{
  return (BooleanExpression *)parse_or_expr();
}

Expression *RecursiveDecentParser::parse_expr_stmt()
{
  Expression *expr = parse_expr();
  match(TokenType::SEMI_COLON_SY);

  return expr;
}

Expression *RecursiveDecentParser::parse_expr()
{
  return parse_assign_expr();
}

Expression *RecursiveDecentParser::parse_assign_expr()
{
  set_start();

  Expression *left = parse_or_expr();

  if (!lookahead(TokenType::EQUAL_OP))
  {
    return left;
  }

  AssignableExpression *assignee = parse_assignable_expr(left);

  match(TokenType::EQUAL_OP);

  set_end();

  Expression *assExpr = new AssignmentExpression(assignee, parse_assign_expr(), start_line, end_line, node_start, node_end);

  return assExpr;
}

AssignableExpression *RecursiveDecentParser::parse_assignable_expr(Expression *expr)
{
  if (!dynamic_cast<AssignableExpression *>(expr))
  {
    syntax_error("Invalid left hand side in assignment expression ");
  }

  return static_cast<AssignableExpression *>(expr);
}

Expression *RecursiveDecentParser::parse_or_expr()
{
  set_start();

  Expression *left = parse_and_expr();

  while (lookahead(TokenType::OR_KW))
  {
    match(TokenType::OR_KW);

    Expression *right = parse_and_expr();

    set_end();

    left = new OrExpression(left, right, start_line, end_line, node_start, node_end);
  }

  return left;
}

Expression *RecursiveDecentParser::parse_and_expr()
{
  set_start();

  Expression *left = parse_equality_expr();

  while (lookahead(TokenType::AND_KW))
  {
    match(TokenType::AND_KW);

    Expression *right = parse_equality_expr();

    set_end();

    left = new AndExpression(left, right, start_line, end_line, node_start, node_end);
  }

  return left;
}

Expression *RecursiveDecentParser::parse_equality_expr()
{
  set_start();

  Expression *left = parse_relational_expr();

  while (lookahead(TokenType::IS_EQUAL_OP) || lookahead(TokenType::NOT_EQUAL_OP))
  {
    string op = current_token.value;

    match(op == "==" ? TokenType::IS_EQUAL_OP : TokenType::NOT_EQUAL_OP);

    Expression *right = parse_relational_expr();

    set_end();

    left = new EqualityExpression(left, right, op, start_line, end_line, node_start, node_end);
  }

  return left;
}

Expression *RecursiveDecentParser::parse_relational_expr()
{
  set_start();

  Expression *left = parse_additive_expr();

  while (lookahead(TokenType::LESS_THAN_OP) || lookahead(TokenType::LESS_EQUAL_OP) || lookahead(TokenType::GREATER_THAN_OP) || lookahead(TokenType::GREATER_EQUAL_OP))
  {
    string op = current_token.value;

    switch (current_token.type)
    {
    case TokenType::LESS_THAN_OP:
      match(TokenType::LESS_THAN_OP);
      break;

    case TokenType::LESS_EQUAL_OP:
      match(TokenType::LESS_EQUAL_OP);
      break;

    case TokenType::GREATER_THAN_OP:
      match(TokenType::GREATER_THAN_OP);
      break;

    case TokenType::GREATER_EQUAL_OP:
      match(TokenType::GREATER_EQUAL_OP);
      break;
    }

    Expression *right = parse_additive_expr();

    set_end();

    left = new RelationalExpression(left, right, op, start_line, end_line, node_start, node_end);
  }

  return left;
}

Expression *RecursiveDecentParser::parse_additive_expr()
{
  set_start();

  Expression *left = parse_multiplicative_expr();

  while (lookahead(TokenType::PLUS_OP) || lookahead(TokenType::MINUS_OP))
  {
    string op = current_token.value;

    match(op == "+" ? TokenType::PLUS_OP : TokenType::MINUS_OP);

    Expression *right = parse_multiplicative_expr();

    set_end();

    left = new AdditiveExpression(left, right, op, start_line, end_line, node_start, node_end);
  }

  return left;
}

Expression *RecursiveDecentParser::parse_multiplicative_expr()
{
  Expression *left = parse_unary_expr();

  while (lookahead(TokenType::MULT_OP) || lookahead(TokenType::DIVIDE_OP) || lookahead(TokenType::MOD_OP))
  {
    set_start();

    string op = current_token.value;

    switch (current_token.type)
    {
    case TokenType::MULT_OP:
      match(TokenType::MULT_OP);
      break;

    case TokenType::DIVIDE_OP:
      match(TokenType::DIVIDE_OP);
      break;

    case TokenType::MOD_OP:
      match(TokenType::MOD_OP);
      break;
    }

    Expression *right = parse_unary_expr();

    set_end();

    left = new MultiplicativeExpression(left, right, op, start_line, end_line, node_start, node_end);
  }

  return left;
}

Expression *RecursiveDecentParser::parse_unary_expr()
{
  set_start();

  if (lookahead(TokenType::MINUS_OP))
  {
    string op = current_token.value;
    match(TokenType::MINUS_OP);
    Expression *operand = parse_expr();

    set_end();

    return new UnaryExpression(operand, op, false, start_line, end_line, node_start, node_end);
  }

  if (lookahead(TokenType::INCREMENT_OP) || lookahead(TokenType::DECREMENT_OP))
  {
    string op = current_token.value;
    match(op == "++" ? TokenType::INCREMENT_OP : TokenType::DECREMENT_OP);
    Expression *operand = parse_expr();

    set_end();

    return new UnaryExpression(operand, op, false, start_line, end_line, node_start, node_end);
  }

  if (lookahead(TokenType::NOT_KW))
  {
    string op = current_token.value;
    match(TokenType::NOT_KW);
    Expression *operand = parse_index_expr();

    set_end();

    return new UnaryExpression(operand, op, false, start_line, end_line, node_start, node_end);
  }

  Expression *primary = parse_index_expr();

  if (lookahead(TokenType::INCREMENT_OP) || lookahead(TokenType::DECREMENT_OP))
  {
    string op = current_token.value;
    match(op == "++" ? TokenType::INCREMENT_OP : TokenType::DECREMENT_OP);

    set_end();

    return new UnaryExpression(primary, op, false, start_line, end_line, node_start, node_end);
  }

  return primary;
}

Expression *RecursiveDecentParser::parse_index_expr()
{
  set_start();

  Expression *base = parse_primary_expr();

  if (!lookahead(TokenType::LEFT_SQUARE_PR))
  {
    return base;
  }

  match(TokenType::LEFT_SQUARE_PR);
  Expression *index = parse_expr();
  match(TokenType::RIGHT_SQUARE_PR);

  set_end();

  return new IndexExpression(base, index, start_line, end_line, node_start, node_end);
}

Expression *RecursiveDecentParser::parse_primary_expr()
{
  if (lookahead(TokenType::LEFT_PR))
  {
    match(TokenType::LEFT_PR);
    Expression *expr = parse_expr();
    match(TokenType::RIGHT_PR);
    return expr;
  }

  return parse_literal();
}

Expression *RecursiveDecentParser::parse_call_expr(Identifier *id)
{
  set_start();

  vector<Expression *> args = {};

  match(TokenType::LEFT_PR);

  if (!lookahead(TokenType::RIGHT_PR))
  {
    args.push_back(parse_expr());

    while (lookahead(TokenType::COMMA_SY))
    {
      match(TokenType::COMMA_SY);
      args.push_back(parse_expr());
    }
  }

  match(TokenType::RIGHT_PR);

  set_end();

  return new CallFunctionExpression(id, args, start_line, end_line, node_start, node_end);
}

Literal *RecursiveDecentParser::parse_literal()
{
  switch (current_token.type)
  {
  case TokenType::INTEGER_NUM:
    return parse_int();

  case TokenType::FLOAT_NUM:
    return parse_float();

  case TokenType::STRING_SY:
    return parse_string();

  case TokenType::TRUE_KW:
  case TokenType::FALSE_KW:
    return parse_bool();

  case TokenType::LEFT_CURLY_PR:
    return parse_array();

  default:
    Identifier *id = parse_identifier();

    if (!lookahead(TokenType::LEFT_PR))
    {
      return (Literal *)id;
    }

    return (Literal *)parse_call_expr(id);
  }
}

Identifier *RecursiveDecentParser::parse_identifier()
{
  set_start();

  string name = current_token.value;
  match(TokenType::ID_SY);

  set_end();

  return new Identifier(name, start_line, end_line, node_start, node_end);
}

IntegerLiteral *RecursiveDecentParser::parse_int()
{
  set_start();

  int value = stoi(current_token.value);
  match(TokenType::INTEGER_NUM);

  set_end();
  return new IntegerLiteral(value, start_line, end_line, node_start, node_end);
}

FloatLiteral *RecursiveDecentParser::parse_float()
{
  set_start();

  float value = stof(current_token.value);
  match(TokenType::FLOAT_NUM);

  set_end();
  return new FloatLiteral(value, start_line, end_line, node_start, node_end);
}

StringLiteral *RecursiveDecentParser::parse_string()
{
  set_start();

  string value = current_token.value;
  match(TokenType::STRING_SY);

  set_end();
  return new StringLiteral(value, start_line, end_line, node_start, node_end);
}

BooleanLiteral *RecursiveDecentParser::parse_bool()
{
  set_start();

  bool value = current_token.value == "true";
  if (value)
  {
    match(TokenType::TRUE_KW);
  }
  else
  {
    match(TokenType::FALSE_KW);
  }

  set_end();
  return new BooleanLiteral(value, start_line, end_line, node_start, node_end);
}

ArrayLiteral *RecursiveDecentParser::parse_array()
{
  set_start();

  vector<Expression *> elements = {};

  match(TokenType::LEFT_CURLY_PR);

  if (!lookahead(TokenType::RIGHT_CURLY_PR))
  {
    elements.push_back(parse_or_expr());

    while (lookahead(TokenType::COMMA_SY))
    {
      match(TokenType::COMMA_SY);
      elements.push_back(parse_or_expr());
    }
  }

  match(TokenType::RIGHT_CURLY_PR);

  set_end();
  return new ArrayLiteral(elements, start_line, end_line, node_start, node_end);
}
