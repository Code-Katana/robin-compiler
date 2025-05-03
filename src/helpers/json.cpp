#include "json.h"

JSON::JSON() {};

// For debugging in json file
bool JSON::debug_file(string path, string json)
{
  ofstream file(path);

  if (!file.is_open())
  {
    return false;
  }

  file << format(json);
  file.close();

  return true;
}

// Helpers
string JSON::quote(string str)
{
  string escaped;
  for (char c : str)
  {
    switch (c)
    {
    case '\"':
      escaped += "\\\"";
      break;
    case '\\':
      escaped += "\\\\";
      break;
    case '\b':
      escaped += "\\b";
      break;
    case '\f':
      escaped += "\\f";
      break;
    case '\n':
      escaped += "\\n";
      break;
    case '\r':
      escaped += "\\r";
      break;
    case '\t':
      escaped += "\\t";
      break;
    default:
      escaped += c;
      break;
    }
  }
  return "\"" + escaped + "\"";
}

string JSON::node_type(const AstNode *node)
{
  return quote("type") + ":" + quote(AstNode::get_node_name(node));
}

string JSON::node_loc(const AstNode *node)
{
  string start_line = quote("start_line") + ":" + to_string(node->start_line);
  string end_line = quote("end_line") + ":" + to_string(node->end_line);
  string node_start = quote("node_start") + ":" + to_string(node->node_start);
  string node_end = quote("node_end") + ":" + to_string(node->node_end);

  return start_line + "," + end_line + "," + node_start + "," + node_end;
}

string JSON::format(string json)
{
  string formatted;
  int indent_level = 0;
  bool in_quotes = false;

  for (int i = 0; i < json.length(); ++i)
  {
    char ch = json[i];

    if (ch == '\\' && i + 1 < json.length() && json[i + 1] == '\"')
    {
      formatted += '\"';
      in_quotes = !in_quotes;
      ++i;
      continue;
    }

    switch (ch)
    {
    case '\"':
      if (i == 0 || json[i - 1] != '\\')
      {
        in_quotes = !in_quotes;
      }

      formatted += ch;
      break;

    case '{':
    case '[':
      formatted += ch;
      if (ch == '[' && i + 1 < json.length() && json[i + 1] == ']')
      {
        formatted += json[++i];
      }
      else if (ch == '{' && i + 1 < json.length() && json[i + 1] == '}')
      {
        formatted += json[++i];
      }
      else if (!in_quotes)
      {
        formatted += "\n";
        ++indent_level;
        formatted.append(indent_level * 2, ' ');
      }
      break;

    case '}':
    case ']':
      if (!in_quotes)
      {
        formatted += "\n";
        --indent_level;
        formatted.append(indent_level * 2, ' ');
      }
      formatted += ch;
      break;

    case ',':
      formatted += ch;
      if (!in_quotes)
      {
        formatted += "\n";
        formatted.append(indent_level * 2, ' ');
      }
      break;

    case ':':
      formatted += ch;
      if (!in_quotes)
      {
        formatted += " ";
      }
      break;

    default:
      formatted += ch;
      break;
    }
  }

  return formatted;
}

// Tokens to json
string JSON::stringify_token(Token tk)
{
  string type = quote("type") + ":" + quote(Token::get_token_name(tk.type));
  string line = quote("line") + ":" + to_string(tk.line);
  string start = quote("start") + ":" + to_string(tk.start);
  string end = quote("end") + ":" + to_string(tk.end);

  string value_str = tk.value;
  if (tk.type == TokenType::ERROR)
  {
    // Process the value to add indentation after newlines
    string processed_value;
    size_t last_pos = 0;
    size_t newline_pos = value_str.find('\n', last_pos);
    while (newline_pos != string::npos)
    {
      processed_value += value_str.substr(last_pos, newline_pos - last_pos + 1); // include the newline
      processed_value += " ";
      last_pos = newline_pos + 1;
      newline_pos = value_str.find('\n', last_pos);
    }
    processed_value += value_str.substr(last_pos);
    value_str = processed_value;
  }

  string val = quote("value") + ":" + quote(value_str);

  return "{" + type + "," + val + "," + line + "," + start + "," + end + "}";
}

