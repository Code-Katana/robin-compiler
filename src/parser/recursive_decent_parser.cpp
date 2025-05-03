#include "recursive_decent_parser.h"

RecursiveDecentParser::RecursiveDecentParser(ScannerBase *sc) : ParserBase(sc) {}

bool RecursiveDecentParser::match(TokenType type)
{
  if (current_token.type == type)
  {
    previous_token = current_token;
    current_token = sc->get_token();
    return true;
  }
  return false;
}

AstNode *RecursiveDecentParser::parse_ast()
{

  AstNode *ast_tree = parse_source();
  reset_parser();
  if (has_error)
  {

    return error_node;
  }

  return ast_tree;
}

Source *RecursiveDecentParser::parse_source()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  vector<FunctionDefinition *> funcs = {};


  while (lookahead(TokenType::FUNC_KW))
  {
    FunctionDefinition *func = parse_function();
    if (!func)
    {
      syntax_error("Invalid function definition");
      return nullptr;
    }
    funcs.push_back(func);
  }

  ProgramDefinition *program = parse_program();
  if (!program)
  {
    syntax_error("Invalid program definition");
    return nullptr;
  }

  if (!match(TokenType::END_OF_FILE))
  {
    syntax_error("Expected end of file after program");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new Source(program, funcs, start_line, end_line, node_start, node_end);
}

FunctionDefinition *RecursiveDecentParser::parse_function()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  if (!match(TokenType::FUNC_KW))
  {
    syntax_error("Expected 'FUNC' keyword at start of function");
    return nullptr;
  }

  ReturnType *return_type = parse_return_type();
  if (!return_type)
  {
    syntax_error("Expected return type after 'FUNC'");
    return nullptr;
  }

  Identifier *id = parse_identifier();
  if (!id)
  {
    syntax_error("Expected function name after return type");
    return nullptr;
  }

  if (!match(TokenType::HAS_KW))
  {
    syntax_error("Expected 'HAS' keyword after function name: " + id->name);
    return nullptr;
  }

  vector<VariableDefinition *> params;
  while (!lookahead(TokenType::BEGIN_KW) && !lookahead(TokenType::END_OF_FILE))
  {
    VariableDefinition *param = parse_var_def();
    if (!param)
    {
      syntax_error("Invalid parameter definition in function: " + id->name);
      return nullptr;
    }
    params.push_back(param);
  }

  if (!match(TokenType::BEGIN_KW))
  {
    syntax_error("Expected 'BEGIN' keyword before function body: " + id->name);
    return nullptr;
  }

  vector<Statement *> body;
  while (!lookahead(TokenType::END_KW) && !lookahead(TokenType::END_OF_FILE))
  {
    Statement *stmt = parse_command();
    if (!stmt)
    {
      syntax_error("Invalid statement inside function body: " + id->name);
      return nullptr;
    }
    body.push_back(stmt);
  }

  if (!match(TokenType::END_KW))
  {
    syntax_error("Expected 'END' keyword after function body: " + id->name);
    return nullptr;
  }

  if (!match(TokenType::FUNC_KW))
  {
    syntax_error("Expected 'FUNC' keyword after 'END' to close function: " + id->name);
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;


  return new FunctionDefinition(id, return_type, params, body, start_line, end_line, node_start, node_end);
}

ProgramDefinition *RecursiveDecentParser::parse_program()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  if (!match(TokenType::PROGRAM_KW))
  {
    syntax_error("Expected 'PROGRAM' keyword at start of program");
    return nullptr;
  }

  Identifier *id = parse_identifier();

  if (!id)
  {
    syntax_error("Expected Program name after Program type");
    return nullptr;
  }

  if (!match(TokenType::IS_KW))
  {
    syntax_error("Expected 'IS' keyword after program name: " + id->name);
    return nullptr;
  }

  vector<VariableDefinition *> globals;
  while (!lookahead(TokenType::BEGIN_KW))
  {
    VariableDefinition *global = parse_var_def();
    if (!global)
    {
      syntax_error("Invalid Global Variable definition in program: " + id->name);
      return nullptr;
    }
    globals.push_back(global);
  }

  if (!match(TokenType::BEGIN_KW))
  {
    syntax_error("Expected 'BEGIN' keyword before program body: " + id->name);
    return nullptr;
  }

  vector<Statement *> body;
  while (!lookahead(TokenType::END_KW))
  {
    Statement *stmt = parse_command();
    if (!stmt)
    {
      syntax_error("Invalid statement inside program body: " + id->name);
      return nullptr;
    }
    body.push_back(stmt);
  }

  if (!match(TokenType::END_KW))
  {
    syntax_error("Expected 'END' keyword after program body: " + id->name);
    return nullptr;
  }

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
    if (!match(current_token.type))
    {
      syntax_error("Expected primitive data type : " + datatype);
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new PrimitiveDataType(datatype, start_line, end_line, node_start, node_end);
  }

  if (!match(TokenType::LEFT_SQUARE_PR))
  {
    syntax_error("Expected '[' to start array type");
    return nullptr;
  }
  ++dim;

  while (lookahead(TokenType::LEFT_SQUARE_PR))
  {
    if (!match(TokenType::LEFT_SQUARE_PR))
    {
      syntax_error("Expected '[' for array dimension");
      return nullptr;
    }
    ++dim;
  }

  if (AstNode::is_data_type(current_token.type))
  {
    datatype = current_token.value;
    if (!match(current_token.type))
    {
      syntax_error("Expected primitive type inside array : " + datatype);
      return nullptr;
    }
  }
  else
  {
    syntax_error("Expected primitive type inside array insted of : " + datatype);
    return nullptr;
  }

  for (int i = 0; i < dim; ++i)
  {
    if (!match(TokenType::RIGHT_SQUARE_PR))
    {
      syntax_error("Expected ']' to close array type");
      return nullptr;
    }
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new ArrayDataType(datatype, dim, start_line, end_line, node_start, node_end);
}

