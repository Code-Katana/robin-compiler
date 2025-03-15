#include "compiler_options.h"

CompilerOptions::CompilerOptions(const string &prog) : program(prog), scOption(ScannerOptions::FiniteAutomaton), prOption(ParserOptions::RecursiveDecent) {}

CompilerOptions::CompilerOptions(const string &prog, const ParserOptions &pr) : program(prog), scOption(ScannerOptions::FiniteAutomaton), prOption(pr) {}

CompilerOptions::CompilerOptions(const string &prog, const ScannerOptions &sc) : program(prog), scOption(sc), prOption(ParserOptions::RecursiveDecent) {}

CompilerOptions::CompilerOptions(const string &prog, const ScannerOptions &sc, const ParserOptions &pr) : program(prog), scOption(sc), prOption(pr) {}

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