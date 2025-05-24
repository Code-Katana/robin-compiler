#include "jsonrpc/transport/sending.h"

namespace rpc
{
  void create_message(const string &json_message)
  {
    cout << Headers::CONTENT_LENGTH << ": " << to_string(json_message.length()) << "\r\n\r\n"
         << json_message;
  }

  void send(Message *message)
  {
    create_message(message->get_json());
  }

  void send(RequestMessage *message)
  {
    create_message(message->get_json());
  }

  void send(NotificationMessage *message)
  {
    create_message(message->get_json());
  }

  void send(variant<RequestMessage *, NotificationMessage *> unknown)
  {
    Message *message;

    if (holds_alternative<RequestMessage *>(unknown))
    {
      message = get<RequestMessage *>(unknown);
    }
    else
    {
      message = get<NotificationMessage *>(unknown);
    }

    create_message(message->get_json());
    delete message;
  }

  void send(ResponseMessage *message)
  {
    create_message(message->get_json());
  }
}
