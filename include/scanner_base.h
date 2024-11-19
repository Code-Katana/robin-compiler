#pragma once

#include <string>
#include <vector>

#include "token.h"

using namespace std;

class ScannerBase
{
public:
  Token check_reserved(string s);
  virtual Token get_token() = 0;
  virtual vector<Token> get_tokens_stream() = 0;

protected:
  ScannerBase(string src);
  virtual ~ScannerBase() = default;
  char eat();
  char peek();
  bool expect(char expected);
  bool is_eof();
  Token create_token(string val, TokenType type, int l = 0, int s = 0, int e = 0);

  char ch;
  int curr;
  string str;
  string source;
  Token error_token;
};
