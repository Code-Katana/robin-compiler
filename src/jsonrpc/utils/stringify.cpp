#include "jsonrpc/utils/stringify.h"

string json::utils::stringify(const rbn::core::Token &token)
{
  json::Object *obj = new json::Object();

  obj->add("type", new json::String(rbn::core::Token::get_token_name(token.type)));
  obj->add("value", new json::String(token.value));
  obj->add("line", new json::Number(token.line));
  obj->add("start", new json::Number(token.start));
  obj->add("end", new json::Number(token.end));

  return json::encode(obj);
}

string json::utils::stringify(const vector<rbn::core::Token> &tokens)
{
  json::Array *stream = new json::Array();

  for (const auto &token : tokens)
  {
    json::Object *obj = new json::Object();

    obj->add("type", new json::String(rbn::core::Token::get_token_name(token.type)));
    obj->add("value", new json::String(token.value));
    obj->add("line", new json::Number(token.line));
    obj->add("start", new json::Number(token.start));
    obj->add("end", new json::Number(token.end));

    stream->add(obj);
  }

  return json::encode(stream);
}
