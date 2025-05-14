#pragma once

#include <string>
#include <vector>

#include "robin/core/token.h"

using namespace std;

namespace rbn::lexical
{
  class ScannerBase
  {
  public:
    core::Token check_reserved(string s);
    void reset_scanner();
    core::Token *get_error();
    virtual core::Token get_token() = 0;
    virtual vector<core::Token> get_tokens_stream() = 0;

  protected:
    ScannerBase(string src);
    virtual ~ScannerBase() = default;
    char eat();
    char peek();
    bool expect(char expected);
    bool is_eof();
    int update_line_count();
    core::Token create_token(string val, core::TokenType type);
    core::Token lexical_error(string message);

    char ch;
    int curr;
    string str;
    string source;
    int line_count;
    int token_start;
    int token_end;
    core::Token error_token;
  };
}