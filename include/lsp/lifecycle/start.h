#pragma once

#include <iostream>
#include <string>

#include "jsonrpc/protocol/messages.h"
#include "jsonrpc/transport/receiving.h"
#include "jsonrpc/transport/sending.h"

#include "lsp/methods/initialize.h"

using namespace std;

namespace lsp::lifecycle
{
  void start_lsp();
  void handle_message(variant<rpc::RequestMessage *, rpc::NotificationMessage *> message);
  void handle_notification(rpc::NotificationMessage *notification);
  rpc::ResponseMessage *handle_request(rpc::RequestMessage *request);
}