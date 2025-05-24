#pragma once

#include <string>
#include <fstream>

#include "jsonrpc/core/values.h"

namespace json
{
  string encode(Json *value);
  void encode_to_file(Json *value, string path);
}
