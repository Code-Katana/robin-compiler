#include "robin/semantic/semantic_analyzer.h"

namespace rbn::semantic
{
  SemanticAnalyzer::SemanticAnalyzer(syntax::ParserBase *pr) : parser(pr)
  {
    error_symbol = core::ErrorSymbol();
    has_error = false;
  }

  core::ErrorSymbol SemanticAnalyzer::get_error()
  {
    if (has_error)
    {
      return error_symbol;
    }
    return {};
  }

  ast::Source *SemanticAnalyzer::analyze()
  {
    ast::Source *source = (ast::Source *)parser->parse_ast();

    if (source->type == ast::AstNodeType::ErrorNode)
    {
      forword_syntax_error(parser->get_error_node());
      return nullptr;
    }

    semantic_source(source);
    return source;
  }

  void SemanticAnalyzer::semantic_source(ast::Source *source)
  {
    SymbolTable *st = new SymbolTable();
    string program_name = source->program->program_name->name;
    bool check = st->insert(new core::Symbol(
        program_name,
        core::SymbolType::Program,
        core::SymbolKind::Function));
    if (!check)
    {
      semantic_error(program_name, core::SymbolType::Program, "Semantic error: Symbol '" + program_name + "' already exists.");
      return;
    }

    for (auto var : source->program->globals)
    {
      call_stack.push(st);
      semantic_var_def(var, st);
      call_stack.pop();
    }

    for (auto func : source->functions)
    {
      string name = func->funcname->name;
      ast::DataType *dt = func->return_type->return_type;
      vector<pair<core::SymbolType, int>> parameters = SymbolTable::get_parameters_type(func->parameters);

      bool found_default = false;
      for (auto def : func->parameters)
      {
        if (dynamic_cast<ast::VariableInitialization *>(def->def))
        {
          found_default = true;
        }
        else if (found_default)
        {
          semantic_error(name, core::SymbolType::Undefined, "Semantic error: Required parameters cannot follow optional parameters.");
          return;
        }
      }

      if (!parameters.empty() && parameters[0].first == core::SymbolType::Undefined)
      {
        semantic_error(name, core::SymbolType::Undefined, "Semantic error: Variable name is already defined.");
        return;
      }

      auto func_symbol = new core::FunctionSymbol(
          name,
          core::Symbol::get_datatype(dt),
          parameters,
          core::Symbol::get_dimension(dt));

      func_symbol->parametersRaw = func->parameters;

      bool check = st->insert(func_symbol);
      if (!check)
      {
        semantic_error(name, core::Symbol::get_datatype(dt), "Semantic error: Symbol '" + name + "' already exists.");
        return;
      }
    }

    call_stack.push(st);

    semantic_program(source->program);
    for (auto func : source->functions)
    {
      sematic_function(func);
    }
  }

  void SemanticAnalyzer::semantic_program(ast::ProgramDefinition *program)
  {
    SymbolTable *st = new SymbolTable();
    call_stack.push(st);
    for (auto stmt : program->body)
    {
      semantic_command(stmt, program->program_name->name);
    }
    call_stack.pop();
  }

  void SemanticAnalyzer::sematic_function(ast::FunctionDefinition *func)
  {
    SymbolTable *st = new SymbolTable();
    SymbolTable *globals = call_stack.top();
    bool is_return = false;
    call_stack.push(st);
    vector<ast::VariableDefinition *> vars = func->parameters;
    for (auto var : vars)
    {
      if (dynamic_cast<ast::VariableInitialization *>(var->def))
      {
        semantic_var_def(var, st);
      }
      else if (dynamic_cast<ast::VariableDeclaration *>(var->def))
      {
        ast::VariableDeclaration *def = (ast::VariableDeclaration *)var->def;

        for (auto id : def->variables)
        {
          bool check = st->insert(new core::VariableSymbol(
              id->name,
              core::Symbol::get_datatype(def->datatype),
              true,
              core::Symbol::get_dimension(def->datatype)));
          if (!check)
          {
            semantic_error(id->name, core::Symbol::get_datatype(def->datatype), "Semantic error: Symbol '" + id->name + "' already exists.");
            return;
          }
        }
      }
    }
    for (auto stmt : func->body)
    {
      semantic_command(stmt, func->funcname->name);
      if (dynamic_cast<ast::ReturnStatement *>(stmt) && globals->get_type(func->funcname->name) != core::SymbolType::Void && !is_return)
      {
        is_return = true;
      }
    }

    if (globals->get_type(func->funcname->name) != core::SymbolType::Void && !is_return)
    {
      semantic_error(func->funcname->name, globals->get_type(func->funcname->name), "Semantic error : missing a return statement in the function body in '" + func->funcname->name + "'.");
      return;
    }

    call_stack.pop();
  }

  void SemanticAnalyzer::semantic_command(ast::Statement *stmt, string name_parent)
  {
    if (dynamic_cast<ast::IfStatement *>(stmt))
    {
      ast::IfStatement *ifStmt = static_cast<ast::IfStatement *>(stmt);
      semantic_if(ifStmt, name_parent);
    }
    else if (dynamic_cast<ast::ReturnStatement *>(stmt))
    {
      ast::ReturnStatement *rtnStmt = static_cast<ast::ReturnStatement *>(stmt);
      semantic_return(rtnStmt, name_parent);
    }
    else if (dynamic_cast<ast::ReadStatement *>(stmt))
    {
      ast::ReadStatement *readStmt = static_cast<ast::ReadStatement *>(stmt);
      semantic_read(readStmt);
    }
    else if (dynamic_cast<ast::WriteStatement *>(stmt))
    {
      ast::WriteStatement *writeStmt = static_cast<ast::WriteStatement *>(stmt);
      semantic_write(writeStmt);
    }
    else if (dynamic_cast<ast::WhileLoop *>(stmt))
    {
      ast::WhileLoop *loop = static_cast<ast::WhileLoop *>(stmt);
      semantic_while(loop, name_parent);
    }
    else if (dynamic_cast<ast::ForLoop *>(stmt))
    {
      ast::ForLoop *loop = static_cast<ast::ForLoop *>(stmt);
      semantic_for(loop, name_parent);
    }
    else if (dynamic_cast<ast::Expression *>(stmt))
    {
      ast::Expression *expr = static_cast<ast::Expression *>(stmt);
      semantic_expr(expr);
    }
    else if (dynamic_cast<ast::VariableDefinition *>(stmt))
    {
      ast::VariableDefinition *def = static_cast<ast::VariableDefinition *>(stmt);
      semantic_var_def(def, call_stack.top());
    }
  }

