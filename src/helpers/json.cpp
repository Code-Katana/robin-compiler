#include "json.h"

string JSON::stringify_token(Token tk)
{
  return "{ \\\"type\\\": \\\"" + Token::get_token_name(tk.type) +
         "\\\", \\\"value\\\": \\\"" + tk.value + "\\\" }";
}

string JSON::stringify_tokens_stream(vector<Token> tokens)
{
  string stream = "[" + JSON::stringify_token(tokens.front());

  for (int i = 1; i < tokens.size(); ++i)
  {
    stream += "," + JSON::stringify_token(tokens[i]);
  }

  return "\b" + stream + "]";
}