string JSON::stringify_tokens_stream(vector<Token> tokens)
{
  string stream = "[ " + JSON::stringify_token(tokens.front());

  for (int i = 1; i < tokens.size(); ++i)
  {
    stream += "," + JSON::stringify_token(tokens[i]);
  }

  return stream + "]";
}

// AST Nodes to json
string JSON::stringify_node(const AstNode *node)
{
  if (dynamic_cast<const Source *>(node))
  {
    const Source *src = static_cast<const Source *>(node);
    return stringify_source(src);
  }
  else if (dynamic_cast<const ProgramDefinition *>(node))
  {
    const ProgramDefinition *program = static_cast<const ProgramDefinition *>(node);
    return stringify_program(program);
  }
  else if (dynamic_cast<const FunctionDefinition *>(node))
  {
    const FunctionDefinition *func = static_cast<const FunctionDefinition *>(node);
    return stringify_function(func);
  }
  else if (dynamic_cast<const Statement *>(node))
  {
    const Statement *stmt = static_cast<const Statement *>(node);
    return stringify_stmt(stmt);
  }

  return stringify_ast_node(node);
}

string JSON::stringify_ast_node(const AstNode *node)
{
  return "{" + node_type(node) + "," + node_loc(node) + "}";
}

// Root Node to json
string JSON::stringify_source(const Source *src)
{
  string type = node_type(src);
  string loc = node_loc(src);
  string program = quote("program") + ":" + stringify_program(src->program);
  string functions = quote("functions") + ":[";

  for (int i = 0; i < src->functions.size(); ++i)
  {
    functions += stringify_function(src->functions[i]);

    if (i != src->functions.size() - 1)
    {
      functions += ",";
    }
  }

  functions += "]";

  return "{" + type + "," + loc + "," + program + "," + functions + "}";
}

// Definitions to json
string JSON::stringify_program(const ProgramDefinition *node)
{
  string type = node_type(node);
  string loc = node_loc(node);
  string program_name = quote("program_name") + ":" + stringify_identifier(node->program_name);
  string globals = quote("globals") + ":[";
  string body = quote("body") + ":[";

  for (int i = 0; i < node->globals.size(); ++i)
  {
    globals += stringify_stmt(node->globals[i]);

    if (i != node->globals.size() - 1)
    {
      globals += ",";
    }
  }

  for (int i = 0; i < node->body.size(); ++i)
  {
    body += stringify_stmt(node->body[i]);

    if (i != node->body.size() - 1)
    {
      body += ",";
    }
  }

  globals += "]";
  body += "]";

  return "{" + type + "," + loc + "," + program_name + "," + globals + "," + body + "}";
}

string JSON::stringify_function(const FunctionDefinition *node)
{
  string type = node_type(node);
  string loc = node_loc(node);
  string name = quote("name") + ":" + stringify_identifier(node->funcname);
  string return_type = quote("returns") + ":" + stringify_return_type(node->return_type);
  string parameters = quote("parameters") + ":[";
  string body = quote("body") + ":[";

  for (int i = 0; i < node->parameters.size(); ++i)
  {
    parameters += stringify_stmt(node->parameters[i]);

    if (i != node->parameters.size() - 1)
    {
      parameters += ",";
    }
  }

  for (int i = 0; i < node->body.size(); ++i)
  {
    body += stringify_stmt(node->body[i]);

    if (i != node->body.size() - 1)
    {
      body += ",";
    }
  }

  parameters += "]";
  body += "]";

  return "{" + type + "," + loc + "," + name + "," + return_type + "," + parameters + "," + body + "}";
}

