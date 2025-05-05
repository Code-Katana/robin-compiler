#include "compiler_options.h"

CompilerOptions::CompilerOptions(const string &prog)
    : program(prog),
      scOption(ScannerOptions::FiniteAutomaton),
      prOption(ParserOptions::RecursiveDecent),
      optOption(CodeOptimization()) {}

CompilerOptions::CompilerOptions(const string &prog, const CodeOptimization &opt)
    : program(prog),
      scOption(ScannerOptions::FiniteAutomaton),
      prOption(ParserOptions::RecursiveDecent),
      optOption(opt) {}

CompilerOptions::CompilerOptions(const string &prog, const ParserOptions &pr)
    : program(prog),
      scOption(ScannerOptions::FiniteAutomaton),
      prOption(pr),
      optOption(CodeOptimization()) {}

CompilerOptions::CompilerOptions(const string &prog, const ScannerOptions &sc)
    : program(prog),
      scOption(sc),
      prOption(ParserOptions::RecursiveDecent),
      optOption(CodeOptimization()) {}

CompilerOptions::CompilerOptions(const string &prog, const ScannerOptions &sc, const ParserOptions &pr)
    : program(prog),
      scOption(sc),
      prOption(pr),
      optOption(CodeOptimization()) {}

// *** MISSING CONSTRUCTOR: ***
CompilerOptions::CompilerOptions(const string &prog, const ScannerOptions &sc, const ParserOptions &pr, const CodeOptimization &opt)
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
CodeOptimization CompilerOptions::get_optimization_option()
{
  return optOption;
}