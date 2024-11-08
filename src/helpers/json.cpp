#include "json.h"

JSON::JSON() {};

// Helpers
string JSON::quote(string str)
{
  return "\\\"" + str + "\\\"";
}

string JSON::pair(string type, string value)
{
  return "{ \\\"type\\\": " + type + ", \\\"value\\\": " + value + " }";
}

string JSON::quoted_pair(string type, string value)
{
  return pair(quote(type), quote(value));
}

// Tokens to json
string JSON::stringify_token(Token tk)
{
  return quoted_pair(Token::get_token_name(tk.type), tk.value);
}

string JSON::stringify_tokens_stream(vector<Token> tokens)
{
  string stream = "[ " + JSON::stringify_token(tokens.front());

  for (int i = 1; i < tokens.size(); ++i)
  {
    stream += "," + JSON::stringify_token(tokens[i]);
  }

  return stream + " ]";
}

string JSON::stringify_node(const AstNode *node)
{
  if (dynamic_cast<const Statement *>(node))
  {
    const Statement *stmt = static_cast<const Statement *>(node);
    return stringify_stmt(stmt);
  }

  return stringify_ast_node(node);
}

string JSON::stringify_ast_node(const AstNode *node)
{
  return "{ " + quote("type") + ": " + quote(AstNode::get_node_name(node)) + " }";
}

