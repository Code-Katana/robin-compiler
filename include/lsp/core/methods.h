#pragma once

#include <stdexcept>
#include <string>
#include <map>

using namespace std;

namespace lsp
{
  enum class MethodType
  {
    Initialize,
    Tokenize,
    ParseAst,
    Unknown
  };

  MethodType get_method(string name);
  string get_method_name(MethodType method);
}
