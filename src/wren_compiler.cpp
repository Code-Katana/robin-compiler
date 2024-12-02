#include "wren_compiler.h"

WrenCompiler::WrenCompiler(string src, ScannerOptions scOpt, ParserOptions prOpt)
{
  symboltable = new SymbolTable();
  
  switch (scOpt)
  {
  case ScannerOptions::HandCoded:
    scanner = new HandCodedScanner(src);
    break;
  case ScannerOptions::FA:
    scanner = new FAScanner(src);
  }
  switch (prOpt)
  {
  case ParserOptions::RecursiveDecent:
    parser = new RecursiveDecentParser(scanner, symboltable);
    break;
  }
}