#include "code_optimization.h"

CodeOptimization::CodeOptimization() : optLevel(OptLevel::O0) {}

CodeOptimization::CodeOptimization(OptLevel level) : optLevel(level) {}
CodeOptimization::CodeOptimization(llvm::Module* mod, OptLevel level): module(mod), optLevel(level) {}

void CodeOptimization::optimize_and_write( const std::string &afterFile)
{
  // {
  //   std::error_code EC1;
  //   llvm::raw_fd_ostream beforeOut(beforeFile, EC1, llvm::sys::fs::OF_None);
  //   if (!EC1)
  //     module->print(beforeOut, nullptr);
  // }

  llvm::LoopAnalysisManager LAM;
  llvm::FunctionAnalysisManager FAM;
  llvm::CGSCCAnalysisManager CGAM;
  llvm::ModuleAnalysisManager MAM;
  llvm::PassBuilder PB;

  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  llvm::ModulePassManager MPM;
  llvm::OptimizationLevel llvmOptLevel = llvm::OptimizationLevel::O0;
  switch (optLevel)
  {
  case OptLevel::O0:
    llvmOptLevel = llvm::OptimizationLevel::O0;
    break;
  case OptLevel::O1:
    llvmOptLevel = llvm::OptimizationLevel::O1;
    break;
  case OptLevel::O2:
    llvmOptLevel = llvm::OptimizationLevel::O2;
    break;
  case OptLevel::O3:
    llvmOptLevel = llvm::OptimizationLevel::O3;
    break;
  case OptLevel::Os:
    llvmOptLevel = llvm::OptimizationLevel::Os;
    break;
  case OptLevel::Oz:
    llvmOptLevel = llvm::OptimizationLevel::Oz;
    break;
  default:
    llvmOptLevel = llvm::OptimizationLevel::O0;
    break;
  }

  MPM = PB.buildPerModuleDefaultPipeline(llvmOptLevel);
  MPM.run(*module, MAM);

  {
    std::error_code EC2;
    llvm::raw_fd_ostream afterOut(afterFile, EC2, llvm::sys::fs::OF_None);
    if (!EC2)
      module->print(afterOut, nullptr);
  }

  // {
  //   std::error_code EC3;
  //   llvm::raw_fd_ostream finalOut(finalFile, EC3, llvm::sys::fs::OF_None);
  //   if (!EC3)
  //     module->print(finalOut, nullptr);
  // }
}
OptLevel CodeOptimization::get_level() const
{
  return optLevel;
}

std::string CodeOptimization::get_level_string() const
{
  switch (optLevel)
  {
  case OptLevel::O0:
    return "O0";
  case OptLevel::O1:
    return "O1";
  case OptLevel::O2:
    return "O2";
  case OptLevel::O3:
    return "O3";
  case OptLevel::Os:
    return "Os";
  case OptLevel::Oz:
    return "Oz";
  default:
    return "O0";
  }
}