#include "lsp/core/methods.h"

namespace lsp
{
  const map<MethodType, string> METHOD_NAMES = {
      {MethodType::Initialize, "initialize"},
      {MethodType::Tokenize, "tokenize"},
      {MethodType::ParseAst, "parseAst"},
      {MethodType::Unknown, "unknown"},
  };

  const map<string, MethodType> METHOD_TYPES = {
      {"initialize", MethodType::Initialize},
      {"tokenize", MethodType::Tokenize},
      {"parseAst", MethodType::ParseAst},
      {"unknown", MethodType::Unknown},
  };

  string get_method_name(MethodType method)
  {
    return METHOD_NAMES.at(method);
  }

  MethodType get_method(string name)
  {
    return METHOD_TYPES.at(name);
  }
}
