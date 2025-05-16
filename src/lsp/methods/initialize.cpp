#include "lsp/methods/initialize.h"

namespace lsp::methods
{
  struct ClientInfo
  {
    string name;
    string version;
  };

  struct InitializeParams
  {
    optional<int> process_id;
    ClientInfo client;
  };

  InitializeParams *get_initialize_params(json::Object *params)
  {
    if (!params->has("clientInfo"))
    {
      return nullptr;
    }

    json::Object *client_info = (json::Object *)params->get("clientInfo");

    optional<int> pid;

    if (params->has("processId") && params->get("processId")->type == json::ValueType::NUMBER)
    {
      pid = params->get_number("processId")->as_integer();
    }

    if (!client_info->has("name") || !client_info->has("version") ||
        client_info->get("name")->type != json::ValueType::STRING ||
        client_info->get("version")->type != json::ValueType::STRING)
    {
      return nullptr;
    }

    return new InitializeParams{
        pid,
        client_info->get_string("name")->value,
        client_info->get_string("version")->value,
    };
  }

  rpc::ResponseMessage *initialize(rpc::RequestMessage *request)
  {
    InitializeParams *params = get_initialize_params((json::Object *)request->params);

    if (params == nullptr)
    {
      return nullptr;
    }

    json::Object *server_info = new json::Object();
    server_info->add("name", new json::String("robinlsp"));
    server_info->add("version", new json::String("0.0.1"));

    json::Object *result = new json::Object();
    result->add("capabilities", new json::Object);
    result->add("serverInfo", server_info);

    return new rpc::ResponseMessage(request->id, result);
  }
}