// Statements to json
string JSON::stringify_stmt(const Statement *stmt)
{
  if (dynamic_cast<const Expression *>(stmt))
  {
    const Expression *expr = static_cast<const Expression *>(stmt);
    return stringify_expr(expr);
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
}

// Loops to json
string JSON::stringify_while_loop(const WhileLoop *loop)
{
  string type = quote("type") + ": " + quote(AstNode::get_node_name(loop));
  string condition = quote("condition") + ": " + stringify_expr(loop->condition);
  string body = quote("body") + ": [";

  for (int i = 0; i < loop->body.size(); ++i)
  {
    body += stringify_stmt(loop->body[i]);

    if (i != loop->body.size() - 1)
    {
      body += ", ";
    }
  }

  body += " ]";

  return "{ " + type + ", " + condition + ", " + body + " }";
}

string JSON::stringify_for_loop(const ForLoop *loop)
{
  string type = quote("type") + ": " + quote(AstNode::get_node_name(loop));
  string init = quote("init") + stringify_assignment_expr(loop->init);
  string condition = quote("condition") + ": " + stringify_expr(loop->condition);
  string update = quote("update") + ": " + stringify_expr(loop->update);
  string body = quote("body") + ": [";

  for (int i = 0; i < loop->body.size(); ++i)
  {
    body += stringify_stmt(loop->body[i]);

    if (i != loop->body.size() - 1)
    {
      body += ", ";
    }
  }

  body += " ]";

  return "{ " + type + ", " + init + ", " + condition + ", " + update + ", " + body + " }";
}

// Expressions to json
string JSON::stringify_expr(const Expression *expr)
{
  if (dynamic_cast<const Literal *>(expr))
  {
    const Literal *litNode = static_cast<const Literal *>(expr);
    return stringify_literal(litNode);
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
  else if (dynamic_cast<const IndexExpression *>(expr))
  {
    const IndexExpression *idxExpr = static_cast<const IndexExpression *>(expr);
    return stringify_index_expr(idxExpr);
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

string JSON::stringify_boolean_expr(const string &exprType, const Expression *exprLeft, const Expression *exprRight)
{
  string type = quote("type") + ": " + quote(exprType);
  string left = quote("left") + ": " + stringify_expr(exprLeft);
  string right = quote("right") + ": " + stringify_expr(exprRight);

  return "{ " + type + ", " + left + ", " + right + " }";
}

string JSON::stringify_binary_expr(const string &exprType, const Expression *exprLeft, const Expression *exprRight, const string &optr)
{
  string type = quote("type") + ": " + quote(exprType);
  string left = quote("left") + ": " + stringify_expr(exprLeft);
  string right = quote("right") + ": " + stringify_expr(exprRight);
  string op = quote("operator") + ": " + quote(optr);

  return "{ " + type + ", " + op + ", " + left + ", " + right + " }";
}

string JSON::stringify_assignment_expr(const AssignmentExpression *assExpr)
{
  string type = quote("type") + ": " + quote(AstNode::get_node_name(assExpr));
  string assignee = quote("assignee") + ": " + stringify_expr(assExpr->assignee);
  string value = quote("value") + ": " + stringify_expr(assExpr->value);

  return "{ " + type + ", " + assignee + ", " + value + " }";
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
  string type = quote("type") + ": " + quote(AstNode::get_node_name(unaryExpr));
  string operand = quote("operand") + ": " + stringify_expr(unaryExpr->operand);
  string optr = quote("operator") + ": " + quote(unaryExpr->optr);
  string postfix = quote("postfix") + ": " + (unaryExpr->postfix ? "true" : "false");

  return "{" + type + ", " + operand + ", " + optr + ", " + postfix + "}";
}

string JSON::stringify_call_function_expr(const CallFunctionExpression *cfExpr)
{
  string type = quote("type") + ": " + quote(AstNode::get_node_name(cfExpr));
  string function = quote("function") + ": " + stringify_identifier_literal(cfExpr->function);
  string args = quote("arguments") + ": [";

  for (int i = 0; i < cfExpr->arguments.size(); ++i)
  {
    args += stringify_expr(cfExpr->arguments[i]);

    if (i != cfExpr->arguments.size() - 1)
    {
      args += ",";
    }
  }

  args += "]";

  return "{ " + type + ", " + function + ", " + args + " }";
}

string JSON::stringify_index_expr(const IndexExpression *idxExpr)
{
  string type = quote("type") + ": " + quote(AstNode::get_node_name(idxExpr));
  string base = stringify_expr(idxExpr->base);
  string index = stringify_expr(idxExpr->index);

  return "{ " + type + ", " + base + ", " + index + " }";
}

string JSON::stringify_primary_expr(const PrimaryExpression *primaryExpr)
{
  return pair(quote(AstNode::get_node_name(primaryExpr)), stringify_literal(primaryExpr->value));
}

// Literals to json
string JSON::stringify_literal(const Literal *litNode)
{
  if (dynamic_cast<const Identifier *>(litNode))
  {
    const Identifier *idNode = static_cast<const Identifier *>(litNode);
    return stringify_identifier_literal(idNode);
  }
  else if (dynamic_cast<const IntegerLiteral *>(litNode))
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

string JSON::stringify_identifier_literal(const Identifier *idNode)
{
  return quoted_pair(AstNode::get_node_name(idNode), idNode->name);
}

string JSON::stringify_integer_literal(const IntegerLiteral *intNode)
{
  return pair(quote(AstNode::get_node_name(intNode)), to_string(intNode->value));
}

string JSON::stringify_float_literal(const FloatLiteral *fNode)
{
  return pair(quote(AstNode::get_node_name(fNode)), to_string(fNode->value));
}

string JSON::stringify_string_literal(const StringLiteral *strNode)
{
  return quoted_pair(AstNode::get_node_name(strNode), strNode->value);
}

string JSON::stringify_boolean_literal(const BooleanLiteral *boolNode)
{
  string value = boolNode->value ? "true" : "false";
  return pair(quote(AstNode::get_node_name(boolNode)), value);
}

string JSON::stringify_array_literal(const ArrayLiteral *arrNode)
{
  string type = quote("type") + ": " + quote(AstNode::get_node_name(arrNode));
  string elements = quote("elements") + ": [";

  for (int i = 0; i < arrNode->elements.size(); ++i)
  {
    elements += stringify_literal(arrNode->elements[i]);

    if (i != arrNode->elements.size() - 1)
    {
      elements += ",";
    }
  }

  elements += "]";

  return "{ " + type + ", " + elements + " }";
}
