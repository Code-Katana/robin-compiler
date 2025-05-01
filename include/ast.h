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
  ArrayLiteral,
  // Error
  ErrorNode
};

// Base Nodes Implementation
class AstNode
{
public:
  int node_start;
  int start_line;
  int node_end;
  int end_line;
  AstNodeType type;

  AstNode();
  AstNode(int sl, int el, int s, int e);
  virtual ~AstNode() = default;
  static bool is_data_type(TokenType type);
  static bool is_return_type(TokenType type);

  static string get_node_name(const AstNode *node);
  static map<TokenType, string> DataTypes;

private:
  static map<AstNodeType, string> NodeNames;
};

class ErrorNode : public AstNode
{
public:
  string message;

  ErrorNode();
  ErrorNode(const string &message, int sl, int el, int s, int e);
};

class Statement : public AstNode
{
public:
  Statement();
  Statement(int sl, int el, int s, int e);
};

class Expression : public Statement
{
public:
  Expression();
  Expression(int sl, int el, int s, int e);
};

class BooleanExpression : public Expression
{
public:
  BooleanExpression(int sl, int el, int s, int e);
};

class AssignableExpression : public Expression
{
public:
  AssignableExpression(int sl, int el, int s, int e);
};

class DataType : public AstNode
{
public:
  DataType(int sl, int el, int s, int e);
};

class Literal : public Expression
{
public:
  Literal(int sl, int el, int s, int e);
};

// Literal Nodes Implementation
class Identifier : public AssignableExpression
{
public:
  string name;

  Identifier(const string &name, int sl, int el, int s, int e);
};

class IntegerLiteral : public Literal
{
public:
  int value;

  IntegerLiteral(int val, int sl, int el, int s, int e);
};

class FloatLiteral : public Literal
{
public:
  float value;

  FloatLiteral(float val, int sl, int el, int s, int e);
};

class StringLiteral : public Literal
{
public:
  string value;

  StringLiteral(const string &val, int sl, int el, int s, int e);
};

class BooleanLiteral : public Literal
{
public:
  bool value;

  BooleanLiteral(bool val, int sl, int el, int s, int e);
};

class ArrayLiteral : public Literal
{
public:
  vector<Expression *> elements;

  ArrayLiteral(const vector<Expression *> &elems, int sl, int el, int s, int e);
  ~ArrayLiteral();
};

// Data Type implementation
class ReturnType : public DataType
{
public:
  DataType *return_type;

  ReturnType(DataType *rt, int sl, int el, int s, int e);
  ~ReturnType();
};

class PrimitiveType : public DataType
{
public:
  string datatype;

  PrimitiveType(const string &ty, int sl, int el, int s, int e);
};

class ArrayType : public DataType
{
public:
  string datatype;
  int dimension;

  ArrayType(const string &ty, int dim, int sl, int el, int s, int e);
};

// Expression Nodes Implementation
class AssignmentExpression : public Expression
{
public:
  AssignableExpression *assignee;
  Expression *value;

  AssignmentExpression(AssignableExpression *var, Expression *val, int sl, int el, int s, int e);
  ~AssignmentExpression();
};

class OrExpression : public BooleanExpression
{
public:
  Expression *left;
  Expression *right;

  OrExpression(Expression *lhs, Expression *rhs, int sl, int el, int s, int e);
  ~OrExpression();
};

class AndExpression : public BooleanExpression
{
public:
  Expression *left;
  Expression *right;

  AndExpression(Expression *lhs, Expression *rhs, int sl, int el, int s, int e);
  ~AndExpression();
};

class EqualityExpression : public BooleanExpression
{
public:
  Expression *left;
  Expression *right;
  string optr; // "==", "<>"

  EqualityExpression(Expression *lhs, Expression *rhs, const string &op, int sl, int el, int s, int e);
  ~EqualityExpression();
};

class RelationalExpression : public BooleanExpression
{
public:
  Expression *left;
  Expression *right;
  string optr; // "<", "<=", ">", ">="

  RelationalExpression(Expression *lhs, Expression *rhs, const string &op, int sl, int el, int s, int e);
  ~RelationalExpression();
};

class AdditiveExpression : public Expression
{
public:
  Expression *left;
  Expression *right;
  string optr; // "+" | "-"

  AdditiveExpression(Expression *lhs, Expression *rhs, const string &op, int sl, int el, int s, int e);
  ~AdditiveExpression();
};

class MultiplicativeExpression : public Expression
{
public:
  Expression *left;
  Expression *right;
  string optr; // "*" | "/" | "%"

