#pragma once

#include "scanner_base.h"

// todo: remove before commit
#include <iostream>
using namespace std;

class HandCodedScanner : ScannerBase
{
public:
  HandCodedScanner(string src);

  char eat();
  char peek();
  bool expect(char expected);
  bool is_eof();

  Token get_token();
  vector<Token> get_tokens_stream();
};