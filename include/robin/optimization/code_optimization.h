#pragma once

#include <string>
#include <memory>

#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>

#include "robin/core/options.h"

using namespace std;
using namespace llvm;

namespace rbn::optimization
{
  class CodeOptimization
  {
  public:
    CodeOptimization();
    CodeOptimization(rbn::options::OptimizationLevels level);
    CodeOptimization(Module *mod, rbn::options::OptimizationLevels level);

    void optimize_and_write(const string &afterFile);
    rbn::options::OptimizationLevels get_level() const;
    string get_level_string() const;

  private:
    Module *module;
    rbn::options::OptimizationLevels optLevel;
  };
}