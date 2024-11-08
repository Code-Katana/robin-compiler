#pragma once

#include <string>
#include <vector>
#include <map>

using namespace std;

enum class AstNodeType
{
  // Root Node
  Source,
  // Declarations
  ProgramDeclaration,
  FunctionDeclaration,
  VariableDefinition,
  VariableDeclaration,
  VariableInitialization,
  // Statements
  IfStatement,
  ReturnStatement,
  SkipStatement,
  StopStatement,
  ReadStatement,
  WriteStatement,
  // Loops
  ForLoop,
  WhileLoop,
  // Expressions
  AssignmentExpression,
  OrExpression,
  AndExpression,
  EqualityExpression,
  RelationalExpression,
  AdditiveExpression,
  MultiplicativeExpression,
  UnaryExpression,
  CallFunctionExpression,
  IndexExpression,
  PrimaryExpression,
  // Literals
  Identifier,
  IntegerLiteral,
  FloatLiteral,
  StringLiteral,
  BooleanLiteral,
  ArrayLiteral
};

// Base Nodes Implementation
class AstNode
{
public:
  AstNodeType type;
  virtual ~AstNode() = default;

  static string get_node_name(const AstNode *node);

private:
  static map<AstNodeType, string> NodeNames;
};

class Statement : public AstNode
{
};

class Expression : public AstNode
{
};

class BooleanExpression : public Expression
{
};

class Literal : public AstNode
{
};

// Literal Nodes Implementation
class Identifier : public Literal
{
public:
  string name;

  Identifier(const string &name);
};

class IntegerLiteral : public Literal
{
public:
  int value;

  IntegerLiteral(int val);
};

class FloatLiteral : public Literal
{
public:
  float value;

  FloatLiteral(float val);
};

class StringLiteral : public Literal
{
public:
  string value;

  StringLiteral(const string &val);
};

class BooleanLiteral : public Literal
{
public:
  bool value;

  BooleanLiteral(bool val);
};

class ArrayLiteral : public Literal
{
public:
  vector<Literal *> elements;

  ArrayLiteral(const vector<Literal *> &elems);
  ~ArrayLiteral();
};

// Expression Nodes Implementation
class AssignmentExpression : public Expression
{
public:
  Expression *assignee;
  Expression *value;

  AssignmentExpression(Expression *var, Expression *val);
  ~AssignmentExpression();
};

class OrExpression : public Expression
{
public:
  Expression *left;
  Expression *right;

  OrExpression(Expression *lhs, Expression *rhs);
  ~OrExpression();
};

class AndExpression : public Expression
{
public:
  Expression *left;
  Expression *right;

  AndExpression(Expression *lhs, Expression *rhs);
  ~AndExpression();
};

class EqualityExpression : public BooleanExpression
{
public:
  Expression *left;
  Expression *right;
  string optr; // "==", "<>"

  EqualityExpression(Expression *lhs, Expression *rhs, const string &op);
  ~EqualityExpression();
};

class RelationalExpression : public BooleanExpression
{
public:
  Expression *left;
  Expression *right;
  string optr; // "<", "<=", ">", ">="

  RelationalExpression(Expression *lhs, Expression *rhs, const string &op);
  ~RelationalExpression();
};

class AdditiveExpression : public Expression
{
public:
  Expression *left;
  Expression *right;
  string optr; // "+" | "-"

  AdditiveExpression(Expression *lhs, Expression *rhs, const string &op);
  ~AdditiveExpression();
};

class MultiplicativeExpression : public Expression
{
public:
  Expression *left;
  Expression *right;
  string optr; // "*" | "/" | "%"

  MultiplicativeExpression(Expression *lhs, Expression *rhs, const string &op);
  ~MultiplicativeExpression();
};

class UnaryExpression : public Expression
{
public:
  Expression *operand;
  string optr; // "-" | "++" | "--" | "NOT"
  bool postfix;

  UnaryExpression(Expression *operand, const string &op, const bool &post);
  ~UnaryExpression();
};

class CallFunctionExpression : public Expression
{
public:
  Identifier *function;
  vector<Expression *> arguments;

  CallFunctionExpression(Identifier *func, const vector<Expression *> &args);
  ~CallFunctionExpression();
};

class IndexExpression : public Expression
{
public:
  Expression *base;
  Expression *index;

  IndexExpression(Expression *baseExpr, Expression *indexExpr);
  ~IndexExpression();
};

class PrimaryExpression : public Expression
{
public:
  Literal *value;

  PrimaryExpression(Literal *val);

  ~PrimaryExpression();
};

// Declaration Nodes Implementation
class VariableDeclaration : public Statement
{
public:
  vector<Identifier *> names;
  string datatype;

  VariableDeclaration(const vector<Identifier *> &names, const string &datatype);
  ~VariableDeclaration();
};

class VariableInitialization : public Statement
{
public:
  Identifier *name;
  string datatype;
  Expression *initializer;

  VariableInitialization(Identifier *name, const string &datatype, Expression *init);
  ~VariableInitialization();
};

class VariableDefinition : public Statement
{
public:
  Statement *def;

  VariableDefinition(Statement *def);
  ~VariableDefinition();
};

class FunctionDeclaration : public Statement
{
public:
  Identifier *funcname;
  string return_type;
  vector<VariableDefinition *> parameters;
  vector<Statement *> body;

  FunctionDeclaration(Identifier *name, const string &ret, const vector<VariableDefinition *> &params);
  ~FunctionDeclaration();
};

class ProgramDeclaration : public Statement
{
public:
  Identifier *program_name;
  vector<VariableDefinition *> globals;
  vector<Statement *> body;

  ProgramDeclaration(Identifier *program_name, const vector<VariableDefinition *> &globals, vector<Statement *> &body);
  ~ProgramDeclaration();
};

// Statement Nodes Implementation
class IfStatement : public Statement
{
public:
  Expression *condition;
  vector<Statement *> consequent;
  vector<Statement *> alternate;

  IfStatement(Expression *cond, const vector<Statement *> &consequent, const vector<Statement *> &alternate = {});
  ~IfStatement();
};

class ReturnStatement : public Statement
{
public:
  Expression *returnValue;

  ReturnStatement(Expression *value = nullptr);
  ~ReturnStatement();
};

class StopStatement : public Statement
{
public:
  StopStatement();
};

class SkipStatement : public Statement
{
public:
  SkipStatement();
};

class ReadStatement : public Statement
{
public:
  vector<Identifier *> variables;

  ReadStatement(const vector<Identifier *> &vars);
  ~ReadStatement();
};

class WriteStatement : public Statement
{
public:
  vector<Expression *> args;

  WriteStatement(const vector<Expression *> &values);
  ~WriteStatement();
};

// Loop Nodes Implementation
class WhileLoop : public Statement
{
public:
  Expression *condition;
  vector<Statement *> body;

  WhileLoop(Expression *cond, const vector<Statement *> &stmts);
  ~WhileLoop();
};

class ForLoop : public Statement
{
public:
  AssignmentExpression *init;
  BooleanExpression *condition;
  UnaryExpression *update;
  vector<Statement *> body;

  ForLoop(AssignmentExpression *init, BooleanExpression *cond, UnaryExpression *iter, const vector<Statement *> &stmts);
  ~ForLoop();
};

// Root Node Implementation
class Source : public AstNode
{
public:
  ProgramDeclaration *program;
  vector<FunctionDeclaration *> functions;

  Source(ProgramDeclaration *prog, const vector<FunctionDeclaration *> &funcs);
  ~Source();
};
