#pragma once

#include "scanner_base.h"

class HandCodedScanner : ScannerBase
{
public:
  HandCodedScanner(string src);

  Token get_token();
  vector<Token> get_tokens_stream();
};