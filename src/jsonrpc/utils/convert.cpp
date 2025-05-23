#include "jsonrpc/utils/convert.h"

namespace json::utils
{
  Object *convert_token(const rbn::core::Token *token)
  {
    Object *obj = new Object();

    obj->add("type", new String(rbn::core::Token::get_token_name(token->type)));
    obj->add("value", new String(token->value));
    obj->add("line", new Number(token->line));
    obj->add("start", new Number(token->start));
    obj->add("end", new Number(token->end));

    return obj;
  }

  Array *convert_token_stream(const vector<rbn::core::Token> *tokens)
  {
    Array *stream = new Array();

    for (const auto &token : *tokens)
    {
      Object *obj = new Object();

      obj->add("type", new String(rbn::core::Token::get_token_name(token.type)));
      obj->add("value", new String(token.value));
      obj->add("line", new Number(token.line));
      obj->add("start", new Number(token.start));
      obj->add("end", new Number(token.end));

      stream->add(obj);
    }

    return stream;
  }
}