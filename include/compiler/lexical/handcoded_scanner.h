#pragma once

#include "compiler/lexical/scanner_base.h"

class HandCodedScanner : public ScannerBase
{
public:
  HandCodedScanner(string src);

  Token get_token();
  vector<Token> get_tokens_stream();
};