string JSON::stringify_var_def(const VariableDefinition *node)
{
  string type = node_type(node);
  string loc = node_loc(node);
  string definition = quote("definition") + ":";

  if (dynamic_cast<const VariableDeclaration *>(node->def))
  {
    const VariableDeclaration *dec = static_cast<const VariableDeclaration *>(node->def);
    definition += stringify_var_dec(dec);
  }
  else if (dynamic_cast<const VariableInitialization *>(node->def))
  {
    const VariableInitialization *init = static_cast<const VariableInitialization *>(node->def);
    definition += stringify_var_init(init);
  }

  return "{" + type + "," + loc + "," + definition + "}";
}

string JSON::stringify_var_dec(const VariableDeclaration *node)
{
  string type = node_type(node);
  string loc = node_loc(node);
  string datatype = quote("datatype") + ":" + stringify_type(node->datatype);
  string vars = quote("variables") + ":[";

  for (int i = 0; i < node->variables.size(); ++i)
  {
    vars += stringify_identifier(node->variables[i]);

    if (i != node->variables.size() - 1)
    {
      vars += ",";
    }
  }

  vars += "]";

  return "{" + type + "," + loc + "," + vars + "," + datatype + "}";
}

string JSON::stringify_var_init(const VariableInitialization *node)
{
  string type = node_type(node);
  string loc = node_loc(node);
  string name = quote("name") + ":" + stringify_identifier(node->name);
  string datatype = quote("datatype") + ":" + stringify_type(node->datatype);
  string initializer = quote("initializer") + ":" + stringify_expr(node->initializer);

  return "{" + type + "," + loc + "," + name + "," + datatype + "," + initializer + "}";
}

// Statements to json
string JSON::stringify_stmt(const Statement *stmt)
{
  if (dynamic_cast<const Expression *>(stmt))
  {
    const Expression *expr = static_cast<const Expression *>(stmt);
    return stringify_expr(expr);
  }
  else if (dynamic_cast<const IfStatement *>(stmt))
  {
    const IfStatement *ifStmt = static_cast<const IfStatement *>(stmt);
    return stringify_if_stmt(ifStmt);
  }
  else if (dynamic_cast<const ReturnStatement *>(stmt))
  {
    const ReturnStatement *rtnStmt = static_cast<const ReturnStatement *>(stmt);
    return stringify_return_stmt(rtnStmt);
  }
  else if (dynamic_cast<const SkipStatement *>(stmt))
  {
    const SkipStatement *skipStmt = static_cast<const SkipStatement *>(stmt);
    return stringify_skip_stmt(skipStmt);
  }
  else if (dynamic_cast<const StopStatement *>(stmt))
  {
    const StopStatement *stopStmt = static_cast<const StopStatement *>(stmt);
    return stringify_stop_stmt(stopStmt);
  }
  else if (dynamic_cast<const ReadStatement *>(stmt))
  {
    const ReadStatement *readStmt = static_cast<const ReadStatement *>(stmt);
    return stringify_read_stmt(readStmt);
  }
  else if (dynamic_cast<const WriteStatement *>(stmt))
  {
    const WriteStatement *writeStmt = static_cast<const WriteStatement *>(stmt);
    return stringify_write_stmt(writeStmt);
  }
  else if (dynamic_cast<const WhileLoop *>(stmt))
  {
    const WhileLoop *loop = static_cast<const WhileLoop *>(stmt);
    return stringify_while_loop(loop);
  }
  else if (dynamic_cast<const ForLoop *>(stmt))
  {
    const ForLoop *loop = static_cast<const ForLoop *>(stmt);
    return stringify_for_loop(loop);
  }
  else if (dynamic_cast<const VariableDefinition *>(stmt))
  {
    const VariableDefinition *def = static_cast<const VariableDefinition *>(stmt);
    return stringify_var_def(def);
  }
  else if (dynamic_cast<const VariableDeclaration *>(stmt))
  {
    const VariableDeclaration *dec = static_cast<const VariableDeclaration *>(stmt);
    return stringify_var_dec(dec);
  }
  else if (dynamic_cast<const VariableInitialization *>(stmt))
  {
    const VariableInitialization *init = static_cast<const VariableInitialization *>(stmt);
    return stringify_var_init(init);
  }

  return stringify_ast_node(stmt);
}

