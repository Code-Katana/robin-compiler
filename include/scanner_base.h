#pragma once

#include <string>
#include <vector>

#include "token.h"

using namespace std;

class ScannerBase
{
public:
  Token check_reserved(string s);

  virtual char eat() = 0;
  virtual char peek() = 0;
  virtual bool expect(char expected) = 0;
  virtual bool is_eof() = 0;

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
