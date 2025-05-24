#include "jsonrpc/utils/convert.h"

namespace json::utils
{
  // converting token to json object
  Object *convert_token(const rbn::core::Token *token)
  {
    Object *obj = new Object();

    obj->add("type", new String(rbn::core::Token::get_token_name(token->type)));
    obj->add("value", new String(token->value));
    obj->add("line", new Number(token->line));
    obj->add("start", new Number(token->start));
    obj->add("end", new Number(token->end));

    return obj;
  }

  // converting token stream to json array
  Array *convert_token_stream(const vector<rbn::core::Token> *tokens)
  {
    Array *stream = new Array();

    for (const auto &token : *tokens)
    {
      Object *obj = new Object();

      obj->add("type", new String(rbn::core::Token::get_token_name(token.type)));
      obj->add("value", new String(token.value));
      obj->add("line", new Number(token.line));
      obj->add("start", new Number(token.start));
      obj->add("end", new Number(token.end));

      stream->add(obj);
    }

    return stream;
  }

  // converting ast node to json object
  Object *get_node_attributes(const rbn::ast::AstNode *node)
  {
    Object *attributes = new Object();

    string location_start = "line: " + to_string(node->start_line) + ", col: " + to_string(node->node_start);
    string location_end = "line: " + to_string(node->end_line) + ", col: " + to_string(node->node_end);

    attributes->add("location", new String(location_start + " - " + location_end));

    return attributes;
  }

  Object *create_child(string name, Value *value)
  {
    Object *child = new Object();
    child->add("name", new String(name));
    child->add("value", value);

    return child;
  }

  Object *preper_node(const rbn::ast::AstNode *node, Object *attributes, Array *children)
  {
    Object *obj = new Object();

    obj->add("name", new String(rbn::ast::AstNode::get_node_name(node)));
    obj->add("attributes", attributes);
    obj->add("children", children);

    return obj;
  }

  Object *preper_statement(const rbn::ast::Statement *node)
  {
    if (auto *expr = dynamic_cast<const rbn::ast::Expression *>(node))
    {
      return preper_expression(expr);
    }

    Object *obj = new Object();
    Object *attributes = get_node_attributes(node);
    Array *children = new Array();

    if (auto *ifStmt = dynamic_cast<const rbn::ast::IfStatement *>(node))
    {
      Array *consequent = new Array();
      Array *alternate = new Array();

      for (const auto &stmt : ifStmt->consequent)
      {
        consequent->add(convert_ast_node(stmt));
      }

      for (const auto &stmt : ifStmt->alternate)
      {
        alternate->add(convert_ast_node(stmt));
      }

      children->add(create_child("condition", convert_ast_node(ifStmt->condition)));
      children->add(create_child("consequent", consequent));
      children->add(create_child("alternate", alternate));
    }
    else if (auto *whileStmt = dynamic_cast<const rbn::ast::WhileLoop *>(node))
    {

      Array *body = new Array();

      for (const auto &stmt : whileStmt->body)
      {
        body->add(convert_ast_node(stmt));
      }

      children->add(create_child("condition", convert_ast_node(whileStmt->condition)));
      children->add(create_child("body", body));
    }
    else if (auto *forStmt = dynamic_cast<const rbn::ast::ForLoop *>(node))
    {
      Array *body = new Array();

      for (const auto &stmt : forStmt->body)
      {
        body->add(convert_ast_node(stmt));
      }

      children->add(create_child("initializer", convert_ast_node(forStmt->init)));
      children->add(create_child("condition", convert_ast_node(forStmt->condition)));
      children->add(create_child("updator", convert_ast_node(forStmt->update)));
      children->add(create_child("body", body));
    }
    else if (auto *returnStmt = dynamic_cast<const rbn::ast::ReturnStatement *>(node))
    {
      children->add(create_child("value", convert_ast_node(returnStmt->returnValue)));
    }
    else if (auto *readStmt = dynamic_cast<const rbn::ast::ReadStatement *>(node))
    {
      Array *variables = new Array();

      for (const auto &variable : readStmt->variables)
      {
        variables->add(convert_ast_node(variable));
      }

      string var_count = "[" + to_string(readStmt->variables.size()) + "Variables]";
      attributes->add("variables", new String(var_count));
      children->add(create_child("variables", variables));
    }
    else if (auto *writeStmt = dynamic_cast<const rbn::ast::WriteStatement *>(node))
    {
      Array *args = new Array();

      for (const auto &arg : writeStmt->args)
      {
        args->add(convert_ast_node(arg));
      }

      string arg_count = "[" + to_string(writeStmt->args.size()) + "Arguments]";
      attributes->add("arguments", new String(arg_count));
      children->add(create_child("arguments", args));
    }

    return preper_node(node, attributes, children);
  }