string JSON::stringify_if_stmt(const IfStatement *stmt)
{
  string type = node_type(stmt);
  string loc = node_loc(stmt);
  string condition = quote("condition") + ":" + stringify_expr(stmt->condition);
  string consequent = quote("consequent") + ":[";
  string alternate = quote("alternate") + ":[";

  for (int i = 0; i < stmt->consequent.size(); ++i)
  {
    consequent += stringify_stmt(stmt->consequent[i]);

    if (i != stmt->consequent.size() - 1)
    {
      consequent += ",";
    }
  }

  for (int i = 0; i < stmt->alternate.size(); ++i)
  {
    alternate += stringify_stmt(stmt->alternate[i]);

    if (i != stmt->alternate.size() - 1)
    {
      alternate += ",";
    }
  }

  consequent += "]";
  alternate += "]";

  return "{" + type + "," + loc + "," + condition + "," + consequent + "," + alternate + "}";
}

string JSON::stringify_return_stmt(const ReturnStatement *stmt)
{
  string type = node_type(stmt);
  string loc = node_loc(stmt);
  string value = quote("value") + ":" + (stmt->returnValue == nullptr ? "null" : stringify_expr(stmt->returnValue));

  return "{" + type + "," + loc + "," + value + "}";
}

string JSON::stringify_skip_stmt(const SkipStatement *stmt)
{
  return stringify_ast_node(stmt);
}

string JSON::stringify_stop_stmt(const StopStatement *stmt)
{
  return stringify_ast_node(stmt);
}

string JSON::stringify_read_stmt(const ReadStatement *stmt)
{
  string type = node_type(stmt);
  string loc = node_loc(stmt);
  string vars = quote("variables") + ":[";

  for (int i = 0; i < stmt->variables.size(); ++i)
  {
    vars += stringify_stmt(stmt->variables[i]);

    if (i != stmt->variables.size() - 1)
    {
      vars += ",";
    }
  }

  vars += "]";

  return "{" + type + "," + loc + "," + vars + "}";
}

string JSON::stringify_write_stmt(const WriteStatement *stmt)
{
  string type = node_type(stmt);
  string loc = node_loc(stmt);
  string args = quote("expressions") + ":[";

  for (int i = 0; i < stmt->args.size(); ++i)
  {
    args += stringify_stmt(stmt->args[i]);

    if (i != stmt->args.size() - 1)
    {
      args += ",";
    }
  }

  args += "]";

  return "{" + type + "," + loc + "," + args + "}";
}

// Loops to json
string JSON::stringify_while_loop(const WhileLoop *loop)
{
  string type = node_type(loop);
  string loc = node_loc(loop);
  string condition = quote("condition") + ":" + stringify_expr(loop->condition);
  string body = quote("body") + ":[";

  for (int i = 0; i < loop->body.size(); ++i)
  {
    body += stringify_stmt(loop->body[i]);

    if (i != loop->body.size() - 1)
    {
      body += ",";
    }
  }

  body += "]";

  return "{" + type + "," + loc + "," + condition + "," + body + "}";
}

string JSON::stringify_for_loop(const ForLoop *loop)
{
  string type = node_type(loop);
  string loc = node_loc(loop);
  string init = quote("init") + ":" + stringify_assignment_expr(loop->init);
  string condition = quote("condition") + ":" + stringify_expr(loop->condition);
  string update = quote("update") + ":" + stringify_expr(loop->update);
  string body = quote("body") + ":[";

  for (int i = 0; i < loop->body.size(); ++i)
  {
    body += stringify_stmt(loop->body[i]);

    if (i != loop->body.size() - 1)
    {
      body += ",";
    }
  }

  body += "]";

  return "{" + type + "," + loc + "," + init + "," + condition + "," + update + "," + body + "}";
}

