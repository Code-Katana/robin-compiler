#pragma once

#include <string>

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

  OptLevel get_level() const;
  std::string get_level_string() const;

private:
  OptLevel optLevel;
};