  Object *preper_expression(const rbn::ast::Expression *node)
  {
    if (auto *primaryExpr = dynamic_cast<const rbn::ast::PrimaryExpression *>(node))
    {
      return preper_literal(primaryExpr->value);
    }

    Object *obj = new Object();
    Object *attributes = get_node_attributes(node);
    Array *children = new Array();

    if (auto *assExpr = dynamic_cast<const rbn::ast::AssignmentExpression *>(node))
    {
      children->add(create_child("assignee", convert_ast_node(assExpr->assignee)));
      children->add(create_child("value", convert_ast_node(assExpr->value)));
    }
    else if (auto *orExpr = dynamic_cast<const rbn::ast::OrExpression *>(node))
    {
      attributes->add("operator", new String("or"));
      children->add(convert_ast_node(orExpr->left));
      children->add(convert_ast_node(orExpr->right));
    }
    else if (auto *andExpr = dynamic_cast<const rbn::ast::AndExpression *>(node))
    {
      attributes->add("operator", new String("and"));
      children->add(convert_ast_node(andExpr->left));
      children->add(convert_ast_node(andExpr->right));
    }
    else if (auto *eqExpr = dynamic_cast<const rbn::ast::EqualityExpression *>(node))
    {
      attributes->add("operator", new String(eqExpr->optr));
      children->add(create_child("left", convert_ast_node(eqExpr->left)));
      children->add(create_child("right", convert_ast_node(eqExpr->right)));
    }
    else if (auto *relExpr = dynamic_cast<const rbn::ast::RelationalExpression *>(node))
    {
      attributes->add("operator", new String(relExpr->optr));
      children->add(create_child("left", convert_ast_node(relExpr->left)));
      children->add(create_child("right", convert_ast_node(relExpr->right)));
    }
    else if (auto *addExpr = dynamic_cast<const rbn::ast::AdditiveExpression *>(node))
    {
      attributes->add("operator", new String(addExpr->optr));
      children->add(create_child("left", convert_ast_node(addExpr->left)));
      children->add(create_child("right", convert_ast_node(addExpr->right)));
    }
    else if (auto *mulExpr = dynamic_cast<const rbn::ast::MultiplicativeExpression *>(node))
    {
      attributes->add("operator", new String(mulExpr->optr));
      children->add(create_child("left", convert_ast_node(mulExpr->left)));
      children->add(create_child("right", convert_ast_node(mulExpr->right)));
    }
    else if (auto *unaryExpr = dynamic_cast<const rbn::ast::UnaryExpression *>(node))
    {
      attributes->add("operator", new String(unaryExpr->optr));
      attributes->add("postfix", new Boolean(unaryExpr->postfix));
      children->add(create_child("operand", convert_ast_node(unaryExpr->operand)));
    }
    else if (auto *callExpr = dynamic_cast<const rbn::ast::CallFunctionExpression *>(node))
    {
      string argc = "[" + to_string(callExpr->arguments.size()) + "Arguments]";
      Array *argv = new Array();

      attributes->add("name", new String(callExpr->function->name));
      attributes->add("arguments", new String(argc));

      for (const auto &argument : callExpr->arguments)
      {
        argv->add(convert_ast_node(argument));
      }

      children->add(create_child("arguments", argv));
    }
    else if (auto *indexExpr = dynamic_cast<const rbn::ast::IndexExpression *>(node))
    {
      children->add(create_child("index", convert_ast_node(indexExpr->index)));
      children->add(create_child("base", convert_ast_node(indexExpr->base)));
    }

    return preper_node(node, attributes, children);
  }

  Object *preper_literal(const rbn::ast::Literal *node)
  {
    Object *obj = new Object();
    Object *attributes = get_node_attributes(node);
    Array *children = new Array();

    if (auto *strLiteral = dynamic_cast<const rbn::ast::StringLiteral *>(node))
    {
      attributes->add("value", new String(strLiteral->value));
    }
    else if (auto *intLiteral = dynamic_cast<const rbn::ast::IntegerLiteral *>(node))
    {
      attributes->add("value", new Integer(intLiteral->value));
    }
    else if (auto *floatLiteral = dynamic_cast<const rbn::ast::FloatLiteral *>(node))
    {
      attributes->add("value", new Number(floatLiteral->value));
    }
    else if (auto *boolLiteral = dynamic_cast<const rbn::ast::BooleanLiteral *>(node))
    {
      attributes->add("value", new Boolean(boolLiteral->value));
    }
    else if (auto *arrayLiteral = dynamic_cast<const rbn::ast::ArrayLiteral *>(node))
    {
      string value = "[" + to_string(arrayLiteral->elements.size()) + "Elements]";
      attributes->add("value", new String(value));

      Array *elements = new Array();

      for (const auto &element : arrayLiteral->elements)
      {
        elements->add(convert_ast_node(element));
      }

      children->add(create_child("elements", elements));
    }
    else if (auto *identifier = dynamic_cast<const rbn::ast::Identifier *>(node))
    {
      attributes->add("varname", new String(identifier->name));
    }

    return preper_node(node, attributes, children);
  }