  void SemanticAnalyzer::semantic_if(ast::IfStatement *ifStmt, string name_parent)
  {
    SymbolTable *st = new SymbolTable();
    call_stack.push(st);

    core::SymbolType ty = semantic_expr(ifStmt->condition);

    if (ty != core::SymbolType::Boolean)
    {
      semantic_error("If_Statement", ty, "Semantic error: condition must be boolean");
      return;
    }
    for (auto stmt : ifStmt->consequent)
    {
      semantic_command(stmt, name_parent);
    }
    call_stack.pop();
    SymbolTable *st2 = new SymbolTable();
    call_stack.push(st2);
    for (auto stmt : ifStmt->alternate)
    {
      semantic_command(stmt, name_parent);
    }
    call_stack.pop();
  }

  void SemanticAnalyzer::semantic_return(ast::ReturnStatement *rtnStmt, string name_parent)
  {
    stack<SymbolTable *> temp = call_stack;
    SymbolTable *global = nullptr;
    while (!temp.empty())
    {
      global = temp.top();
      temp.pop();
    }

    core::SymbolType funcType = global->get_type(name_parent);
    core::SymbolType returnType = semantic_expr(rtnStmt->returnValue);

    if (funcType == core::SymbolType::Void || funcType == core::SymbolType::Program)
    {
      if (returnType != core::SymbolType::Undefined)
      {
        semantic_error(name_parent, returnType, "Semantic error: 'return' in block '" + name_parent + "' must not have an expression.");
        return;
      }
    }
    else if (funcType != returnType)
    {
      if (!((funcType == core::SymbolType::Integer || funcType == core::SymbolType::Float) &&
            (returnType == core::SymbolType::Integer || returnType == core::SymbolType::Float)))
      {
        semantic_error(name_parent, returnType, "Semantic error: 'return' in function block '" + name_parent + "' doesn't match the function type.");
        return;
      }
    }

    // Dimension of return
    int dim_func = global->retrieve_function(name_parent)->dim;
    int dim_return = 0;
    if (dynamic_cast<ast::ArrayLiteral *>(rtnStmt->returnValue))
    {
      pair<core::SymbolType, int> array_value = semantic_array((ast::ArrayLiteral *)rtnStmt->returnValue);
      dim_return = array_value.second;
    }
    else if (dynamic_cast<ast::Identifier *>(rtnStmt->returnValue))
    {
      ast::Identifier *var = static_cast<ast::Identifier *>(rtnStmt->returnValue);
      core::VariableSymbol *vs = is_initialized_var(var);
      dim_return = vs->dim;
    }
    if (dim_func != dim_return)
    {
      semantic_error(name_parent, returnType, "Semantic error: 'return' in function block '" + name_parent + "' doesn't match the function dimensions.");
      return;
    }
  }

  void SemanticAnalyzer::semantic_read(ast::ReadStatement *readStmt)
  {
    stack<SymbolTable *> temp = call_stack;
    vector<ast::AssignableExpression *> vars = readStmt->variables;
    for (auto var : vars)
    {
      semantic_assignable_expr(var);
    }
  }

  core::SymbolType SemanticAnalyzer::semantic_assignable_expr(ast::AssignableExpression *assignable)
  {
    if (dynamic_cast<ast::Identifier *>(assignable))
    {
      ast::Identifier *id = static_cast<ast::Identifier *>(assignable);
      return semantic_id(id, true);
    }
    return semantic_index_expr(assignable, true);
  }

  void SemanticAnalyzer::semantic_write(ast::WriteStatement *writeStmt)
  {
    for (auto expr : writeStmt->args)
    {
      semantic_expr(expr);

      is_array(expr);
    }
  }

  void SemanticAnalyzer::semantic_while(ast::WhileLoop *loop, string name_parent)
  {
    SymbolTable *st = new SymbolTable();
    call_stack.push(st);
    core::SymbolType ty = semantic_expr(loop->condition);

    if (ty != core::SymbolType::Boolean)
    {
      semantic_error("While_loop", ty, "Semantic error: condition must be boolean");
      return;
    }
    for (auto stmt : loop->body)
    {
      semantic_command(stmt, name_parent);
    }

    call_stack.pop();
  }

  void SemanticAnalyzer::semantic_for(ast::ForLoop *loop, string name_parent)
  {
    SymbolTable *st = new SymbolTable();
    call_stack.push(st);

    semantic_int_assign(loop->init);
    core::SymbolType ty = semantic_expr(loop->condition);

    if (ty != core::SymbolType::Boolean)
    {
      semantic_error("For_loop", ty, "Semantic error: condition must be boolean");
      return;
    }

    core::SymbolType ty_update = semantic_expr(loop->update);

    if (ty_update != core::SymbolType::Integer)
    {
      semantic_error("For_loop", ty_update, "Semantic error: Update for loop must be integer");
      return;
    }
    for (auto stmt : loop->body)
    {
      semantic_command(stmt, name_parent);
    }
    call_stack.pop();
  }

