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

#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Type.h"

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
  Value *codegenOrExpr(OrExpression *expr);
  Value *codegenAndExpr(AndExpression *expr);
  Value *codegenRelationalExpr(RelationalExpression *expr);
  Value *codegenEqualityExpr(EqualityExpression *expr);
  Value *codegenAdditiveExpr(AdditiveExpression *expr);
  Value *codegenMultiplicativeExpr(MultiplicativeExpression *expr);
  Value *codegenCall(CallFunctionExpression *call);
  Value *codegenConditional(IfStatement *ifStmt);
  Value *codegenLoop(WhileLoop *loop);
  Value *codegenForLoop(ForLoop *loop);
  Value *codegenWriteStatement(WriteStatement *write);
  Value *codegenReadStatement(ReadStatement *read);
  Value *codegenIndexExpression(IndexExpression *expr);
  Value *createArrayAllocation(Type *elementType, Value *size);

  Constant *createNestedArray(ArrayLiteral *lit, ArrayType *parentType);
  Value *createArrayAllocation(Type *baseType, const std::vector<Value*> &dims);

  void pushScope() { symbolTable.push({}); }
  void popScope() { symbolTable.pop(); }
  Value *findValue(const string &name);
};
