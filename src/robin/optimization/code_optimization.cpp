#include "robin/optimization/code_optimization.h"

namespace rbn::optimization
{
  CodeOptimization::CodeOptimization() : optLevel(options::OptimizationLevels::O0) {}

  CodeOptimization::CodeOptimization(options::OptimizationLevels level) : optLevel(level) {}
  CodeOptimization::CodeOptimization(Module *mod, options::OptimizationLevels level) : module(mod), optLevel(level) {}

  void CodeOptimization::optimize_and_write(const std::string &afterFile)
  {
    // {
    //   std::error_code EC1;
    //   raw_fd_ostream beforeOut(beforeFile, EC1, sys::fs::OF_None);
    //   if (!EC1)
    //     module->print(beforeOut, nullptr);
    // }

    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;
    PassBuilder PB;

    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    ModulePassManager MPM;
    OptimizationLevel llvmOptLevel = OptimizationLevel::O0;
    switch (optLevel)
    {
    case options::OptimizationLevels::O0:
      llvmOptLevel = OptimizationLevel::O0;
      break;
    case options::OptimizationLevels::O1:
      llvmOptLevel = OptimizationLevel::O1;
      break;
    case options::OptimizationLevels::O2:
      llvmOptLevel = OptimizationLevel::O2;
      break;
    case options::OptimizationLevels::O3:
      llvmOptLevel = OptimizationLevel::O3;
      break;
    case options::OptimizationLevels::Os:
      llvmOptLevel = OptimizationLevel::Os;
      break;
    case options::OptimizationLevels::Oz:
      llvmOptLevel = OptimizationLevel::Oz;
      break;
    default:
      llvmOptLevel = OptimizationLevel::O0;
      break;
    }

    MPM = PB.buildPerModuleDefaultPipeline(llvmOptLevel);
    MPM.run(*module, MAM);

    {
      std::error_code EC2;
      raw_fd_ostream afterOut(afterFile, EC2, sys::fs::OF_None);
      if (!EC2)
        module->print(afterOut, nullptr);
    }

    // {
    //   std::error_code EC3;
    //   raw_fd_ostream finalOut(finalFile, EC3, sys::fs::OF_None);
    //   if (!EC3)
    //     module->print(finalOut, nullptr);
    // }
  }
  options::OptimizationLevels CodeOptimization::get_level() const
  {
    return optLevel;
  }

  std::string CodeOptimization::get_level_string() const
  {
    switch (optLevel)
    {
    case options::OptimizationLevels::O0:
      return "O0";
    case options::OptimizationLevels::O1:
      return "O1";
    case options::OptimizationLevels::O2:
      return "O2";
    case options::OptimizationLevels::O3:
      return "O3";
    case options::OptimizationLevels::Os:
      return "Os";
    case options::OptimizationLevels::Oz:
      return "Oz";
    default:
      return "O0";
    }
  }
}