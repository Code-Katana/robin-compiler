#pragma once

#include <string>
#include "code_optimization.h"
using namespace std;

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

class CompilerOptions
{
public:
  CompilerOptions(const string &prog);
  CompilerOptions(const string &prog, const CodeOptimization &opt);
  CompilerOptions(const string &prog, const ParserOptions &pr);
  CompilerOptions(const string &prog, const ScannerOptions &sc);
  CompilerOptions(const string &prog, const ScannerOptions &sc, const ParserOptions &pr);
  CompilerOptions(const string &prog, const ScannerOptions &sc, const ParserOptions &pr, const CodeOptimization &opt);

  string get_program();
  ScannerOptions get_scanner_option();
  ParserOptions get_parser_option();
  CodeOptimization get_optimization_option();

private:
  string program;
  ScannerOptions scOption;
  ParserOptions prOption;
  CodeOptimization optOption;
};
