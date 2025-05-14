#pragma once

#include "robin/core/ast.h"
#include "robin/core/symbol.h"
#include "robin/semantic/semantic_analyzer.h"
#include "robin/optimization/code_optimization.h"

#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/DiagnosticPrinter.h>
#include <llvm/Support/raw_ostream.h>

#include <unordered_map>
#include <string>
#include <memory>
#include <stack>
#include <map>

using namespace std;
using namespace llvm;
namespace rbn::ir
{
  struct SymbolEntry
  {
    llvm::Type *llvmType;
    llvm::Value *llvmValue;
    core::SymbolType baseType;
    int dimensions;
    size_t arrayLength = 0;
    llvm::Value *lengthAlloca = nullptr;
  };

  class IRGenerator
  {
  public:
    IRGenerator(semantic::SemanticAnalyzer *semantic);
    Module *getModule() { return module.get(); }

    void generate(const string &filename);

  private:
    ast::Source *source;
    semantic::SemanticAnalyzer *semantic;
    static LLVMContext context;
    unique_ptr<Module> module;
    IRBuilder<> builder;
    stack<unordered_map<string, SymbolEntry>> symbolTable;
    std::unordered_map<std::string, ast::FunctionDefinition *> functionTable;
    options::OptimizationLevels optLevel = options::OptimizationLevels::O0;

    Type *getLLVMType(core::SymbolType type, int dim = 0);
    Value *generate_node(ast::AstNode *node);
    Value *generate_program(ast::ProgramDefinition *program);
    Value *generate_function(ast::FunctionDefinition *func);

    // definition
    void generate_global_variable(ast::VariableDefinition *dif);
    Value *generate_variable_definition(ast::VariableDefinition *def);
    Value *generate_variable_declaration(ast::VariableDeclaration *decl);
    Value *generate_variable_initialization(ast::VariableInitialization *init);
    // expression
    Value *generate_expression(ast::Expression *expr);
    Value *generate_assignment(ast::AssignmentExpression *assign);
    Value *generate_identifier(ast::Identifier *id);
    Value *generate_literal(ast::Literal *lit);
    Value *generate_or_expr(ast::OrExpression *expr);
    Value *generate_and_expr(ast::AndExpression *expr);
    Value *generate_relational_expr(ast::RelationalExpression *expr);
    Value *generate_equality_expr(ast::EqualityExpression *expr);
    Value *generate_additive_expr(ast::AdditiveExpression *expr);
    Value *generate_multiplicative_expr(ast::MultiplicativeExpression *expr);
    Value *generate_unary_expr(ast::UnaryExpression *expr);
    Value *generate_call(ast::CallFunctionExpression *call);
    Value *generate_index_expression(ast::IndexExpression *expr, Type **outElementType = nullptr);
    // statements
    Value *generate_statement(ast::Statement *stmt);
    Value *generate_while_loop(ast::WhileLoop *loop);
    Value *generate_for_loop(ast::ForLoop *forLoop);
    Value *generate_if_statement(ast::IfStatement *ifStmt);
    Value *generate_return_statement(ast::ReturnStatement *retStmt);
    Value *generate_write_statement(ast::WriteStatement *write);
    Value *generate_read_statement(ast::ReadStatement *read);
    Value *generate_stop_statement(ast::StopStatement *stmt);
    Value *generate_skip_statement(ast::SkipStatement *stmt);
    // helpers
    void pushScope() { symbolTable.push({}); }
    void popScope() { symbolTable.pop(); }
    void printSymbolTable();

    Value *castToBoolean(Value *value);
    Value *findValue(const string &name);
    Value *generate_identifier_address(ast::Identifier *id);
    optional<SymbolEntry> findSymbol(const std::string &name);
    // array helpers
    bool isUniformArray(ast::ArrayLiteral *lit);
    Type *getElementType(ast::ArrayLiteral *lit, int *outDim);
    Value *createJaggedArray(ast::ArrayLiteral *lit, Function *mallocFn,
                             core::SymbolType *outBaseType, int *outDimensions);
    Constant *createNestedArray(ast::ArrayLiteral *lit, ArrayType *arrType);
    ArrayType *inferArrayType(ast::ArrayLiteral *lit);
    Function *declareMalloc();
    Value *createArrayAllocation(Type *elementType, Value *size);
    Value *generate_l_value(ast::Expression *expr);
    Value *createJaggedArrayHelper(ast::ArrayLiteral *lit, Function *mallocFn,
                                   int remainingDims, Type *elementType);
    bool isSingleDimensionArray(const SymbolEntry &entry);
  };
}