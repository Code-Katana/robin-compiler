#include "semantic_analyzer.h"

#include <typeinfo>

SemanticAnalyzer::SemanticAnalyzer(ParserBase *pr) : parser(pr) {};

void SemanticAnalyzer::analyze()
{
  Source *source = (Source *)parser->parse_ast();
  semantic_source(source);
}

void SemanticAnalyzer::semantic_source(Source *source)
{
  SymbolTable *st = new SymbolTable();
  string program_name = source->program->program_name->name;
  st->insert(new Symbol(
      program_name,
      SymbolType::Program,
      SymbolKind::Function));

  for (auto var : source->program->globals)
  {
    semantic_var_def(var, st);
  }

  for (auto func : source->functions)
  {
    string name = func->funcname->name;
    DataType *dt = func->return_type->return_type;
    vector<pair<SymbolType, int>> parameters = SymbolTable::get_parameters_type(func->parameters);

    st->insert(new FunctionSymbol(
        name,
        Symbol::get_datatype(dt),
        parameters,
        Symbol::get_dimension(dt)));
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
        st->insert(new VariableSymbol(
            id->name,
            Symbol::get_datatype(def->datatype),
            true,
            Symbol::get_dimension(def->datatype)));
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
    SymbolTable::semantic_error("Semantic error : missing a return statement in the function body in '" + func->funcname->name + "'.");
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
    // counter++;
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
    semantic_all_expr(expr);
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

  semantic_bool_expr(ifStmt->condition);
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
  SymbolTable *global = new SymbolTable();
  SymbolType returnType = SymbolType::Undefined;
  while (!temp.empty())
  {
    global = temp.top();
    temp.pop();
  }

  if (global->get_type(name_parent) == SymbolType::Void)
  {
    if (semantic_expr(rtnStmt->returnValue) != SymbolType::Undefined)
    {
      SymbolTable::semantic_error("Semantic error: 'return' in function block '" + name_parent + "' doesn't match the function type void.");
    }
  }
  else if (global->get_type(name_parent) == SymbolType::Program)
  {
    if (semantic_expr(rtnStmt->returnValue) != SymbolType::Undefined)
    {
      SymbolTable::semantic_error("Semantic error: 'return' in the program block '" + name_parent + "' must not have an expression.");
    }
  }

  else if (global->get_type(name_parent) == semantic_expr(rtnStmt->returnValue))
  {
    if (dynamic_cast<Identifier *>(rtnStmt->returnValue))
    {
      Identifier *var = static_cast<Identifier *>(rtnStmt->returnValue);
      stack<SymbolTable *> temp = call_stack;
      while (!temp.empty())
      {
        SymbolTable *st = temp.top();
        if (st->is_exist(var->name))
        {
          if (!st->is_initialized(var->name))
          {
            SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
          }
        }
        temp.pop();
      }
    }
  }
  else
  {
    SymbolTable::semantic_error("Semantic error: 'return' in function block '" + name_parent + "' doesn't match the function type.");
  }
}

void SemanticAnalyzer::semantic_read(ReadStatement *readStmt)
{
  stack<SymbolTable *> temp = call_stack;
  vector<Identifier *> vars = readStmt->variables;
  while (!temp.empty())
  {
    SymbolTable *t = temp.top();
    for (int i = vars.size() - 1; i >= 0; --i)
    {
      if (t->is_exist(vars[i]->name))
      {
        vars.erase(vars.begin() + i);
      }
    }
    temp.pop();
  }
  if (!vars.empty())
  {
    SymbolTable::semantic_error("Semantic error: Symbol '" + vars[0]->name + "' not Declared.");
  }
}

void SemanticAnalyzer::semantic_write(WriteStatement *writeStmt)
{
  for (auto expr : writeStmt->args)
  {
    if (dynamic_cast<Identifier *>(expr))
    {
      Identifier *var = static_cast<Identifier *>(expr);
      stack<SymbolTable *> temp = call_stack;
      while (!temp.empty())
      {
        SymbolTable *st = temp.top();
        if (st->is_exist(var->name))
        {
          if (!st->is_initialized(var->name))
          {
            SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
          }
        }
        temp.pop();
      }
    }
    semantic_expr(expr);
  }
}

void SemanticAnalyzer::semantic_while(WhileLoop *loop, string name_parent)
{
  SymbolTable *st = new SymbolTable();
  call_stack.push(st);

  semantic_bool_expr(loop->condition);
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
  semantic_bool_expr(loop->condition);
  semantic_all_expr(loop->update);
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
        SymbolTable::semantic_error("Semantic error: Symbol '" + var->name + "' Declared.");
      }
      temp.pop();
    }
    call_stack.top()->insert(new VariableSymbol(
        var->name,
        SymbolType::Integer,
        true,
        0));

    SymbolType type = semantic_expr(init->value);
    if (type != SymbolType::Integer)
    {
      SymbolTable::semantic_error("Semantic error: Value of '" + var->name + "' Must be integer.");
    }
  }
  else
  {
    SymbolTable::semantic_error("Semantic error: in initialization part of forLoop must be identifier .");
  }
}

