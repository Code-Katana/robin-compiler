#pragma once

#include "ast.h"
#include "symbol.h"

#include <llvm/Support/TargetSelect.h>
#include "llvm/Support/FileSystem.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"

#include <stack>
#include <map>

using namespace std;
using namespace llvm;

class IRGenerator
{
public:
  IRGenerator();
  Module *getModule() { return module.get(); }

  void generate(Source *source, const string &filename);

private:
  static LLVMContext context;
  unique_ptr<Module> module;
  IRBuilder<> builder;
  stack<map<string, Value *>> symbolTable;

  Type *getLLVMType(SymbolType type, int dim = 0);
  Value *codegen(AstNode *node);
  Value *codegenProgram(ProgramDefinition *program);
  Value *codegenFunction(FunctionDefinition *func);
  Value *codegenStatement(Statement *stmt);
  Value *codegenExpression(Expression *expr);
  Value *codegenVariableDeclaration(VariableDeclaration *decl);

  Value *codegenAssignment(AssignmentExpression *assign);
  Value *codegenIdentifier(Identifier *id);
  Value *codegenLiteral(Literal *lit);
  // we can make it id
  Value *codegenAssignableExpr(AssignableExpression *expr);
  Value *codegenBinaryOp(Expression *lhs, Expression *rhs,string op);
  Value *codegenCall(CallFunctionExpression *call);
  Value *codegenConditional(IfStatement *ifStmt);
  Value *codegenLoop(WhileLoop *loop);
  Value *codegenForLoop(ForLoop *loop);
  Value *codegenWriteStatement(WriteStatement *write);
  Value *codegenReadStatement(ReadStatement *read);

  void pushScope() { symbolTable.push({}); }
  void popScope() { symbolTable.pop(); }
  Value *findValue(const string &name);
};
