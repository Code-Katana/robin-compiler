#include "jsonrpc/utils/stringify.h"

string json::utils::stringify(const rbn::core::Token *token)
{
  return json::encode(convert_token(token));
}

string json::utils::stringify(const vector<rbn::core::Token> *tokens)
{
  return json::encode(convert_token_stream(tokens));
}