  void SemanticAnalyzer::semantic_int_assign(ast::AssignmentExpression *init)
  {
    stack<SymbolTable *> temp = call_stack;
    if (dynamic_cast<ast::Identifier *>(init->assignee))
    {
      ast::Identifier *var = static_cast<ast::Identifier *>(init->assignee);
      while (!temp.empty())
      {
        SymbolTable *t = temp.top();
        if (t->is_exist(var->name))
        {
          semantic_error(var->name, t->get_type(var->name), "Semantic error: Symbol '" + var->name + "' Declared.");
          return;
        }
        temp.pop();
      }
      bool check = call_stack.top()->insert(new core::VariableSymbol(
          var->name,
          core::SymbolType::Integer,
          true,
          0));
      if (!check)
      {
        semantic_error(var->name, core::SymbolType::Integer, "Semantic error: Symbol '" + var->name + "' already exists.");
        return;
      }

      core::SymbolType type = semantic_expr(init->value);
      if (type != core::SymbolType::Integer)
      {
        semantic_error(var->name, type, "Semantic error: Value of '" + var->name + "' Must be integer.");
        return;
      }
    }
    else
    {
      semantic_error("For_loop", core::SymbolType::Integer, "Semantic error: in initialization part of forLoop must be identifier .");
      return;
    }
  }

  void SemanticAnalyzer::semantic_var_def(ast::VariableDefinition *var, SymbolTable *st)
  {
    if (dynamic_cast<ast::VariableDeclaration *>(var->def))
    {
      ast::VariableDeclaration *def = (ast::VariableDeclaration *)var->def;

      for (auto id : def->variables)
      {
        bool check = st->insert(new core::VariableSymbol(
            id->name,
            core::Symbol::get_datatype(def->datatype),
            false,
            core::Symbol::get_dimension(def->datatype)));
        if (!check)
        {
          semantic_error(id->name, core::Symbol::get_datatype(def->datatype), "Semantic error: Symbol '" + id->name + "' already exists.");
          return;
        }
      }
    }
    else if (dynamic_cast<ast::VariableInitialization *>(var->def))
    {
      ast::VariableInitialization *def = (ast::VariableInitialization *)var->def;
      core::SymbolType dt = core::Symbol::get_datatype(def->datatype);
      core::SymbolType dt_init;
      int dim_left = core::Symbol::get_dimension(def->datatype);
      int dim_init = 0;

      if (dynamic_cast<ast::ArrayLiteral *>(def->initializer))
      {
        pair<core::SymbolType, int> array_value = semantic_array((ast::ArrayLiteral *)def->initializer);

        dt_init = array_value.first;
        dim_init = array_value.second;
      }
      else if (dynamic_cast<ast::IndexExpression *>(def->initializer))
      {
        ast::IndexExpression *var = static_cast<ast::IndexExpression *>(def->initializer);
        dt_init = semantic_index_expr(def->initializer, false, true);
        dim_init = 1;
        while (dynamic_cast<ast::IndexExpression *>(var->base))
        {
          dim_init += 1;
          var = static_cast<ast::IndexExpression *>(var->base);
        }
        ast::Identifier *id = static_cast<ast::Identifier *>(var->base);
        core::VariableSymbol *sv = is_initialized_var(id);
        dim_init = sv->dim - dim_init;
      }
      else if (dynamic_cast<ast::Identifier *>(def->initializer))
      {
        ast::Identifier *var = static_cast<ast::Identifier *>(def->initializer);
        core::VariableSymbol *sv = is_initialized_var(var);
        dt_init = sv->type;
        dim_init = sv->dim;
      }
      else
      {
        dt_init = semantic_expr(def->initializer);
        dim_init = 0;
      }
      core::SymbolType result = TypeChecker::is_valid_assign(dt, dt_init, dim_left, dim_init);

      if (result == core::SymbolType::Undefined)
      {
        semantic_error(def->name->name, result, "Semantic error: invalid initialization.");
        return;
      }

      bool check = st->insert(new core::VariableSymbol(
          def->name->name,
          core::Symbol::get_datatype(def->datatype),
          true,
          core::Symbol::get_dimension(def->datatype)));
      if (!check)
      {
        semantic_error(def->name->name, core::Symbol::get_datatype(def->datatype), "Semantic error: Symbol '" + def->name->name + "' already exists.");
        return;
      }
    }
  }

  core::SymbolType SemanticAnalyzer::semantic_expr(ast::Expression *expr)
  {
    return semantic_assign_expr(expr);
  }

