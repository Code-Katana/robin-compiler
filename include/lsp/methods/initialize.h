#pragma once

#include <optional>
#include <string>

#include "jsonrpc/protocol/messages.h"

using namespace std;

namespace lsp::methods
{
  rpc::ResponseMessage *initialize(rpc::RequestMessage *request);
}