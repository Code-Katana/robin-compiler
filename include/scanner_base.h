#pragma once

#include <string>
#include <vector>

#include "token.h"

using namespace std;

class ScannerBase
{
public:
  Token check_reserved(string s);
  void reset_scanner();
  virtual Token get_token() = 0;
  virtual vector<Token> get_tokens_stream() = 0;

protected:
  ScannerBase(string src);
  virtual ~ScannerBase() = default;
  char eat();
  char peek();
  bool expect(char expected);
  bool is_eof();
  int update_line_count();
  Token create_token(string val, TokenType type);
  // TODO: Create a `lexical_error` function, here some more details
  /** the function should not stop the program but stop the scanner itself
   * every place you type 'error_token = something;' you should replace this code with 'return lexical_error("the error message");'
   * if you look at the fa_scanner.cpp or the handcoded_scanner.cpp file you will find more details
   */

  char ch;
  int curr;
  string str;
  string source;
  int line_count;
  int token_start;
  int token_end;
  Token error_token;
};
