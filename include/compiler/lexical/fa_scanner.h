#pragma once

#include "compiler/lexical/scanner_base.h"

class FAScanner : public ScannerBase
{
public:
  FAScanner(string src);

  Token get_token();
  vector<Token> get_tokens_stream();

protected:
  const int START_STATE = 0;
  const int END_STATE = 50;
};