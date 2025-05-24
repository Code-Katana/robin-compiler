#pragma once

#include <string>
#include <variant>
#include <map>

#include "jsonrpc/protocol/constants.h"
#include "jsonrpc/protocol/messages.h"
#include "jsonrpc/core/decoding.h"

using namespace std;

namespace rpc
{
  variant<RequestMessage *, NotificationMessage *> receive(const string &message);
  int get_content_length(const string &headers);
}