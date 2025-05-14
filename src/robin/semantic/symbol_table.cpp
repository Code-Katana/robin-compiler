#include "robin/semantic/symbol_table.h"

namespace rbn::semantic
{
  SymbolTable::SymbolTable() {}

  vector<pair<core::SymbolType, int>> SymbolTable::get_parameters_type(vector<ast::VariableDefinition *> params)
  {
    vector<pair<core::SymbolType, int>> params_type = {};
    set<string> declared_names;

    for (auto param : params)
    {
      if (dynamic_cast<ast::VariableInitialization *>(param->def))
      {
        ast::VariableInitialization *def = (ast::VariableInitialization *)param->def;
        if (declared_names.find(def->name->name) != declared_names.end())
        {

          return {{core::SymbolType::Undefined, 0}};
        }
        else
        {
          declared_names.insert(def->name->name);
          params_type.emplace_back(core::Symbol::get_datatype(def->datatype), core::Symbol::get_dimension(def->datatype));
        }
      }
      else if (dynamic_cast<ast::VariableDeclaration *>(param->def))
      {
        ast::VariableDeclaration *def = (ast::VariableDeclaration *)param->def;
        for (auto id : def->variables)
        {
          if (declared_names.find(id->name) != declared_names.end())
          {
            // SymbolTable::semantic_error("Error: Variable '" + id->name + "' is already defined.");
            return {{core::SymbolType::Undefined, 0}};
          }
          else
          {
            declared_names.insert(id->name);
            params_type.emplace_back(core::Symbol::get_datatype(def->datatype), core::Symbol::get_dimension(def->datatype));
          }
        }
      }
    }
    return params_type;
  }

  bool SymbolTable::insert(core::Symbol *s)
  {
    if (hashtable.find(s->name) != hashtable.end())
    {
      return false;
    }
    hashtable[s->name] = s;
    return true;
  }

  void SymbolTable::insert_vars_list(vector<core::VariableSymbol *> vars)
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
      core::VariableSymbol *var = static_cast<core::VariableSymbol *>(it->second);
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
      core::VariableSymbol *var = static_cast<core::VariableSymbol *>(it->second);
      if (var)
        var->is_initialized = true;
    }
  }

  core::SymbolType SymbolTable::get_type(string s)
  {
    auto it = hashtable.find(s);
    if (it != hashtable.end())
    {
      return it->second->type;
    }
    return core::SymbolType::Undefined;
  }

  vector<pair<core::SymbolType, int>> SymbolTable::get_arguments(string func_name)
  {
    auto it = hashtable.find(func_name);
    if (it != hashtable.end() && it->second->kind == core::SymbolKind::Function)
    {
      core::FunctionSymbol *func = static_cast<core::FunctionSymbol *>(it->second);
      return func->parameters;
    }
    return {{core::SymbolType::Undefined, 0}};
  }

  vector<pair<core::SymbolType, int>> SymbolTable::get_required_arguments(string func_name)
  {
    auto it = hashtable.find(func_name);
    if (it != hashtable.end())
    {
      if (it->second->kind == core::SymbolKind::Function)
      {
        core::FunctionSymbol *func = static_cast<core::FunctionSymbol *>(it->second);
        vector<pair<core::SymbolType, int>> required_args;

        for (auto def : func->parametersRaw)
        {
          if (dynamic_cast<ast::VariableDeclaration *>(def->def))
          {
            ast::VariableDeclaration *var_decl = static_cast<ast::VariableDeclaration *>(def->def);
            for (auto id : var_decl->variables)
            {
              required_args.push_back({core::Symbol::get_datatype(var_decl->datatype), core::Symbol::get_dimension(var_decl->datatype)});
            }
          }
        }

        return required_args;
      }
    }

    return {{core::SymbolType::Undefined, 0}};
  }

  core::Symbol *SymbolTable::retrieve_symbol(string s)
  {
    auto it = hashtable.find(s);
    if (it != hashtable.end())
    {
      return it->second;
    }
    return nullptr;
  }

  core::VariableSymbol *SymbolTable::retrieve_variable(string s)
  {
    core::Symbol *symbol = retrieve_symbol(s);
    if (symbol && symbol->kind == core::SymbolKind::Variable)
    {
      return static_cast<core::VariableSymbol *>(symbol);
    }
    return nullptr;
  }

  core::FunctionSymbol *SymbolTable::retrieve_function(string s)
  {
    core::Symbol *symbol = retrieve_symbol(s);
    if (symbol && symbol->kind == core::SymbolKind::Function)
    {
      return static_cast<core::FunctionSymbol *>(symbol);
    }
    return nullptr;
  }
}