  core::SymbolType SemanticAnalyzer::semantic_assign_expr(ast::Expression *assignExpr)
  {
    if (!dynamic_cast<ast::AssignmentExpression *>(assignExpr))
    {
      return semantic_or_expr(assignExpr);
    }

    ast::AssignmentExpression *assign_Expr = static_cast<ast::AssignmentExpression *>(assignExpr);

    string assignee_name;
    int dim_assignee = 0;
    int dim_value = 0;

    if (dynamic_cast<ast::Identifier *>(assign_Expr->assignee))
    {
      ast::Identifier *id = static_cast<ast::Identifier *>(assign_Expr->assignee);
      assignee_name = id->name;
      SymbolTable *scope = retrieve_scope(assignee_name);
      if (scope == nullptr)
      {
        return core::SymbolType::Undefined;
      }
      core::VariableSymbol *vr = scope->retrieve_variable(assignee_name);
      if (vr == nullptr)
      {
        semantic_error(assignee_name, core::SymbolType::Undefined, "Semantic error: Variable '" + assignee_name + "' must be Declared.");
        return core::SymbolType::Undefined;
      }
      dim_assignee = vr->dim;
    }
    else if (dynamic_cast<ast::IndexExpression *>(assign_Expr->assignee))
    {
      ast::Expression *base_expr = static_cast<ast::IndexExpression *>(assign_Expr->assignee)->base;
      int accessed_dim = 1;

      while (dynamic_cast<ast::IndexExpression *>(base_expr))
      {
        base_expr = static_cast<ast::IndexExpression *>(base_expr)->base;
        accessed_dim++;
      }
      assignee_name = static_cast<ast::Identifier *>(base_expr)->name;

      SymbolTable *scope = retrieve_scope(assignee_name);
      if (scope == nullptr)
      {
        return core::SymbolType::Undefined;
      }
      core::VariableSymbol *vr = scope->retrieve_variable(assignee_name);
      if (vr == nullptr)
      {
        semantic_error(assignee_name, core::SymbolType::Undefined, "Semantic error: Variable '" + assignee_name + "' must be Declared.");
        return core::SymbolType::Undefined;
      }

      int total_dim = vr->dim;
      dim_assignee = total_dim - accessed_dim;

      if (dim_assignee < 0)
      {
        semantic_error(assignee_name, core::SymbolType::Undefined, "Semantic error: Invalid array access for variable '" + assignee_name + "'.");
        return core::SymbolType::Undefined;
      }
    }

    core::SymbolType type_assignee;
    if (dynamic_cast<ast::IndexExpression *>(assign_Expr->assignee))
    {
      type_assignee = semantic_index_expr(assign_Expr->assignee, true, true);
    }
    else
    {
      type_assignee = semantic_assignable_expr(assign_Expr->assignee);
    }
    core::SymbolType type_value;

    if (dynamic_cast<ast::ArrayLiteral *>(assign_Expr->value))
    {
      type_value = semantic_expr(assign_Expr->value);
      dim_value = semantic_array(static_cast<ast::ArrayLiteral *>(assign_Expr->value)).second;
    }
    else if (dynamic_cast<ast::Identifier *>(assign_Expr->value))
    {
      type_value = semantic_expr(assign_Expr->value);
      ast::Identifier *id = static_cast<ast::Identifier *>(assign_Expr->value);
      SymbolTable *scope = retrieve_scope(id->name);
      if (scope == nullptr)
      {
        semantic_error(id->name, core::SymbolType::Undefined, "Semantic error: Variable '" + id->name + "' must be Declared.");
        return core::SymbolType::Undefined;
      }
      core::VariableSymbol *vr = scope->retrieve_variable(id->name);
      if (vr == nullptr)
      {
        semantic_error(id->name, core::SymbolType::Undefined, "Semantic error: Variable '" + id->name + "' must be Declared.");
        return core::SymbolType::Undefined;
      }
      dim_value = vr->dim;
    }
    else if (dynamic_cast<ast::IndexExpression *>(assign_Expr->value))
    {
      ast::IndexExpression *var = static_cast<ast::IndexExpression *>(assign_Expr->value);
      type_value = semantic_index_expr(assign_Expr->value, false, true);
      dim_value = 1;
      while (dynamic_cast<ast::IndexExpression *>(var->base))
      {
        dim_value += 1;
        var = static_cast<ast::IndexExpression *>(var->base);
      }
      ast::Identifier *id = static_cast<ast::Identifier *>(var->base);
      core::VariableSymbol *sv = is_initialized_var(id);
      dim_value = sv->dim - dim_value;
    }
    else
    {
      type_value = semantic_expr(assign_Expr->value);
      dim_value = 0;
    }

    core::SymbolType result = TypeChecker::is_valid_assign(type_assignee, type_value, dim_assignee, dim_value);

    if (result == core::SymbolType::Undefined)
    {
      semantic_error(assignee_name, result, "Semantic error: Assignment Expression must be same datatype and same dimension.");
      return core::SymbolType::Undefined;
    }

    return result;
  }

  core::SymbolType SemanticAnalyzer::semantic_or_expr(ast::Expression *orExpr)
  {
    if (!dynamic_cast<ast::OrExpression *>(orExpr))
    {
      return semantic_and_expr(orExpr);
    }

    ast::OrExpression *or_Expr = static_cast<ast::OrExpression *>(orExpr);
    core::SymbolType left = semantic_expr(or_Expr->left);
    is_array(or_Expr->left);
    core::SymbolType right = semantic_expr(or_Expr->right);
    is_array(or_Expr->right);

    core::SymbolType result = TypeChecker::is_valid_or_and(left, right);

    if (result == core::SymbolType::Undefined)
    {
      semantic_error("or_expression", result, "Semantic error: Both sides must be Boolean in or expression.");
      return core::SymbolType::Undefined;
    }

    return result;
  }

  core::SymbolType SemanticAnalyzer::semantic_and_expr(ast::Expression *andExpr)
  {
    if (!dynamic_cast<ast::AndExpression *>(andExpr))
    {
      return semantic_equality_expr(andExpr);
    }

    ast::AndExpression *and_Expr = static_cast<ast::AndExpression *>(andExpr);
    core::SymbolType left = semantic_expr(and_Expr->left);
    is_array(and_Expr->left);
    core::SymbolType right = semantic_expr(and_Expr->right);
    is_array(and_Expr->right);

    core::SymbolType result = TypeChecker::is_valid_or_and(left, right);

    if (result == core::SymbolType::Undefined)
    {
      semantic_error("And_expression", result, "Semantic error: Both sides must be Boolean in and expression.");
      return core::SymbolType::Undefined;
    }

    return result;
  }

