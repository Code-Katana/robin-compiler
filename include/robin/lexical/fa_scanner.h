#pragma once

#include "robin/lexical/scanner_base.h"

namespace rbn::lexical
{

  class FAScanner : public ScannerBase
  {
  public:
    FAScanner(string src);

    core::Token get_token();
    vector<core::Token> get_tokens_stream();

  protected:
    const int START_STATE = 0;
    const int END_STATE = 50;
  };
}