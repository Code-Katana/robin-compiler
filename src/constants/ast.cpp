#include "ast.h"

// AstNode
AstNode::AstNode() {}

AstNode::AstNode(int sl, int el, int s, int e) : start_line(sl), end_line(el), node_start(s), node_end(e) {}

map<AstNodeType, string> AstNode::NodeNames = {
    // Root Node
    {AstNodeType::Source, "Source"},
    // Declarations
    {AstNodeType::Program, "ProgramDeclaration"},
    {AstNodeType::Function, "FunctionDeclaration"},
    {AstNodeType::VariableDefinition, "VariableDefinition"},
    {AstNodeType::VariableDeclaration, "VariableDeclaration"},
    {AstNodeType::VariableInitialization, "VariableInitialization"},
    // Types
    {AstNodeType::ReturnType, "ReturnType"},
    {AstNodeType::PrimitiveType, "PrimitiveType"},
    {AstNodeType::ArrayType, "ArrayType"},
    // Statements
    {AstNodeType::IfStatement, "IfStatement"},
    {AstNodeType::ReturnStatement, "ReturnStatement"},
    {AstNodeType::SkipStatement, "SkipStatement"},
    {AstNodeType::StopStatement, "StopStatement"},
    {AstNodeType::ReadStatement, "ReadStatement"},
    {AstNodeType::WriteStatement, "WriteStatement"},
    // Loops
    {AstNodeType::ForLoop, "ForLoop"},
    {AstNodeType::WhileLoop, "WhileLoop"},
    // Expressions
    {AstNodeType::AssignmentExpression, "AssignmentExpression"},
    {AstNodeType::OrExpression, "OrExpression"},
    {AstNodeType::AndExpression, "AndExpression"},
    {AstNodeType::EqualityExpression, "EqualityExpression"},
    {AstNodeType::RelationalExpression, "RelationalExpression"},
    {AstNodeType::AdditiveExpression, "AdditiveExpression"},
    {AstNodeType::MultiplicativeExpression, "MultiplicativeExpression"},
    {AstNodeType::UnaryExpression, "UnaryExpression"},
    {AstNodeType::CallFunctionExpression, "CallFunctionExpression"},
    {AstNodeType::IndexExpression, "IndexExpression"},
    {AstNodeType::PrimaryExpression, "PrimaryExpression"},
    // Literals
    {AstNodeType::Identifier, "Identifier"},
    {AstNodeType::IntegerLiteral, "IntegerLiteral"},
    {AstNodeType::FloatLiteral, "FloatLiteral"},
    {AstNodeType::StringLiteral, "StringLiteral"},
    {AstNodeType::BooleanLiteral, "BooleanLiteral"},
    {AstNodeType::ArrayLiteral, "ArrayLiteral"},
};

map<TokenType, string> AstNode::DataTypes = {
    {TokenType::INTEGER_TY, "integer"},
    {TokenType::BOOLEAN_TY, "boolean"},
    {TokenType::STRING_TY, "string"},
    {TokenType::FLOAT_TY, "float"},
};

bool AstNode::is_data_type(TokenType type)
{
  return DataTypes.find(type) != DataTypes.end();
}

bool AstNode::is_return_type(TokenType type)
{
  return is_data_type(type) || type == TokenType::VOID_TY;
}

string AstNode::get_node_name(const AstNode *node)
{
  if (NodeNames.find(node->type) == NodeNames.end())
  {
    return "";
  }

  return NodeNames[node->type];
}

// Basic Nodes Implementation

Statement::Statement() {}

Statement::Statement(int sl, int el, int s, int e) : AstNode(sl, el, s, e) {}

Expression::Expression() {}

Expression::Expression(int sl, int el, int s, int e) : Statement(sl, el, s, e) {}

BooleanExpression::BooleanExpression(int sl, int el, int s, int e) : Expression(sl, el, s, e) {}

AssignableExpression::AssignableExpression(int sl, int el, int s, int e) : Expression(sl, el, s, e) {}

