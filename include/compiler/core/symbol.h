#pragma once

#include <string>
#include <vector>
#include <map>

#include "compiler/core/ast.h"

using namespace std;

enum class SymbolType
{
  Integer,
  Boolean,
  Float,
  String,
  Void,
  Program,
  Undefined
};

enum class SymbolKind
{
  Variable,
  Function,
  Error,
  Undefined
};

class Symbol
{
public:
  Symbol();
  Symbol(string n, SymbolType t, SymbolKind k, int d = 0);
  static SymbolType get_type_name(string type);
  static SymbolType get_datatype(DataType *dt);
  static string get_name_symboltype(SymbolType st);
  static int get_dimension(DataType *dt);
  string name;
  SymbolType type;
  SymbolKind kind;
  int dim;

private:
  static map<string, SymbolType> TypeName;
};

class FunctionSymbol : public Symbol
{
public:
  FunctionSymbol(string n, SymbolType t, vector<pair<SymbolType, int>> p = {}, int dim = 0);

  vector<pair<SymbolType, int>> parameters;
  vector<VariableDefinition *> parametersRaw;
};

class VariableSymbol : public Symbol
{
public:
  VariableSymbol(string n, SymbolType t, bool initialized = false, int dim = 0);

  bool is_initialized;
};

class ErrorSymbol : public Symbol
{
public:
  string message_error;

  ErrorSymbol();
  ErrorSymbol(string n, SymbolType t, string message_error = "", int dim = 0);
};
