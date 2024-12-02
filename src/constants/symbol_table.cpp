#include "symbol_table.h"

SymbolTable::SymbolTable(int initialSize = 10)
{
  hashtable.resize(initialSize);
}

void SymbolTable::semantic_error(string err)
{
  cerr << err << endl;
  system("pause");
  throw runtime_error(err);
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

void SymbolTable::insert(string s, SymbolType t, SymbolKind k, vector<SymbolType> parameters)
{
  int index = hash(s);
  Symbol *newSymbol = new Symbol(s, t, k, parameters);

  for (Symbol *symbol : hashtable[index])
  {
    if (symbol->name == s)
    {
      semantic_error("Semantic error: Symbol '" + s + "' already exists.");
      return;
    }
  }

  hashtable[index].push_back(newSymbol);
}

void SymbolTable::insert_vars_list(SymbolType t, vector<string> names)
{
  for (const string &name : names)
  {
    insert(name, t, SymbolKind::Variable);
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

SymbolKind SymbolTable::get_kind(string s)
{
  int index = hash(s);

  for (Symbol *symbol : hashtable[index])
  {
    if (symbol->name == s)
    {
      return symbol->kind;
    }
  }

  return SymbolKind::Undefined;
}

void SymbolTable::union_symbol(SymbolTable &s)
{
  for (int i = 0; i < s.hashtable.size(); i++)
  {
    for (Symbol *symbol : s.hashtable[i])
    {
      if (!is_exist(symbol->name))
      {
        insert(symbol->name, symbol->type, symbol->kind);
      }
    }
  }
}

bool SymbolTable::intersect_name(SymbolTable &s)
{
  for (int i = 0; i < hashtable.size(); i++)
  {
    for (Symbol *symbol : hashtable[i])
    {
      if (s.is_exist(symbol->name))
      {
        return true;
      }
    }
  }

  return false;
}

void SymbolTable::set_parameters_type(string func_name, vector<SymbolType> &parameters)
{
  for (int i = 0; i < hashtable.size(); i++)
  {
    for (Symbol *symbol : hashtable[i])
    {
      if (symbol->name == func_name && symbol->kind == SymbolKind::Function)
      {
        symbol->parameters = parameters;
        return;
      }
    }
  }

  semantic_error("Function with name " + func_name + " not found!");
}

vector<SymbolType> SymbolTable::get_parameters_type(string func_name)
{
  for (int i = 0; i < hashtable.size(); i++)
  {
    for (Symbol *symbol : hashtable[i])
    {
      if (symbol->name == func_name && symbol->kind == SymbolKind::Function)
      {
        return symbol->parameters;
      }
    }
  }

  semantic_error("Function with name " + func_name + " not found!");
}