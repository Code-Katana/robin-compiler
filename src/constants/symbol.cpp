#include "symbol.h"

map<string, SymbolType> Symbol::TypeName = {
    {"integer", SymbolType::Integer},
    {"boolean", SymbolType::Boolean},
    {"string", SymbolType::String},
    {"float", SymbolType::Float},
    {"void", SymbolType::Void},
};

Symbol::Symbol(string n, SymbolType t, SymbolKind k, vector<SymbolType> p)
{
  name = n;
  type = t;
  kind = k;
  parameters = p;
}

SymbolType Symbol::get_type_name(string type)
{
  return TypeName[type];
}