DataType::DataType(int sl, int el, int s, int e) : AstNode(sl, el, s, e) {}

Literal::Literal(int sl, int el, int s, int e) : Expression(sl, el, s, e) {}

// Identifier Node Implementation
Identifier::Identifier(const string &name, int sl, int el, int s, int e) : AssignableExpression(sl, el, s, e), name(name)
{
  type = AstNodeType::Identifier;
}

// IntegerLiteral Node Implementation
IntegerLiteral::IntegerLiteral(int val, int sl, int el, int s, int e) : Literal(sl, el, s, e), value(val)
{
  type = AstNodeType::IntegerLiteral;
}

// FloatLiteral Node Implementation
FloatLiteral::FloatLiteral(float val, int sl, int el, int s, int e) : Literal(sl, el, s, e), value(val)
{
  type = AstNodeType::FloatLiteral;
}

// StringLiteral Node Implementation
StringLiteral::StringLiteral(const string &val, int sl, int el, int s, int e) : Literal(sl, el, s, e), value(val)
{
  type = AstNodeType::StringLiteral;
}

// BooleanLiteral Node Implementation
BooleanLiteral::BooleanLiteral(bool val, int sl, int el, int s, int e) : Literal(sl, el, s, e), value(val)
{
  type = AstNodeType::BooleanLiteral;
}

// ArrayLiteral Node Implementation
ArrayLiteral::ArrayLiteral(const vector<Expression *> &elems, int sl, int el, int s, int e) : Literal(sl, el, s, e), elements(elems)
{
  type = AstNodeType::ArrayLiteral;
}

ArrayLiteral::~ArrayLiteral()
{
  for (Expression *elem : elements)
  {
    delete elem;
  }
}

// Primitive Data Type Implementation
ReturnType::ReturnType(DataType *rt, int sl, int el, int s, int e) : return_type(rt), DataType(sl, el, s, e)
{
  type = AstNodeType::ReturnType;
}

ReturnType::~ReturnType()
{
  delete return_type;
}

PrimitiveType::PrimitiveType(const string &ty, int sl, int el, int s, int e) : DataType(sl, el, s, e), datatype(ty)
{
  type = AstNodeType::PrimitiveType;
}

// Array Data Type Implementation
ArrayType::ArrayType(const string &ty, int dim, int sl, int el, int s, int e) : DataType(sl, el, s, e), datatype(ty), dimension(dim)
{
  type = AstNodeType::ArrayType;
}

// AssignmentExpression Node Implementation
AssignmentExpression::AssignmentExpression(AssignableExpression *var, Expression *val, int sl, int el, int s, int e)
    : Expression(sl, el, s, e), assignee(var), value(val)
{
  type = AstNodeType::AssignmentExpression;
}

AssignmentExpression::~AssignmentExpression()
{
  delete assignee;
  delete value;
}

// OrExpression Node Implementation
OrExpression::OrExpression(Expression *lhs, Expression *rhs, int sl, int el, int s, int e)
    : Expression(sl, el, s, e), left(lhs), right(rhs)
{
  type = AstNodeType::OrExpression;
}

OrExpression::~OrExpression()
{
  delete left;
  delete right;
}

// AndExpression Node Implementation
AndExpression::AndExpression(Expression *lhs, Expression *rhs, int sl, int el, int s, int e)
    : Expression(sl, el, s, e), left(lhs), right(rhs)
{
  type = AstNodeType::AndExpression;
}

AndExpression::~AndExpression()
{
  delete left;
  delete right;
}

// EqualityExpression Node Implementation
EqualityExpression::EqualityExpression(Expression *lhs, Expression *rhs, const string &op, int sl, int el, int s, int e)
    : BooleanExpression(sl, el, s, e), left(lhs), right(rhs), optr(op)
{
  type = AstNodeType::EqualityExpression;
}

EqualityExpression::~EqualityExpression()
{
  delete left;
  delete right;
}

// RelationalExpression Node Implementation
RelationalExpression::RelationalExpression(Expression *lhs, Expression *rhs, const string &op, int sl, int el, int s, int e)
    : BooleanExpression(sl, el, s, e), left(lhs), right(rhs), optr(op)
{
  type = AstNodeType::RelationalExpression;
}

