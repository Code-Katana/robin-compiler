#pragma once

#include <string>
#include <vector>

#include "token.h"

using namespace std;

class ScannerBase
{
public:
  Token check_reserved(string s);

  char eat();
  char peek();
  bool expect(char expected);
  bool is_eof();

  virtual Token get_token() = 0;
  virtual vector<Token> get_tokens_stream() = 0;

protected:
  char ch;
  int curr;
  string str;
  string source;
  Token error_token;
  // vector<Token> tokens;

  ScannerBase(string src);
};
