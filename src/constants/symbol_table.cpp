#include "symbol_table.h"

#include <set>

SymbolTable::SymbolTable()
{
}

vector<pair<SymbolType, int>> SymbolTable::get_parameters_type(vector<VariableDefinition *> params)
{
  vector<pair<SymbolType, int>> params_type = {};
  set<string> declared_names;

  for (auto param : params)
  {
    if (dynamic_cast<VariableInitialization *>(param->def))
    {
      VariableInitialization *def = (VariableInitialization *)param->def;
      if (declared_names.find(def->name->name) != declared_names.end())
      {

        return {{SymbolType::Undefined, 0}};
      }
      else
      {
        declared_names.insert(def->name->name);
        params_type.emplace_back(Symbol::get_datatype(def->datatype), Symbol::get_dimension(def->datatype));
      }
    }
    else if (dynamic_cast<VariableDeclaration *>(param->def))
    {
      VariableDeclaration *def = (VariableDeclaration *)param->def;
      for (auto id : def->variables)
      {
        if (declared_names.find(id->name) != declared_names.end())
        {
          // SymbolTable::semantic_error("Error: Variable '" + id->name + "' is already defined.");
          return {{SymbolType::Undefined, 0}};
        }
        else
        {
          declared_names.insert(id->name);
          params_type.emplace_back(Symbol::get_datatype(def->datatype), Symbol::get_dimension(def->datatype));
        }
      }
    }
  }
  return params_type;
}

bool SymbolTable::insert(Symbol *s)
{
  if (hashtable.find(s->name) != hashtable.end())
  {
    return false;
  }
  hashtable[s->name] = s;
  return true;
}

void SymbolTable::insert_vars_list(vector<VariableSymbol *> vars)
{
  for (auto var : vars)
  {
    insert(var);
  }
}

bool SymbolTable::is_exist(string s)
{
  return hashtable.find(s) != hashtable.end();
}

bool SymbolTable::is_initialized(string s)
{
  auto it = hashtable.find(s);
  if (it != hashtable.end())
  {
    VariableSymbol *var = static_cast<VariableSymbol *>(it->second);
    if (var)
      return var->is_initialized;
  }
  return false;
}

void SymbolTable::set_initialized(string s)
{
  auto it = hashtable.find(s);
  if (it != hashtable.end())
  {
    VariableSymbol *var = static_cast<VariableSymbol *>(it->second);
    if (var)
      var->is_initialized = true;
  }
}

SymbolType SymbolTable::get_type(string s)
{
  auto it = hashtable.find(s);
  if (it != hashtable.end())
  {
    return it->second->type;
  }
  return SymbolType::Undefined;
}

vector<pair<SymbolType, int>> SymbolTable::get_arguments(string func_name)
{
  auto it = hashtable.find(func_name);
  if (it != hashtable.end() && it->second->kind == SymbolKind::Function)
  {
    FunctionSymbol *func = static_cast<FunctionSymbol *>(it->second);
    return func->parameters;
  }
  return {{SymbolType::Undefined, 0}};
}

vector<pair<SymbolType, int>> SymbolTable::get_required_arguments(string func_name)
{
  auto it = hashtable.find(func_name);
  if (it != hashtable.end())
  {
    if (it->second->kind == SymbolKind::Function)
    {
      FunctionSymbol *func = static_cast<FunctionSymbol *>(it->second);
      vector<pair<SymbolType, int>> required_args;

      for (auto def : func->parametersRaw)
      {
        if (dynamic_cast<VariableDeclaration *>(def->def))
        {
          VariableDeclaration *var_decl = static_cast<VariableDeclaration *>(def->def);
          for (auto id : var_decl->variables)
          {
            required_args.push_back({Symbol::get_datatype(var_decl->datatype), Symbol::get_dimension(var_decl->datatype)});
          }
        }
      }

      return required_args;
    }
  }

  return {{SymbolType::Undefined, 0}};
}

Symbol *SymbolTable::retrieve_symbol(string s)
{
  auto it = hashtable.find(s);
  if (it != hashtable.end())
  {
    return it->second;
  }
  return nullptr;
}

VariableSymbol *SymbolTable::retrieve_variable(string s)
{
  Symbol *symbol = retrieve_symbol(s);
  if (symbol && symbol->kind == SymbolKind::Variable)
  {
    return static_cast<VariableSymbol *>(symbol);
  }
  return nullptr;
}

FunctionSymbol *SymbolTable::retrieve_function(string s)
{
  Symbol *symbol = retrieve_symbol(s);
  if (symbol && symbol->kind == SymbolKind::Function)
  {
    return static_cast<FunctionSymbol *>(symbol);
  }
  return nullptr;
}
