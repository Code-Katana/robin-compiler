#pragma once

#include <iostream>
#include <typeinfo>
#include <unordered_map>

#include "compiler/core/symbol.h"

using namespace std;

class SymbolTable
{
private:
  unordered_map<string, Symbol *> hashtable;

public:
  SymbolTable();
  static vector<pair<SymbolType, int>> get_parameters_type(vector<VariableDefinition *> params);

  bool insert(Symbol *s);
  void insert_vars_list(vector<VariableSymbol *> vars);
  bool is_exist(string s);
  bool is_initialized(string s);
  void set_initialized(string s);
  SymbolType get_type(string s);
  vector<pair<SymbolType, int>> get_arguments(string func_name);
  vector<pair<SymbolType, int>> get_required_arguments(string func_name);
  Symbol *retrieve_symbol(string name);
  VariableSymbol *retrieve_variable(string name);
  FunctionSymbol *retrieve_function(string name);
};