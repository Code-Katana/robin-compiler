#pragma once

#include "ast.h"
#include "symbol.h"

#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
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

  void codegenGlobalVariable(VariableDefinition *dif);

  Type *getLLVMType(SymbolType type, int dim = 0);
  Value *codegen(AstNode *node);
  Value *codegenProgram(ProgramDefinition *program);
  Value *codegenFunction(FunctionDefinition *func);
  

  //def
  Value *codegenVariableDefinition(VariableDefinition *def);
  Value *codegenVariableDeclaration(VariableDeclaration *decl);
  Value *codegenVariableInitialization(VariableInitialization *init);
  //expr
  Value *codegenExpression(Expression *expr);
  Value *codegenIdentifier(Identifier *id);
  Value *codegenLiteral(Literal *lit);
  Value *codegenOrExpr(OrExpression *expr);
  Value *codegenAndExpr(AndExpression *expr);
  Value *codegenRelationalExpr(RelationalExpression *expr);
  Value *codegenEqualityExpr(EqualityExpression *expr);
  Value *codegenAdditiveExpr(AdditiveExpression *expr);
  Value *codegenMultiplicativeExpr(MultiplicativeExpression *expr);
  Value *codegenUnaryExpr(UnaryExpression *expr);
  //stat
  Value *codegenStatement(Statement *stmt);
  Value *codegenAssignment(AssignmentExpression *assign);
  Value *codegenCall(CallFunctionExpression *call);
  Value *codegenWhileLoop(WhileLoop *loop);
  Value *codegenForLoop(ForLoop *forLoop);
  Value *codegenIfStatement(IfStatement *ifStmt);
  Value *codegenReturnStatement(ReturnStatement *retStmt);
  Value *codegenWriteStatement(WriteStatement *write);
  Value *codegenReadStatement(ReadStatement *read);
  //helpers
  void pushScope() { symbolTable.push({}); }
  void popScope() { symbolTable.pop(); }
  Value *castToBoolean(Value *value);
  Value *findValue(const string &name);
  Value *codegenIdentifierAddress(Identifier *id);
  Value *codegenLvalue(AssignableExpression *expr);
  void printSymbolTable();
};