  core::SymbolType SemanticAnalyzer::semantic_equality_expr(ast::Expression *eqExpr)
  {
    if (!dynamic_cast<ast::EqualityExpression *>(eqExpr))
    {
      return semantic_relational_expr(eqExpr);
    }

    ast::EqualityExpression *eq_Expr = static_cast<ast::EqualityExpression *>(eqExpr);

    core::SymbolType left = semantic_expr(eq_Expr->left);
    is_array(eq_Expr->left);
    core::SymbolType right = semantic_expr(eq_Expr->right);
    is_array(eq_Expr->right);

    core::SymbolType result = TypeChecker::is_valid_equality(left, right);

    if (result == core::SymbolType::Undefined)
    {
      semantic_error("Equality_expression", result, "Semantic error: Both sides must be the same type in equality.");
      return core::SymbolType::Undefined;
    }

    return result;
  }

  core::SymbolType SemanticAnalyzer::semantic_relational_expr(ast::Expression *relExpr)
  {
    if (!dynamic_cast<ast::RelationalExpression *>(relExpr))
    {
      return semantic_additive_expr(relExpr);
    }

    ast::RelationalExpression *rel_Expr = static_cast<ast::RelationalExpression *>(relExpr);

    core::SymbolType left = semantic_expr(rel_Expr->left);
    core::SymbolType right = semantic_expr(rel_Expr->right);

    core::SymbolType result = TypeChecker::is_valid_relational(left, right);

    if (result == core::SymbolType::Undefined)
    {
      semantic_error("Relational_expression", result, "Semantic error: Both sides must be numbers in relational.");
      return core::SymbolType::Undefined;
    }

    return result;
  }

  core::SymbolType SemanticAnalyzer::semantic_additive_expr(ast::Expression *addExpr)
  {
    if (!dynamic_cast<ast::AdditiveExpression *>(addExpr))
    {
      return semantic_multiplicative_expr(addExpr);
    }

    ast::AdditiveExpression *add_Expr = static_cast<ast::AdditiveExpression *>(addExpr);

    core::SymbolType left = semantic_expr(add_Expr->left);
    is_array(add_Expr->left);
    core::SymbolType right = semantic_expr(add_Expr->right);
    is_array(add_Expr->right);
    string op = add_Expr->optr;

    core::SymbolType result = TypeChecker::is_valid_addition(left, right, op);

    if (result == core::SymbolType::Undefined)
    {
      semantic_error("Additive_expression", result, "Semantic error: Both sides must be numbers or strings in additive.");
      return core::SymbolType::Undefined;
    }

    return result;
  }

  core::SymbolType SemanticAnalyzer::semantic_multiplicative_expr(ast::Expression *mulExpr)
  {
    if (!dynamic_cast<ast::MultiplicativeExpression *>(mulExpr))
    {
      return semantic_unary_expr(mulExpr);
    }

    ast::MultiplicativeExpression *mul_Expr = static_cast<ast::MultiplicativeExpression *>(mulExpr);

    core::SymbolType left = semantic_expr(mul_Expr->left);
    is_array(mul_Expr->left);
    core::SymbolType right = semantic_expr(mul_Expr->right);
    is_array(mul_Expr->right);
    string op = mul_Expr->optr;

    core::SymbolType result = TypeChecker::is_valid_multiplicative(left, right, op);

    if (result == core::SymbolType::Undefined)
    {
      if (op == "%")
      {
        semantic_error("Multiplicative_expression", result, "Semantic error: Both sides must be Integers in (%).");
        return core::SymbolType::Undefined;
      }
      semantic_error("Multiplicative_expression", result, "Semantic error: Both sides must be numbers in multiplicative.");
      return core::SymbolType::Undefined;
    }

    return result;
  }

  core::SymbolType SemanticAnalyzer::semantic_unary_expr(ast::Expression *unaryExpr)
  {
    if (!dynamic_cast<ast::UnaryExpression *>(unaryExpr))
    {
      return semantic_index_expr(unaryExpr);
    }

    ast::UnaryExpression *unary_Expr = static_cast<ast::UnaryExpression *>(unaryExpr);
    int dim = 0;

    core::SymbolType type;

    if (unary_Expr->optr == "#")
    {
      type = semantic_index_expr(unary_Expr->operand, false, true);
    }
    else
    {
      type = semantic_expr(unary_Expr->operand);
    }
    if (dynamic_cast<ast::Identifier *>(unary_Expr->operand))
    {
      ast::Identifier *id = static_cast<ast::Identifier *>(unary_Expr->operand);
      SymbolTable *scope = retrieve_scope(id->name);
      if (!scope)
      {
        semantic_error(id->name, core::SymbolType::Undefined, "Semantic error: Variable '" + id->name + "' must be Declared.");
        return core::SymbolType::Undefined;
      }

      core::VariableSymbol *vs = scope->retrieve_variable(id->name);
      if (!vs)
      {
        semantic_error(id->name, core::SymbolType::Undefined, "Semantic error: Variable '" + id->name + "' must be Declared.");
        return core::SymbolType::Undefined;
      }
      dim = vs->dim;
    }
    else if (dynamic_cast<ast::IndexExpression *>(unary_Expr->operand))
    {
      ast::IndexExpression *indexExpr = static_cast<ast::IndexExpression *>(unary_Expr->operand);

      int accessed_dim = 1;
      ast::Expression *base = indexExpr->base;

      while (dynamic_cast<ast::IndexExpression *>(base))
      {
        base = static_cast<ast::IndexExpression *>(base)->base;
        accessed_dim++;
      }

      if (dynamic_cast<ast::Identifier *>(base))
      {
        ast::Identifier *base_id = static_cast<ast::Identifier *>(base);
        SymbolTable *scope = retrieve_scope(base_id->name);
        if (!scope)
        {
          semantic_error(base_id->name, core::SymbolType::Undefined, "Semantic error: Variable '" + base_id->name + "' must be Declared.");
          return core::SymbolType::Undefined;
        }

        core::VariableSymbol *symbol = scope->retrieve_variable(base_id->name);
        if (!symbol)
        {
          semantic_error(base_id->name, core::SymbolType::Undefined, "Semantic error: Variable '" + base_id->name + "' must be Declared.");
          return core::SymbolType::Undefined;
        }
        dim = symbol->dim - accessed_dim;
      }
      else
      {
        semantic_error("IndexExpr", core::SymbolType::Undefined, "Semantic error: Invalid base expression in indexing.");
        return core::SymbolType::Undefined;
      }
    }

    string op = unary_Expr->optr;
    if (op != "#")
    {
      is_array(unary_Expr->operand);
    }

    core::SymbolType result = TypeChecker::is_valid_Unary(type, op, dim);

    if (result == core::SymbolType::Undefined)
    {
      if (op == "-")
      {
        semantic_error("Unary_expression (-)", result, "Semantic error: Variable must be Integer or Float in Unary (-).");
        return core::SymbolType::Undefined;
      }
      else if (op == "not")
      {
        semantic_error("Unary_expression (not)", result, "Semantic error: Variable must be Boolean in Unary (not).");
        return core::SymbolType::Undefined;
      }
      else if (op == "++" || op == "--")
      {
        semantic_error("Unary_expression (++,--)", result, "Semantic error: Variable must be Integer or Float in Unary (++ , --).");
        return core::SymbolType::Undefined;
      }
      else if (op == "@")
      {
        semantic_error("Unary_expression (@)", result, "Semantic error: Variable must be Integer or Float or boolean in Unary (@).");
        return core::SymbolType::Undefined;
      }
      else if (op == "#")
      {
        semantic_error("Unary_expression (#)", result, "Semantic error: Variable must be String or array in Unary (#).");
        return core::SymbolType::Undefined;
      }
    }

    return result;
  }

