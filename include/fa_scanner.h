#pragma once

#include "scanner_base.h"

class FAScanner : ScannerBase
{
public:
  FAScanner(string src);
  Token get_token();
  vector<Token> get_tokens_stream();
};