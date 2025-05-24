#pragma once

#include <iostream>
#include <string>
#include <variant>

#include "jsonrpc/protocol/constants.h"
#include "jsonrpc/protocol/messages.h"

using namespace std;

namespace rpc
{
  void send(Message *message);
  void send(RequestMessage *message);
  void send(NotificationMessage *message);
  void send(variant<RequestMessage *, NotificationMessage *> message);
  void send(ResponseMessage *message);
}