  core::SymbolType SemanticAnalyzer::semantic_index_expr(ast::Expression *expr, bool set_init, bool allow_partial_indexing)
  {
    if (!dynamic_cast<ast::IndexExpression *>(expr))
    {
      return semantic_primary_expr(expr);
    }

    ast::IndexExpression *idxExpr = static_cast<ast::IndexExpression *>(expr);

    int dim = 1;
    core::SymbolType type;
    core::Symbol *symbol;

    core::SymbolType ty;
    while (dynamic_cast<ast::IndexExpression *>(idxExpr->base))
    {
      ty = semantic_expr(idxExpr->index);
      dim += 1;
      if (ty != core::SymbolType::Integer)
      {
        semantic_error("Index_expression", ty, "semantic error: invalid index.");
        return core::SymbolType::Undefined;
      }
      idxExpr = static_cast<ast::IndexExpression *>(idxExpr->base);
    }
    ty = semantic_expr(idxExpr->index);
    if (ty != core::SymbolType::Integer)
    {
      semantic_error("Index_expression", ty, "semantic error: invalid index.");
      return core::SymbolType::Undefined;
    }

    if (dynamic_cast<ast::Identifier *>(idxExpr->base))
    {
      ast::Identifier *var = static_cast<ast::Identifier *>(idxExpr->base);

      SymbolTable *scope = retrieve_scope(var->name);
      if (!scope)
      {
        semantic_error(var->name, core::SymbolType::Undefined, "Semantic error: Symbol '" + var->name + "' must be Declared.");
        return core::SymbolType::Undefined;
      }
      if (set_init)
      {
        scope->set_initialized(var->name);
      }

      symbol = scope->retrieve_symbol(var->name);
      if (!symbol)
      {
        semantic_error(var->name, core::SymbolType::Undefined, "Semantic error: Symbol '" + var->name + "' must be Declared.");
        return core::SymbolType::Undefined;
      }
    }
    else
    {
      ast::CallFunctionExpression *cfExpr = static_cast<ast::CallFunctionExpression *>(idxExpr->base);
      SymbolTable *st = retrieve_scope(cfExpr->function->name);
      symbol = retrieve_scope(cfExpr->function->name)->retrieve_symbol(cfExpr->function->name);
      if (symbol == nullptr)
      {
        semantic_error(cfExpr->function->name, core::SymbolType::Undefined, "Semantic error: Symbol '" + cfExpr->function->name + "' must be Declared.");
        return core::SymbolType::Undefined;
      }
    }

    type = semantic_expr(idxExpr->base);

    if (((dim != symbol->dim) && !allow_partial_indexing) || (dim > symbol->dim))
    {
      semantic_error(symbol->name, type, "Dimension mismatch for variable " + symbol->name + ": expected " + to_string(symbol->dim) + ", but got " + to_string(dim));
      return core::SymbolType::Undefined;
    }
    if (type != symbol->type)
    {
      semantic_error(symbol->name, type, "Datatype mismatch for variable " + symbol->name + ": expected " + core::Symbol::get_name_symboltype(type) + ", but got " + core::Symbol::get_name_symboltype(symbol->type));
      return core::SymbolType::Undefined;
    }

    return type;
  }

  core::SymbolType SemanticAnalyzer::semantic_primary_expr(ast::Expression *primaryExpr)
  {
    if (dynamic_cast<ast::Identifier *>(primaryExpr))
    {
      ast::Identifier *id = static_cast<ast::Identifier *>(primaryExpr);
      return semantic_id(id);
    }
    return semantic_literal(primaryExpr);
  }

