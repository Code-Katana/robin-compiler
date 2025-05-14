#pragma once

#include <string>

using namespace std;

namespace rbn::options
{
  enum class ScannerOptions
  {
    HandCoded,
    FiniteAutomaton
  };

  enum class ParserOptions
  {
    RecursiveDecent,
    LL1,
    LR
  };

  enum class OptimizationLevels
  {
    O0,
    O1,
    O2,
    O3,
    Os,
    Oz
  };

  class CompilerOptions
  {
  public:
    CompilerOptions(const string &prog);
    CompilerOptions(const string &prog, const OptimizationLevels &opt);
    CompilerOptions(const string &prog, const ParserOptions &pr);
    CompilerOptions(const string &prog, const ScannerOptions &sc);
    CompilerOptions(const string &prog, const ScannerOptions &sc, const ParserOptions &pr);
    CompilerOptions(const string &prog, const ScannerOptions &sc, const ParserOptions &pr, const OptimizationLevels &opt);

    string get_program();
    ScannerOptions get_scanner_option();
    ParserOptions get_parser_option();
    OptimizationLevels get_optimization_option();

  private:
    string program;
    ScannerOptions scOption;
    ParserOptions prOption;
    OptimizationLevels optOption;
  };
}