// Expressions to json
string JSON::stringify_expr(const Expression *expr)
{
  if (dynamic_cast<const Literal *>(expr))
  {
    const Literal *litNode = static_cast<const Literal *>(expr);
    return stringify_literal(litNode);
  }
  else if (dynamic_cast<const AssignableExpression *>(expr))
  {
    const AssignableExpression *assignable = static_cast<const AssignableExpression *>(expr);
    return stringify_assignable_expr(assignable);
  }
  else if (dynamic_cast<const AssignmentExpression *>(expr))
  {
    const AssignmentExpression *assExpr = static_cast<const AssignmentExpression *>(expr);
    return stringify_assignment_expr(assExpr);
  }
  else if (dynamic_cast<const OrExpression *>(expr))
  {
    const OrExpression *orExpr = static_cast<const OrExpression *>(expr);
    return stringify_or_expr(orExpr);
  }
  else if (dynamic_cast<const AndExpression *>(expr))
  {
    const AndExpression *andExpr = static_cast<const AndExpression *>(expr);
    return stringify_and_expr(andExpr);
  }
  else if (dynamic_cast<const EqualityExpression *>(expr))
  {
    const EqualityExpression *eqExpr = static_cast<const EqualityExpression *>(expr);
    return stringify_equality_expr(eqExpr);
  }
  else if (dynamic_cast<const RelationalExpression *>(expr))
  {
    const RelationalExpression *relExpr = static_cast<const RelationalExpression *>(expr);
    return stringify_relational_expr(relExpr);
  }
  else if (dynamic_cast<const AdditiveExpression *>(expr))
  {
    const AdditiveExpression *addExpr = static_cast<const AdditiveExpression *>(expr);
    return stringify_additive_expr(addExpr);
  }
  else if (dynamic_cast<const MultiplicativeExpression *>(expr))
  {
    const MultiplicativeExpression *mulExpr = static_cast<const MultiplicativeExpression *>(expr);
    return stringify_multiplicative_expr(mulExpr);
  }
  else if (dynamic_cast<const UnaryExpression *>(expr))
  {
    const UnaryExpression *unaryExpr = static_cast<const UnaryExpression *>(expr);
    return stringify_unary_expr(unaryExpr);
  }
  else if (dynamic_cast<const CallFunctionExpression *>(expr))
  {
    const CallFunctionExpression *cfExpr = static_cast<const CallFunctionExpression *>(expr);
    return stringify_call_function_expr(cfExpr);
  }
  else if (dynamic_cast<const PrimaryExpression *>(expr))
  {
    const PrimaryExpression *primaryExpr = static_cast<const PrimaryExpression *>(expr);
    return stringify_primary_expr(primaryExpr);
  }

  return stringify_ast_node(expr);
}

string JSON::stringify_assignable_expr(const AssignableExpression *expr)
{
  if (dynamic_cast<const Identifier *>(expr))
  {
    const Identifier *idNode = static_cast<const Identifier *>(expr);
    return stringify_identifier(idNode);
  }
  else if (dynamic_cast<const IndexExpression *>(expr))
  {
    const IndexExpression *idxExpr = static_cast<const IndexExpression *>(expr);
    return stringify_index_expr(idxExpr);
  }

  return stringify_ast_node(expr);
}

string JSON::stringify_boolean_expr(const string &exprType, const Expression *exprLeft, const Expression *exprRight)
{
  string type = quote("type") + ":" + quote(exprType);
  string left = quote("left") + ":" + stringify_expr(exprLeft);
  string right = quote("right") + ":" + stringify_expr(exprRight);

  return "{" + type + "," + left + "," + right + "}";
}

string JSON::stringify_binary_expr(const string &exprType, const Expression *exprLeft, const Expression *exprRight, const string &optr)
{
  string type = quote("type") + ":" + quote(exprType);
  string left = quote("left") + ":" + stringify_expr(exprLeft);
  string right = quote("right") + ":" + stringify_expr(exprRight);
  string op = quote("operator") + ":" + quote(optr);

  return "{" + type + "," + op + "," + left + "," + right + "}";
}

