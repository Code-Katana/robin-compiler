#include "robin/syntax/recursive_decent_parser.h"
namespace rbn::syntax
{
  RecursiveDecentParser::RecursiveDecentParser(lexical::ScannerBase *sc) : ParserBase(sc) {}

  bool RecursiveDecentParser::match(core::TokenType type)
  {
    if (current_token.type == type)
    {
      previous_token = current_token;
      current_token = sc->get_token();
      return true;
    }
    return false;
  }

  ast::AstNode *RecursiveDecentParser::parse_ast()
  {

    ast::AstNode *ast_tree = parse_source();
    reset_parser();
    if (has_error)
    {

      return error_node;
    }

    return ast_tree;
  }

  ast::Source *RecursiveDecentParser::parse_source()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    vector<ast::FunctionDefinition *> funcs = {};

    while (lookahead(core::TokenType::FUNC_KW))
    {
      ast::FunctionDefinition *func = parse_function();
      if (!func)
      {
        syntax_error("Invalid function definition");
        return nullptr;
      }
      funcs.push_back(func);
    }

    ast::ProgramDefinition *program = parse_program();
    if (!program)
    {
      syntax_error("Invalid program definition");
      return nullptr;
    }

    if (!match(core::TokenType::END_OF_FILE))
    {
      syntax_error("Expected end of file after program");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::Source(program, funcs, start_line, end_line, node_start, node_end);
  }

  ast::FunctionDefinition *RecursiveDecentParser::parse_function()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    if (!match(core::TokenType::FUNC_KW))
    {
      syntax_error("Expected 'FUNC' keyword at start of function");
      return nullptr;
    }

    ast::ReturnType *return_type = parse_return_type();
    if (!return_type)
    {
      syntax_error("Expected return type after 'FUNC'");
      return nullptr;
    }

    ast::Identifier *id = parse_identifier();
    if (!id)
    {
      syntax_error("Expected function name after return type");
      return nullptr;
    }

    if (!match(core::TokenType::HAS_KW))
    {
      syntax_error("Expected 'HAS' keyword after function name: " + id->name);
      return nullptr;
    }

    vector<ast::VariableDefinition *> params;
    while (!lookahead(core::TokenType::BEGIN_KW) && !lookahead(core::TokenType::END_OF_FILE))
    {
      ast::VariableDefinition *param = parse_var_def();
      if (!param)
      {
        syntax_error("Invalid parameter definition in function: " + id->name);
        return nullptr;
      }
      params.push_back(param);
    }

    if (!match(core::TokenType::BEGIN_KW))
    {
      syntax_error("Expected 'BEGIN' keyword before function body: " + id->name);
      return nullptr;
    }

    vector<ast::Statement *> body;
    while (!lookahead(core::TokenType::END_KW) && !lookahead(core::TokenType::END_OF_FILE))
    {
      ast::Statement *stmt = parse_command();
      if (!stmt)
      {
        syntax_error("Invalid statement inside function body: " + id->name);
        return nullptr;
      }
      body.push_back(stmt);
    }

    if (!match(core::TokenType::END_KW))
    {
      syntax_error("Expected 'END' keyword after function body: " + id->name);
      return nullptr;
    }

    if (!match(core::TokenType::FUNC_KW))
    {
      syntax_error("Expected 'FUNC' keyword after 'END' to close function: " + id->name);
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::FunctionDefinition(id, return_type, params, body, start_line, end_line, node_start, node_end);
  }

  ast::ProgramDefinition *RecursiveDecentParser::parse_program()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    if (!match(core::TokenType::PROGRAM_KW))
    {
      syntax_error("Expected 'PROGRAM' keyword at start of program");
      return nullptr;
    }

    ast::Identifier *id = parse_identifier();

    if (!id)
    {
      syntax_error("Expected Program name after Program type");
      return nullptr;
    }

    if (!match(core::TokenType::IS_KW))
    {
      syntax_error("Expected 'IS' keyword after program name: " + id->name);
      return nullptr;
    }

    vector<ast::VariableDefinition *> globals;
    while (!lookahead(core::TokenType::BEGIN_KW))
    {
      ast::VariableDefinition *global = parse_var_def();
      if (!global)
      {
        syntax_error("Invalid Global Variable definition in program: " + id->name);
        return nullptr;
      }
      globals.push_back(global);
    }

    if (!match(core::TokenType::BEGIN_KW))
    {
      syntax_error("Expected 'BEGIN' keyword before program body: " + id->name);
      return nullptr;
    }

    vector<ast::Statement *> body;
    while (!lookahead(core::TokenType::END_KW))
    {
      ast::Statement *stmt = parse_command();
      if (!stmt)
      {
        syntax_error("Invalid statement inside program body: " + id->name);
        return nullptr;
      }
      body.push_back(stmt);
    }

    if (!match(core::TokenType::END_KW))
    {
      syntax_error("Expected 'END' keyword after program body: " + id->name);
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::ProgramDefinition(id, globals, body, start_line, end_line, node_start, node_end);
  }