RelationalExpression::~RelationalExpression()
{
  delete left;
  delete right;
}

// AdditiveExpression Node Implementation
AdditiveExpression::AdditiveExpression(Expression *lhs, Expression *rhs, const string &op, int sl, int el, int s, int e)
    : Expression(sl, el, s, e), left(lhs), right(rhs), optr(op)
{
  type = AstNodeType::AdditiveExpression;
}

AdditiveExpression::~AdditiveExpression()
{
  delete left;
  delete right;
}

// MultiplicativeExpression Node Implementation
MultiplicativeExpression::MultiplicativeExpression(Expression *lhs, Expression *rhs, const string &op, int sl, int el, int s, int e)
    : Expression(sl, el, s, e), left(lhs), right(rhs), optr(op)
{
  type = AstNodeType::MultiplicativeExpression;
}

MultiplicativeExpression::~MultiplicativeExpression()
{
  delete left;
  delete right;
}

// UnaryExpression Node Implementation
UnaryExpression::UnaryExpression(Expression *operand, const string &op, const bool &post, int sl, int el, int s, int e)
    : Expression(sl, el, s, e), operand(operand), optr(op), postfix(post)
{
  type = AstNodeType::UnaryExpression;
}

UnaryExpression::~UnaryExpression()
{
  delete operand;
}

// CallFunctionExpression Node Implementation
CallFunctionExpression::CallFunctionExpression(Identifier *func, const vector<Expression *> &args, int sl, int el, int s, int e)
    : Expression(sl, el, s, e), function(func), arguments(args)
{
  type = AstNodeType::CallFunctionExpression;
}

CallFunctionExpression::~CallFunctionExpression()
{
  delete function;
  for (auto arg : arguments)
  {
    delete arg;
  }
}

// IndexExpression Node Implementation
IndexExpression::IndexExpression(Expression *baseExpr, Expression *indexExpr, int sl, int el, int s, int e)
    : AssignableExpression(sl, el, s, e), base(baseExpr), index(indexExpr)
{
  type = AstNodeType::IndexExpression;
}

IndexExpression::~IndexExpression()
{
  delete base;
  delete index;
}

// PrimaryExpression Node Implementation
PrimaryExpression::PrimaryExpression(Literal *val, int sl, int el, int s, int e)
    : Expression(sl, el, s, e), value(val)
{
  type = AstNodeType::PrimaryExpression;
}

PrimaryExpression::~PrimaryExpression()
{
  delete value;
}

// VariableDeclaration Node Implementation
VariableDeclaration::VariableDeclaration(const vector<Identifier *> &vars, DataType *dt, int sl, int el, int s, int e)
    : Statement(sl, el, s, e), variables(vars), datatype(dt)
{
  type = AstNodeType::VariableDeclaration;
}

VariableDeclaration::~VariableDeclaration()
{
  delete datatype;

  for (Identifier *var : variables)
  {
    delete var;
  }
}

// VariableInitialization Node Implementation
VariableInitialization::VariableInitialization(Identifier *name, DataType *datatype, Expression *init, int sl, int el, int s, int e)
    : Statement(sl, el, s, e), name(name), datatype(datatype), initializer(init)
{
  type = AstNodeType::VariableInitialization;
}

VariableInitialization::~VariableInitialization()
{
  delete name;
  delete datatype;
  delete initializer;
}

// VariableDefinition Node Implementation
VariableDefinition::VariableDefinition(Statement *def, int sl, int el, int s, int e) : Statement(sl, el, s, e), def(def)
{
  type = AstNodeType::VariableDefinition;
}

VariableDefinition::~VariableDefinition()
{
  delete def;
}

// FunctionDeclaration Node Implementation
Function::Function(Identifier *name, ReturnType *ret, const vector<VariableDefinition *> &params, const vector<Statement *> &body, int sl, int el, int s, int e)
    : AstNode(sl, el, s, e), funcname(name), return_type(ret), parameters(params), body(body)
{
  type = AstNodeType::Function;
}

