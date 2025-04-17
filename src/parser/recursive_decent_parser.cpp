#include "recursive_decent_parser.h"

RecursiveDecentParser::RecursiveDecentParser(ScannerBase *sc) : ParserBase(sc) {}

AstNode *RecursiveDecentParser::parse_ast()
{
  AstNode *ast_tree = parse_source();
  reset_parser();

  return ast_tree;
}

Source *RecursiveDecentParser::parse_source()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  vector<FunctionDefinition *> funcs = {};

  while (lookahead(TokenType::FUNC_KW))
  {
    funcs.push_back(parse_function());
  }

  ProgramDefinition *program = parse_program();

  match(TokenType::END_OF_FILE);

  int node_end = previous_token.end;
  int end_line = previous_token.line;
  return new Source(program, funcs, start_line, end_line, node_start, node_end);
}

FunctionDefinition *RecursiveDecentParser::parse_function()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

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

  int node_end = previous_token.end;
  int end_line = previous_token.line;
  return new FunctionDefinition(id, return_type, params, body, start_line, end_line, node_start, node_end);
}

ProgramDefinition *RecursiveDecentParser::parse_program()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

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

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new ProgramDefinition(id, globals, body, start_line, end_line, node_start, node_end);
}

DataType *RecursiveDecentParser::parse_data_type()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  string datatype;
  int dim = 0;

  if (!lookahead(TokenType::LEFT_SQUARE_PR) && AstNode::is_data_type(current_token.type))
  {
    datatype = current_token.value;
    match(current_token.type);

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new PrimitiveDataType(datatype, start_line, end_line, node_start, node_end);
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

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new ArrayDataType(datatype, dim, start_line, end_line, node_start, node_end);
}

ReturnType *RecursiveDecentParser::parse_return_type()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  DataType *return_type;

  string datatype;
  int dim = 0;

  if (!lookahead(TokenType::LEFT_SQUARE_PR) && AstNode::is_return_type(current_token.type))
  {
    datatype = current_token.value;
    match(current_token.type);

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return_type = new PrimitiveDataType(datatype, start_line, end_line, node_start, node_end);
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

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return_type = new ArrayDataType(datatype, dim, start_line, end_line, node_start, node_end);
  return new ReturnType(return_type, start_line, end_line, node_start, node_end);
}

VariableDefinition *RecursiveDecentParser::parse_var_def()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

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

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      Statement *declaration = new VariableDeclaration(variables, datatype, start_line, end_line, node_start, node_end);

      return new VariableDefinition(declaration, start_line, end_line, node_start, node_end);
    }
    else if (lookahead(TokenType::EQUAL_OP))
    {
      match(TokenType::EQUAL_OP);

      Expression *initializer;

      if (lookahead(TokenType::LEFT_CURLY_PR))
      {
        initializer = (Expression *)parse_array();
      }
      else
      {
        initializer = parse_or_expr();
      }

      match(TokenType::SEMI_COLON_SY);

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      Statement *initialization = new VariableInitialization(variables[0], datatype, initializer, start_line, end_line, node_start, node_end);

      return new VariableDefinition(initialization, start_line, end_line, node_start, node_end);
    }
  }

  match(TokenType::COLON_SY);

  datatype = parse_data_type();

  match(TokenType::SEMI_COLON_SY);

  int node_end = previous_token.end;
  int end_line = previous_token.line;

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
  int node_start = current_token.start;
  int start_line = current_token.line;

  match(TokenType::SKIP_KW);
  match(TokenType::SEMI_COLON_SY);

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new SkipStatement(start_line, end_line, node_start, node_end);
}

StopStatement *RecursiveDecentParser::parse_stop_expr()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  match(TokenType::STOP_KW);
  match(TokenType::SEMI_COLON_SY);

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new StopStatement(start_line, end_line, node_start, node_end);
}