  MultiplicativeExpression(Expression *lhs, Expression *rhs, const string &op, int sl, int el, int s, int e);
  ~MultiplicativeExpression();
};

class UnaryExpression : public Expression
{
public:
  Expression *operand;
  string optr; // "-" | "++" | "--" | "NOT()" | "$" | "#" | "@" | "?"
  bool postfix;

  UnaryExpression(Expression *operand, const string &op, const bool &post, int sl, int el, int s, int e);
  ~UnaryExpression();
};

class CallFunctionExpression : public Expression
{
public:
  Identifier *function;
  vector<Expression *> arguments;

  CallFunctionExpression(Identifier *func, const vector<Expression *> &args, int sl, int el, int s, int e);
  ~CallFunctionExpression();
};

class IndexExpression : public AssignableExpression
{
public:
  Expression *base;
  Expression *index;

  IndexExpression(Expression *baseExpr, Expression *indexExpr, int sl, int el, int s, int e);
  ~IndexExpression();
};

class PrimaryExpression : public Expression
{
public:
  Literal *value;

  PrimaryExpression(Literal *val, int sl, int el, int s, int e);

  ~PrimaryExpression();
};

// Declaration Nodes Implementation
class VariableDeclaration : public Statement
{
public:
  vector<Identifier *> variables;
  DataType *datatype;

  VariableDeclaration(const vector<Identifier *> &variables, DataType *datatype, int sl, int el, int s, int e);
  ~VariableDeclaration();
};

class VariableInitialization : public Statement
{
public:
  Identifier *name;
  DataType *datatype;
  Expression *initializer;

  VariableInitialization(Identifier *name, DataType *datatype, Expression *init, int sl, int el, int s, int e);
  ~VariableInitialization();
};

class VariableDefinition : public Statement
{
public:
  Statement *def;

  VariableDefinition(Statement *def, int sl, int el, int s, int e);
  ~VariableDefinition();
};

class Function : public AstNode
{
public:
  Identifier *funcname;
  ReturnType *return_type;
  vector<VariableDefinition *> parameters;
  vector<Statement *> body;

  Function(Identifier *name, ReturnType *ret, const vector<VariableDefinition *> &params, const vector<Statement *> &body, int sl, int el, int s, int e);
  ~Function();
};

class Program : public AstNode
{
public:
  Identifier *program_name;
  vector<VariableDefinition *> globals;
  vector<Statement *> body;

  Program(Identifier *prog, const vector<VariableDefinition *> &glob, const vector<Statement *> &body, int sl, int el, int s, int e);
  ~Program();
};

// Statement Nodes Implementation
class IfStatement : public Statement
{
public:
  Expression *condition;
  vector<Statement *> consequent;
  vector<Statement *> alternate;

  IfStatement(Expression *cond, const vector<Statement *> &consequent, const vector<Statement *> &alternate, int sl, int el, int s, int e);
  ~IfStatement();
};

class ReturnStatement : public Statement
{
public:
  Expression *returnValue;

  ReturnStatement(Expression *value, int sl, int el, int s, int e);
  ~ReturnStatement();
};

class StopStatement : public Statement
{
public:
  StopStatement(int sl, int el, int s, int e);
};

class SkipStatement : public Statement
{
public:
  SkipStatement(int sl, int el, int s, int e);
};

class ReadStatement : public Statement
{
public:
  vector<AssignableExpression *> variables;

  ReadStatement(const vector<AssignableExpression *> &vars, int sl, int el, int s, int e);
  ~ReadStatement();
};

class WriteStatement : public Statement
{
public:
  vector<Expression *> args;

  WriteStatement(const vector<Expression *> &values, int sl, int el, int s, int e);
  ~WriteStatement();
};

// Loop Nodes Implementation
class WhileLoop : public Statement
{
public:
  Expression *condition;
  vector<Statement *> body;

  WhileLoop(Expression *cond, const vector<Statement *> &stmts, int sl, int el, int s, int e);
  ~WhileLoop();
};

class ForLoop : public Statement
{
public:
  AssignmentExpression *init;
  Expression *condition;
  Expression *update;
  vector<Statement *> body;

  ForLoop(AssignmentExpression *init, Expression *cond, Expression *iter, const vector<Statement *> &stmts, int sl, int el, int s, int e);
  ~ForLoop();
};

// Root Node Implementation
class Source : public AstNode
{
public:
  Program *program;
  vector<Function *> functions;

  Source(Program *prog, const vector<Function *> &funcs, int sl, int el, int s, int e);
  ~Source();
};
