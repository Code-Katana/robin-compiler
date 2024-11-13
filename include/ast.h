#pragma once

#include <string>
#include <vector>
#include <map>

#include "token.h"

using namespace std;

enum class AstNodeType
{
  // Root Node
  Source,
  // Declarations
  Program,
  Function,
  VariableDefinition,
  VariableDeclaration,
  VariableInitialization,
  // Types
  ReturnType,
  PrimitiveType,
  ArrayType,
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
  static bool is_data_type(TokenType type);
  static bool is_return_type(TokenType type);

  static string get_node_name(const AstNode *node);
  static map<TokenType, string> DataTypes;

private:
  static map<AstNodeType, string> NodeNames;
};

class Statement : public AstNode
{
};

class Expression : public Statement
{
};

class BooleanExpression : public Expression
{
};

class AssignableExpression : public Expression
{
};

class DataType : public AstNode
{
};

class Literal : public Expression
{
};

// Literal Nodes Implementation
class Identifier : public AssignableExpression
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
  vector<Expression *> elements;

  ArrayLiteral(const vector<Expression *> &elems);
  ~ArrayLiteral();
};

// Data Type implementation
class ReturnType : public DataType
{
public:
  DataType *return_type;

  ReturnType(DataType *rt);
  ~ReturnType();
};

class PrimitiveType : public DataType
{
public:
  string datatype;

  PrimitiveType(const string &ty);
};

class ArrayType : public DataType
{
public:
  string datatype;
  int dimension;

  ArrayType(const string &ty, int dim);
};

// Expression Nodes Implementation
class AssignmentExpression : public Expression
{
public:
  AssignableExpression *assignee;
  Expression *value;

  AssignmentExpression(AssignableExpression *var, Expression *val);
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
  string optr; // "-" | "++" | "--" | "NOT()"
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

class IndexExpression : public AssignableExpression
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
  vector<Identifier *> variables;
  DataType *datatype;

  VariableDeclaration(const vector<Identifier *> &variables, DataType *datatype);
  ~VariableDeclaration();
};

class VariableInitialization : public Statement
{
public:
  Identifier *name;
  DataType *datatype;
  Expression *initializer;

  VariableInitialization(Identifier *name, DataType *datatype, Expression *init);
  ~VariableInitialization();
};

class VariableDefinition : public Statement
{
public:
  Statement *def;

  VariableDefinition(Statement *def);
  ~VariableDefinition();
};

class Function : public AstNode
{
public:
  Identifier *funcname;
  ReturnType *return_type;
  vector<VariableDefinition *> parameters;
  vector<Statement *> body;

  Function(Identifier *name, ReturnType *ret, const vector<VariableDefinition *> &params, const vector<Statement *> &body);
  ~Function();
};

class Program : public AstNode
{
public:
  Identifier *program_name;
  vector<VariableDefinition *> globals;
  vector<Statement *> body;

  Program(Identifier *prog, const vector<VariableDefinition *> &glob, const vector<Statement *> &body);
  ~Program();
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
  Expression *update;
  vector<Statement *> body;

  ForLoop(AssignmentExpression *init, BooleanExpression *cond, Expression *iter, const vector<Statement *> &stmts);
  ~ForLoop();
};

// Root Node Implementation
class Source : public AstNode
{
public:
  Program *program;
  vector<Function *> functions;

  Source(Program *prog, const vector<Function *> &funcs);
  ~Source();
};
