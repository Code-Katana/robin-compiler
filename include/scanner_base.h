#pragma once
#include "token.h"
#include <string>
#include <vector>
using namespace std;


class ScannerBase
{
public:
  Token check_reserved(string s);
  virtual Token get_token() = 0;
  virtual void display_tokens() = 0;

protected:
  string source;
  int curr;
  char ch;
  string str;
  vector<Token> tokens;

  ScannerBase(string src);
};