  ast::DataType *RecursiveDecentParser::parse_data_type()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    string datatype;
    int dim = 0;

    if (!lookahead(core::TokenType::LEFT_SQUARE_PR) && ast::AstNode::is_data_type(current_token.type))
    {
      datatype = current_token.value;
      if (!match(current_token.type))
      {
        syntax_error("Expected primitive data type : " + datatype);
        return nullptr;
      }

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      return new ast::PrimitiveDataType(datatype, start_line, end_line, node_start, node_end);
    }

    if (!match(core::TokenType::LEFT_SQUARE_PR))
    {
      syntax_error("Expected '[' to start array type");
      return nullptr;
    }
    ++dim;

    while (lookahead(core::TokenType::LEFT_SQUARE_PR))
    {
      if (!match(core::TokenType::LEFT_SQUARE_PR))
      {
        syntax_error("Expected '[' for array dimension");
        return nullptr;
      }
      ++dim;
    }

    if (ast::AstNode::is_data_type(current_token.type))
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
      if (!match(core::TokenType::RIGHT_SQUARE_PR))
      {
        syntax_error("Expected ']' to close array type");
        return nullptr;
      }
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::ArrayDataType(datatype, dim, start_line, end_line, node_start, node_end);
  }

  ast::ReturnType *RecursiveDecentParser::parse_return_type()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    ast::DataType *return_type = nullptr;
    string datatype;
    int dim = 0;

    // Case 1: Primitive return type
    if (!lookahead(core::TokenType::LEFT_SQUARE_PR) && ast::AstNode::is_return_type(current_token.type))
    {
      datatype = current_token.value;

      if (!match(current_token.type))
      {
        syntax_error("Expected primitive return type : " + datatype);
        return nullptr;
      }

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      return_type = new ast::PrimitiveDataType(datatype, start_line, end_line, node_start, node_end);
      return new ast::ReturnType(return_type, start_line, end_line, node_start, node_end);
    }

    // Case 2: Array return type
    if (!match(core::TokenType::LEFT_SQUARE_PR))
    {
      syntax_error("Expected '[' to start array return type");
      return nullptr;
    }
    ++dim;

    while (lookahead(core::TokenType::LEFT_SQUARE_PR))
    {
      if (!match(core::TokenType::LEFT_SQUARE_PR))
      {
        syntax_error("Expected '[' for array dimension");
        return nullptr;
      }
      ++dim;
    }

    if (ast::AstNode::is_data_type(current_token.type))
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
      if (!match(core::TokenType::RIGHT_SQUARE_PR))
      {
        syntax_error("Expected ']' to close array return type");
        return nullptr;
      }
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return_type = new ast::ArrayDataType(datatype, dim, start_line, end_line, node_start, node_end);
    return new ast::ReturnType(return_type, start_line, end_line, node_start, node_end);
  }

  ast::VariableDefinition *RecursiveDecentParser::parse_var_def()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    vector<ast::Identifier *> variables;
    ast::DataType *datatype = nullptr;

    if (!match(core::TokenType::VAR_KW))
    {
      syntax_error("Expected 'VAR' keyword at start of variable definition");
      return nullptr;
    }

    ast::Identifier *first_var = parse_identifier();
    if (!first_var)
    {
      syntax_error("Expected identifier after 'VAR'");
      return nullptr;
    }
    variables.push_back(first_var);

    while (lookahead(core::TokenType::COMMA_SY))
    {
      if (!match(core::TokenType::COMMA_SY))
      {
        syntax_error("Expected ',' between variable names");
        return nullptr;
      }

      ast::Identifier *var = parse_identifier();
      if (!var)
      {
        syntax_error("Expected identifier after ','");
        return nullptr;
      }
      variables.push_back(var);
    }

    if (variables.size() == 1)
    {
      if (!match(core::TokenType::COLON_SY))
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

      if (lookahead(core::TokenType::SEMI_COLON_SY))
      {
        if (!match(core::TokenType::SEMI_COLON_SY))
        {
          syntax_error("Expected ';' after variable declaration");
          return nullptr;
        }

        int node_end = previous_token.end;
        int end_line = previous_token.line;

        ast::Statement *declaration = new ast::VariableDeclaration(variables, datatype, start_line, end_line, node_start, node_end);
        return new ast::VariableDefinition(declaration, start_line, end_line, node_start, node_end);
      }
      else if (lookahead(core::TokenType::EQUAL_OP))
      {
        if (!match(core::TokenType::EQUAL_OP))
        {
          syntax_error("Expected '=' for variable initialization");
          return nullptr;
        }

        ast::Expression *initializer = nullptr;

        if (lookahead(core::TokenType::LEFT_CURLY_PR))
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

        if (!match(core::TokenType::SEMI_COLON_SY))
        {
          syntax_error("Expected ';' after variable initialization");
          return nullptr;
        }

        int node_end = previous_token.end;
        int end_line = previous_token.line;

        ast::Statement *initialization = new ast::VariableInitialization(variables[0], datatype, initializer, start_line, end_line, node_start, node_end);
        return new ast::VariableDefinition(initialization, start_line, end_line, node_start, node_end);
      }
      else
      {
        syntax_error("Expected ';' or '=' after variable declaration");
        return nullptr;
      }
    }

    // Multiple variables case
    if (!match(core::TokenType::COLON_SY))
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

    if (!match(core::TokenType::SEMI_COLON_SY))
    {
      syntax_error("Expected ';' after variable declaration");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    ast::Statement *declaration = new ast::VariableDeclaration(variables, datatype, start_line, end_line, node_start, node_end);
    return new ast::VariableDefinition(declaration, start_line, end_line, node_start, node_end);
  }

  ast::Statement *RecursiveDecentParser::parse_command()
  {
    ast::Statement *stmt = nullptr;

    if (lookahead(core::TokenType::SKIP_KW))
    {
      stmt = parse_skip_expr();
    }
    else if (lookahead(core::TokenType::STOP_KW))
    {
      stmt = parse_stop_expr();
    }
    else if (lookahead(core::TokenType::WRITE_KW))
    {
      stmt = parse_write();
    }
    else if (lookahead(core::TokenType::READ_KW))
    {
      stmt = parse_read();
    }
    else if (lookahead(core::TokenType::IF_KW))
    {
      stmt = parse_if();
    }
    else if (lookahead(core::TokenType::FOR_KW))
    {
      stmt = parse_for();
    }
    else if (lookahead(core::TokenType::WHILE_KW))
    {
      stmt = parse_while();
    }
    else if (lookahead(core::TokenType::RETURN_KW))
    {
      stmt = parse_return_stmt();
    }
    else if (lookahead(core::TokenType::VAR_KW))
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

  ast::SkipStatement *RecursiveDecentParser::parse_skip_expr()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    if (!match(core::TokenType::SKIP_KW))
    {
      syntax_error("Expected 'SKIP' keyword");
      return nullptr;
    }

    if (!match(core::TokenType::SEMI_COLON_SY))
    {
      syntax_error("Expected ';' after 'SKIP'");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::SkipStatement(start_line, end_line, node_start, node_end);
  }

  ast::StopStatement *RecursiveDecentParser::parse_stop_expr()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    if (!match(core::TokenType::STOP_KW))
    {
      syntax_error("Expected 'STOP' keyword");
      return nullptr;
    }

    if (!match(core::TokenType::SEMI_COLON_SY))
    {
      syntax_error("Expected ';' after 'STOP'");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::StopStatement(start_line, end_line, node_start, node_end);
  }

  ast::ReadStatement *RecursiveDecentParser::parse_read()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    vector<ast::AssignableExpression *> variables;

    if (!match(core::TokenType::READ_KW))
    {
      syntax_error("Expected 'READ' keyword");
      return nullptr;
    }

    ast::Expression *expr = parse_expr();
    if (!expr)
    {
      syntax_error("Expected expression after 'READ'");
      return nullptr;
    }

    ast::AssignableExpression *assignable = parse_assignable_expr(expr);
    if (!assignable)
    {
      syntax_error("Expected assignable expression after 'READ'");
      return nullptr;
    }
    variables.push_back(assignable);

    while (!lookahead(core::TokenType::SEMI_COLON_SY))
    {
      if (!match(core::TokenType::COMMA_SY))
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

    if (!match(core::TokenType::SEMI_COLON_SY))
    {
      syntax_error("Expected ';' at the end of 'READ' statement");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::ReadStatement(variables, start_line, end_line, node_start, node_end);
  }

  ast::WriteStatement *RecursiveDecentParser::parse_write()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    vector<ast::Expression *> args;

    if (!match(core::TokenType::WRITE_KW))
    {
      syntax_error("Expected 'WRITE' keyword");
      return nullptr;
    }

    ast::Expression *expr = parse_or_expr();
    if (!expr)
    {
      syntax_error("Expected expression after 'WRITE'");
      return nullptr;
    }
    args.push_back(expr);

    while (!lookahead(core::TokenType::SEMI_COLON_SY))
    {
      if (!match(core::TokenType::COMMA_SY))
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

    if (!match(core::TokenType::SEMI_COLON_SY))
    {
      syntax_error("Expected ';' at the end of 'WRITE' statement");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::WriteStatement(args, start_line, end_line, node_start, node_end);
  }

  ast::ReturnStatement *RecursiveDecentParser::parse_return_stmt()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    ast::Expression *val = nullptr;

    if (!match(core::TokenType::RETURN_KW))
    {
      syntax_error("Expected 'RETURN' keyword");
      return nullptr;
    }

    if (!lookahead(core::TokenType::SEMI_COLON_SY))
    {
      val = parse_or_expr();
      if (!val)
      {
        syntax_error("Expected expression after 'RETURN'");
        return nullptr;
      }
    }

    if (!match(core::TokenType::SEMI_COLON_SY))
    {
      syntax_error("Expected ';' after 'RETURN' statement");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::ReturnStatement(val, start_line, end_line, node_start, node_end);
  }

  ast::IfStatement *RecursiveDecentParser::parse_if()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    ast::Expression *condition = nullptr;
    vector<ast::Statement *> consequent;
    vector<ast::Statement *> alternate;

    if (!match(core::TokenType::IF_KW))
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

    if (!match(core::TokenType::THEN_KW))
    {
      syntax_error("Expected 'THEN' after IF condition");
      return nullptr;
    }

    while (!lookahead(core::TokenType::ELSE_KW) && !lookahead(core::TokenType::END_KW) && !lookahead(core::TokenType::END_OF_FILE))
    {
      ast::Statement *stmt = parse_command();
      if (!stmt)
      {
        syntax_error("Invalid statement inside IF consequent block");
        return nullptr;
      }
      consequent.push_back(stmt);
    }

    if (lookahead(core::TokenType::ELSE_KW))
    {
      if (!match(core::TokenType::ELSE_KW))
      {
        syntax_error("Expected 'ELSE' keyword");
        return nullptr;
      }

      if (lookahead(core::TokenType::IF_KW))
      {
        ast::IfStatement *else_if = parse_if();
        if (!else_if)
        {
          syntax_error("Invalid ELSE IF block");
          return nullptr;
        }

        int node_end = previous_token.end;
        int end_line = previous_token.line;

        alternate.push_back(else_if);
        return new ast::IfStatement(condition, consequent, alternate, start_line, end_line, node_start, node_end);
      }

      while (!lookahead(core::TokenType::END_KW) && !lookahead(core::TokenType::END_OF_FILE))
      {
        ast::Statement *stmt = parse_command();
        if (!stmt)
        {
          syntax_error("Invalid statement inside ELSE block");
          return nullptr;
        }
        alternate.push_back(stmt);
      }
    }

    if (!match(core::TokenType::END_KW))
    {
      syntax_error("Expected 'END' to close IF block");
      return nullptr;
    }

    if (!match(core::TokenType::IF_KW))
    {
      syntax_error("Expected 'IF' after 'END' to close IF block");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::IfStatement(condition, consequent, alternate, start_line, end_line, node_start, node_end);
  }

  ast::ForLoop *RecursiveDecentParser::parse_for()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    ast::AssignmentExpression *init = nullptr;
    ast::Expression *condition = nullptr;
    ast::Expression *update = nullptr;
    vector<ast::Statement *> body;

    if (!match(core::TokenType::FOR_KW))
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

    if (!match(core::TokenType::SEMI_COLON_SY))
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

    if (!match(core::TokenType::SEMI_COLON_SY))
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

    if (!match(core::TokenType::DO_KW))
    {
      syntax_error("Expected 'DO' keyword after update expression in 'FOR' loop");
      return nullptr;
    }

    while (!lookahead(core::TokenType::END_KW) && !lookahead(core::TokenType::END_OF_FILE))
    {
      ast::Statement *stmt = parse_command();
      if (!stmt)
      {
        syntax_error("Invalid statement inside 'FOR' loop body");
        return nullptr;
      }
      body.push_back(stmt);
    }

    if (!match(core::TokenType::END_KW))
    {
      syntax_error("Expected 'END' to close 'FOR' loop");
      return nullptr;
    }

    if (!match(core::TokenType::FOR_KW))
    {
      syntax_error("Expected 'FOR' after 'END' to close 'FOR' loop");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::ForLoop(init, condition, update, body, start_line, end_line, node_start, node_end);
  }

  ast::AssignmentExpression *RecursiveDecentParser::parse_int_assign()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    ast::Identifier *id = parse_identifier();
    if (!id)
    {
      syntax_error("Expected identifier in initialization of 'FOR' loop");
      return nullptr;
    }

    if (!match(core::TokenType::EQUAL_OP))
    {
      syntax_error("Expected '=' after identifier in 'FOR' loop initialization");
      return nullptr;
    }

    ast::Expression *val = parse_or_expr();
    if (!val)
    {
      syntax_error("Expected expression after '=' in 'FOR' loop initialization");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::AssignmentExpression(id, val, start_line, end_line, node_start, node_end);
  }

  ast::WhileLoop *RecursiveDecentParser::parse_while()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    ast::Expression *condition = nullptr;
    vector<ast::Statement *> body;

    if (!match(core::TokenType::WHILE_KW))
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

    if (!match(core::TokenType::DO_KW))
    {
      syntax_error("Expected 'DO' keyword after 'WHILE' condition");
      return nullptr;
    }

    while (!lookahead(core::TokenType::END_KW) && !lookahead(core::TokenType::END_OF_FILE))
    {
      ast::Statement *stmt = parse_command();
      if (!stmt)
      {
        syntax_error("Invalid statement inside 'WHILE' loop body");
        return nullptr;
      }
      body.push_back(stmt);
    }

    if (!match(core::TokenType::END_KW))
    {
      syntax_error("Expected 'END' to close 'WHILE' loop");
      return nullptr;
    }

    if (!match(core::TokenType::WHILE_KW))
    {
      syntax_error("Expected 'WHILE' after 'END' to close 'WHILE' loop");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::WhileLoop(condition, body, start_line, end_line, node_start, node_end);
  }

  ast::Expression *RecursiveDecentParser::parse_bool_expr()
  {
    ast::Expression *expr = parse_or_expr();
    if (!expr)
    {
      syntax_error("Expected boolean expression");
      return nullptr;
    }
    ast::AstNodeType type = expr->type;

    switch (type)
    {
    case ast::AstNodeType::OrExpression:
    case ast::AstNodeType::AndExpression:
    case ast::AstNodeType::EqualityExpression:
    case ast::AstNodeType::RelationalExpression:
    case ast::AstNodeType::Identifier:
    case ast::AstNodeType::IndexExpression:
    case ast::AstNodeType::CallFunctionExpression:
    case ast::AstNodeType::BooleanLiteral:
      return expr;

    default:
      syntax_error("Invalid expression in boolean context");
      return nullptr;
    }
  }

  ast::Expression *RecursiveDecentParser::parse_expr_stmt()
  {
    ast::Expression *expr = parse_expr();
    if (!expr)
    {
      syntax_error("Expected expression in expression statement");
      return nullptr;
    }

    if (!match(core::TokenType::SEMI_COLON_SY))
    {
      syntax_error("Expected ';' after expression");
      return nullptr;
    }

    return expr;
  }

  ast::Expression *RecursiveDecentParser::parse_expr()
  {
    ast::Expression *expr = parse_assign_expr();
    if (!expr)
    {
      syntax_error("Expected expression");
      return nullptr;
    }

    return expr;
  }

  ast::Expression *RecursiveDecentParser::parse_assign_expr()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    ast::Expression *left = parse_or_expr();
    if (!left)
    {
      syntax_error("Expected expression on the left-hand side of assignment");
      return nullptr;
    }

    if (!lookahead(core::TokenType::EQUAL_OP))
    {
      return left; // No assignment, just return the left expression
    }

    ast::AssignableExpression *assignee = parse_assignable_expr(left);
    if (!assignee)
    {
      syntax_error("Invalid left-hand side in assignment");
      return nullptr;
    }

    if (!match(core::TokenType::EQUAL_OP))
    {
      syntax_error("Expected '=' in assignment expression");
      return nullptr;
    }

    ast::Expression *value = nullptr;
    if (lookahead(core::TokenType::LEFT_CURLY_PR))
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

    return new ast::AssignmentExpression(assignee, value, start_line, end_line, node_start, node_end);
  }

  ast::AssignableExpression *RecursiveDecentParser::parse_assignable_expr(ast::Expression *expr)
  {
    if (!expr)
    {
      syntax_error("Expected an expression to assign to");
      return nullptr;
    }

    ast::AssignableExpression *assignable = dynamic_cast<ast::AssignableExpression *>(expr);
    if (!assignable)
    {
      syntax_error("Invalid left-hand side in assignment expression");
      return nullptr;
    }

    return assignable;
  }

  ast::Expression *RecursiveDecentParser::parse_or_expr()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    ast::Expression *left = parse_and_expr();
    if (!left)
    {
      syntax_error("Expected expression on the left-hand side of 'OR'");
      return nullptr;
    }

    while (lookahead(core::TokenType::OR_KW))
    {
      if (!match(core::TokenType::OR_KW))
      {
        syntax_error("Expected 'OR' operator");
        return nullptr;
      }

      ast::Expression *right = parse_and_expr();
      if (!right)
      {
        syntax_error("Expected expression on the right-hand side of 'OR'");
        return nullptr;
      }

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      left = new ast::OrExpression(left, right, start_line, end_line, node_start, node_end);
    }

    return left;
  }

  ast::Expression *RecursiveDecentParser::parse_and_expr()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    ast::Expression *left = parse_equality_expr();
    if (!left)
    {
      syntax_error("Expected expression on the left-hand side of 'AND'");
      return nullptr;
    }

    while (lookahead(core::TokenType::AND_KW))
    {
      if (!match(core::TokenType::AND_KW))
      {
        syntax_error("Expected 'AND' operator");
        return nullptr;
      }

      ast::Expression *right = parse_equality_expr();
      if (!right)
      {
        syntax_error("Expected expression on the right-hand side of 'AND'");
        return nullptr;
      }

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      left = new ast::AndExpression(left, right, start_line, end_line, node_start, node_end);
    }

    return left;
  }

  ast::Expression *RecursiveDecentParser::parse_equality_expr()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    ast::Expression *left = parse_relational_expr();
    if (!left)
    {
      syntax_error("Expected expression on the left-hand side of equality operator");
      return nullptr;
    }

    while (lookahead(core::TokenType::IS_EQUAL_OP) || lookahead(core::TokenType::NOT_EQUAL_OP))
    {
      string op = current_token.value;

      if (op == "==")
      {
        if (!match(core::TokenType::IS_EQUAL_OP))
        {
          syntax_error("Expected '==' operator");
          return nullptr;
        }
      }
      else
      {
        if (!match(core::TokenType::NOT_EQUAL_OP))
        {
          syntax_error("Expected '<>' operator");
          return nullptr;
        }
      }

      ast::Expression *right = parse_relational_expr();
      if (!right)
      {
        syntax_error("Expected expression on the right-hand side of equality operator");
        return nullptr;
      }

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      left = new ast::EqualityExpression(left, right, op, start_line, end_line, node_start, node_end);
    }

    return left;
  }

  ast::Expression *RecursiveDecentParser::parse_relational_expr()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    ast::Expression *left = parse_additive_expr();
    if (!left)
    {
      syntax_error("Expected expression on the left-hand side of relational operator");
      return nullptr;
    }

    while (lookahead(core::TokenType::LESS_THAN_OP) || lookahead(core::TokenType::LESS_EQUAL_OP) ||
           lookahead(core::TokenType::GREATER_THAN_OP) || lookahead(core::TokenType::GREATER_EQUAL_OP))
    {
      string op = current_token.value;

      switch (current_token.type)
      {
      case core::TokenType::LESS_THAN_OP:
        if (!match(core::TokenType::LESS_THAN_OP))
        {
          syntax_error("Expected '<' operator");
          return nullptr;
        }
        break;

      case core::TokenType::LESS_EQUAL_OP:
        if (!match(core::TokenType::LESS_EQUAL_OP))
        {
          syntax_error("Expected '<=' operator");
          return nullptr;
        }
        break;

      case core::TokenType::GREATER_THAN_OP:
        if (!match(core::TokenType::GREATER_THAN_OP))
        {
          syntax_error("Expected '>' operator");
          return nullptr;
        }
        break;

      case core::TokenType::GREATER_EQUAL_OP:
        if (!match(core::TokenType::GREATER_EQUAL_OP))
        {
          syntax_error("Expected '>=' operator");
          return nullptr;
        }
        break;
      }

      ast::Expression *right = parse_additive_expr();
      if (!right)
      {
        syntax_error("Expected expression on the right-hand side of relational operator");
        return nullptr;
      }

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      left = new ast::RelationalExpression(left, right, op, start_line, end_line, node_start, node_end);
    }

    return left;
  }

  ast::Expression *RecursiveDecentParser::parse_additive_expr()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    ast::Expression *left = parse_multiplicative_expr();
    if (!left)
    {
      syntax_error("Expected expression on the left-hand side of additive operator");
      return nullptr;
    }

    while (lookahead(core::TokenType::PLUS_OP) || lookahead(core::TokenType::MINUS_OP))
    {
      string op = current_token.value;

      if (op == "+")
      {
        if (!match(core::TokenType::PLUS_OP))
        {
          syntax_error("Expected '+' operator");
          return nullptr;
        }
      }
      else
      {
        if (!match(core::TokenType::MINUS_OP))
        {
          syntax_error("Expected '-' operator");
          return nullptr;
        }
      }

      ast::Expression *right = parse_multiplicative_expr();
      if (!right)
      {
        syntax_error("Expected expression on the right-hand side of additive operator");
        return nullptr;
      }

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      left = new ast::AdditiveExpression(left, right, op, start_line, end_line, node_start, node_end);
    }

    return left;
  }

  ast::Expression *RecursiveDecentParser::parse_multiplicative_expr()
  {
    ast::Expression *left = parse_unary_expr();
    if (!left)
    {
      syntax_error("Expected expression on the left-hand side of multiplicative operator");
      return nullptr;
    }

    while (lookahead(core::TokenType::MULT_OP) || lookahead(core::TokenType::DIVIDE_OP) || lookahead(core::TokenType::MOD_OP))
    {
      int node_start = current_token.start;
      int start_line = current_token.line;

      string op = current_token.value;

      switch (current_token.type)
      {
      case core::TokenType::MULT_OP:
        if (!match(core::TokenType::MULT_OP))
        {
          syntax_error("Expected '*' operator");
          return nullptr;
        }
        break;

      case core::TokenType::DIVIDE_OP:
        if (!match(core::TokenType::DIVIDE_OP))
        {
          syntax_error("Expected '/' operator");
          return nullptr;
        }
        break;

      case core::TokenType::MOD_OP:
        if (!match(core::TokenType::MOD_OP))
        {
          syntax_error("Expected '%' operator");
          return nullptr;
        }
        break;
      }

      ast::Expression *right = parse_unary_expr();
      if (!right)
      {
        syntax_error("Expected expression on the right-hand side of multiplicative operator");
        return nullptr;
      }

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      left = new ast::MultiplicativeExpression(left, right, op, start_line, end_line, node_start, node_end);
    }

    return left;
  }

  ast::Expression *RecursiveDecentParser::parse_unary_expr()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    if (lookahead(core::TokenType::MINUS_OP) || lookahead(core::TokenType::STRINGIFY_OP) ||
        lookahead(core::TokenType::BOOLEAN_OP) || lookahead(core::TokenType::ROUND_OP) ||
        lookahead(core::TokenType::LENGTH_OP))
    {
      string op = current_token.value;

      if (!match(current_token.type))
      {
        syntax_error("Expected unary operator");
        return nullptr;
      }

      ast::Expression *operand = parse_index_expr();
      if (!operand)
      {
        syntax_error("Expected expression after unary operator");
        return nullptr;
      }

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      return new ast::UnaryExpression(operand, op, false, start_line, end_line, node_start, node_end);
    }

    if (lookahead(core::TokenType::INCREMENT_OP) || lookahead(core::TokenType::DECREMENT_OP))
    {
      string op = current_token.value;

      if (!match(op == "++" ? core::TokenType::INCREMENT_OP : core::TokenType::DECREMENT_OP))
      {
        syntax_error("Expected '++' or '--' operator");
        return nullptr;
      }

      ast::Expression *operand = parse_assignable_expr(parse_index_expr());
      if (!operand)
      {
        syntax_error("Expected assignable expression after '++' or '--'");
        return nullptr;
      }

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      return new ast::UnaryExpression(operand, op, false, start_line, end_line, node_start, node_end);
    }

    if (lookahead(core::TokenType::NOT_KW))
    {
      string op = current_token.value;

      if (!match(core::TokenType::NOT_KW))
      {
        syntax_error("Expected 'NOT' keyword");
        return nullptr;
      }

      ast::Expression *operand = parse_index_expr();
      if (!operand)
      {
        syntax_error("Expected expression after 'NOT'");
        return nullptr;
      }

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      return new ast::UnaryExpression(operand, op, false, start_line, end_line, node_start, node_end);
    }

    // Parse primary expression
    ast::Expression *primary = parse_index_expr();
    if (!primary)
    {
      syntax_error("Expected primary expression");
      return nullptr;
    }

    if (lookahead(core::TokenType::INCREMENT_OP) || lookahead(core::TokenType::DECREMENT_OP))
    {
      if (!parse_assignable_expr(primary))
      {
        syntax_error("Expected assignable expression before postfix '++' or '--'");
        return nullptr;
      }

      string op = current_token.value;

      if (!match(op == "++" ? core::TokenType::INCREMENT_OP : core::TokenType::DECREMENT_OP))
      {
        syntax_error("Expected postfix '++' or '--' operator");
        return nullptr;
      }

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      return new ast::UnaryExpression(primary, op, true, start_line, end_line, node_start, node_end);
    }

    return primary;
  }

  ast::Expression *RecursiveDecentParser::parse_index_expr()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    ast::Expression *base = parse_primary_expr();
    if (!base)
    {
      syntax_error("Expected primary expression for indexing");
      return nullptr;
    }

    while (lookahead(core::TokenType::LEFT_SQUARE_PR))
    {
      if (!match(core::TokenType::LEFT_SQUARE_PR))
      {
        syntax_error("Expected '[' to start index expression");
        return nullptr;
      }

      ast::Expression *index = parse_expr();
      if (!index)
      {
        syntax_error("Expected expression inside index brackets");
        return nullptr;
      }

      if (!match(core::TokenType::RIGHT_SQUARE_PR))
      {
        syntax_error("Expected ']' to close index expression");
        return nullptr;
      }

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      base = new ast::IndexExpression(base, index, start_line, end_line, node_start, node_end);
    }

    return base;
  }

  ast::Expression *RecursiveDecentParser::parse_primary_expr()
  {
    if (lookahead(core::TokenType::LEFT_PR))
    {
      if (!match(core::TokenType::LEFT_PR))
      {
        syntax_error("Expected '(' to start grouped expression");
        return nullptr;
      }

      ast::Expression *expr = parse_expr();
      if (!expr)
      {
        syntax_error("Expected expression inside parentheses");
        return nullptr;
      }

      if (!match(core::TokenType::RIGHT_PR))
      {
        syntax_error("Expected ')' to close grouped expression");
        return nullptr;
      }

      return expr;
    }

    ast::Expression *literal = parse_literal();
    if (!literal)
    {
      syntax_error("Expected literal or identifier");
      return nullptr;
    }

    return literal;
  }

  ast::Expression *RecursiveDecentParser::parse_call_expr(ast::Identifier *id)
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    vector<ast::Expression *> args;

    if (!match(core::TokenType::LEFT_PR))
    {
      syntax_error("Expected '(' to start function call arguments");
      return nullptr;
    }

    if (!lookahead(core::TokenType::RIGHT_PR))
    {
      ast::Expression *arg = nullptr;
      if (lookahead(core::TokenType::LEFT_CURLY_PR))
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

      while (lookahead(core::TokenType::COMMA_SY))
      {
        if (!match(core::TokenType::COMMA_SY))
        {
          syntax_error("Expected ',' between arguments");
          return nullptr;
        }

        if (lookahead(core::TokenType::LEFT_CURLY_PR))
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

    if (!match(core::TokenType::RIGHT_PR))
    {
      syntax_error("Expected ')' to close function call arguments");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::CallFunctionExpression(id, args, start_line, end_line, node_start, node_end);
  }

  ast::Literal *RecursiveDecentParser::parse_literal()
  {
    switch (current_token.type)
    {
    case core::TokenType::INTEGER_NUM:
    {
      ast::Literal *lit = parse_int();
      if (!lit)
      {
        syntax_error("Expected integer literal");
        return nullptr;
      }
      return lit;
    }

    case core::TokenType::FLOAT_NUM:
    {
      ast::Literal *lit = parse_float();
      if (!lit)
      {
        syntax_error("Expected float literal");
        return nullptr;
      }
      return lit;
    }

    case core::TokenType::STRING_SY:
    {
      ast::Literal *lit = parse_string();
      if (!lit)
      {
        syntax_error("Expected string literal");
        return nullptr;
      }
      return lit;
    }

    case core::TokenType::TRUE_KW:
    case core::TokenType::FALSE_KW:
    {
      ast::Literal *lit = parse_bool();
      if (!lit)
      {
        syntax_error("Expected boolean literal");
        return nullptr;
      }
      return lit;
    }

    default:
    {
      ast::Identifier *id = parse_identifier();
      if (!id)
      {
        syntax_error("Expected identifier or literal");
        return nullptr;
      }

      if (!lookahead(core::TokenType::LEFT_PR))
      {
        return (ast::Literal *)id;
      }

      ast::Expression *call = parse_call_expr(id);
      if (!call)
      {
        syntax_error("Invalid function call after identifier");
        return nullptr;
      }

      return (ast::Literal *)call;
    }
    }
  }

  ast::Identifier *RecursiveDecentParser::parse_identifier()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    string name = current_token.value;

    if (!match(core::TokenType::ID_SY))
    {
      syntax_error("Expected identifier");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::Identifier(name, start_line, end_line, node_start, node_end);
  }

  ast::IntegerLiteral *RecursiveDecentParser::parse_int()
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

    if (!match(core::TokenType::INTEGER_NUM))
    {
      syntax_error("Expected integer literal");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::IntegerLiteral(value, start_line, end_line, node_start, node_end);
  }

  ast::FloatLiteral *RecursiveDecentParser::parse_float()
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

    if (!match(core::TokenType::FLOAT_NUM))
    {
      syntax_error("Expected float literal");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::FloatLiteral(value, start_line, end_line, node_start, node_end);
  }

  ast::StringLiteral *RecursiveDecentParser::parse_string()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    string value = current_token.value;

    if (!match(core::TokenType::STRING_SY))
    {
      syntax_error("Expected string literal");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::StringLiteral(value, start_line, end_line, node_start, node_end);
  }

  ast::BooleanLiteral *RecursiveDecentParser::parse_bool()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    bool value = false;

    if (current_token.type == core::TokenType::TRUE_KW)
    {
      value = true;
      if (!match(core::TokenType::TRUE_KW))
      {
        syntax_error("Expected 'true' keyword");
        return nullptr;
      }
    }
    else if (current_token.type == core::TokenType::FALSE_KW)
    {
      value = false;
      if (!match(core::TokenType::FALSE_KW))
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

    return new ast::BooleanLiteral(value, start_line, end_line, node_start, node_end);
  }

  ast::ArrayLiteral *RecursiveDecentParser::parse_array()
  {
    int node_start = current_token.start;
    int start_line = current_token.line;

    vector<ast::Expression *> elements;

    if (!match(core::TokenType::LEFT_CURLY_PR))
    {
      syntax_error("Expected '{' to start array literal");
      return nullptr;
    }

    if (!lookahead(core::TokenType::LEFT_CURLY_PR))
    {
      elements = parse_array_value();
      if (elements.empty() && has_error)
      {
        syntax_error("Expected elements inside array literal");
        return nullptr;
      }

      if (!match(core::TokenType::RIGHT_CURLY_PR))
      {
        syntax_error("Expected '}' to close array literal");
        return nullptr;
      }

      int node_end = previous_token.end;
      int end_line = previous_token.line;

      return new ast::ArrayLiteral(elements, start_line, end_line, node_start, node_end);
    }

    // Nested arrays
    ast::Expression *nested = parse_array();
    if (!nested)
    {
      syntax_error("Expected nested array inside array literal");
      return nullptr;
    }
    elements.push_back(nested);

    while (!lookahead(core::TokenType::RIGHT_CURLY_PR))
    {
      if (!match(core::TokenType::COMMA_SY))
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

    if (!match(core::TokenType::RIGHT_CURLY_PR))
    {
      syntax_error("Expected '}' to close nested array literal");
      return nullptr;
    }

    int node_end = previous_token.end;
    int end_line = previous_token.line;

    return new ast::ArrayLiteral(elements, start_line, end_line, node_start, node_end);
  }

  vector<ast::Expression *> RecursiveDecentParser::parse_array_value()
  {
    vector<ast::Expression *> elements;

    if (!lookahead(core::TokenType::RIGHT_CURLY_PR))
    {
      ast::Expression *expr = parse_or_expr();
      if (!expr)
      {
        syntax_error("Expected expression inside array");
        return {nullptr};
      }
      elements.push_back(expr);

      while (lookahead(core::TokenType::COMMA_SY))
      {
        if (!match(core::TokenType::COMMA_SY))
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
}