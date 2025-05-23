#pragma once

#include <typeinfo>
#include <string>
#include <vector>

#include "robin/core/token.h"
#include "robin/core/ast.h"
#include "jsonrpc/core/values.h"

using namespace std;

namespace json::utils
{
  Object *convert_token(const rbn::core::Token *token);
  Array *convert_token_stream(const vector<rbn::core::Token> *tokens);
  Object *convert_ast_node(const rbn::ast::AstNode *node);
}