#include "symbol_table.h"

#include <set>

SymbolTable::SymbolTable(int initialSize)
{
  hashtable.resize(initialSize);
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
        // SymbolTable::semantic_error("Error: Variable '" + def->name->name + "' is already defined.");
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

int SymbolTable::hash(string word)
{
  int sum = 0;
  for (char ch : word)
  {
    sum += int(ch);
  }
  return sum % hashtable.size();
}

bool SymbolTable::insert(Symbol *s)
{
  int index = hash(s->name);

  for (Symbol *symbol : hashtable[index])
  {
    if (symbol->name == s->name)
    {
      // semantic_error("Semantic error: Symbol '" + s->name + "' already exists.");
      return false;
    }
  }

  hashtable[index].push_back(s);
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
  int index = hash(s);

  for (Symbol *symbol : hashtable[index])
  {
    if (symbol->name == s)
    {
      return true;
    }
  }

  return false;
}

bool SymbolTable::is_initialized(string s)
{
  int index = hash(s);

  for (Symbol *symbol : hashtable[index])
  {
    if (static_cast<VariableSymbol *>(symbol))
    {
      VariableSymbol *varSymbol = static_cast<VariableSymbol *>(symbol);
      if (varSymbol->name == s)
      {
        return varSymbol->is_initialized;
      }
    }
  }

  return NULL;
}

void SymbolTable::set_initialized(string s)
{
  int index = hash(s);

  for (Symbol *symbol : hashtable[index])
  {
    if (static_cast<VariableSymbol *>(symbol))
    {
      VariableSymbol *varSymbol = static_cast<VariableSymbol *>(symbol);
      if (varSymbol->name == s)
      {
        varSymbol->is_initialized = true;
      }
    }
  }
}

SymbolType SymbolTable::get_type(string s)
{
  int index = hash(s);

  for (Symbol *symbol : hashtable[index])
  {
    if (symbol->name == s)
    {
      return symbol->type;
    }
  }

  return SymbolType::Undefined;
}

vector<pair<SymbolType, int>> SymbolTable::get_arguments(string func_name)
{
  for (int i = 0; i < hashtable.size(); i++)
  {
    for (Symbol *symbol : hashtable[i])
    {
      if (symbol->kind == SymbolKind::Function)
      {
        FunctionSymbol *func_symbol = static_cast<FunctionSymbol *>(symbol);
        if (func_symbol && func_symbol->name == func_name)
        {
          return func_symbol->parameters;
        }
      }
    }
  }
  // semantic_error("Function with name " + func_name + " not found!");
  return {{SymbolType::Undefined, 0}};
}
