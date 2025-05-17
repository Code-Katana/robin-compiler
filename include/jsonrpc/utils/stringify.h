#pragma once

#include <vector>

#include "robin/core/token.h"
#include "jsonrpc/core/values.h"
#include "jsonrpc/core/encoding.h"

using namespace std;

namespace json::utils
{
  std::string stringify(const rbn::core::Token &token);
  std::string stringify(const vector<rbn::core::Token> &token);
}