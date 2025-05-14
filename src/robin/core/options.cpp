#include "robin/core/options.h"

namespace rbn::options
{
  CompilerOptions::CompilerOptions(const string &prog)
      : program(prog),
        scOption(ScannerOptions::FiniteAutomaton),
        prOption(ParserOptions::RecursiveDecent),
        optOption(OptimizationLevels::O0) {}

  CompilerOptions::CompilerOptions(const string &prog, const OptimizationLevels &opt)
      : program(prog),
        scOption(ScannerOptions::FiniteAutomaton),
        prOption(ParserOptions::RecursiveDecent),
        optOption(opt) {}

  CompilerOptions::CompilerOptions(const string &prog, const ParserOptions &pr)
      : program(prog),
        scOption(ScannerOptions::FiniteAutomaton),
        prOption(pr),
        optOption(OptimizationLevels::O0) {}

  CompilerOptions::CompilerOptions(const string &prog, const ScannerOptions &sc)
      : program(prog),
        scOption(sc),
        prOption(ParserOptions::RecursiveDecent),
        optOption(OptimizationLevels::O0) {}

  CompilerOptions::CompilerOptions(const string &prog, const ScannerOptions &sc, const ParserOptions &pr)
      : program(prog),
        scOption(sc),
        prOption(pr),
        optOption(OptimizationLevels::O0) {}

  CompilerOptions::CompilerOptions(const string &prog, const ScannerOptions &sc, const ParserOptions &pr, const OptimizationLevels &opt)
      : program(prog),
        scOption(sc),
        prOption(pr),
        optOption(opt) {}

  string CompilerOptions::get_program()
  {
    return program;
  }

  ScannerOptions CompilerOptions::get_scanner_option()
  {
    return scOption;
  }

  ParserOptions CompilerOptions::get_parser_option()
  {
    return prOption;
  }

  OptimizationLevels CompilerOptions::get_optimization_option()
  {
    return optOption;
  }
}