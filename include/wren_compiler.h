#pragma once

#include <string>

#include "compiler_components.h"
#include "scanner_base.h"
#include "handcoded_scanner.h"
#include "fa_scanner.h"

class WrenCompiler
{
public:
  WrenCompiler(string src, ScannerOptions scOpt);
  virtual ~WrenCompiler() = default;

  ScannerBase *scanner;
};