  Object *preper_datatype(const rbn::ast::DataType *node)
  {
    Object *obj = new Object();
    Object *attributes = get_node_attributes(node);

    if (auto *primitive = dynamic_cast<const rbn::ast::PrimitiveDataType *>(node))
    {
      attributes->add("type", new String(primitive->datatype));
    }
    else if (auto *array = dynamic_cast<const rbn::ast::ArrayDataType *>(node))
    {
      attributes->add("type", new String(array->datatype));
      attributes->add("dimension", new Number(array->dimension));
    }

    return preper_node(node, attributes, new Array());
  }

  Object *preper_declaration(const rbn::ast::VariableDefinition *node)
  {
    Object *obj = new Object();
    Object *attributes = get_node_attributes(node);
    Array *children = new Array();

    if (auto *varDec = dynamic_cast<const rbn::ast::VariableDeclaration *>(node->def))
    {
      Array *variables = new Array();
      string list = "[" + to_string(varDec->variables.size()) + "Variables]";

      for (const auto &variable : varDec->variables)
      {
        variables->add(create_child("name", new String(variable->name)));
      }

      attributes->add("type", new String("declaration"));
      attributes->add("list", new String(list));
      children->add(create_child("variables", variables));
      children->add(create_child("datatype", convert_ast_node(varDec->datatype)));
    }
    else if (auto *varInit = dynamic_cast<const rbn::ast::VariableInitialization *>(node->def))
    {
      attributes->add("type", new String("initialization"));
      children->add(create_child("identifier", convert_ast_node(varInit->name)));
      children->add(create_child("initializer", convert_ast_node(varInit->initializer)));
      children->add(create_child("datatype", convert_ast_node(varDec->datatype)));
    }

    return preper_node(node, attributes, children);
  }

  Object *preper_function(const rbn::ast::FunctionDefinition *node)
  {
    Object *obj = new Object();
    Object *attributes = get_node_attributes(node);
    Array *children = new Array();

    string params = "[" + to_string(node->parameters.size()) + "Parameters]";

    Array *parameters = new Array();
    for (const auto &parameter : node->parameters)
    {
      parameters->add(convert_ast_node(parameter));
    }

    Array *body = new Array();
    for (const auto &stmt : node->body)
    {
      body->add(convert_ast_node(stmt));
    }

    attributes->add("params", new String(params));

    children->add(create_child("funcName", convert_ast_node(node->funcname)));
    children->add(create_child("returnType", convert_ast_node(node->return_type)));
    children->add(create_child("parameters", parameters));
    children->add(create_child("body", body));

    return preper_node(node, attributes, children);
  }

  Object *preper_program(const rbn::ast::ProgramDefinition *node)
  {
    Object *obj = new Object();
    Object *attributes = get_node_attributes(node);
    Array *children = new Array();

    string globals_count = "[" + to_string(node->globals.size()) + "Globals]";

    Array *globals = new Array();
    for (const auto &global : node->globals)
    {
      globals->add(convert_ast_node(global));
    }

    Array *body = new Array();
    for (const auto &stmt : node->body)
    {
      body->add(convert_ast_node(stmt));
    }

    attributes->add("globals", new String(globals_count));

    children->add(create_child("programName", convert_ast_node(node->program_name)));
    children->add(create_child("globals", globals));
    children->add(create_child("body", body));

    return preper_node(node, attributes, children);
  }

  Object *preper_source(const rbn::ast::Source *node)
  {
    Object *obj = new Object();
    Object *attributes = get_node_attributes(node);
    Array *children = new Array();

    string funcs_count = "[" + to_string(node->functions.size()) + "Functions]";

    Array *functions = new Array();
    for (const auto &function : node->functions)
    {
      functions->add(convert_ast_node(function));
    }

    children->add(create_child("program", convert_ast_node(node->program)));
    children->add(create_child("functions", functions));

    return preper_node(node, attributes, children);
  }

  Object *convert_ast_node(const rbn::ast::AstNode *node)
  {
    if (auto *literal = dynamic_cast<const rbn::ast::Literal *>(node))
    {
      return preper_literal(literal);
    }
    else if (auto *expression = dynamic_cast<const rbn::ast::Expression *>(node))
    {
      return preper_expression(expression);
    }
    else if (auto *declaration = dynamic_cast<const rbn::ast::VariableDefinition *>(node))
    {
      return preper_declaration(declaration);
    }
    else if (auto *statement = dynamic_cast<const rbn::ast::Statement *>(node))
    {
      return preper_statement(statement);
    }
    else if (auto *function = dynamic_cast<const rbn::ast::FunctionDefinition *>(node))
    {
      return preper_function(function);
    }
    else if (auto *program = dynamic_cast<const rbn::ast::ProgramDefinition *>(node))
    {
      return preper_program(program);
    }
    else if (auto *source = dynamic_cast<const rbn::ast::Source *>(node))
    {
      return preper_source(source);
    }

    return preper_node(node, get_node_attributes(node), new Array());
  }
}
