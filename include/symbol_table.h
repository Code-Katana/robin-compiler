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
  void semantic_error(string err);
  void insert(string s, SymbolType t, SymbolKind k, vector<SymbolType> parameters = {});
  void insert_vars_list(SymbolType t, vector<string> v);
  bool is_exist(string s);
  SymbolType get_type(string s);
  SymbolKind get_kind(string s);
  void union_symbol(SymbolTable &s2);
  bool intersect_name(SymbolTable &s2);
  void set_parameters_type(string func_name, vector<SymbolType> &parameters);
  vector<SymbolType> get_parameters_type(string func_name);
};