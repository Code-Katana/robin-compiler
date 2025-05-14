#pragma once

#include "robin/lexical/scanner_base.h"
namespace rbn::lexical
{
  class HandCodedScanner : public ScannerBase
  {
  public:
    HandCodedScanner(string src);

    core::Token get_token();
    vector<core::Token> get_tokens_stream();
  };
}