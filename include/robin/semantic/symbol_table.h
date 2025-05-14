#pragma once

#include <iostream>
#include <typeinfo>
#include <set>
#include <unordered_map>

#include "robin/core/symbol.h"

using namespace std;

namespace rbn::semantic
{
  class SymbolTable
  {
  private:
    unordered_map<string, core::Symbol *> hashtable;

  public:
    SymbolTable();
    static vector<pair<core::SymbolType, int>> get_parameters_type(vector<ast::VariableDefinition *> params);

    bool insert(core::Symbol *s);
    void insert_vars_list(vector<core::VariableSymbol *> vars);
    bool is_exist(string s);
    bool is_initialized(string s);
    void set_initialized(string s);
    core::SymbolType get_type(string s);
    vector<pair<core::SymbolType, int>> get_arguments(string func_name);
    vector<pair<core::SymbolType, int>> get_required_arguments(string func_name);
    core::Symbol *retrieve_symbol(string name);
    core::VariableSymbol *retrieve_variable(string name);
    core::FunctionSymbol *retrieve_function(string name);
  };
}