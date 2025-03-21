#pragma once

#include <string>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

using namespace std;
using namespace llvm;

class IRGenerator
{
public:
  IRGenerator();
  void generate(const string &filename);

private:
  LLVMContext context;
  Module *module;
  IRBuilder<> builder;

  void temp_test_method(const string &filename);
};