ReadStatement *RecursiveDecentParser::parse_read()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  vector<AssignableExpression *> variables;

  match(TokenType::READ_KW);
  Expression *expr = parse_expr();
  variables.push_back(parse_assignable_expr(expr));
  while (!lookahead(TokenType::SEMI_COLON_SY))
  {
    match(TokenType::COMMA_SY);
    expr = parse_expr();
    variables.push_back(parse_assignable_expr(expr));
  }
  match(TokenType::SEMI_COLON_SY);

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new ReadStatement(variables, start_line, end_line, node_start, node_end);
}

WriteStatement *RecursiveDecentParser::parse_write()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  vector<Expression *> args;
  match(TokenType::WRITE_KW);

  args.push_back(parse_or_expr());

  while (!lookahead(TokenType::SEMI_COLON_SY))
  {
    match(TokenType::COMMA_SY);
    args.push_back(parse_or_expr());
  }
  match(TokenType::SEMI_COLON_SY);

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new WriteStatement(args, start_line, end_line, node_start, node_end);
}

ReturnStatement *RecursiveDecentParser::parse_return_stmt()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  Expression *val = nullptr;
  match(TokenType::RETURN_KW);

  if (!lookahead(TokenType::SEMI_COLON_SY))
  {
    if (lookahead(TokenType::LEFT_CURLY_PR))
    {
      val = (Expression *)parse_array();
    }
    else
    {
      val = parse_or_expr();
    }
  }

  match(TokenType::SEMI_COLON_SY);

  int node_end = previous_token.end;
  int end_line = previous_token.line;
  return new ReturnStatement(val, start_line, end_line, node_start, node_end);
}

IfStatement *RecursiveDecentParser::parse_if()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

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
    int node_end = previous_token.end;
    int end_line = previous_token.line;

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

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new IfStatement(condition, consequent, alternate, start_line, end_line, node_start, node_end);
}

ForLoop *RecursiveDecentParser::parse_for()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

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

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new ForLoop(init, condition, update, body, start_line, end_line, node_start, node_end);
}

AssignmentExpression *RecursiveDecentParser::parse_int_assign()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  Identifier *id = parse_identifier();

  match(TokenType::EQUAL_OP);

  Expression *val = parse_or_expr();

  int node_end = previous_token.end;
  int end_line = previous_token.line;
  return new AssignmentExpression(id, val, start_line, end_line, node_start, node_end);
}

WhileLoop *RecursiveDecentParser::parse_while()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

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

  int node_end = previous_token.end;
  int end_line = previous_token.line;
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
  int node_start = current_token.start;
  int start_line = current_token.line;

  Expression *left = parse_or_expr();

  if (!lookahead(TokenType::EQUAL_OP))
  {
    return left;
  }

  AssignableExpression *assignee = parse_assignable_expr(left);

  match(TokenType::EQUAL_OP);

  Expression *value;

  if (lookahead(TokenType::LEFT_CURLY_PR))
  {
    value = (Expression *)parse_array();
  }
  else
  {
    value = parse_expr();
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;
  return new AssignmentExpression(assignee, value, start_line, end_line, node_start, node_end);
  ;
}

AssignableExpression *RecursiveDecentParser::parse_assignable_expr(Expression *expr)
{
  AssignableExpression *assignable = dynamic_cast<AssignableExpression *>(expr);
  if (!assignable)
  {
    syntax_error("Invalid left hand side in assignment expression");
  }
  return assignable;
}

Expression *RecursiveDecentParser::parse_or_expr()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  Expression *left = parse_and_expr();

  while (lookahead(TokenType::OR_KW))
  {
    match(TokenType::OR_KW);

    Expression *right = parse_and_expr();

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    left = new OrExpression(left, right, start_line, end_line, node_start, node_end);
  }

  return left;
}

Expression *RecursiveDecentParser::parse_and_expr()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  Expression *left = parse_equality_expr();

  while (lookahead(TokenType::AND_KW))
  {
    match(TokenType::AND_KW);

    Expression *right = parse_equality_expr();

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    left = new AndExpression(left, right, start_line, end_line, node_start, node_end);
  }

  return left;
}

