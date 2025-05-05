#include "code_optimization.h"

CodeOptimization::CodeOptimization() : optLevel(OptLevel::O0) {}

CodeOptimization::CodeOptimization(OptLevel level) : optLevel(level) {}

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