void SemanticAnalyzer::semantic_var_def(VariableDefinition *var, SymbolTable *st)
{
  if (dynamic_cast<VariableDeclaration *>(var->def))
  {
    VariableDeclaration *def = (VariableDeclaration *)var->def;

    for (auto id : def->variables)
    {
      st->insert(new VariableSymbol(
          id->name,
          Symbol::get_datatype(def->datatype),
          false,
          Symbol::get_dimension(def->datatype)));
    }
  }
  else if (dynamic_cast<VariableInitialization *>(var->def))
  {
    VariableInitialization *def = (VariableInitialization *)var->def;

    if (dynamic_cast<ArrayLiteral *>(def->initializer))
    {
      pair<SymbolType, int> array_value = semantic_array((ArrayLiteral *)def->initializer);

      ArrayType *type = (ArrayType *)def->datatype;
      if (Symbol::get_datatype(type) != array_value.first && array_value.first != SymbolType::Undefined)
      {
        SymbolTable::semantic_error("Semantic error: invalid initialization.");
      }

      if (array_value.second != type->dimension)
      {
        SymbolTable::semantic_error("Semantic error: invalid array dimension.");
      }
    }

    // else if (dynamic_cast<Identifier *>(def->initializer)) {
    //   static<Identifier *>(def->initializer)

    //   if (dynamic_cast<ArrayType *>(def->datatype)) {
    //     ArrayType *type = (ArrayType *)def->datatype;
    //   if (Symbol::get_datatype(type) != array_value.first && array_value.first != SymbolType::Undefined)
    //   {
    //     SymbolTable::semantic_error("Semantic error: invalid initialization.");
    //   }

    //   if (array_value.second != type->dimension)
    //   {
    //     SymbolTable::semantic_error("Semantic error: invalid array dimension.");
    //   }
    //   }
    // }

    else
    {
      PrimitiveType *type = (PrimitiveType *)def->datatype;
      SymbolType dt = semantic_expr(def->initializer);

      if (Symbol::get_datatype(type) != dt)
      {
        SymbolTable::semantic_error("Semantic error: invalid initialization.");
      }
    }

    st->insert(new VariableSymbol(
        def->name->name,
        Symbol::get_datatype(def->datatype),
        true,
        Symbol::get_dimension(def->datatype)));
  }
}

void SemanticAnalyzer::semantic_all_expr(Expression *expr)
{
  if (dynamic_cast<AssignmentExpression *>(expr))
  {
    AssignmentExpression *assExpr = static_cast<AssignmentExpression *>(expr);
    semantic_assign_expr(assExpr);
  }
  else
  {
    semantic_expr(expr);
  }
}

SymbolType SemanticAnalyzer::semantic_expr(Expression *expr)
{
  if (dynamic_cast<Literal *>(expr))
  {
    Literal *lit = static_cast<Literal *>(expr);
    return semantic_literal(lit);
  }
  else if (dynamic_cast<Identifier *>(expr))
  {
    Identifier *id = static_cast<Identifier *>(expr);
    return semantic_id(id);
  }
  else if (dynamic_cast<AdditiveExpression *>(expr))
  {
    AdditiveExpression *addExpr = static_cast<AdditiveExpression *>(expr);
    return semantic_additive_expr(addExpr);
  }
  else if (dynamic_cast<MultiplicativeExpression *>(expr))
  {
    MultiplicativeExpression *mulExpr = static_cast<MultiplicativeExpression *>(expr);
    return semantic_multiplicative_expr(mulExpr);
  }
  else if (dynamic_cast<UnaryExpression *>(expr))
  {
    UnaryExpression *unaryExpr = static_cast<UnaryExpression *>(expr);
    return semantic_unary_expr(unaryExpr);
  }
  else if (dynamic_cast<CallFunctionExpression *>(expr))
  {
    CallFunctionExpression *cfExpr = static_cast<CallFunctionExpression *>(expr);
    return semantic_call_function_expr(cfExpr);
  }
  else if (dynamic_cast<PrimaryExpression *>(expr))
  {
    PrimaryExpression *primaryExpr = static_cast<PrimaryExpression *>(expr);
    return semantic_primary_expr(primaryExpr);
  }
  else if (dynamic_cast<AssignmentExpression *>(expr))
  {
    SymbolTable::semantic_error("Semantic error: Chained assignment is not allowed .");
  }
  return semantic_bool_expr(expr);
}

