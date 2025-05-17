#pragma once

#include <string>

#include "robin/core/options.h"

#include "robin/lexical/scanner_base.h"
#include "robin/lexical/handcoded_scanner.h"
#include "robin/lexical/fa_scanner.h"

#include "jsonrpc/protocol/messages.h"

#include "utils/file_reader.h"

using namespace std;

namespace lsp::methods
{
  rpc::ResponseMessage *compiler_action_tokenize(rpc::RequestMessage *request);
}