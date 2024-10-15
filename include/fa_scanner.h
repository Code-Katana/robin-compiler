#pragma once
#include "scanner_base.h"
#include <iostream>
using namespace std;

class FAScanner : ScannerBase
{
public:
  FAScanner(string src);
  Token get_token();
  void display_tokens();
};