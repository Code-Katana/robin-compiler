#include "robin/core/symbol.h"

namespace rbn::core
{
  map<string, SymbolType> Symbol::TypeName = {
      {"integer", SymbolType::Integer},
      {"boolean", SymbolType::Boolean},
      {"string", SymbolType::String},
      {"float", SymbolType::Float},
      {"void", SymbolType::Void},
  };

  Symbol::Symbol() {}

  Symbol::Symbol(string n, SymbolType t, SymbolKind k, int d)
  {
    name = n;
    type = t;
    kind = k;
    dim = d;
  }

  SymbolType Symbol::get_type_name(string type)
  {
    if (TypeName.find(type) == TypeName.end())
    {
      return SymbolType::Undefined;
    }

    return TypeName[type];
  }

  string Symbol::get_name_symboltype(SymbolType st)
  {
    for (auto &pair : TypeName)
    {
      if (pair.second == st)
      {
        return pair.first;
      }
    }

    return "Undefined";
  }

  SymbolType Symbol::get_datatype(ast::DataType *dt)
  {
    if (dynamic_cast<ast::ArrayDataType *>(dt))
    {
      ast::ArrayDataType *arr = (ast::ArrayDataType *)dt;
      return Symbol::get_type_name(arr->datatype);
    }
    else if (dynamic_cast<ast::PrimitiveDataType *>(dt))
    {
      ast::PrimitiveDataType *primitive = (ast::PrimitiveDataType *)dt;
      return Symbol::get_type_name(primitive->datatype);
    }

    return SymbolType::Undefined;
  }

  int Symbol::get_dimension(ast::DataType *dt)
  {
    if (dynamic_cast<ast::ArrayDataType *>(dt))
    {
      ast::ArrayDataType *arr = (ast::ArrayDataType *)dt;
      return arr->dimension;
    }

    return 0;
  }

  FunctionSymbol::FunctionSymbol(string n, SymbolType t, vector<pair<SymbolType, int>> p, int d) : Symbol(n, t, SymbolKind::Function, d)
  {
    parameters = p;
  }

  VariableSymbol::VariableSymbol(string n, SymbolType t, bool initialized, int d) : Symbol(n, t, SymbolKind::Variable, d)
  {
    is_initialized = initialized;
  }

  ErrorSymbol::ErrorSymbol() {}

  ErrorSymbol::ErrorSymbol(string n, SymbolType t, string message_error, int d)
      : Symbol(n, t, SymbolKind::Error, d)
  {
    this->message_error = message_error;
  }
}