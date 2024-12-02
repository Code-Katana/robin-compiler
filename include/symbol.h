#pragma once

#include <string>
#include <vector>

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

  string name;
  SymbolType type;
  SymbolKind kind;
  vector<SymbolType> parameters;
};