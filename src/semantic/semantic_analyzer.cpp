#include "semantic_analyzer.h"

#include <typeinfo>

SemanticAnalyzer::SemanticAnalyzer(ParserBase *pr) : parser(pr)
{
  error_symbol = ErrorSymbol();
  has_error = false;
}

ErrorSymbol SemanticAnalyzer::get_error()
{
  if (has_error)
  {
    return error_symbol;
  }
  return {};
}

void SemanticAnalyzer::analyze()
{
  Source *source = (Source *)parser->parse_ast();
  semantic_source(source);
  if (has_error)
  {
    cout << error_symbol.message_error << endl;
  }
}

void SemanticAnalyzer::semantic_source(Source *source)
{
  SymbolTable *st = new SymbolTable();
  string program_name = source->program->program_name->name;
  bool check = st->insert(new Symbol(
      program_name,
      SymbolType::Program,
      SymbolKind::Function));
  if (!check)
  {
    semantic_error(program_name, SymbolType::Program, "Semantic error: Symbol '" + program_name + "' already exists.");
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
    DataType *dt = func->return_type->return_type;
    vector<pair<SymbolType, int>> parameters = SymbolTable::get_parameters_type(func->parameters);

    bool found_default = false;
    for (auto def : func->parameters)
    {
      if (dynamic_cast<VariableInitialization *>(def->def))
      {
        found_default = true;
      }
      else if (found_default)
      {
        semantic_error(name, SymbolType::Undefined, "Semantic error: Required parameters cannot follow optional parameters.");
        return;
      }
    }

    if (!parameters.empty() && parameters[0].first == SymbolType::Undefined)
    {
      semantic_error(name, SymbolType::Undefined, "Semantic error: Variable name is already defined.");
      return;
    }

    auto func_symbol = new FunctionSymbol(
        name,
        Symbol::get_datatype(dt),
        parameters,
        Symbol::get_dimension(dt));

    func_symbol->parametersRaw = func->parameters;

    bool check = st->insert(func_symbol);
    if (!check)
    {
      semantic_error(name, Symbol::get_datatype(dt), "Semantic error: Symbol '" + name + "' already exists.");
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

void SemanticAnalyzer::semantic_program(Program *program)
{
  SymbolTable *st = new SymbolTable();
  call_stack.push(st);
  for (auto stmt : program->body)
  {
    semantic_command(stmt, program->program_name->name);
  }
  call_stack.pop();
}

void SemanticAnalyzer::sematic_function(Function *func)
{
  SymbolTable *st = new SymbolTable();
  SymbolTable *globals = call_stack.top();
  bool is_return = false;
  call_stack.push(st);
  vector<VariableDefinition *> vars = func->parameters;
  for (auto var : vars)
  {
    if (dynamic_cast<VariableInitialization *>(var->def))
    {
      semantic_var_def(var, st);
    }
    else if (dynamic_cast<VariableDeclaration *>(var->def))
    {
      VariableDeclaration *def = (VariableDeclaration *)var->def;

      for (auto id : def->variables)
      {
        bool check = st->insert(new VariableSymbol(
            id->name,
            Symbol::get_datatype(def->datatype),
            true,
            Symbol::get_dimension(def->datatype)));
        if (!check)
        {
          semantic_error(id->name, Symbol::get_datatype(def->datatype), "Semantic error: Symbol '" + id->name + "' already exists.");
          return;
        }
      }
    }
  }
  for (auto stmt : func->body)
  {
    semantic_command(stmt, func->funcname->name);
    if (dynamic_cast<ReturnStatement *>(stmt) && globals->get_type(func->funcname->name) != SymbolType::Void && !is_return)
    {
      is_return = true;
    }
  }

  if (globals->get_type(func->funcname->name) != SymbolType::Void && !is_return)
  {
    semantic_error(func->funcname->name, globals->get_type(func->funcname->name), "Semantic error : missing a return statement in the function body in '" + func->funcname->name + "'.");
    return;
  }

  call_stack.pop();
}

void SemanticAnalyzer::semantic_command(Statement *stmt, string name_parent)
{
  if (dynamic_cast<IfStatement *>(stmt))
  {
    IfStatement *ifStmt = static_cast<IfStatement *>(stmt);
    semantic_if(ifStmt, name_parent);
  }
  else if (dynamic_cast<ReturnStatement *>(stmt))
  {
    ReturnStatement *rtnStmt = static_cast<ReturnStatement *>(stmt);
    semantic_return(rtnStmt, name_parent);
  }
  else if (dynamic_cast<ReadStatement *>(stmt))
  {
    ReadStatement *readStmt = static_cast<ReadStatement *>(stmt);
    semantic_read(readStmt);
  }
  else if (dynamic_cast<WriteStatement *>(stmt))
  {
    WriteStatement *writeStmt = static_cast<WriteStatement *>(stmt);
    semantic_write(writeStmt);
  }
  else if (dynamic_cast<WhileLoop *>(stmt))
  {
    WhileLoop *loop = static_cast<WhileLoop *>(stmt);
    semantic_while(loop, name_parent);
  }
  else if (dynamic_cast<ForLoop *>(stmt))
  {
    ForLoop *loop = static_cast<ForLoop *>(stmt);
    semantic_for(loop, name_parent);
  }
  else if (dynamic_cast<Expression *>(stmt))
  {
    Expression *expr = static_cast<Expression *>(stmt);
    semantic_expr(expr);
  }
  else if (dynamic_cast<VariableDefinition *>(stmt))
  {
    VariableDefinition *def = static_cast<VariableDefinition *>(stmt);
    semantic_var_def(def, call_stack.top());
  }
}

void SemanticAnalyzer::semantic_if(IfStatement *ifStmt, string name_parent)
{
  SymbolTable *st = new SymbolTable();
  call_stack.push(st);

  SymbolType ty = semantic_expr(ifStmt->condition);

  if (ty != SymbolType::Boolean)
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

void SemanticAnalyzer::semantic_return(ReturnStatement *rtnStmt, string name_parent)
{
  stack<SymbolTable *> temp = call_stack;
  SymbolTable *global = nullptr;
  while (!temp.empty())
  {
    global = temp.top();
    temp.pop();
  }

  SymbolType funcType = global->get_type(name_parent);
  SymbolType returnType = semantic_expr(rtnStmt->returnValue);

  if (funcType == SymbolType::Void || funcType == SymbolType::Program)
  {
    if (returnType != SymbolType::Undefined)
    {
      semantic_error(name_parent, returnType, "Semantic error: 'return' in block '" + name_parent + "' must not have an expression.");
      return;
    }
  }
  else if (funcType != returnType)
  {
    if (!((funcType == SymbolType::Integer || funcType == SymbolType::Float) &&
          (returnType == SymbolType::Integer || returnType == SymbolType::Float)))
    {
      semantic_error(name_parent, returnType, "Semantic error: 'return' in function block '" + name_parent + "' doesn't match the function type.");
      return;
    }
  }

  // Dimension of return
  int dim_func = global->retrieve_function(name_parent)->dim;
  int dim_return = 0;
  if (dynamic_cast<ArrayLiteral *>(rtnStmt->returnValue))
  {
    pair<SymbolType, int> array_value = semantic_array((ArrayLiteral *)rtnStmt->returnValue);
    dim_return = array_value.second;
  }
  else if (dynamic_cast<Identifier *>(rtnStmt->returnValue))
  {
    Identifier *var = static_cast<Identifier *>(rtnStmt->returnValue);
    VariableSymbol *vs = is_initialized_var(var);
    dim_return = vs->dim;
  }
  if (dim_func != dim_return)
  {
    semantic_error(name_parent, returnType, "Semantic error: 'return' in function block '" + name_parent + "' doesn't match the function dimensions.");
    return;
  }
}

void SemanticAnalyzer::semantic_read(ReadStatement *readStmt)
{
  stack<SymbolTable *> temp = call_stack;
  vector<AssignableExpression *> vars = readStmt->variables;
  for (auto var : vars)
  {
    semantic_assignable_expr(var);
  }
}

SymbolType SemanticAnalyzer::semantic_assignable_expr(AssignableExpression *assignable)
{
  if (dynamic_cast<Identifier *>(assignable))
  {
    Identifier *id = static_cast<Identifier *>(assignable);
    return semantic_id(id, true);
  }
  return semantic_index_expr(assignable, true);
}

void SemanticAnalyzer::semantic_write(WriteStatement *writeStmt)
{
  for (auto expr : writeStmt->args)
  {
    semantic_expr(expr);

    is_array(expr);
  }
}

void SemanticAnalyzer::semantic_while(WhileLoop *loop, string name_parent)
{
  SymbolTable *st = new SymbolTable();
  call_stack.push(st);
  SymbolType ty = semantic_expr(loop->condition);

  if (ty != SymbolType::Boolean)
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

void SemanticAnalyzer::semantic_for(ForLoop *loop, string name_parent)
{
  SymbolTable *st = new SymbolTable();
  call_stack.push(st);

  semantic_int_assign(loop->init);
  SymbolType ty = semantic_expr(loop->condition);

  if (ty != SymbolType::Boolean)
  {
    semantic_error("For_loop", ty, "Semantic error: condition must be boolean");
    return;
  }

  SymbolType ty_update = semantic_expr(loop->update);

  if (ty_update != SymbolType::Integer)
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

void SemanticAnalyzer::semantic_int_assign(AssignmentExpression *init)
{
  stack<SymbolTable *> temp = call_stack;
  if (dynamic_cast<Identifier *>(init->assignee))
  {
    Identifier *var = static_cast<Identifier *>(init->assignee);
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
    bool check = call_stack.top()->insert(new VariableSymbol(
        var->name,
        SymbolType::Integer,
        true,
        0));
    if (!check)
    {
      semantic_error(var->name, SymbolType::Integer, "Semantic error: Symbol '" + var->name + "' already exists.");
      return;
    }

    SymbolType type = semantic_expr(init->value);
    if (type != SymbolType::Integer)
    {
      semantic_error(var->name, type, "Semantic error: Value of '" + var->name + "' Must be integer.");
      return;
    }
  }
  else
  {
    semantic_error("For_loop", SymbolType::Integer, "Semantic error: in initialization part of forLoop must be identifier .");
    return;
  }
}

void SemanticAnalyzer::semantic_var_def(VariableDefinition *var, SymbolTable *st)
{
  if (dynamic_cast<VariableDeclaration *>(var->def))
  {
    VariableDeclaration *def = (VariableDeclaration *)var->def;

    for (auto id : def->variables)
    {
      bool check = st->insert(new VariableSymbol(
          id->name,
          Symbol::get_datatype(def->datatype),
          false,
          Symbol::get_dimension(def->datatype)));
      if (!check)
      {
        semantic_error(id->name, Symbol::get_datatype(def->datatype), "Semantic error: Symbol '" + id->name + "' already exists.");
        return;
      }
    }
  }
  else if (dynamic_cast<VariableInitialization *>(var->def))
  {
    VariableInitialization *def = (VariableInitialization *)var->def;
    SymbolType dt = Symbol::get_datatype(def->datatype);
    SymbolType dt_init;
    int dim_left = Symbol::get_dimension(def->datatype);
    int dim_init = 0;

    if (dynamic_cast<ArrayLiteral *>(def->initializer))
    {
      pair<SymbolType, int> array_value = semantic_array((ArrayLiteral *)def->initializer);

      dt_init = array_value.first;
      dim_init = array_value.second;
    }

    else if (dynamic_cast<Identifier *>(def->initializer))
    {
      Identifier *var = static_cast<Identifier *>(def->initializer);
      VariableSymbol *sv = is_initialized_var(var);
      dt_init = sv->type;
      dim_init = sv->dim;
    }
    else
    {
      dt_init = semantic_expr(def->initializer);
      dim_init = 0;
    }
    SymbolType result = TypeChecker::is_valid_assign(dt, dt_init, dim_left, dim_init);

    if (result == SymbolType::Undefined)
    {
      semantic_error(def->name->name, result, "Semantic error: invalid initialization.");
      return;
    }

    bool check = st->insert(new VariableSymbol(
        def->name->name,
        Symbol::get_datatype(def->datatype),
        true,
        Symbol::get_dimension(def->datatype)));
    if (!check)
    {
      semantic_error(def->name->name, Symbol::get_datatype(def->datatype), "Semantic error: Symbol '" + def->name->name + "' already exists.");
      return;
    }
  }
}

SymbolType SemanticAnalyzer::semantic_expr(Expression *expr)
{
  return semantic_assign_expr(expr);
}

SymbolType SemanticAnalyzer::semantic_assign_expr(Expression *assignExpr)
{
  if (!dynamic_cast<AssignmentExpression *>(assignExpr))
  {
    return semantic_or_expr(assignExpr);
  }

  AssignmentExpression *assign_Expr = static_cast<AssignmentExpression *>(assignExpr);

  string assignee_name;
  int dim_assignee = 0;
  int dim_value = 0;

  if (dynamic_cast<Identifier *>(assign_Expr->assignee))
  {
    Identifier *id = static_cast<Identifier *>(assign_Expr->assignee);
    assignee_name = id->name;
    SymbolTable *scope = retrieve_scope(assignee_name);
    if (scope == nullptr)
    {
      return SymbolType::Undefined;
    }
    VariableSymbol *vr = scope->retrieve_variable(assignee_name);
    if (vr == nullptr)
    {
      semantic_error(assignee_name, SymbolType::Undefined, "Semantic error: Variable '" + assignee_name + "' must be Declared.");
      return SymbolType::Undefined;
    }
    dim_assignee = vr->dim;
  }
  else if (dynamic_cast<IndexExpression *>(assign_Expr->assignee))
  {
    Expression *base_expr = static_cast<IndexExpression *>(assign_Expr->assignee)->base;
    int accessed_dim = 1;

    while (dynamic_cast<IndexExpression *>(base_expr))
    {
      base_expr = static_cast<IndexExpression *>(base_expr)->base;
      accessed_dim++;
    }
    assignee_name = static_cast<Identifier *>(base_expr)->name;

    SymbolTable *scope = retrieve_scope(assignee_name);
    if (scope == nullptr)
    {
      return SymbolType::Undefined;
    }
    VariableSymbol *vr = scope->retrieve_variable(assignee_name);
    if (vr == nullptr)
    {
      semantic_error(assignee_name, SymbolType::Undefined, "Semantic error: Variable '" + assignee_name + "' must be Declared.");
      return SymbolType::Undefined;
    }

    int total_dim = vr->dim;
    dim_assignee = total_dim - accessed_dim;

    if (dim_assignee < 0)
    {
      semantic_error(assignee_name, SymbolType::Undefined, "Semantic error: Invalid array access for variable '" + assignee_name + "'.");
      return SymbolType::Undefined;
    }
  }

  SymbolType type_assignee = semantic_assignable_expr(assign_Expr->assignee);
  SymbolType type_value = semantic_expr(assign_Expr->value);

  if (dynamic_cast<ArrayLiteral *>(assign_Expr->value))
  {
    dim_value = semantic_array(static_cast<ArrayLiteral *>(assign_Expr->value)).second;
  }
  else if (dynamic_cast<Identifier *>(assign_Expr->value))
  {
    Identifier *id = static_cast<Identifier *>(assign_Expr->value);
    SymbolTable *scope = retrieve_scope(id->name);
    if (scope == nullptr)
    {
      semantic_error(id->name, SymbolType::Undefined, "Semantic error: Variable '" + id->name + "' must be Declared.");
      return SymbolType::Undefined;
    }
    VariableSymbol *vr = scope->retrieve_variable(id->name);
    if (vr == nullptr)
    {
      semantic_error(id->name, SymbolType::Undefined, "Semantic error: Variable '" + id->name + "' must be Declared.");
      return SymbolType::Undefined;
    }
    dim_value = vr->dim;
  }
  else
  {
    dim_value = 0;
  }
  SymbolType result = TypeChecker::is_valid_assign(type_assignee, type_value, dim_assignee, dim_value);

  if (result == SymbolType::Undefined)
  {
    semantic_error(assignee_name, result, "Semantic error: Assignment Expression must be same datatype and same dimension.");
    return SymbolType::Undefined;
  }

  return result;
}

SymbolType SemanticAnalyzer::semantic_or_expr(Expression *orExpr)
{
  if (!dynamic_cast<OrExpression *>(orExpr))
  {
    return semantic_and_expr(orExpr);
  }

  OrExpression *or_Expr = static_cast<OrExpression *>(orExpr);
  SymbolType left = semantic_expr(or_Expr->left);
  is_array(or_Expr->left);
  SymbolType right = semantic_expr(or_Expr->right);
  is_array(or_Expr->right);

  SymbolType result = TypeChecker::is_valid_or_and(left, right);

  if (result == SymbolType::Undefined)
  {
    semantic_error("or_expression", result, "Semantic error: Both sides must be Boolean in or expression.");
    return SymbolType::Undefined;
  }

  return result;
}

SymbolType SemanticAnalyzer::semantic_and_expr(Expression *andExpr)
{
  if (!dynamic_cast<AndExpression *>(andExpr))
  {
    return semantic_equality_expr(andExpr);
  }

  AndExpression *and_Expr = static_cast<AndExpression *>(andExpr);
  SymbolType left = semantic_expr(and_Expr->left);
  is_array(and_Expr->left);
  SymbolType right = semantic_expr(and_Expr->right);
  is_array(and_Expr->right);

  SymbolType result = TypeChecker::is_valid_or_and(left, right);

  if (result == SymbolType::Undefined)
  {
    semantic_error("And_expression", result, "Semantic error: Both sides must be Boolean in and expression.");
    return SymbolType::Undefined;
  }

  return result;
}

SymbolType SemanticAnalyzer::semantic_equality_expr(Expression *eqExpr)
{
  if (!dynamic_cast<EqualityExpression *>(eqExpr))
  {
    return semantic_relational_expr(eqExpr);
  }

  EqualityExpression *eq_Expr = static_cast<EqualityExpression *>(eqExpr);

  SymbolType left = semantic_expr(eq_Expr->left);
  is_array(eq_Expr->left);
  SymbolType right = semantic_expr(eq_Expr->right);
  is_array(eq_Expr->right);

  SymbolType result = TypeChecker::is_valid_equality(left, right);

  if (result == SymbolType::Undefined)
  {
    semantic_error("Equality_expression", result, "Semantic error: Both sides must be the same type in equality.");
    return SymbolType::Undefined;
  }

  return result;
}

SymbolType SemanticAnalyzer::semantic_relational_expr(Expression *relExpr)
{
  if (!dynamic_cast<RelationalExpression *>(relExpr))
  {
    return semantic_additive_expr(relExpr);
  }

  RelationalExpression *rel_Expr = static_cast<RelationalExpression *>(relExpr);

  SymbolType left = semantic_expr(rel_Expr->left);
  SymbolType right = semantic_expr(rel_Expr->right);

  SymbolType result = TypeChecker::is_valid_relational(left, right);

  if (result == SymbolType::Undefined)
  {
    semantic_error("Relational_expression", result, "Semantic error: Both sides must be numbers in relational.");
    return SymbolType::Undefined;
  }

  return result;
}

SymbolType SemanticAnalyzer::semantic_additive_expr(Expression *addExpr)
{
  if (!dynamic_cast<AdditiveExpression *>(addExpr))
  {
    return semantic_multiplicative_expr(addExpr);
  }

  AdditiveExpression *add_Expr = static_cast<AdditiveExpression *>(addExpr);

  SymbolType left = semantic_expr(add_Expr->left);
  is_array(add_Expr->left);
  SymbolType right = semantic_expr(add_Expr->right);
  is_array(add_Expr->right);
  string op = add_Expr->optr;

  SymbolType result = TypeChecker::is_valid_addition(left, right, op);

  if (result == SymbolType::Undefined)
  {
    semantic_error("Additive_expression", result, "Semantic error: Both sides must be numbers or strings in additive.");
    return SymbolType::Undefined;
  }

  return result;
}

SymbolType SemanticAnalyzer::semantic_multiplicative_expr(Expression *mulExpr)
{
  if (!dynamic_cast<MultiplicativeExpression *>(mulExpr))
  {
    return semantic_unary_expr(mulExpr);
  }

  MultiplicativeExpression *mul_Expr = static_cast<MultiplicativeExpression *>(mulExpr);

  SymbolType left = semantic_expr(mul_Expr->left);
  is_array(mul_Expr->left);
  SymbolType right = semantic_expr(mul_Expr->right);
  is_array(mul_Expr->right);
  string op = mul_Expr->optr;

  SymbolType result = TypeChecker::is_valid_multiplicative(left, right, op);

  if (result == SymbolType::Undefined)
  {
    if (op == "%")
    {
      semantic_error("Multiplicative_expression", result, "Semantic error: Both sides must be Integers in (%).");
      return SymbolType::Undefined;
    }
    semantic_error("Multiplicative_expression", result, "Semantic error: Both sides must be numbers in multiplicative.");
    return SymbolType::Undefined;
  }

  return result;
}

SymbolType SemanticAnalyzer::semantic_unary_expr(Expression *unaryExpr)
{
  if (!dynamic_cast<UnaryExpression *>(unaryExpr))
  {
    return semantic_index_expr(unaryExpr);
  }

  UnaryExpression *unary_Expr = static_cast<UnaryExpression *>(unaryExpr);
  int dim = 0;

  SymbolType type;

  if (unary_Expr->optr == "#")
  {
    type = semantic_index_expr(unary_Expr->operand, false, true);
  }
  else
  {
    type = semantic_expr(unary_Expr->operand);
  }
  if (dynamic_cast<Identifier *>(unary_Expr->operand))
  {
    Identifier *id = static_cast<Identifier *>(unary_Expr->operand);
    SymbolTable *scope = retrieve_scope(id->name);
    if (!scope)
    {
      semantic_error(id->name, SymbolType::Undefined, "Semantic error: Variable '" + id->name + "' must be Declared.");
      return SymbolType::Undefined;
    }

    VariableSymbol *vs = scope->retrieve_variable(id->name);
    if (!vs)
    {
      semantic_error(id->name, SymbolType::Undefined, "Semantic error: Variable '" + id->name + "' must be Declared.");
      return SymbolType::Undefined;
    }
    dim = vs->dim;
  }
  else if (dynamic_cast<IndexExpression *>(unary_Expr->operand))
  {
    IndexExpression *indexExpr = static_cast<IndexExpression *>(unary_Expr->operand);

    int accessed_dim = 1;
    Expression *base = indexExpr->base;

    while (dynamic_cast<IndexExpression *>(base))
    {
      base = static_cast<IndexExpression *>(base)->base;
      accessed_dim++;
    }

    if (dynamic_cast<Identifier *>(base))
    {
      Identifier *base_id = static_cast<Identifier *>(base);
      SymbolTable *scope = retrieve_scope(base_id->name);
      if (!scope)
      {
        semantic_error(base_id->name, SymbolType::Undefined, "Semantic error: Variable '" + base_id->name + "' must be Declared.");
        return SymbolType::Undefined;
      }

      VariableSymbol *symbol = scope->retrieve_variable(base_id->name);
      if (!symbol)
      {
        semantic_error(base_id->name, SymbolType::Undefined, "Semantic error: Variable '" + base_id->name + "' must be Declared.");
        return SymbolType::Undefined;
      }
      dim = symbol->dim - accessed_dim;
    }
    else
    {
      semantic_error("IndexExpr", SymbolType::Undefined, "Semantic error: Invalid base expression in indexing.");
      return SymbolType::Undefined;
    }
  }

  string op = unary_Expr->optr;
  if (op != "#")
  {
    is_array(unary_Expr->operand);
  }

  SymbolType result = TypeChecker::is_valid_Unary(type, op, dim);

  if (result == SymbolType::Undefined)
  {
    if (op == "-")
    {
      semantic_error("Unary_expression (-)", result, "Semantic error: Variable must be Integer or Float in Unary (-).");
      return SymbolType::Undefined;
    }
    else if (op == "not")
    {
      semantic_error("Unary_expression (not)", result, "Semantic error: Variable must be Boolean in Unary (not).");
      return SymbolType::Undefined;
    }
    else if (op == "++" || op == "--")
    {
      semantic_error("Unary_expression (++,--)", result, "Semantic error: Variable must be Integer or Float in Unary (++ , --).");
      return SymbolType::Undefined;
    }
    else if (op == "@")
    {
      semantic_error("Unary_expression (@)", result, "Semantic error: Variable must be Integer or Float or boolean in Unary (@).");
      return SymbolType::Undefined;
    }
    else if (op == "#")
    {
      semantic_error("Unary_expression (#)", result, "Semantic error: Variable must be String or array in Unary (#).");
      return SymbolType::Undefined;
    }
  }

  return result;
}

SymbolType SemanticAnalyzer::semantic_index_expr(Expression *expr, bool set_init, bool allow_partial_indexing)
{
  if (!dynamic_cast<IndexExpression *>(expr))
  {
    return semantic_primary_expr(expr);
  }

  IndexExpression *idxExpr = static_cast<IndexExpression *>(expr);

  int dim = 1;
  SymbolType type;
  Symbol *symbol;

  SymbolType ty;
  while (dynamic_cast<IndexExpression *>(idxExpr->base))
  {
    ty = semantic_expr(idxExpr->index);
    dim += 1;
    if (ty != SymbolType::Integer)
    {
      semantic_error("Index_expression", ty, "semantic error: invalid index.");
      return SymbolType::Undefined;
    }
    idxExpr = static_cast<IndexExpression *>(idxExpr->base);
  }
  ty = semantic_expr(idxExpr->index);
  if (ty != SymbolType::Integer)
  {
    semantic_error("Index_expression", ty, "semantic error: invalid index.");
    return SymbolType::Undefined;
  }

  if (dynamic_cast<Identifier *>(idxExpr->base))
  {
    Identifier *var = static_cast<Identifier *>(idxExpr->base);

    SymbolTable *scope = retrieve_scope(var->name);
    if (!scope)
    {
      semantic_error(var->name, SymbolType::Undefined, "Semantic error: Symbol '" + var->name + "' must be Declared.");
      return SymbolType::Undefined;
    }
    if (set_init)
    {
      scope->set_initialized(var->name);
    }

    symbol = scope->retrieve_symbol(var->name);
    if (!symbol)
    {
      semantic_error(var->name, SymbolType::Undefined, "Semantic error: Symbol '" + var->name + "' must be Declared.");
      return SymbolType::Undefined;
    }
  }
  else
  {
    CallFunctionExpression *cfExpr = static_cast<CallFunctionExpression *>(idxExpr->base);
    SymbolTable *st = retrieve_scope(cfExpr->function->name);
    symbol = retrieve_scope(cfExpr->function->name)->retrieve_symbol(cfExpr->function->name);
    if (symbol == nullptr)
    {
      semantic_error(cfExpr->function->name, SymbolType::Undefined, "Semantic error: Symbol '" + cfExpr->function->name + "' must be Declared.");
      return SymbolType::Undefined;
    }
  }

  type = semantic_expr(idxExpr->base);

  if (((dim != symbol->dim) && !allow_partial_indexing) || (dim > symbol->dim))
  {
    semantic_error(symbol->name, type, "Dimension mismatch for variable " + symbol->name + ": expected " + to_string(symbol->dim) + ", but got " + to_string(dim));
    return SymbolType::Undefined;
  }
  if (type != symbol->type)
  {
    semantic_error(symbol->name, type, "Datatype mismatch for variable " + symbol->name + ": expected " + Symbol::get_name_symboltype(type) + ", but got " + Symbol::get_name_symboltype(symbol->type));
    return SymbolType::Undefined;
  }

  return type;
}

SymbolType SemanticAnalyzer::semantic_primary_expr(Expression *primaryExpr)
{
  if (dynamic_cast<Identifier *>(primaryExpr))
  {
    Identifier *id = static_cast<Identifier *>(primaryExpr);
    return semantic_id(id);
  }
  return semantic_literal(primaryExpr);
}

SymbolType SemanticAnalyzer::semantic_literal(Expression *lit)
{
  if (!dynamic_cast<Literal *>(lit))
  {
    return semantic_call_function_expr(lit);
  }

  if (dynamic_cast<IntegerLiteral *>(lit))
  {
    return SymbolType::Integer;
  }
  else if (dynamic_cast<FloatLiteral *>(lit))
  {
    return SymbolType::Float;
  }
  else if (dynamic_cast<StringLiteral *>(lit))
  {
    return SymbolType::String;
  }
  else if (dynamic_cast<BooleanLiteral *>(lit))
  {
    return SymbolType::Boolean;
  }
  else if (dynamic_cast<ArrayLiteral *>(lit))
  {
    return semantic_array(static_cast<ArrayLiteral *>(lit)).first;
  }
  return SymbolType::Undefined;
}

SymbolType SemanticAnalyzer::semantic_call_function_expr(Expression *cfExpr)
{
  if (!dynamic_cast<CallFunctionExpression *>(cfExpr))
  {
    return SymbolType::Undefined;
  }

  CallFunctionExpression *cf_Expr = static_cast<CallFunctionExpression *>(cfExpr);

  stack<SymbolTable *> temp = call_stack;
  SymbolTable *global = nullptr;
  Identifier *func_name = cf_Expr->function;
  vector<Expression *> arguments = cf_Expr->arguments;
  while (!temp.empty())
  {
    global = temp.top();
    temp.pop();
  }

  if (!global || !global->is_exist(func_name->name))
  {
    semantic_error(func_name->name, SymbolType::Undefined, "Semantic error: Function '" + func_name->name + "' Not Declared.");
    return SymbolType::Undefined;
  }

  FunctionSymbol *fn = global->retrieve_function(func_name->name);
  if (!fn)
  {
    semantic_error(func_name->name, SymbolType::Undefined, "Semantic error: Invalid function object.");
    return SymbolType::Undefined;
  }
  auto all_args = fn->parameters;
  auto required_args = global->get_required_arguments(func_name->name);

  if (arguments.size() < required_args.size() || arguments.size() > all_args.size())
  {
    semantic_error(func_name->name, SymbolType::Undefined, "Semantic error: Function '" + func_name->name + "' expects between " + to_string(required_args.size()) + " and " + to_string(all_args.size()) + " arguments, but got " + to_string(arguments.size()) + ".");
    return SymbolType::Undefined;
  }

  for (int i = 0; i < arguments.size(); ++i)
  {
    SymbolType type = semantic_expr(arguments[i]);

    // 1. Get dimension of current argument
    int arg_dim = 0;
    if (dynamic_cast<ArrayLiteral *>(arguments[i]))
    {
      arg_dim = semantic_array(static_cast<ArrayLiteral *>(arguments[i])).second;
    }
    else if (dynamic_cast<Identifier *>(arguments[i]))
    {
      Identifier *id = static_cast<Identifier *>(arguments[i]);
      SymbolTable *scope = retrieve_scope(id->name);
      if (scope)
      {
        VariableSymbol *vs = scope->retrieve_variable(id->name);
        if (vs)
          arg_dim = vs->dim;
      }
    }

    // 2. Get expected type + dimension from function signature
    SymbolType expected_type = all_args[i].first;
    int expected_dim = all_args[i].second;

    // 3. Type check
    if (type != expected_type)
    {
      semantic_error(func_name->name, SymbolType::Undefined,
                     "Semantic error: Argument " + to_string(i + 1) + " in function '" + func_name->name +
                         "' should be of type " + Symbol::get_name_symboltype(expected_type) +
                         ", but got " + Symbol::get_name_symboltype(type) + ".");
      return SymbolType::Undefined;
    }

    // 4. Dimension check
    if (arg_dim != expected_dim)
    {
      semantic_error(func_name->name, SymbolType::Undefined,
                     "Semantic error: Dimension mismatch in argument " + to_string(i + 1) +
                         " in function '" + func_name->name +
                         "': expected dim " + to_string(expected_dim) +
                         ", but got " + to_string(arg_dim) + ".");
      return SymbolType::Undefined;
    }
  }

  return fn->type;
}

SymbolType SemanticAnalyzer::semantic_id(Identifier *id, bool set_init)
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
  semantic_error(var, SymbolType::Undefined, "Semantic error: Variable '" + var + "' Not Declared.");
  return SymbolType::Undefined;
}

pair<SymbolType, int> SemanticAnalyzer::semantic_array(ArrayLiteral *arrNode)
{
  vector<Expression *> elements = arrNode->elements;
  pair<SymbolType, int> value_list = pair<SymbolType, int>(SymbolType::Undefined, 1);

  if (elements.size() == 0)
  {
    return value_list;
  }

  if (elements.size() > 0 && !dynamic_cast<ArrayLiteral *>(elements[0]))
  {
    value_list = semantic_array_value(elements);
    return value_list;
  }

  for (auto el : elements)
  {
    ArrayLiteral *nested = static_cast<ArrayLiteral *>(el);
    int dim = value_list.second;
    value_list = semantic_array(nested);

    if (value_list.second != dim && el != elements.front())
    {
      semantic_error("Array_literal", value_list.first, "semantic error: Inconsistent array dimension.");
      return pair<SymbolType, int>(SymbolType::Undefined, 0);
    }
  }

  value_list.second++;

  return value_list;
}

pair<SymbolType, int> SemanticAnalyzer::semantic_array_value(vector<Expression *> elements)
{
  SymbolType dt = SymbolType::Undefined;
  SymbolType element_dt = SymbolType::Undefined;
  int max_inner_dim = 0;

  for (auto el : elements)
  {
    int el_dim = 0;

    element_dt = semantic_expr(el);

    if (dynamic_cast<Identifier *>(el))
    {
      Identifier *id = static_cast<Identifier *>(el);
      SymbolTable *scope = retrieve_scope(id->name);
      if (scope)
      {
        VariableSymbol *var = scope->retrieve_variable(id->name);
        if (var)
          el_dim = var->dim;
      }
    }
    else if (dynamic_cast<ArrayLiteral *>(el))
    {
      el_dim = semantic_array(static_cast<ArrayLiteral *>(el)).second;
    }

    if (el_dim > max_inner_dim)
    {
      max_inner_dim = el_dim;
    }

    if (dt == SymbolType::Undefined)
    {
      dt = element_dt;
    }
    else if (element_dt != dt)
    {
      semantic_error("Array_literal", element_dt, "semantic error: array contain value of multiple datatypes");
      return pair<SymbolType, int>(SymbolType::Undefined, 0);
    }
  }
  return pair<SymbolType, int>(dt, max_inner_dim + 1);
}

VariableSymbol *SemanticAnalyzer::is_initialized_var(Identifier *id)
{
  SymbolTable *st = retrieve_scope(id->name);
  VariableSymbol *var = st->retrieve_variable(id->name);
  if (var == nullptr)
  {
    semantic_error(id->name, SymbolType::Undefined, "Semantic error: Variable '" + id->name + "' must be Declared.");
    return nullptr;
  }

  if (!var->is_initialized)
  {
    semantic_error(id->name, SymbolType::Undefined, "Semantic error: Variable '" + id->name + "' must be Initialized.");
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

  semantic_error(sn, SymbolType::Undefined, "Semantic error: Variable '" + sn + "' must be Declared.");
  return nullptr;
}

void SemanticAnalyzer::is_array(Expression *Expr)
{
  if (dynamic_cast<Identifier *>(Expr))
  {
    Identifier *id = static_cast<Identifier *>(Expr);

    SymbolTable *scope = retrieve_scope(id->name);
    if (scope == nullptr)
    {
      semantic_error(id->name, SymbolType::Undefined, "Semantic error: Variable '" + id->name + "' must be Declared.");
      return;
    }
    VariableSymbol *vr = scope->retrieve_variable(id->name);
    if (vr == nullptr)
    {
      semantic_error(id->name, SymbolType::Undefined, "Semantic error: Variable '" + id->name + "' must be Declared.");
      return;
    }
    int dim = vr->dim;
    if (dim > 0)
    {
      semantic_error(id->name, SymbolType::Undefined, "Semantic error: Invalid Expression can't use array.");
      return;
    }
  }
  else if (dynamic_cast<CallFunctionExpression *>(Expr))
  {
    CallFunctionExpression *cf = static_cast<CallFunctionExpression *>(Expr);
    SymbolTable *scope = retrieve_scope(cf->function->name);
    if (!scope)
    {
      semantic_error(cf->function->name, SymbolType::Undefined, "Semantic error: Function '" + cf->function->name + "' must be Declared.");
      return;
    }
    FunctionSymbol *fn = scope->retrieve_function(cf->function->name);
    if (!fn)
    {
      semantic_error(cf->function->name, SymbolType::Undefined, "Semantic error: Function '" + cf->function->name + "' must be Declared.");
      return;
    }
    int dim = fn->dim;
    if (dim > 0)
    {
      semantic_error(cf->function->name, SymbolType::Undefined, "Semantic error: Invalid Expression can't use array.");
      return;
    }
  }
}

ErrorSymbol SemanticAnalyzer::semantic_error(string name, SymbolType st, string err)
{
  if (!has_error)
  {
    has_error = true;
    error_symbol = ErrorSymbol(name, st, err);
  }

  return error_symbol;
}