ReturnType *RecursiveDecentParser::parse_return_type()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  DataType *return_type = nullptr;
  string datatype;
  int dim = 0;

  // Case 1: Primitive return type
  if (!lookahead(TokenType::LEFT_SQUARE_PR) && AstNode::is_return_type(current_token.type))
  {
    datatype = current_token.value;

    if (!match(current_token.type))
    {
      syntax_error("Expected primitive return type : " + datatype);
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return_type = new PrimitiveDataType(datatype, start_line, end_line, node_start, node_end);
    return new ReturnType(return_type, start_line, end_line, node_start, node_end);
  }

  // Case 2: Array return type
  if (!match(TokenType::LEFT_SQUARE_PR))
  {
    syntax_error("Expected '[' to start array return type");
    return nullptr;
  }
  ++dim;

  while (lookahead(TokenType::LEFT_SQUARE_PR))
  {
    if (!match(TokenType::LEFT_SQUARE_PR))
    {
      syntax_error("Expected '[' for array dimension");
      return nullptr;
    }
    ++dim;
  }

  if (AstNode::is_data_type(current_token.type))
  {
    datatype = current_token.value;

    if (!match(current_token.type))
    {
      syntax_error("Expected primitive type inside array return type : " + datatype);
      return nullptr;
    }
  }
  else
  {
    syntax_error("Expected primitive type inside array return type insted of : " + datatype);
    return nullptr;
  }

  for (int i = 0; i < dim; ++i)
  {
    if (!match(TokenType::RIGHT_SQUARE_PR))
    {
      syntax_error("Expected ']' to close array return type");
      return nullptr;
    }
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
  DataType *datatype = nullptr;

  if (!match(TokenType::VAR_KW))
  {
    syntax_error("Expected 'VAR' keyword at start of variable definition");
    return nullptr;
  }

  Identifier *first_var = parse_identifier();
  if (!first_var)
  {
    syntax_error("Expected identifier after 'VAR'");
    return nullptr;
  }
  variables.push_back(first_var);

  while (lookahead(TokenType::COMMA_SY))
  {
    if (!match(TokenType::COMMA_SY))
    {
      syntax_error("Expected ',' between variable names");
      return nullptr;
    }

    Identifier *var = parse_identifier();
    if (!var)
    {
      syntax_error("Expected identifier after ','");
      return nullptr;
    }
    variables.push_back(var);
  }

  if (variables.size() == 1)
  {
    if (!match(TokenType::COLON_SY))
    {
      syntax_error("Expected ':' after variable name" + variables.back()->name);
      return nullptr;
    }

    datatype = parse_data_type();
    if (!datatype)
    {
      syntax_error("Expected data type after ':'");
      return nullptr;
    }

    if (lookahead(TokenType::SEMI_COLON_SY))
    {
      if (!match(TokenType::SEMI_COLON_SY))
      {
        syntax_error("Expected ';' after variable declaration");
        return nullptr;
      }

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      Statement *declaration = new VariableDeclaration(variables, datatype, start_line, end_line, node_start, node_end);
      return new VariableDefinition(declaration, start_line, end_line, node_start, node_end);
    }
    else if (lookahead(TokenType::EQUAL_OP))
    {
      if (!match(TokenType::EQUAL_OP))
      {
        syntax_error("Expected '=' for variable initialization");
        return nullptr;
      }

      Expression *initializer = nullptr;

      if (lookahead(TokenType::LEFT_CURLY_PR))
      {
        initializer = parse_array();
        if (!initializer)
        {
          syntax_error("Expected '{' for array literal");
          return nullptr;
        }
      }
      else
      {
        initializer = parse_or_expr();
        if (!initializer)
        {
          syntax_error("Expected a literal");
          return nullptr;
        }
      }

      if (!match(TokenType::SEMI_COLON_SY))
      {
        syntax_error("Expected ';' after variable initialization");
        return nullptr;
      }

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      Statement *initialization = new VariableInitialization(variables[0], datatype, initializer, start_line, end_line, node_start, node_end);
      return new VariableDefinition(initialization, start_line, end_line, node_start, node_end);
    }
    else
    {
      syntax_error("Expected ';' or '=' after variable declaration");
      return nullptr;
    }
  }

  // Multiple variables case
  if (!match(TokenType::COLON_SY))
  {
    syntax_error("Expected ':' after variable names");
    return nullptr;
  }

  datatype = parse_data_type();
  if (!datatype)
  {
    syntax_error("Expected data type after ':'");
    return nullptr;
  }

  if (!match(TokenType::SEMI_COLON_SY))
  {
    syntax_error("Expected ';' after variable declaration");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  Statement *declaration = new VariableDeclaration(variables, datatype, start_line, end_line, node_start, node_end);
  return new VariableDefinition(declaration, start_line, end_line, node_start, node_end);
}

Statement *RecursiveDecentParser::parse_command()
{
  Statement *stmt = nullptr;

  if (lookahead(TokenType::SKIP_KW))
  {
    stmt = parse_skip_expr();
  }
  else if (lookahead(TokenType::STOP_KW))
  {
    stmt = parse_stop_expr();
  }
  else if (lookahead(TokenType::WRITE_KW))
  {
    stmt = parse_write();
  }
  else if (lookahead(TokenType::READ_KW))
  {
    stmt = parse_read();
  }
  else if (lookahead(TokenType::IF_KW))
  {
    stmt = parse_if();
  }
  else if (lookahead(TokenType::FOR_KW))
  {
    stmt = parse_for();
  }
  else if (lookahead(TokenType::WHILE_KW))
  {
    stmt = parse_while();
  }
  else if (lookahead(TokenType::RETURN_KW))
  {
    stmt = parse_return_stmt();
  }
  else if (lookahead(TokenType::VAR_KW))
  {
    stmt = parse_var_def();
  }
  else
  {
    stmt = parse_expr_stmt();
  }

  if (!stmt)
  {
    syntax_error("Invalid command or statement");
    return nullptr;
  }

  return stmt;
}

SkipStatement *RecursiveDecentParser::parse_skip_expr()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  if (!match(TokenType::SKIP_KW))
  {
    syntax_error("Expected 'SKIP' keyword");
    return nullptr;
  }

  if (!match(TokenType::SEMI_COLON_SY))
  {
    syntax_error("Expected ';' after 'SKIP'");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new SkipStatement(start_line, end_line, node_start, node_end);
}

StopStatement *RecursiveDecentParser::parse_stop_expr()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  if (!match(TokenType::STOP_KW))
  {
    syntax_error("Expected 'STOP' keyword");
    return nullptr;
  }

  if (!match(TokenType::SEMI_COLON_SY))
  {
    syntax_error("Expected ';' after 'STOP'");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new StopStatement(start_line, end_line, node_start, node_end);
}

ReadStatement *RecursiveDecentParser::parse_read()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  vector<AssignableExpression *> variables;

  if (!match(TokenType::READ_KW))
  {
    syntax_error("Expected 'READ' keyword");
    return nullptr;
  }

  Expression *expr = parse_expr();
  if (!expr)
  {
    syntax_error("Expected expression after 'READ'");
    return nullptr;
  }

  AssignableExpression *assignable = parse_assignable_expr(expr);
  if (!assignable)
  {
    syntax_error("Expected assignable expression after 'READ'");
    return nullptr;
  }
  variables.push_back(assignable);

  while (!lookahead(TokenType::SEMI_COLON_SY))
  {
    if (!match(TokenType::COMMA_SY))
    {
      syntax_error("Expected ',' between variables in 'READ' statement");
      return nullptr;
    }

    expr = parse_expr();
    if (!expr)
    {
      syntax_error("Expected expression after ',' in 'READ' statement");
      return nullptr;
    }

    assignable = parse_assignable_expr(expr);
    if (!assignable)
    {
      syntax_error("Expected assignable expression after ',' in 'READ' statement");
      return nullptr;
    }

    variables.push_back(assignable);
  }

  if (!match(TokenType::SEMI_COLON_SY))
  {
    syntax_error("Expected ';' at the end of 'READ' statement");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new ReadStatement(variables, start_line, end_line, node_start, node_end);
}

WriteStatement *RecursiveDecentParser::parse_write()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  vector<Expression *> args;

  if (!match(TokenType::WRITE_KW))
  {
    syntax_error("Expected 'WRITE' keyword");
    return nullptr;
  }

  Expression *expr = parse_or_expr();
  if (!expr)
  {
    syntax_error("Expected expression after 'WRITE'");
    return nullptr;
  }
  args.push_back(expr);

  while (!lookahead(TokenType::SEMI_COLON_SY))
  {
    if (!match(TokenType::COMMA_SY))
    {
      syntax_error("Expected ',' between expressions in 'WRITE' statement");
      return nullptr;
    }

    expr = parse_or_expr();
    if (!expr)
    {
      syntax_error("Expected expression after ',' in 'WRITE' statement");
      return nullptr;
    }
    args.push_back(expr);
  }

  if (!match(TokenType::SEMI_COLON_SY))
  {
    syntax_error("Expected ';' at the end of 'WRITE' statement");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new WriteStatement(args, start_line, end_line, node_start, node_end);
}

ReturnStatement *RecursiveDecentParser::parse_return_stmt()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  Expression *val = nullptr;

  if (!match(TokenType::RETURN_KW))
  {
    syntax_error("Expected 'RETURN' keyword");
    return nullptr;
  }

  if (!lookahead(TokenType::SEMI_COLON_SY))
  {
    val = parse_or_expr();
    if (!val)
    {
      syntax_error("Expected expression after 'RETURN'");
      return nullptr;
    }
  }

  if (!match(TokenType::SEMI_COLON_SY))
  {
    syntax_error("Expected ';' after 'RETURN' statement");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new ReturnStatement(val, start_line, end_line, node_start, node_end);
}

IfStatement *RecursiveDecentParser::parse_if()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  Expression *condition = nullptr;
  vector<Statement *> consequent;
  vector<Statement *> alternate;

  if (!match(TokenType::IF_KW))
  {
    syntax_error("Expected 'IF' keyword");
    return nullptr;
  }

  condition = parse_bool_expr();
  if (!condition)
  {
    syntax_error("Expected condition after 'IF'");
    return nullptr;
  }

  if (!match(TokenType::THEN_KW))
  {
    syntax_error("Expected 'THEN' after IF condition");
    return nullptr;
  }

  while (!lookahead(TokenType::ELSE_KW) && !lookahead(TokenType::END_KW) && !lookahead(TokenType::END_OF_FILE))
  {
    Statement *stmt = parse_command();
    if (!stmt)
    {
      syntax_error("Invalid statement inside IF consequent block");
      return nullptr;
    }
    consequent.push_back(stmt);
  }

  if (lookahead(TokenType::ELSE_KW))
  {
    if (!match(TokenType::ELSE_KW))
    {
      syntax_error("Expected 'ELSE' keyword");
      return nullptr;
    }

    if (lookahead(TokenType::IF_KW))
    {
      IfStatement *else_if = parse_if();
      if (!else_if)
      {
        syntax_error("Invalid ELSE IF block");
        return nullptr;
      }

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      alternate.push_back(else_if);
      return new IfStatement(condition, consequent, alternate, start_line, end_line, node_start, node_end);
    }

    while (!lookahead(TokenType::END_KW) && !lookahead(TokenType::END_OF_FILE))
    {
      Statement *stmt = parse_command();
      if (!stmt)
      {
        syntax_error("Invalid statement inside ELSE block");
        return nullptr;
      }
      alternate.push_back(stmt);
    }
  }

  if (!match(TokenType::END_KW))
  {
    syntax_error("Expected 'END' to close IF block");
    return nullptr;
  }

  if (!match(TokenType::IF_KW))
  {
    syntax_error("Expected 'IF' after 'END' to close IF block");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new IfStatement(condition, consequent, alternate, start_line, end_line, node_start, node_end);
}

ForLoop *RecursiveDecentParser::parse_for()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  AssignmentExpression *init = nullptr;
  Expression *condition = nullptr;
  Expression *update = nullptr;
  vector<Statement *> body;

  if (!match(TokenType::FOR_KW))
  {
    syntax_error("Expected 'FOR' keyword");
    return nullptr;
  }

  init = parse_int_assign();
  if (!init)
  {
    syntax_error("Expected initialization assignment after 'FOR'");
    return nullptr;
  }

  if (!match(TokenType::SEMI_COLON_SY))
  {
    syntax_error("Expected ';' after initialization in 'FOR' loop");
    return nullptr;
  }

  condition = parse_bool_expr();
  if (!condition)
  {
    syntax_error("Expected boolean condition after initialization in 'FOR' loop");
    return nullptr;
  }

  if (!match(TokenType::SEMI_COLON_SY))
  {
    syntax_error("Expected ';' after condition in 'FOR' loop");
    return nullptr;
  }

  update = parse_expr();
  if (!update)
  {
    syntax_error("Expected update expression after condition in 'FOR' loop");
    return nullptr;
  }

  if (!match(TokenType::DO_KW))
  {
    syntax_error("Expected 'DO' keyword after update expression in 'FOR' loop");
    return nullptr;
  }

  while (!lookahead(TokenType::END_KW) && !lookahead(TokenType::END_OF_FILE))
  {
    Statement *stmt = parse_command();
    if (!stmt)
    {
      syntax_error("Invalid statement inside 'FOR' loop body");
      return nullptr;
    }
    body.push_back(stmt);
  }

  if (!match(TokenType::END_KW))
  {
    syntax_error("Expected 'END' to close 'FOR' loop");
    return nullptr;
  }

  if (!match(TokenType::FOR_KW))
  {
    syntax_error("Expected 'FOR' after 'END' to close 'FOR' loop");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new ForLoop(init, condition, update, body, start_line, end_line, node_start, node_end);
}

AssignmentExpression *RecursiveDecentParser::parse_int_assign()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  Identifier *id = parse_identifier();
  if (!id)
  {
    syntax_error("Expected identifier in initialization of 'FOR' loop");
    return nullptr;
  }

  if (!match(TokenType::EQUAL_OP))
  {
    syntax_error("Expected '=' after identifier in 'FOR' loop initialization");
    return nullptr;
  }

  Expression *val = parse_or_expr();
  if (!val)
  {
    syntax_error("Expected expression after '=' in 'FOR' loop initialization");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new AssignmentExpression(id, val, start_line, end_line, node_start, node_end);
}

WhileLoop *RecursiveDecentParser::parse_while()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  Expression *condition = nullptr;
  vector<Statement *> body;

  if (!match(TokenType::WHILE_KW))
  {
    syntax_error("Expected 'WHILE' keyword");
    return nullptr;
  }

  condition = parse_bool_expr();
  if (!condition)
  {
    syntax_error("Expected boolean condition after 'WHILE'");
    return nullptr;
  }

  if (!match(TokenType::DO_KW))
  {
    syntax_error("Expected 'DO' keyword after 'WHILE' condition");
    return nullptr;
  }

  while (!lookahead(TokenType::END_KW) && !lookahead(TokenType::END_OF_FILE))
  {
    Statement *stmt = parse_command();
    if (!stmt)
    {
      syntax_error("Invalid statement inside 'WHILE' loop body");
      return nullptr;
    }
    body.push_back(stmt);
  }

  if (!match(TokenType::END_KW))
  {
    syntax_error("Expected 'END' to close 'WHILE' loop");
    return nullptr;
  }

  if (!match(TokenType::WHILE_KW))
  {
    syntax_error("Expected 'WHILE' after 'END' to close 'WHILE' loop");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new WhileLoop(condition, body, start_line, end_line, node_start, node_end);
}

Expression *RecursiveDecentParser::parse_bool_expr()
{
  Expression *expr = parse_or_expr();
  if (!expr)
  {
    syntax_error("Expected boolean expression");
    return nullptr;
  }
  AstNodeType type = expr->type;

  switch (type)
  {
  case AstNodeType::OrExpression:
  case AstNodeType::AndExpression:
  case AstNodeType::EqualityExpression:
  case AstNodeType::RelationalExpression:
  case AstNodeType::Identifier:
  case AstNodeType::IndexExpression:
  case AstNodeType::CallFunctionExpression:
  case AstNodeType::BooleanLiteral:
    return expr;

  default:
    syntax_error("Invalid expression in boolean context");
    return nullptr;
  }
}

Expression *RecursiveDecentParser::parse_expr_stmt()
{
  Expression *expr = parse_expr();
  if (!expr)
  {
    syntax_error("Expected expression in expression statement");
    return nullptr;
  }

  if (!match(TokenType::SEMI_COLON_SY))
  {
    syntax_error("Expected ';' after expression");
    return nullptr;
  }

  return expr;
}

Expression *RecursiveDecentParser::parse_expr()
{
  Expression *expr = parse_assign_expr();
  if (!expr)
  {
    syntax_error("Expected expression");
    return nullptr;
  }

  return expr;
}

Expression *RecursiveDecentParser::parse_assign_expr()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  Expression *left = parse_or_expr();
  if (!left)
  {
    syntax_error("Expected expression on the left-hand side of assignment");
    return nullptr;
  }

  if (!lookahead(TokenType::EQUAL_OP))
  {
    return left; // No assignment, just return the left expression
  }

  AssignableExpression *assignee = parse_assignable_expr(left);
  if (!assignee)
  {
    syntax_error("Invalid left-hand side in assignment");
    return nullptr;
  }

  if (!match(TokenType::EQUAL_OP))
  {
    syntax_error("Expected '=' in assignment expression");
    return nullptr;
  }

  Expression *value = nullptr;
  if (lookahead(TokenType::LEFT_CURLY_PR))
  {
    value = parse_array();
  }
  else
  {
    value = parse_expr();
  }

  if (!value)
  {
    syntax_error("Expected expression or array after '=' in assignment");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new AssignmentExpression(assignee, value, start_line, end_line, node_start, node_end);
}

AssignableExpression *RecursiveDecentParser::parse_assignable_expr(Expression *expr)
{
  if (!expr)
  {
    syntax_error("Expected an expression to assign to");
    return nullptr;
  }

  AssignableExpression *assignable = dynamic_cast<AssignableExpression *>(expr);
  if (!assignable)
  {
    syntax_error("Invalid left-hand side in assignment expression");
    return nullptr;
  }

  return assignable;
}

Expression *RecursiveDecentParser::parse_or_expr()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  Expression *left = parse_and_expr();
  if (!left)
  {
    syntax_error("Expected expression on the left-hand side of 'OR'");
    return nullptr;
  }

  while (lookahead(TokenType::OR_KW))
  {
    if (!match(TokenType::OR_KW))
    {
      syntax_error("Expected 'OR' operator");
      return nullptr;
    }

    Expression *right = parse_and_expr();
    if (!right)
    {
      syntax_error("Expected expression on the right-hand side of 'OR'");
      return nullptr;
    }

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
  if (!left)
  {
    syntax_error("Expected expression on the left-hand side of 'AND'");
    return nullptr;
  }

  while (lookahead(TokenType::AND_KW))
  {
    if (!match(TokenType::AND_KW))
    {
      syntax_error("Expected 'AND' operator");
      return nullptr;
    }

    Expression *right = parse_equality_expr();
    if (!right)
    {
      syntax_error("Expected expression on the right-hand side of 'AND'");
      return nullptr;
    }

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
  if (!left)
  {
    syntax_error("Expected expression on the left-hand side of equality operator");
    return nullptr;
  }

  while (lookahead(TokenType::IS_EQUAL_OP) || lookahead(TokenType::NOT_EQUAL_OP))
  {
    string op = current_token.value;

    if (op == "==")
    {
      if (!match(TokenType::IS_EQUAL_OP))
      {
        syntax_error("Expected '==' operator");
        return nullptr;
      }
    }
    else
    {
      if (!match(TokenType::NOT_EQUAL_OP))
      {
        syntax_error("Expected '<>' operator");
        return nullptr;
      }
    }

    Expression *right = parse_relational_expr();
    if (!right)
    {
      syntax_error("Expected expression on the right-hand side of equality operator");
      return nullptr;
    }

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
  if (!left)
  {
    syntax_error("Expected expression on the left-hand side of relational operator");
    return nullptr;
  }

  while (lookahead(TokenType::LESS_THAN_OP) || lookahead(TokenType::LESS_EQUAL_OP) ||
         lookahead(TokenType::GREATER_THAN_OP) || lookahead(TokenType::GREATER_EQUAL_OP))
  {
    string op = current_token.value;

    switch (current_token.type)
    {
    case TokenType::LESS_THAN_OP:
      if (!match(TokenType::LESS_THAN_OP))
      {
        syntax_error("Expected '<' operator");
        return nullptr;
      }
      break;

    case TokenType::LESS_EQUAL_OP:
      if (!match(TokenType::LESS_EQUAL_OP))
      {
        syntax_error("Expected '<=' operator");
        return nullptr;
      }
      break;

    case TokenType::GREATER_THAN_OP:
      if (!match(TokenType::GREATER_THAN_OP))
      {
        syntax_error("Expected '>' operator");
        return nullptr;
      }
      break;

    case TokenType::GREATER_EQUAL_OP:
      if (!match(TokenType::GREATER_EQUAL_OP))
      {
        syntax_error("Expected '>=' operator");
        return nullptr;
      }
      break;
    }

    Expression *right = parse_additive_expr();
    if (!right)
    {
      syntax_error("Expected expression on the right-hand side of relational operator");
      return nullptr;
    }

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
  if (!left)
  {
    syntax_error("Expected expression on the left-hand side of additive operator");
    return nullptr;
  }

  while (lookahead(TokenType::PLUS_OP) || lookahead(TokenType::MINUS_OP))
  {
    string op = current_token.value;

    if (op == "+")
    {
      if (!match(TokenType::PLUS_OP))
      {
        syntax_error("Expected '+' operator");
        return nullptr;
      }
    }
    else
    {
      if (!match(TokenType::MINUS_OP))
      {
        syntax_error("Expected '-' operator");
        return nullptr;
      }
    }

    Expression *right = parse_multiplicative_expr();
    if (!right)
    {
      syntax_error("Expected expression on the right-hand side of additive operator");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    left = new AdditiveExpression(left, right, op, start_line, end_line, node_start, node_end);
  }

  return left;
}

Expression *RecursiveDecentParser::parse_multiplicative_expr()
{
  Expression *left = parse_unary_expr();
  if (!left)
  {
    syntax_error("Expected expression on the left-hand side of multiplicative operator");
    return nullptr;
  }

  while (lookahead(TokenType::MULT_OP) || lookahead(TokenType::DIVIDE_OP) || lookahead(TokenType::MOD_OP))
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    string op = current_token.value;

    switch (current_token.type)
    {
    case TokenType::MULT_OP:
      if (!match(TokenType::MULT_OP))
      {
        syntax_error("Expected '*' operator");
        return nullptr;
      }
      break;

    case TokenType::DIVIDE_OP:
      if (!match(TokenType::DIVIDE_OP))
      {
        syntax_error("Expected '/' operator");
        return nullptr;
      }
      break;

    case TokenType::MOD_OP:
      if (!match(TokenType::MOD_OP))
      {
        syntax_error("Expected '%' operator");
        return nullptr;
      }
      break;
    }

    Expression *right = parse_unary_expr();
    if (!right)
    {
      syntax_error("Expected expression on the right-hand side of multiplicative operator");
      return nullptr;
    }

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

  if (lookahead(TokenType::MINUS_OP) || lookahead(TokenType::STRINGIFY_OP) ||
      lookahead(TokenType::BOOLEAN_OP) || lookahead(TokenType::ROUND_OP) ||
      lookahead(TokenType::LENGTH_OP))
  {
    string op = current_token.value;

    if (!match(current_token.type))
    {
      syntax_error("Expected unary operator");
      return nullptr;
    }

    Expression *operand = parse_index_expr();
    if (!operand)
    {
      syntax_error("Expected expression after unary operator");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new UnaryExpression(operand, op, false, start_line, end_line, node_start, node_end);
  }

  if (lookahead(TokenType::INCREMENT_OP) || lookahead(TokenType::DECREMENT_OP))
  {
    string op = current_token.value;

    if (!match(op == "++" ? TokenType::INCREMENT_OP : TokenType::DECREMENT_OP))
    {
      syntax_error("Expected '++' or '--' operator");
      return nullptr;
    }

    Expression *operand = parse_assignable_expr(parse_index_expr());
    if (!operand)
    {
      syntax_error("Expected assignable expression after '++' or '--'");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new UnaryExpression(operand, op, false, start_line, end_line, node_start, node_end);
  }

  if (lookahead(TokenType::NOT_KW))
  {
    string op = current_token.value;

    if (!match(TokenType::NOT_KW))
    {
      syntax_error("Expected 'NOT' keyword");
      return nullptr;
    }

    Expression *operand = parse_index_expr();
    if (!operand)
    {
      syntax_error("Expected expression after 'NOT'");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new UnaryExpression(operand, op, false, start_line, end_line, node_start, node_end);
  }

  // Parse primary expression
  Expression *primary = parse_index_expr();
  if (!primary)
  {
    syntax_error("Expected primary expression");
    return nullptr;
  }

  if (lookahead(TokenType::INCREMENT_OP) || lookahead(TokenType::DECREMENT_OP))
  {
    if (!parse_assignable_expr(primary))
    {
      syntax_error("Expected assignable expression before postfix '++' or '--'");
      return nullptr;
    }

    string op = current_token.value;

    if (!match(op == "++" ? TokenType::INCREMENT_OP : TokenType::DECREMENT_OP))
    {
      syntax_error("Expected postfix '++' or '--' operator");
      return nullptr;
    }

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
  if (!base)
  {
    syntax_error("Expected primary expression for indexing");
    return nullptr;
  }

  while (lookahead(TokenType::LEFT_SQUARE_PR))
  {
    if (!match(TokenType::LEFT_SQUARE_PR))
    {
      syntax_error("Expected '[' to start index expression");
      return nullptr;
    }

    Expression *index = parse_expr();
    if (!index)
    {
      syntax_error("Expected expression inside index brackets");
      return nullptr;
    }

    if (!match(TokenType::RIGHT_SQUARE_PR))
    {
      syntax_error("Expected ']' to close index expression");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    base = new IndexExpression(base, index, start_line, end_line, node_start, node_end);
  }

  return base;
}

Expression *RecursiveDecentParser::parse_primary_expr()
{
  if (lookahead(TokenType::LEFT_PR))
  {
    if (!match(TokenType::LEFT_PR))
    {
      syntax_error("Expected '(' to start grouped expression");
      return nullptr;
    }

    Expression *expr = parse_expr();
    if (!expr)
    {
      syntax_error("Expected expression inside parentheses");
      return nullptr;
    }

    if (!match(TokenType::RIGHT_PR))
    {
      syntax_error("Expected ')' to close grouped expression");
      return nullptr;
    }

    return expr;
  }

  Expression *literal = parse_literal();
  if (!literal)
  {
    syntax_error("Expected literal or identifier");
    return nullptr;
  }

  return literal;
}

Expression *RecursiveDecentParser::parse_call_expr(Identifier *id)
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  vector<Expression *> args;

  if (!match(TokenType::LEFT_PR))
  {
    syntax_error("Expected '(' to start function call arguments");
    return nullptr;
  }

  if (!lookahead(TokenType::RIGHT_PR))
  {
    Expression *arg = nullptr;
    if (lookahead(TokenType::LEFT_CURLY_PR))
    {
      arg = parse_array();
    }
    else
    {
      arg = parse_or_expr();
    }

    if (!arg)
    {
      syntax_error("Expected expression or array literal as function call argument");
      return nullptr;
    }

    args.push_back(arg);

    while (lookahead(TokenType::COMMA_SY))
    {
      if (!match(TokenType::COMMA_SY))
      {
        syntax_error("Expected ',' between arguments");
        return nullptr;
      }

      if (lookahead(TokenType::LEFT_CURLY_PR))
      {
        arg = parse_array();
      }
      else
      {
        arg = parse_or_expr();
      }

      if (!arg)
      {
        syntax_error("Expected expression after ',' in function call arguments");
        return nullptr;
      }

      args.push_back(arg);
    }
  }

  if (!match(TokenType::RIGHT_PR))
  {
    syntax_error("Expected ')' to close function call arguments");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new CallFunctionExpression(id, args, start_line, end_line, node_start, node_end);
}

Literal *RecursiveDecentParser::parse_literal()
{
  switch (current_token.type)
  {
  case TokenType::INTEGER_NUM:
  {
    Literal *lit = parse_int();
    if (!lit)
    {
      syntax_error("Expected integer literal");
      return nullptr;
    }
    return lit;
  }

  case TokenType::FLOAT_NUM:
  {
    Literal *lit = parse_float();
    if (!lit)
    {
      syntax_error("Expected float literal");
      return nullptr;
    }
    return lit;
  }

  case TokenType::STRING_SY:
  {
    Literal *lit = parse_string();
    if (!lit)
    {
      syntax_error("Expected string literal");
      return nullptr;
    }
    return lit;
  }

  case TokenType::TRUE_KW:
  case TokenType::FALSE_KW:
  {
    Literal *lit = parse_bool();
    if (!lit)
    {
      syntax_error("Expected boolean literal");
      return nullptr;
    }
    return lit;
  }

  default:
  {
    Identifier *id = parse_identifier();
    if (!id)
    {
      syntax_error("Expected identifier or literal");
      return nullptr;
    }

    if (!lookahead(TokenType::LEFT_PR))
    {
      return (Literal *)id;
    }

    Expression *call = parse_call_expr(id);
    if (!call)
    {
      syntax_error("Invalid function call after identifier");
      return nullptr;
    }

    return (Literal *)call;
  }
  }
}

Identifier *RecursiveDecentParser::parse_identifier()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  string name = current_token.value;

  if (!match(TokenType::ID_SY))
  {
    syntax_error("Expected identifier");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new Identifier(name, start_line, end_line, node_start, node_end);
}

IntegerLiteral *RecursiveDecentParser::parse_int()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  int value = 0;
  try
  {
    value = stoi(current_token.value);
  }
  catch (const std::exception &)
  {
    syntax_error("Invalid integer literal");
    return nullptr;
  }

  if (!match(TokenType::INTEGER_NUM))
  {
    syntax_error("Expected integer literal");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new IntegerLiteral(value, start_line, end_line, node_start, node_end);
}

FloatLiteral *RecursiveDecentParser::parse_float()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  float value = 0.0f;
  try
  {
    value = stof(current_token.value);
  }
  catch (const std::exception &)
  {
    syntax_error("Invalid float literal");
    return nullptr;
  }

  if (!match(TokenType::FLOAT_NUM))
  {
    syntax_error("Expected float literal");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new FloatLiteral(value, start_line, end_line, node_start, node_end);
}

StringLiteral *RecursiveDecentParser::parse_string()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  string value = current_token.value;

  if (!match(TokenType::STRING_SY))
  {
    syntax_error("Expected string literal");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new StringLiteral(value, start_line, end_line, node_start, node_end);
}

BooleanLiteral *RecursiveDecentParser::parse_bool()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  bool value = false;

  if (current_token.type == TokenType::TRUE_KW)
  {
    value = true;
    if (!match(TokenType::TRUE_KW))
    {
      syntax_error("Expected 'true' keyword");
      return nullptr;
    }
  }
  else if (current_token.type == TokenType::FALSE_KW)
  {
    value = false;
    if (!match(TokenType::FALSE_KW))
    {
      syntax_error("Expected 'false' keyword");
      return nullptr;
    }
  }
  else
  {
    syntax_error("Expected boolean literal 'true' or 'false'");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new BooleanLiteral(value, start_line, end_line, node_start, node_end);
}

ArrayLiteral *RecursiveDecentParser::parse_array()
{
  int node_start = current_token.start;
  int start_line = current_token.line;

  vector<Expression *> elements;

  if (!match(TokenType::LEFT_CURLY_PR))
  {
    syntax_error("Expected '{' to start array literal");
    return nullptr;
  }

  if (!lookahead(TokenType::LEFT_CURLY_PR))
  {
    elements = parse_array_value();
    if (elements.empty() && has_error)
    {
      syntax_error("Expected elements inside array literal");
      return nullptr;
    }

    if (!match(TokenType::RIGHT_CURLY_PR))
    {
      syntax_error("Expected '}' to close array literal");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ArrayLiteral(elements, start_line, end_line, node_start, node_end);
  }

  // Nested arrays
  Expression *nested = parse_array();
  if (!nested)
  {
    syntax_error("Expected nested array inside array literal");
    return nullptr;
  }
  elements.push_back(nested);

  while (!lookahead(TokenType::RIGHT_CURLY_PR))
  {
    if (!match(TokenType::COMMA_SY))
    {
      syntax_error("Expected ',' between array elements");
      return nullptr;
    }

    nested = parse_array();
    if (!nested)
    {
      syntax_error("Expected nested array after ','");
      return nullptr;
    }
    elements.push_back(nested);
  }

  if (!match(TokenType::RIGHT_CURLY_PR))
  {
    syntax_error("Expected '}' to close nested array literal");
    return nullptr;
  }

  int node_end = previous_token.end;
  int end_line = previous_token.line;

  return new ArrayLiteral(elements, start_line, end_line, node_start, node_end);
}

vector<Expression *> RecursiveDecentParser::parse_array_value()
{
  vector<Expression *> elements;

  if (!lookahead(TokenType::RIGHT_CURLY_PR))
  {
    Expression *expr = parse_or_expr();
    if (!expr)
    {
      syntax_error("Expected expression inside array");
      return {nullptr};
    }
    elements.push_back(expr);

    while (lookahead(TokenType::COMMA_SY))
    {
      if (!match(TokenType::COMMA_SY))
      {
        syntax_error("Expected ',' between array elements");
        return {nullptr};
      }

      expr = parse_or_expr();
      if (!expr)
      {
        syntax_error("Expected expression after ',' in array");
        return {nullptr};
      }
      elements.push_back(expr);
    }
  }

  return elements;
}
