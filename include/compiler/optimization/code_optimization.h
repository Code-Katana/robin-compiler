#pragma once

#include <string>
#include <memory>

#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/raw_ostream.h>
#include "llvm/Support/FileSystem.h"

using namespace std;

enum class OptLevel
{
  O0,
  O1,
  O2,
  O3,
  Os,
  Oz
};

class CodeOptimization
{
public:
  CodeOptimization();
  CodeOptimization(OptLevel level);
  CodeOptimization(llvm::Module *mod, OptLevel level);

  void optimize_and_write(const string &afterFile);
  OptLevel get_level() const;
  string get_level_string() const;

private:
  llvm::Module *module;
  OptLevel optLevel;
};