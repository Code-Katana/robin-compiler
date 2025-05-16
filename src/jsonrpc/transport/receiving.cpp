#include "jsonrpc/transport/receiving.h"

namespace rpc
{
  variant<RequestMessage *, NotificationMessage *> receive(const string &message)
  {
    json::Object *obj = (json::Object *)json::decode(message);

    if (obj->has("id"))
    {
      return new rpc::RequestMessage(
          obj->get_string("jsonrpc")->value,
          obj->get_number("id")->as_integer(),
          lsp::get_method(obj->get_string("method")->value),
          (json::Json *)obj->get("params"));
    }
    else
    {
      return new rpc::NotificationMessage(
          obj->get_string("jsonrpc")->value,
          lsp::get_method(obj->get_string("method")->value),
          (json::Json *)obj->get("params"));
    }
  }
}