Expression *RecursiveDecentParser::parse_equality_expr()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  Expression *left = parse_relational_expr();

  while (lookahead(TokenType::IS_EQUAL_OP) || lookahead(TokenType::NOT_EQUAL_OP))
  {
    string op = current_token.value;

    match(op == "==" ? TokenType::IS_EQUAL_OP : TokenType::NOT_EQUAL_OP);

    Expression *right = parse_relational_expr();

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    left = new EqualityExpression(left, right, op, start_line, end_line, node_start, node_end);
  }

  return left;
}

Expression *RecursiveDecentParser::parse_relational_expr()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

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

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    left = new RelationalExpression(left, right, op, start_line, end_line, node_start, node_end);
  }

  return left;
}

Expression *RecursiveDecentParser::parse_additive_expr()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  Expression *left = parse_multiplicative_expr();

  while (lookahead(TokenType::PLUS_OP) || lookahead(TokenType::MINUS_OP))
  {
    string op = current_token.value;

    match(op == "+" ? TokenType::PLUS_OP : TokenType::MINUS_OP);

    Expression *right = parse_multiplicative_expr();

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    left = new AdditiveExpression(left, right, op, start_line, end_line, node_start, node_end);
  }

  return left;
}

Expression *RecursiveDecentParser::parse_multiplicative_expr()
{
  Expression *left = parse_unary_expr();

  while (lookahead(TokenType::MULT_OP) || lookahead(TokenType::DIVIDE_OP) || lookahead(TokenType::MOD_OP))
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

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

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    left = new MultiplicativeExpression(left, right, op, start_line, end_line, node_start, node_end);
  }

  return left;
}

Expression *RecursiveDecentParser::parse_unary_expr()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  if (lookahead(TokenType::MINUS_OP))
  {
    string op = current_token.value;
    match(TokenType::MINUS_OP);
    Expression *operand = parse_expr();

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new UnaryExpression(operand, op, false, start_line, end_line, node_start, node_end);
  }

  if (lookahead(TokenType::STRINGIFY_OP))
  {
    string op = current_token.value;
    match(TokenType::STRINGIFY_OP);
    Expression *operand = parse_expr();

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new UnaryExpression(operand, op, false, start_line, end_line, node_start, node_end);
  }

  if (lookahead(TokenType::BOOLEAN_OP))
  {
    string op = current_token.value;
    match(TokenType::BOOLEAN_OP);
    Expression *operand = parse_expr();

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new UnaryExpression(operand, op, false, start_line, end_line, node_start, node_end);
  }

  if (lookahead(TokenType::ROUND_OP))
  {
    string op = current_token.value;
    match(TokenType::ROUND_OP);
    Expression *operand = parse_expr();

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new UnaryExpression(operand, op, false, start_line, end_line, node_start, node_end);
  }
  if (lookahead(TokenType::LENGTH_OP))
  {
    string op = current_token.value;
    match(TokenType::LENGTH_OP);
    Expression *operand = parse_expr();

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new UnaryExpression(operand, op, false, start_line, end_line, node_start, node_end);
  }

  if (lookahead(TokenType::INCREMENT_OP) || lookahead(TokenType::DECREMENT_OP))
  {
    string op = current_token.value;
    match(op == "++" ? TokenType::INCREMENT_OP : TokenType::DECREMENT_OP);
    Expression *operand = parse_assignable_expr(parse_index_expr());

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new UnaryExpression(operand, op, false, start_line, end_line, node_start, node_end);
  }

  if (lookahead(TokenType::NOT_KW))
  {
    string op = current_token.value;
    match(TokenType::NOT_KW);
    Expression *operand = parse_index_expr();

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new UnaryExpression(operand, op, false, start_line, end_line, node_start, node_end);
  }

  Expression *primary = parse_index_expr();

  if (lookahead(TokenType::INCREMENT_OP) || lookahead(TokenType::DECREMENT_OP))
  {
    parse_assignable_expr(primary);
    string op = current_token.value;
    match(op == "++" ? TokenType::INCREMENT_OP : TokenType::DECREMENT_OP);

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new UnaryExpression(primary, op, true, start_line, end_line, node_start, node_end);
  }

  return primary;
}

