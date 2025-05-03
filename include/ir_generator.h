#pragma once

#include "ast.h"
#include "symbol.h"
#include "semantic_analyzer.h"

#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/TargetSelect.h>
#include "llvm/Support/FileSystem.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"

#include <unordered_map>
#include <string>
#include <memory>
#include <stack>
#include <map>

using namespace std;
using namespace llvm;

struct SymbolEntry
{
  llvm::Type *llvmType;
  llvm::Value *llvmValue;
  SymbolType baseType;
  int dimensions;
  size_t arrayLength = 0;
  llvm::Value *lengthAlloca = nullptr;
};
class IRGenerator
{
public:
  IRGenerator(SemanticAnalyzer *semantic);
  Module *getModule() { return module.get(); }

  void generate(const string &filename);

private:
  Source *source;
  static LLVMContext context;
  unique_ptr<Module> module;
  IRBuilder<> builder;
  stack<unordered_map<string, SymbolEntry>> symbolTable;
  std::unordered_map<std::string, FunctionDefinition *> functionTable;

  Type *getLLVMType(SymbolType type, int dim = 0);
  Value *codegen(AstNode *node);
  Value *codegenProgram(ProgramDefinition *program);
  Value *codegenFunction(FunctionDefinition *func);

  // def
  void codegenGlobalVariable(VariableDefinition *dif);
  Value *codegenVariableDefinition(VariableDefinition *def);
  Value *codegenVariableDeclaration(VariableDeclaration *decl);
  Value *codegenVariableInitialization(VariableInitialization *init);
  // expr
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
  // stat
  Value *codegenStatement(Statement *stmt);
  Value *codegenAssignment(AssignmentExpression *assign);
  Value *codegenCall(CallFunctionExpression *call);
  Value *codegenWhileLoop(WhileLoop *loop);
  Value *codegenForLoop(ForLoop *forLoop);
  Value *codegenIfStatement(IfStatement *ifStmt);
  Value *codegenReturnStatement(ReturnStatement *retStmt);
  Value *codegenWriteStatement(WriteStatement *write);
  Value *codegenReadStatement(ReadStatement *read);
  Value *codegenStopStatement(StopStatement *stmt);
  Value *codegenSkipStatement(SkipStatement *stmt);
  // helpers
  void pushScope() { symbolTable.push({}); }
  void popScope() { symbolTable.pop(); }
  void printSymbolTable();

  Value *castToBoolean(Value *value);
  Value *findValue(const string &name);
  Value *codegenIdentifierAddress(Identifier *id);
  Value *codegenLvalue(AssignableExpression *expr);

  optional<SymbolEntry> findSymbol(const std::string &name);

  bool isUniformArray(ArrayLiteral *lit);
  Type *getElementType(ArrayLiteral *lit, int *outDim);
  Value *createJaggedArray(ArrayLiteral *lit, Function *mallocFn,
                           SymbolType *outBaseType, int *outDimensions);
  Constant *createNestedArray(ArrayLiteral *lit, ArrayType *arrType);
  ArrayType *inferArrayType(ArrayLiteral *lit);
  Value *codegenIndexExpression(IndexExpression *expr, Type **outElementType = nullptr);
  Function *declareMalloc();
  Value *createArrayAllocation(Type *elementType, Value *size);
  // void printArrayLiteral(ArrayLiteral* lit, int depth = 0);
  Value *codegenLValue(Expression *expr);
  Value *createJaggedArrayHelper(ArrayLiteral *lit, Function *mallocFn,
                                 int remainingDims, Type *elementType);

  bool isSingleDimensionArray(const SymbolEntry &entry);
};