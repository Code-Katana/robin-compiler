#pragma once

#include <string>
#include <vector>
#include <map>

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
  Constant,
  Function,
  Array,
  Undefined
};

class Symbol
{
public:
  Symbol(string n, SymbolType t, SymbolKind k, vector<SymbolType> p = {});
  static SymbolType get_type_name(string type);

  string name;
  SymbolType type;
  SymbolKind kind;
  vector<SymbolType> parameters;

private:
  static map<string, SymbolType> TypeName;
};