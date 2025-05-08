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
#include "llvm/IR/Instructions.h" 
#include "llvm/IR/Function.h"

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
  std::unordered_map<std::string, llvm::StructType *> arrayStructCache;

  StructType* getArrayStruct(const SymbolEntry &entry);
  Value* loadArrayLength(Value *arrayAlloca, StructType *arrSt, const Twine &name);
  Value* callStrlen(Value *strPtr, const Twine &name);
  StructType *getOrCreateArrayStruct(llvm::Type *elementType, unsigned dimension, const std::string &baseName);
  Value *createJaggedArrayStruct(ArrayLiteral *lit, Function *mallocFn, SymbolType *outBaseType, int *outDimensions);

  Type *getLLVMType(SymbolType type, int dim = 0);
  Value *generate_node(AstNode *node);
  Value *generate_program(ProgramDefinition *program);
  Value *generate_function(FunctionDefinition *func);

  // definition
  void generate_global_variable(VariableDefinition *dif);
  Value *generate_variable_definition(VariableDefinition *def);
  Value *generate_variable_declaration(VariableDeclaration *decl);
  Value *generate_variable_initialization(VariableInitialization *init);
  // expression
  Value *generate_expression(Expression *expr);
  Value *generate_assignment(AssignmentExpression *assign);
  Value *generate_identifier(Identifier *id);
  Value *generate_literal(Literal *lit);
  Value *generate_or_expr(OrExpression *expr);
  Value *generate_and_expr(AndExpression *expr);
  Value *generate_relational_expr(RelationalExpression *expr);
  Value *generate_equality_expr(EqualityExpression *expr);
  Value *generate_additive_expr(AdditiveExpression *expr);
  Value *generate_multiplicative_expr(MultiplicativeExpression *expr);
  Value *generate_unary_expr(UnaryExpression *expr);
  Value *generate_call(CallFunctionExpression *call);
  Value *generate_index_expression(IndexExpression *expr, Type **outElementType = nullptr);
  // statements
  Value *generate_statement(Statement *stmt);
  Value *generate_while_loop(WhileLoop *loop);
  Value *generate_for_loop(ForLoop *forLoop);
  Value *generate_if_statement(IfStatement *ifStmt);
  Value *generate_return_statement(ReturnStatement *retStmt);
  Value *generate_write_statement(WriteStatement *write);
  Value *generate_read_statement(ReadStatement *read);
  Value *generate_stop_statement(StopStatement *stmt);
  Value *generate_skip_statement(SkipStatement *stmt);
  // helpers
  void pushScope() { symbolTable.push({}); }
  void popScope() { symbolTable.pop(); }
  void printSymbolTable();

  Value *castToBoolean(Value *value);
  Value *findValue(const string &name);
  Value *generate_identifier_address(Identifier *id);
  optional<SymbolEntry> findSymbol(const std::string &name);
  // array helpers
  bool isUniformArray(ArrayLiteral *lit);
  Type *getElementType(ArrayLiteral *lit, int *outDim);
  Value *createJaggedArray(ArrayLiteral *lit, Function *mallocFn,
                           SymbolType *outBaseType, int *outDimensions);
  Constant *createNestedArray(ArrayLiteral *lit, ArrayType *arrType);
  ArrayType *inferArrayType(ArrayLiteral *lit);
  Function *declareMalloc();
  Value *createArrayAllocation(Type *elementType, Value *size);
  Value *generate_l_value(Expression *expr);
  Value *createJaggedArrayHelper(ArrayLiteral *lit, Function *mallocFn,
                                 int remainingDims, Type *elementType);
  bool isSingleDimensionArray(const SymbolEntry &entry);
};