string JSON::stringify_assignment_expr(const AssignmentExpression *assExpr)
{
  string type = node_type(assExpr);
  string loc = node_loc(assExpr);
  string assignee = quote("assignee") + ":" + stringify_assignable_expr(assExpr->assignee);
  string value = quote("value") + ":" + stringify_expr(assExpr->value);

  return "{" + type + "," + loc + "," + assignee + "," + value + "}";
}

string JSON::stringify_or_expr(const OrExpression *orExpr)
{
  return stringify_boolean_expr(AstNode::get_node_name(orExpr), orExpr->left, orExpr->right);
}

string JSON::stringify_and_expr(const AndExpression *andExpr)
{
  return stringify_boolean_expr(AstNode::get_node_name(andExpr), andExpr->left, andExpr->right);
}

string JSON::stringify_equality_expr(const EqualityExpression *eqExpr)
{
  return stringify_binary_expr(AstNode::get_node_name(eqExpr), eqExpr->left, eqExpr->right, eqExpr->optr);
}

string JSON::stringify_relational_expr(const RelationalExpression *relExpr)
{
  return stringify_binary_expr(AstNode::get_node_name(relExpr), relExpr->left, relExpr->right, relExpr->optr);
}

string JSON::stringify_additive_expr(const AdditiveExpression *addExpr)
{
  return stringify_binary_expr(AstNode::get_node_name(addExpr), addExpr->left, addExpr->right, addExpr->optr);
}

string JSON::stringify_multiplicative_expr(const MultiplicativeExpression *mulExpr)
{
  return stringify_binary_expr(AstNode::get_node_name(mulExpr), mulExpr->left, mulExpr->right, mulExpr->optr);
}

string JSON::stringify_unary_expr(const UnaryExpression *unaryExpr)
{
  string type = node_type(unaryExpr);
  string loc = node_loc(unaryExpr);
  string operand = quote("operand") + ":" + stringify_expr(unaryExpr->operand);
  string optr = quote("operator") + ":" + quote(unaryExpr->optr);
  string postfix = quote("postfix") + ":" + (unaryExpr->postfix ? "true" : "false");

  return "{" + type + "," + loc + "," + operand + "," + optr + "," + postfix + "}";
}

string JSON::stringify_call_function_expr(const CallFunctionExpression *cfExpr)
{
  string type = node_type(cfExpr);
  string loc = node_loc(cfExpr);
  string function = quote("function") + ":" + stringify_identifier(cfExpr->function);
  string args = quote("arguments") + ":[";

  for (int i = 0; i < cfExpr->arguments.size(); ++i)
  {
    args += stringify_expr(cfExpr->arguments[i]);

    if (i != cfExpr->arguments.size() - 1)
    {
      args += ",";
    }
  }

  args += "]";

  return "{" + type + "," + loc + "," + function + "," + args + "}";
}

string JSON::stringify_index_expr(const IndexExpression *idxExpr)
{
  string type = node_type(idxExpr);
  string loc = node_loc(idxExpr);
  string base = quote("base") + ":" + stringify_expr(idxExpr->base);
  string index = quote("index") + ":" + stringify_expr(idxExpr->index);

  return "{" + type + "," + loc + "," + base + "," + index + "}";
}

string JSON::stringify_primary_expr(const PrimaryExpression *primaryExpr)
{
  string type = node_type(primaryExpr);
  string loc = node_loc(primaryExpr);
  string value = quote("value") + ":" + stringify_literal(primaryExpr->value);

  return "{" + type + "," + loc + "," + value + "}";
}

// Types
string JSON::stringify_type(const DataType *type)
{
  if (dynamic_cast<const PrimitiveDataType *>(type))
  {
    const PrimitiveDataType *prime = static_cast<const PrimitiveDataType *>(type);
    return stringify_primitive_type(prime);
  }
  else if (dynamic_cast<const ArrayDataType *>(type))
  {
    const ArrayDataType *array = static_cast<const ArrayDataType *>(type);
    return stringify_array_type(array);
  }

  return stringify_ast_node(type);
}

