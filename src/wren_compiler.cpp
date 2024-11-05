#include "wren_compiler.h"

WrenCompiler::WrenCompiler(string src, ScannerOptions scOpt)
{
  switch (scOpt)
  {
  case ScannerOptions::HandCoded:
    scanner = new HandCodedScanner(src);
    break;
  case ScannerOptions::FA:
    scanner = new FAScanner(src);
  }
}