Function::~Function()
{
  delete funcname;
  delete return_type;
  for (VariableDefinition *param : parameters)
  {
    delete param;
  }
}

// ProgramDeclaration Node Implementation
Program::Program(Identifier *prog, const vector<VariableDefinition *> &glob, const vector<Statement *> &body, int sl, int el, int s, int e)
    : AstNode(sl, el, s, e), program_name(prog), globals(glob), body(body)
{
  type = AstNodeType::Program;
}

Program::~Program()
{
  delete program_name;
  for (VariableDefinition *global : globals)
  {
    delete global;
  }
  for (Statement *stmt : body)
  {
    delete stmt;
  }
}

// IfStatement Node Implementation
IfStatement::IfStatement(Expression *cond, const vector<Statement *> &consequent, const vector<Statement *> &alternate, int sl, int el, int s, int e)
    : Statement(sl, el, s, e), condition(cond), consequent(consequent), alternate(alternate)
{
  type = AstNodeType::IfStatement;
}

IfStatement::~IfStatement()
{
  delete condition;
  for (Statement *stmt : consequent)
  {
    delete stmt;
  }
  for (Statement *stmt : alternate)
  {
    delete stmt;
  }
}

// ReturnStatement Node Implementation
ReturnStatement::ReturnStatement(Expression *value, int sl, int el, int s, int e) : Statement(sl, el, s, e), returnValue(value)
{
  type = AstNodeType::ReturnStatement;
}

ReturnStatement::~ReturnStatement()
{
  delete returnValue;
}

// StopStatement Node Implementation
StopStatement::StopStatement(int sl, int el, int s, int e) : Statement(sl, el, s, e)
{
  type = AstNodeType::StopStatement;
}

// SkipStatement Node Implementation
SkipStatement::SkipStatement(int sl, int el, int s, int e) : Statement(sl, el, s, e)
{
  type = AstNodeType::SkipStatement;
}

// ReadStatement Node Implementation
ReadStatement::ReadStatement(const vector<AssignableExpression *> &vars, int sl, int el, int s, int e) : Statement(sl, el, s, e), variables(vars)
{
  type = AstNodeType::ReadStatement;
}

ReadStatement::~ReadStatement()
{
  for (AssignableExpression *var : variables)
  {
    delete var;
  }
}

// WriteStatement Node Implementation
WriteStatement::WriteStatement(const vector<Expression *> &values, int sl, int el, int s, int e) : Statement(sl, el, s, e), args(values)
{
  type = AstNodeType::WriteStatement;
}

WriteStatement::~WriteStatement()
{
  for (Expression *arg : args)
  {
    delete arg;
  }
}

// WhileLoop Node Implementation
WhileLoop::WhileLoop(Expression *cond, const vector<Statement *> &stmts, int sl, int el, int s, int e)
    : Statement(sl, el, s, e), condition(cond), body(stmts)
{
  type = AstNodeType::WhileLoop;
}

WhileLoop::~WhileLoop()
{
  delete condition;
  for (Statement *stmt : body)
  {
    delete stmt;
  }
}

// ForLoop Node Implementation
ForLoop::ForLoop(AssignmentExpression *init, BooleanExpression *cond, Expression *iter, const vector<Statement *> &stmts, int sl, int el, int s, int e)
    : Statement(sl, el, s, e), init(init), condition(cond), update(iter), body(stmts)
{
  type = AstNodeType::ForLoop;
}

ForLoop::~ForLoop()
{
  delete init;
  delete condition;
  delete update;
  for (Statement *stmt : body)
  {
    delete stmt;
  }
}

// Root Node Implementation
Source::Source(Program *prog, const vector<Function *> &funcs, int sl, int el, int s, int e)
    : AstNode(sl, el, s, e), program(prog), functions(funcs)
{
  type = AstNodeType::Source;
}

Source::~Source()
{
  delete program;
  for (Function *func : functions)
  {
    delete func;
  }
}