SymbolType SemanticAnalyzer::semantic_bool_expr(Expression *expr)
{
  if (dynamic_cast<OrExpression *>(expr))
  {
    OrExpression *orExpr = static_cast<OrExpression *>(expr);
    return semantic_or_expr(orExpr);
  }
  else if (dynamic_cast<AndExpression *>(expr))
  {
    AndExpression *andExpr = static_cast<AndExpression *>(expr);
    return semantic_and_expr(andExpr);
  }
  else if (dynamic_cast<EqualityExpression *>(expr))
  {
    EqualityExpression *eqExpr = static_cast<EqualityExpression *>(expr);
    return semantic_equality_expr(eqExpr);
  }
  else if (dynamic_cast<RelationalExpression *>(expr))
  {
    RelationalExpression *relExpr = static_cast<RelationalExpression *>(expr);
    return semantic_relational_expr(relExpr);
  }
  return SymbolType::Undefined;
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
SymbolType SemanticAnalyzer::semantic_literal(Literal *lit)
{
  if (dynamic_cast<IntegerLiteral *>(lit))
  {
    IntegerLiteral *intNode = static_cast<IntegerLiteral *>(lit);
    return SymbolType::Integer;
  }
  else if (dynamic_cast<FloatLiteral *>(lit))
  {
    FloatLiteral *fNode = static_cast<FloatLiteral *>(lit);
    return SymbolType::Float;
  }
  else if (dynamic_cast<StringLiteral *>(lit))
  {
    StringLiteral *strNode = static_cast<StringLiteral *>(lit);
    return SymbolType::String;
  }
  else if (dynamic_cast<BooleanLiteral *>(lit))
  {
    BooleanLiteral *boolNode = static_cast<BooleanLiteral *>(lit);
    return SymbolType::Boolean;
  }
  return SymbolType::Undefined;
}

void SemanticAnalyzer::semantic_assign_expr(AssignmentExpression *assignExpr)
{
  stack<SymbolTable *> temp = call_stack;
  Expression *var;
  if (dynamic_cast<Identifier *>(assignExpr->assignee))
  {
    bool isExist = false;
    var = static_cast<Identifier *>(assignExpr->assignee);
    while (!temp.empty())
    {
      SymbolTable *t = temp.top();
      if (t->is_exist(static_cast<Identifier *>(var)->name))
      {
        isExist = true;
        if (dynamic_cast<Identifier *>(assignExpr->value))
        {
          Identifier *var = static_cast<Identifier *>(assignExpr->value);
          stack<SymbolTable *> temp2 = call_stack;
          while (!temp2.empty())
          {
            SymbolTable *st = temp2.top();
            if (st->is_exist(var->name))
            {
              if (!st->is_initialized(var->name))
              {
                SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
              }
            }
            temp2.pop();
          }
        }
        SymbolType type = semantic_expr(assignExpr->value);
        if (type != t->get_type(static_cast<Identifier *>(var)->name))
        {
          SymbolTable::semantic_error("Semantic error: Symbol '" + static_cast<Identifier *>(var)->name + "' must be same datatype as " + Symbol::get_name_symboltype(type) + ". ");
        }
        t->set_initialized(static_cast<Identifier *>(var)->name);
      }
      temp.pop();
    }
    if (!isExist)
    {
      SymbolTable::semantic_error("Semantic error: Symbol '" + static_cast<Identifier *>(var)->name + "' must be Declared. ");
    }
  }
  else if (dynamic_cast<IndexExpression *>(assignExpr->assignee))
  {
    bool isExist = false;
    var = static_cast<IndexExpression *>(assignExpr->assignee);
    IndexExpression *indexExpr = static_cast<IndexExpression *>(var);
    if (semantic_expr(indexExpr->index) != SymbolType::Integer)
    {
      SymbolTable::semantic_error("Semantic error: Symbol '" + static_cast<Identifier *>(indexExpr->base)->name + "' it's index must be integer. ");
    }
    while (!temp.empty())
    {
      SymbolTable *t = temp.top();
      if (t->is_exist(static_cast<Identifier *>(indexExpr->base)->name))
      {
        isExist = true;
        if (dynamic_cast<Identifier *>(assignExpr->value))
        {
          Identifier *var = static_cast<Identifier *>(assignExpr->value);
          stack<SymbolTable *> temp2 = call_stack;
          while (!temp2.empty())
          {
            SymbolTable *st = temp2.top();
            if (st->is_exist(var->name))
            {
              if (!st->is_initialized(var->name))
              {
                SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
              }
            }
            temp2.pop();
          }
        }
        SymbolType type = semantic_expr(assignExpr->value);
        if (type != t->get_type(static_cast<Identifier *>(indexExpr->base)->name))
        {
          SymbolTable::semantic_error("Semantic error: Symbol '" + static_cast<Identifier *>(indexExpr->base)->name + "' must be same datatype as " + Symbol::get_name_symboltype(type) + ". ");
        }
        t->set_initialized(static_cast<Identifier *>(indexExpr->base)->name);
      }
      temp.pop();
    }
    if (!isExist)
    {
      SymbolTable::semantic_error("Semantic error: Symbol '" + static_cast<Identifier *>(indexExpr->base)->name + "' must be Declared. ");
    }
  }
  else
  {
    SymbolTable::semantic_error("Semantic error: left hand must be assignable.");
  }
}

SymbolType SemanticAnalyzer::semantic_or_expr(OrExpression *orExpr)
{
  if (dynamic_cast<Identifier *>(orExpr->left))
  {
    Identifier *var = static_cast<Identifier *>(orExpr->left);
    stack<SymbolTable *> temp = call_stack;
    while (!temp.empty())
    {
      SymbolTable *st = temp.top();
      if (st->is_exist(var->name))
      {
        if (!st->is_initialized(var->name))
        {
          SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
        }
      }
      temp.pop();
    }
  }
  if (dynamic_cast<Identifier *>(orExpr->right))
  {
    Identifier *var = static_cast<Identifier *>(orExpr->right);
    stack<SymbolTable *> temp = call_stack;
    while (!temp.empty())
    {
      SymbolTable *st = temp.top();
      if (st->is_exist(var->name))
      {
        if (!st->is_initialized(var->name))
        {
          SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
        }
      }
      temp.pop();
    }
  }

  if (semantic_expr(orExpr->left) != SymbolType::Boolean)
  {
    SymbolTable::semantic_error("Semantic error: left hand must be Boolean in or.");
  }
  if (semantic_expr(orExpr->right) != SymbolType::Boolean)
  {
    SymbolTable::semantic_error("Semantic error: right hand must be Boolean in or.");
  }

  return SymbolType::Boolean;
}

SymbolType SemanticAnalyzer::semantic_and_expr(AndExpression *andExpr)
{
  if (dynamic_cast<Identifier *>(andExpr->left))
  {
    Identifier *var = static_cast<Identifier *>(andExpr->left);
    stack<SymbolTable *> temp = call_stack;
    while (!temp.empty())
    {
      SymbolTable *st = temp.top();
      if (st->is_exist(var->name))
      {
        if (!st->is_initialized(var->name))
        {
          SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
        }
      }
      temp.pop();
    }
  }
  if (dynamic_cast<Identifier *>(andExpr->right))
  {
    Identifier *var = static_cast<Identifier *>(andExpr->right);
    stack<SymbolTable *> temp = call_stack;
    while (!temp.empty())
    {
      SymbolTable *st = temp.top();
      if (st->is_exist(var->name))
      {
        if (!st->is_initialized(var->name))
        {
          SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
        }
      }
      temp.pop();
    }
  }

  if (semantic_expr(andExpr->left) != SymbolType::Boolean)
  {
    SymbolTable::semantic_error("Semantic error: left hand must be Boolean in and.");
  }
  if (semantic_expr(andExpr->right) != SymbolType::Boolean)
  {
    SymbolTable::semantic_error("Semantic error: right hand must be Boolean in and.");
  }

  return SymbolType::Boolean;
}

SymbolType SemanticAnalyzer::semantic_equality_expr(EqualityExpression *eqExpr)
{
  SymbolType left = semantic_expr(eqExpr->left);
  SymbolType right = semantic_expr(eqExpr->right);

  if (dynamic_cast<Identifier *>(eqExpr->left))
  {
    Identifier *var = static_cast<Identifier *>(eqExpr->left);
    stack<SymbolTable *> temp = call_stack;
    while (!temp.empty())
    {
      SymbolTable *st = temp.top();
      if (st->is_exist(var->name))
      {
        if (!st->is_initialized(var->name))
        {
          SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
        }
      }
      temp.pop();
    }
  }
  if (dynamic_cast<Identifier *>(eqExpr->right))
  {
    Identifier *var = static_cast<Identifier *>(eqExpr->right);
    stack<SymbolTable *> temp = call_stack;
    while (!temp.empty())
    {
      SymbolTable *st = temp.top();
      if (st->is_exist(var->name))
      {
        if (!st->is_initialized(var->name))
        {
          SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
        }
      }
      temp.pop();
    }
  }

  if ((left == SymbolType::Boolean && right != SymbolType::Boolean) ||
      (right == SymbolType::Boolean && left != SymbolType::Boolean))
  {
    SymbolTable::semantic_error("Semantic error: Both sides must be Boolean in Equality.");
  }
  else if ((left == SymbolType::String && right != SymbolType::String) ||
           (right == SymbolType::String && left != SymbolType::String))
  {
    SymbolTable::semantic_error("Semantic error: Both sides must be String in Equality.");
  }
  else if (((left == SymbolType::Integer || left == SymbolType::Float) &&
            !(right == SymbolType::Integer || right == SymbolType::Float)) ||
           ((right == SymbolType::Integer || right == SymbolType::Float) &&
            !(left == SymbolType::Integer || left == SymbolType::Float)))
  {
    SymbolTable::semantic_error("Semantic error: Both sides must be Integer or Float in Equality.");
  }

  return SymbolType::Boolean;
}

SymbolType SemanticAnalyzer::semantic_relational_expr(RelationalExpression *relExpr)
{
  SymbolType left = semantic_expr(relExpr->left);
  SymbolType right = semantic_expr(relExpr->right);

  if (dynamic_cast<Identifier *>(relExpr->left))
  {
    Identifier *var = static_cast<Identifier *>(relExpr->left);
    stack<SymbolTable *> temp = call_stack;
    while (!temp.empty())
    {
      SymbolTable *st = temp.top();
      if (st->is_exist(var->name))
      {
        if (!st->is_initialized(var->name))
        {
          SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
        }
      }
      temp.pop();
    }
  }
  if (dynamic_cast<Identifier *>(relExpr->right))
  {
    Identifier *var = static_cast<Identifier *>(relExpr->right);
    stack<SymbolTable *> temp = call_stack;
    while (!temp.empty())
    {
      SymbolTable *st = temp.top();
      if (st->is_exist(var->name))
      {
        if (!st->is_initialized(var->name))
        {
          SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
        }
      }
      temp.pop();
    }
  }

  if ((left == SymbolType::Boolean && right != SymbolType::Boolean) ||
      (right == SymbolType::Boolean && left != SymbolType::Boolean))
  {
    SymbolTable::semantic_error("Semantic error: Both sides must be Boolean in Relational.");
  }
  else if ((left == SymbolType::String && right != SymbolType::String) ||
           (right == SymbolType::String && left != SymbolType::String))
  {
    SymbolTable::semantic_error("Semantic error: Both sides must be String in Relational.");
  }
  else if (((left == SymbolType::Integer || left == SymbolType::Float) &&
            !(right == SymbolType::Integer || right == SymbolType::Float)) ||
           ((right == SymbolType::Integer || right == SymbolType::Float) &&
            !(left == SymbolType::Integer || left == SymbolType::Float)))
  {
    SymbolTable::semantic_error("Semantic error: Both sides must be Integer or Float in Relational.");
  }

  return SymbolType::Boolean;
}

SymbolType SemanticAnalyzer::semantic_additive_expr(AdditiveExpression *addExpr)
{
  SymbolType left = semantic_expr(addExpr->left);
  SymbolType right = semantic_expr(addExpr->right);
  if (dynamic_cast<Identifier *>(addExpr->left))
  {
    Identifier *var = static_cast<Identifier *>(addExpr->left);
    stack<SymbolTable *> temp = call_stack;
    while (!temp.empty())
    {
      SymbolTable *st = temp.top();
      if (st->is_exist(var->name))
      {
        if (!st->is_initialized(var->name))
        {
          SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
        }
      }
      temp.pop();
    }
  }
  if (dynamic_cast<Identifier *>(addExpr->right))
  {
    Identifier *var = static_cast<Identifier *>(addExpr->right);
    stack<SymbolTable *> temp = call_stack;
    while (!temp.empty())
    {
      SymbolTable *st = temp.top();
      if (st->is_exist(var->name))
      {
        if (!st->is_initialized(var->name))
        {
          SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
        }
      }
      temp.pop();
    }
  }
  if ((left == SymbolType::Boolean && right != SymbolType::Boolean) ||
      (right == SymbolType::Boolean && left != SymbolType::Boolean))
  {
    SymbolTable::semantic_error("Semantic error: Both sides must be Boolean in Additive.");
  }
  else if ((left == SymbolType::String && right != SymbolType::String) ||
           (right == SymbolType::String && left != SymbolType::String))
  {
    SymbolTable::semantic_error("Semantic error: Both sides must be String in Additive.");
  }
  else if (((left == SymbolType::Integer || left == SymbolType::Float) &&
            !(right == SymbolType::Integer || right == SymbolType::Float)) ||
           ((right == SymbolType::Integer || right == SymbolType::Float) &&
            !(left == SymbolType::Integer || left == SymbolType::Float)))
  {
    SymbolTable::semantic_error("Semantic error: Both sides must be Integer or Float in Additive.");
  }
  if (left == right)
  {
    return left;
  }

  return SymbolType::Float;
}

SymbolType SemanticAnalyzer::semantic_multiplicative_expr(MultiplicativeExpression *mulExpr)
{
  SymbolType left = semantic_expr(mulExpr->left);
  SymbolType right = semantic_expr(mulExpr->right);
  string op = mulExpr->optr;
  if (dynamic_cast<Identifier *>(mulExpr->left))
  {
    Identifier *var = static_cast<Identifier *>(mulExpr->left);
    stack<SymbolTable *> temp = call_stack;
    while (!temp.empty())
    {
      SymbolTable *st = temp.top();
      if (st->is_exist(var->name))
      {
        if (!st->is_initialized(var->name))
        {
          SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
        }
      }
      temp.pop();
    }
  }
  if (dynamic_cast<Identifier *>(mulExpr->right))
  {
    Identifier *var = static_cast<Identifier *>(mulExpr->right);
    stack<SymbolTable *> temp = call_stack;
    while (!temp.empty())
    {
      SymbolTable *st = temp.top();
      if (st->is_exist(var->name))
      {
        if (!st->is_initialized(var->name))
        {
          SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
        }
      }
      temp.pop();
    }
  }
  if (op == "%")
  {
    if ((left == SymbolType::Integer && right != SymbolType::Integer) ||
        (right == SymbolType::Integer && left != SymbolType::Integer) ||
        (right != SymbolType::Integer && left != SymbolType::Integer))
    {
      SymbolTable::semantic_error("Semantic error: Both sides must be Integer in Multiplicative (%).");
    }
    else
    {
      return left;
    }
  }
  if ((left == SymbolType::Boolean && right != SymbolType::Boolean) ||
      (right == SymbolType::Boolean && left != SymbolType::Boolean))
  {
    SymbolTable::semantic_error("Semantic error: Both sides must be Boolean in Multiplicative.");
  }
  else if ((left == SymbolType::String && right != SymbolType::String) ||
           (right == SymbolType::String && left != SymbolType::String))
  {
    SymbolTable::semantic_error("Semantic error: Both sides must be String in Multiplicative.");
  }
  else if (((left == SymbolType::Integer || left == SymbolType::Float) &&
            !(right == SymbolType::Integer || right == SymbolType::Float)) ||
           ((right == SymbolType::Integer || right == SymbolType::Float) &&
            !(left == SymbolType::Integer || left == SymbolType::Float)))
  {
    SymbolTable::semantic_error("Semantic error: Both sides must be Integer or Float in Multiplicative.");
  }
  if (left == right)
  {
    return left;
  }

  return SymbolType::Float;
}

SymbolType SemanticAnalyzer::semantic_unary_expr(UnaryExpression *unaryExpr)
{
  SymbolType type = semantic_expr(unaryExpr->operand);
  string op = unaryExpr->optr;
  if (dynamic_cast<Identifier *>(unaryExpr->operand))
  {
    Identifier *var = static_cast<Identifier *>(unaryExpr->operand);
    stack<SymbolTable *> temp = call_stack;
    while (!temp.empty())
    {
      SymbolTable *st = temp.top();
      if (st->is_exist(var->name))
      {
        if (!st->is_initialized(var->name))
        {
          SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
        }
      }
      temp.pop();
    }
  }
  if (op == "-")
  {
    if (type != SymbolType::Integer && type != SymbolType::Float)
    {
      SymbolTable::semantic_error("Semantic error: Variable must be Integer or Float in Unary (-).");
    }

    return type;
  }
  else if (op == "not")
  {
    if (type != SymbolType::Boolean)
    {
      SymbolTable::semantic_error("Semantic error: Variable must be Boolean in Unary (not).");
    }
    return type;
  }
  else if (op == "++" || op == "--")
  {
    if (type != SymbolType::Integer && type != SymbolType::Float)
    {
      SymbolTable::semantic_error("Semantic error: Variable must be Integer or Float in Unary (++ , --).");
    }
    return type;
  }
  return SymbolType::Undefined;
}
//! handle Array
SymbolType SemanticAnalyzer::semantic_call_function_expr(CallFunctionExpression *cfExpr)
{
  stack<SymbolTable *> temp = call_stack;
  Identifier *func_name = cfExpr->function;
  vector<Expression *> arguments = cfExpr->arguments;
  for (auto ar : arguments)
  {
    if (dynamic_cast<Identifier *>(ar))
    {
      Identifier *var = static_cast<Identifier *>(ar);
      stack<SymbolTable *> temp = call_stack;
      while (!temp.empty())
      {
        SymbolTable *st = temp.top();
        if (st->is_exist(var->name))
        {
          if (!st->is_initialized(var->name))
          {
            SymbolTable::semantic_error("Semantic error: Variable '" + var->name + "' must be Initialized.");
          }
        }
        temp.pop();
      }
    }
  }
  SymbolTable *global = new SymbolTable();
  while (!temp.empty())
  {
    global = temp.top();
    temp.pop();
  }
  if (!(global->is_exist(func_name->name)))
  {
    SymbolTable::semantic_error("Semantic error: Function '" + func_name->name + "' Not Declared.");
  }
  vector<pair<SymbolType, int>> args = global->get_arguments(func_name->name);
  if (arguments.size() != args.size())
  {
    SymbolTable::semantic_error("Semantic error: Function '" + func_name->name + "' Arguments doesn't match in size.");
  }
  for (int i = 0; i < args.size(); ++i)
  {
    SymbolType type = semantic_expr(arguments[i]);
    if (type != args[i].first)
    {
      SymbolTable::semantic_error("Semantic error: Function '" + func_name->name + "' Arguments doesn't match in datatype.");
    }
  }
  return global->get_type(func_name->name);
}

SymbolType SemanticAnalyzer::semantic_primary_expr(PrimaryExpression *primaryExpr)
{
  return semantic_literal(primaryExpr->value);
}

SymbolType SemanticAnalyzer::semantic_id(Identifier *id)
{
  stack<SymbolTable *> temp = call_stack;
  string var = id->name;
  SymbolTable *st = new SymbolTable();
  while (!temp.empty())
  {
    st = temp.top();
    if (st->is_exist(var))
    {
      return st->get_type(var);
    }
    temp.pop();
  }
  SymbolTable::semantic_error("Semantic error: Variable '" + var + "' Not Declared.");

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
      SymbolTable::semantic_error("semantic error: Inconsistent array dimension.");
    }
  }

  value_list.second++;

  return value_list;
}

pair<SymbolType, int> SemanticAnalyzer::semantic_array_value(vector<Expression *> elements)
{
  SymbolType dt = SymbolType::Undefined;

  for (auto el : elements)
  {
    if (dt == SymbolType::Undefined)
    {
      dt = semantic_expr(el);
    }
    else if (semantic_expr(el) != dt)
    {
      SymbolTable::semantic_error("semantic error: array contain value of multiple datatypes");
    }
  }

  return pair<SymbolType, int>(dt, 1);
}