string JSON::stringify_primitive_type(const PrimitiveDataType *prime)
{
  string type = node_type(prime);
  string loc = node_loc(prime);
  string datatype = quote("datatype") + ":" + quote(prime->datatype);

  return "{" + type + "," + loc + "," + datatype + "}";
}

string JSON::stringify_array_type(const ArrayDataType *array)
{
  string type = node_type(array);
  string loc = node_loc(array);
  string datatype = quote("datatype") + ":" + quote(array->datatype);
  string dim = quote("dimension") + ":" + to_string(array->dimension);

  return "{" + type + "," + loc + "," + datatype + "," + dim + "}";
}

string JSON::stringify_return_type(const ReturnType *ret)
{
  string type = node_type(ret);
  string loc = node_loc(ret);
  string datatype = quote("return_type") + ":" + stringify_type(ret->return_type);

  return "{" + type + "," + loc + "," + datatype + "}";
}

// Literals to json
string JSON::stringify_literal(const Literal *litNode)
{
  if (dynamic_cast<const IntegerLiteral *>(litNode))
  {
    const IntegerLiteral *intNode = static_cast<const IntegerLiteral *>(litNode);
    return stringify_integer_literal(intNode);
  }
  else if (dynamic_cast<const FloatLiteral *>(litNode))
  {
    const FloatLiteral *fNode = static_cast<const FloatLiteral *>(litNode);
    return stringify_float_literal(fNode);
  }
  else if (dynamic_cast<const StringLiteral *>(litNode))
  {
    const StringLiteral *strNode = static_cast<const StringLiteral *>(litNode);
    return stringify_string_literal(strNode);
  }
  else if (dynamic_cast<const BooleanLiteral *>(litNode))
  {
    const BooleanLiteral *boolNode = static_cast<const BooleanLiteral *>(litNode);
    return stringify_boolean_literal(boolNode);
  }
  else if (dynamic_cast<const ArrayLiteral *>(litNode))
  {
    const ArrayLiteral *arrNode = static_cast<const ArrayLiteral *>(litNode);
    return stringify_array_literal(arrNode);
  }

  return stringify_ast_node(litNode);
}

string JSON::stringify_identifier(const Identifier *idNode)
{
  string type = node_type(idNode);
  string loc = node_loc(idNode);
  string value = quote("name") + ":" + quote(idNode->name);

  return "{" + type + "," + loc + "," + value + "}";
}

string JSON::stringify_integer_literal(const IntegerLiteral *intNode)
{
  string type = node_type(intNode);
  string loc = node_loc(intNode);
  string value = quote("value") + ":" + to_string(intNode->value);

  return "{" + type + "," + loc + "," + value + "}";
}

string JSON::stringify_float_literal(const FloatLiteral *fNode)
{
  string type = node_type(fNode);
  string loc = node_loc(fNode);
  string value = quote("value") + ":" + to_string(fNode->value);

  return "{" + type + "," + loc + "," + value + "}";
}

string JSON::stringify_string_literal(const StringLiteral *strNode)
{
  string type = node_type(strNode);
  string loc = node_loc(strNode);
  string value = quote("value") + ":" + quote(strNode->value);

  return "{" + type + "," + loc + "," + value + "}";
}

string JSON::stringify_boolean_literal(const BooleanLiteral *boolNode)
{

  string type = node_type(boolNode);
  string loc = node_loc(boolNode);
  string value = quote("value") + ":" + (boolNode->value ? "true" : "false");

  return "{" + type + "," + loc + "," + value + "}";
}

string JSON::stringify_array_literal(const ArrayLiteral *arrNode)
{
  string type = node_type(arrNode);
  string loc = node_loc(arrNode);
  string elements = quote("elements") + ":[";

  for (int i = 0; i < arrNode->elements.size(); ++i)
  {
    elements += stringify_expr(arrNode->elements[i]);

    if (i != arrNode->elements.size() - 1)
    {
      elements += ",";
    }
  }

  elements += "]";

  return "{" + type + "," + loc + "," + elements + "}";
}
