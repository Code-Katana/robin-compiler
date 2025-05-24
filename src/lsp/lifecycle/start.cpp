#include "lsp/lifecycle/start.h"

namespace lsp::lifecycle
{
  bool is_running = true;

  void start_lsp()
  {
    string line;

    while (is_running)
    {
      getline(cin, line);

      if (line.empty() || line.find("Content-Length: ") != 0)
        continue;

      size_t content_length = 0;
      content_length = stoi(line.substr(16));

      getline(cin, line);
      if (!line.empty())
        continue;

      string message;
      message.resize(content_length);
      cin.read(&message[0], content_length);

      // Parse and handle the message
      auto json_message = rpc::receive(message);
      handle_message(json_message);
    }
  }

  void handle_message(variant<rpc::RequestMessage *, rpc::NotificationMessage *> message)
  {
    if (holds_alternative<rpc::NotificationMessage *>(message))
    {
      handle_notification(get<rpc::NotificationMessage *>(message));
    }
    else
    {
      rpc::ResponseMessage *response = handle_request(get<rpc::RequestMessage *>(message));

      if (response == nullptr)
        return;

      rpc::send(response);
    }
  }

  void handle_notification(rpc::NotificationMessage *notification) { return; }

  rpc::ResponseMessage *handle_request(rpc::RequestMessage *request)
  {
    switch (request->method)
    {
    case MethodType::Initialize:
      return methods::initialize(request);

    case MethodType::Tokenize:
      return methods::compiler_action_tokenize(request);

    case MethodType::ParseAst:
    case MethodType::Unknown:
      return nullptr;
    }
  }
}