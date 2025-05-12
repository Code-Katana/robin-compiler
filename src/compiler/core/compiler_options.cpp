#include "compiler/core/compiler_options.h"

CompilerOptions::CompilerOptions(const string &prog) : program(prog), scOption(ScannerOptions::FiniteAutomaton), prOption(ParserOptions::RecursiveDecent), optOption(OptLevel::O0) {}

CompilerOptions::CompilerOptions(const string &prog, const OptLevel &opt) : program(prog), scOption(ScannerOptions::FiniteAutomaton), prOption(ParserOptions::RecursiveDecent), optOption(opt) {}

CompilerOptions::CompilerOptions(const string &prog, const ParserOptions &pr) : program(prog), scOption(ScannerOptions::FiniteAutomaton), prOption(pr), optOption(OptLevel::O0) {}

CompilerOptions::CompilerOptions(const string &prog, const ScannerOptions &sc) : program(prog), scOption(sc), prOption(ParserOptions::RecursiveDecent), optOption(OptLevel::O0) {}

CompilerOptions::CompilerOptions(const string &prog, const ScannerOptions &sc, const ParserOptions &pr) : program(prog), scOption(sc), prOption(pr), optOption(OptLevel::O0) {}

CompilerOptions::CompilerOptions(const string &prog, const ScannerOptions &sc, const ParserOptions &pr, const OptLevel &opt) : program(prog), scOption(sc), prOption(pr), optOption(opt) {}

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
OptLevel CompilerOptions::get_optimization_option()
{
  return optOption;
}