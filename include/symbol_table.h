#pragma once

#include <iostream>

#include "symbol.h"

using namespace std;

class SymbolTable
{
private:
  vector<vector<Symbol *>> hashtable;

  int hash(string word);

public:
  SymbolTable(int initialSize = 10);
  static vector<pair<SymbolType, int>> get_parameters_type(vector<VariableDefinition *> params);

  bool insert(Symbol *s);
  void insert_vars_list(vector<VariableSymbol *> vars);
  bool is_exist(string s);
  bool is_initialized(string s);
  void set_initialized(string s);
  SymbolType get_type(string s);
  vector<pair<SymbolType, int>> get_arguments(string func_name);
};