  core::SymbolType SemanticAnalyzer::semantic_literal(ast::Expression *lit)
  {
    if (!dynamic_cast<ast::Literal *>(lit))
    {
      return semantic_call_function_expr(lit);
    }

    if (dynamic_cast<ast::IntegerLiteral *>(lit))
    {
      return core::SymbolType::Integer;
    }
    else if (dynamic_cast<ast::FloatLiteral *>(lit))
    {
      return core::SymbolType::Float;
    }
    else if (dynamic_cast<ast::StringLiteral *>(lit))
    {
      return core::SymbolType::String;
    }
    else if (dynamic_cast<ast::BooleanLiteral *>(lit))
    {
      return core::SymbolType::Boolean;
    }
    else if (dynamic_cast<ast::ArrayLiteral *>(lit))
    {
      return semantic_array(static_cast<ast::ArrayLiteral *>(lit)).first;
    }
    return core::SymbolType::Undefined;
  }

  core::SymbolType SemanticAnalyzer::semantic_call_function_expr(ast::Expression *cfExpr)
  {
    if (!dynamic_cast<ast::CallFunctionExpression *>(cfExpr))
    {
      return core::SymbolType::Undefined;
    }

    ast::CallFunctionExpression *cf_Expr = static_cast<ast::CallFunctionExpression *>(cfExpr);

    stack<SymbolTable *> temp = call_stack;
    SymbolTable *global = nullptr;
    ast::Identifier *func_name = cf_Expr->function;
    vector<ast::Expression *> arguments = cf_Expr->arguments;
    while (!temp.empty())
    {
      global = temp.top();
      temp.pop();
    }

    if (!global || !global->is_exist(func_name->name))
    {
      semantic_error(func_name->name, core::SymbolType::Undefined, "Semantic error: Function '" + func_name->name + "' Not Declared.");
      return core::SymbolType::Undefined;
    }

    core::FunctionSymbol *fn = global->retrieve_function(func_name->name);
    if (!fn)
    {
      semantic_error(func_name->name, core::SymbolType::Undefined, "Semantic error: Invalid function object.");
      return core::SymbolType::Undefined;
    }
    auto all_args = fn->parameters;
    auto required_args = global->get_required_arguments(func_name->name);

    if (arguments.size() < required_args.size() || arguments.size() > all_args.size())
    {
      semantic_error(func_name->name, core::SymbolType::Undefined, "Semantic error: Function '" + func_name->name + "' expects between " + to_string(required_args.size()) + " and " + to_string(all_args.size()) + " arguments, but got " + to_string(arguments.size()) + ".");
      return core::SymbolType::Undefined;
    }

    for (int i = 0; i < arguments.size(); ++i)
    {
      core::SymbolType type;
      if (!dynamic_cast<ast::IndexExpression *>(arguments[i]))
      {
        core::SymbolType type = semantic_expr(arguments[i]);
      }

      // 1. Get dimension of current argument
      int arg_dim = 0;
      if (dynamic_cast<ast::ArrayLiteral *>(arguments[i]))
      {
        arg_dim = semantic_array(static_cast<ast::ArrayLiteral *>(arguments[i])).second;
      }
      else if (dynamic_cast<ast::Identifier *>(arguments[i]))
      {
        ast::Identifier *id = static_cast<ast::Identifier *>(arguments[i]);
        SymbolTable *scope = retrieve_scope(id->name);
        if (scope)
        {
          core::VariableSymbol *vs = scope->retrieve_variable(id->name);
          if (vs)
            arg_dim = vs->dim;
        }
      }
      else if (dynamic_cast<ast::IndexExpression *>(arguments[i]))
      {
        ast::IndexExpression *var = static_cast<ast::IndexExpression *>(arguments[i]);
        type = semantic_index_expr(arguments[i], false, true);
        arg_dim = 1;
        while (dynamic_cast<ast::IndexExpression *>(var->base))
        {
          arg_dim += 1;
          var = static_cast<ast::IndexExpression *>(var->base);
        }
        ast::Identifier *id = static_cast<ast::Identifier *>(var->base);
        core::VariableSymbol *sv = is_initialized_var(id);
        arg_dim = sv->dim - arg_dim;
      }

      // 2. Get expected type + dimension from function signature
      core::SymbolType expected_type = all_args[i].first;
      int expected_dim = all_args[i].second;

      // 3. Type check
      if (type != expected_type)
      {
        semantic_error(func_name->name, core::SymbolType::Undefined,
                       "Semantic error: Argument " + to_string(i + 1) + " in function '" + func_name->name +
                           "' should be of type " + core::Symbol::get_name_symboltype(expected_type) +
                           ", but got " + core::Symbol::get_name_symboltype(type) + ".");
        return core::SymbolType::Undefined;
      }

      // 4. Dimension check
      if (arg_dim != expected_dim)
      {
        semantic_error(func_name->name, core::SymbolType::Undefined,
                       "Semantic error: Dimension mismatch in argument " + to_string(i + 1) +
                           " in function '" + func_name->name +
                           "': expected dim " + to_string(expected_dim) +
                           ", but got " + to_string(arg_dim) + ".");
        return core::SymbolType::Undefined;
      }
    }

    return fn->type;
  }

  core::SymbolType SemanticAnalyzer::semantic_id(ast::Identifier *id, bool set_init)
  {
    stack<SymbolTable *> temp = call_stack;
    string var = id->name;
    SymbolTable *st = nullptr;
    while (!temp.empty())
    {
      st = temp.top();
      if (st->is_exist(var))
      {
        if (set_init)
        {
          st->set_initialized(var);
        }
        is_initialized_var(id);
        return st->get_type(var);
      }
      temp.pop();
    }
    semantic_error(var, core::SymbolType::Undefined, "Semantic error: Variable '" + var + "' Not Declared.");
    return core::SymbolType::Undefined;
  }

