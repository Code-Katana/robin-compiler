#pragma once

#include <string>
#include <vector>

#include "token.h"

using namespace std;

class JSON
{
public:
  static string stringify_token(Token tk);
  static string stringify_tokens_stream(vector<Token> tokens);
};