Expression *RecursiveDecentParser::parse_index_expr()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  Expression *base = parse_primary_expr();

  while (lookahead(TokenType::LEFT_SQUARE_PR))
  {
    ItertableExpression *base_index = parse_itertable_expr(base);

    match(TokenType::LEFT_SQUARE_PR);
    Expression *index = parse_expr();
    match(TokenType::RIGHT_SQUARE_PR);

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    base = new IndexExpression(base_index, index, start_line, end_line, node_start, node_end);
  }

  return base;
}

ItertableExpression *RecursiveDecentParser::parse_itertable_expr(Expression *expr)
{
  ItertableExpression *Itertable = dynamic_cast<ItertableExpression *>(expr);
  if (!Itertable)
  {
    syntax_error("Invalid base in index expression");
  }
  return Itertable;
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
  int node_start = current_token.start;
  int start_line = current_token.line;

  vector<Expression *> args = {};

  match(TokenType::LEFT_PR);

  if (!lookahead(TokenType::RIGHT_PR))
  {
    args.push_back(parse_or_expr());

    while (lookahead(TokenType::COMMA_SY))
    {
      match(TokenType::COMMA_SY);
      args.push_back(parse_or_expr());
    }
  }

  match(TokenType::RIGHT_PR);

  int node_end = previous_token.end;
  int end_line = previous_token.line;

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
  int node_start = current_token.start;
  int start_line = current_token.line;

  string name = current_token.value;
  match(TokenType::ID_SY);

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new Identifier(name, start_line, end_line, node_start, node_end);
}

IntegerLiteral *RecursiveDecentParser::parse_int()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  int value = stoi(current_token.value);
  match(TokenType::INTEGER_NUM);

  int node_end = previous_token.end;
  int end_line = previous_token.line;
  return new IntegerLiteral(value, start_line, end_line, node_start, node_end);
}

FloatLiteral *RecursiveDecentParser::parse_float()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  float value = stof(current_token.value);
  match(TokenType::FLOAT_NUM);

  int node_end = previous_token.end;
  int end_line = previous_token.line;
  return new FloatLiteral(value, start_line, end_line, node_start, node_end);
}

StringLiteral *RecursiveDecentParser::parse_string()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  string value = current_token.value;
  match(TokenType::STRING_SY);

  int node_end = previous_token.end;
  int end_line = previous_token.line;
  return new StringLiteral(value, start_line, end_line, node_start, node_end);
}

BooleanLiteral *RecursiveDecentParser::parse_bool()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  bool value = current_token.value == "true";
  if (value)
  {
    match(TokenType::TRUE_KW);
  }
  else
  {
    match(TokenType::FALSE_KW);
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;
  return new BooleanLiteral(value, start_line, end_line, node_start, node_end);
}

ArrayLiteral *RecursiveDecentParser::parse_array()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  vector<Expression *> elements = {};

  match(TokenType::LEFT_CURLY_PR);

  if (!lookahead(TokenType::LEFT_CURLY_PR))
  {
    elements = parse_array_value();

    match(TokenType::RIGHT_CURLY_PR);

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ArrayLiteral(elements, start_line, end_line, node_start, node_end);
  }

  elements.push_back(parse_array());

  while (!lookahead(TokenType::RIGHT_CURLY_PR))
  {
    match(TokenType::COMMA_SY);
    elements.push_back(parse_array());
  }

  match(TokenType::RIGHT_CURLY_PR);

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new ArrayLiteral(elements, start_line, end_line, node_start, node_end);
}

vector<Expression *> RecursiveDecentParser::parse_array_value()
{
  vector<Expression *> elements = {};

  if (!lookahead(TokenType::RIGHT_CURLY_PR))
  {
    elements.push_back(parse_or_expr());

    while (lookahead(TokenType::COMMA_SY))
    {
      match(TokenType::COMMA_SY);
      elements.push_back(parse_or_expr());
    }
  }

  return elements;
}
