#pragma once

#include <string>

#include "compiler_components.h"

#include "scanner_base.h"
#include "handcoded_scanner.h"
#include "fa_scanner.h"

#include "parser_base.h"
#include "recursive_decent_parser.h"

#include "symbol_table.h"

class WrenCompiler
{
public:
  WrenCompiler(string src, ScannerOptions scOpt, ParserOptions prOpt);
  virtual ~WrenCompiler() = default;

  ScannerBase *scanner;
  ParserBase *parser;
  SymbolTable *symboltable;
};