  pair<core::SymbolType, int> SemanticAnalyzer::semantic_array(ast::ArrayLiteral *arrNode)
  {
    vector<ast::Expression *> elements = arrNode->elements;
    pair<core::SymbolType, int> value_list = pair<core::SymbolType, int>(core::SymbolType::Undefined, 1);

    if (elements.size() == 0)
    {
      return value_list;
    }

    if (elements.size() > 0 && !dynamic_cast<ast::ArrayLiteral *>(elements[0]))
    {
      value_list = semantic_array_value(elements);
      return value_list;
    }

    for (auto el : elements)
    {
      ast::ArrayLiteral *nested = static_cast<ast::ArrayLiteral *>(el);
      int dim = value_list.second;
      value_list = semantic_array(nested);

      if (value_list.second != dim && el != elements.front())
      {
        semantic_error("Array_literal", value_list.first, "semantic error: Inconsistent array dimension.");
        return pair<core::SymbolType, int>(core::SymbolType::Undefined, 0);
      }
    }

    value_list.second++;

    return value_list;
  }

  pair<core::SymbolType, int> SemanticAnalyzer::semantic_array_value(vector<ast::Expression *> elements)
  {
    core::SymbolType dt = core::SymbolType::Undefined;
    core::SymbolType element_dt = core::SymbolType::Undefined;
    int max_inner_dim = 0;

    for (auto el : elements)
    {
      int el_dim = 0;

      element_dt = semantic_expr(el);

      if (dynamic_cast<ast::Identifier *>(el))
      {
        ast::Identifier *id = static_cast<ast::Identifier *>(el);
        SymbolTable *scope = retrieve_scope(id->name);
        if (scope)
        {
          core::VariableSymbol *var = scope->retrieve_variable(id->name);
          if (var)
            el_dim = var->dim;
        }
      }
      else if (dynamic_cast<ast::ArrayLiteral *>(el))
      {
        el_dim = semantic_array(static_cast<ast::ArrayLiteral *>(el)).second;
      }

      if (el_dim > max_inner_dim)
      {
        max_inner_dim = el_dim;
      }

      if (dt == core::SymbolType::Undefined)
      {
        dt = element_dt;
      }
      else if (element_dt != dt)
      {
        semantic_error("Array_literal", element_dt, "semantic error: array contain value of multiple datatypes");
        return pair<core::SymbolType, int>(core::SymbolType::Undefined, 0);
      }
    }
    return pair<core::SymbolType, int>(dt, max_inner_dim + 1);
  }

  core::VariableSymbol *SemanticAnalyzer::is_initialized_var(ast::Identifier *id)
  {
    SymbolTable *st = retrieve_scope(id->name);
    core::VariableSymbol *var = st->retrieve_variable(id->name);
    if (var == nullptr)
    {
      semantic_error(id->name, core::SymbolType::Undefined, "Semantic error: Variable '" + id->name + "' must be Declared.");
      return nullptr;
    }

    if (!var->is_initialized)
    {
      semantic_error(id->name, core::SymbolType::Undefined, "Semantic error: Variable '" + id->name + "' must be Initialized.");
      return nullptr;
    }

    return var;
  }

  SymbolTable *SemanticAnalyzer::retrieve_scope(string sn)
  {
    stack<SymbolTable *> temp = call_stack;

    while (!temp.empty())
    {
      SymbolTable *st = temp.top();
      if (st->is_exist(sn))
      {
        return st;
      }
      temp.pop();
    }

    semantic_error(sn, core::SymbolType::Undefined, "Semantic error: Variable '" + sn + "' must be Declared.");
    return nullptr;
  }

  void SemanticAnalyzer::is_array(ast::Expression *Expr)
  {
    if (dynamic_cast<ast::Identifier *>(Expr))
    {
      ast::Identifier *id = static_cast<ast::Identifier *>(Expr);

      SymbolTable *scope = retrieve_scope(id->name);
      if (scope == nullptr)
      {
        semantic_error(id->name, core::SymbolType::Undefined, "Semantic error: Variable '" + id->name + "' must be Declared.");
        return;
      }
      core::VariableSymbol *vr = scope->retrieve_variable(id->name);
      if (vr == nullptr)
      {
        semantic_error(id->name, core::SymbolType::Undefined, "Semantic error: Variable '" + id->name + "' must be Declared.");
        return;
      }
      int dim = vr->dim;
      if (dim > 0)
      {
        semantic_error(id->name, core::SymbolType::Undefined, "Semantic error: Invalid Expression can't use array.");
        return;
      }
    }
    else if (dynamic_cast<ast::CallFunctionExpression *>(Expr))
    {
      ast::CallFunctionExpression *cf = static_cast<ast::CallFunctionExpression *>(Expr);
      SymbolTable *scope = retrieve_scope(cf->function->name);
      if (!scope)
      {
        semantic_error(cf->function->name, core::SymbolType::Undefined, "Semantic error: Function '" + cf->function->name + "' must be Declared.");
        return;
      }
      core::FunctionSymbol *fn = scope->retrieve_function(cf->function->name);
      if (!fn)
      {
        semantic_error(cf->function->name, core::SymbolType::Undefined, "Semantic error: Function '" + cf->function->name + "' must be Declared.");
        return;
      }
      int dim = fn->dim;
      if (dim > 0)
      {
        semantic_error(cf->function->name, core::SymbolType::Undefined, "Semantic error: Invalid Expression can't use array.");
        return;
      }
    }
  }

  core::ErrorSymbol SemanticAnalyzer::semantic_error(string name, core::SymbolType st, string err)
  {
    if (!has_error)
    {
      has_error = true;
      error_symbol = core::ErrorSymbol(name, st, err);
    }

    return error_symbol;
  }

  core::ErrorSymbol SemanticAnalyzer::forword_syntax_error(ast::ErrorNode *err)
  {
    semantic_error("", core::SymbolType::Undefined, err->message);
    return error_symbol;
  }
}