#pragma once

#include <string>
#include <fstream>

#include "jsonrpc/core/values.h"

namespace json
{
  Value *decode(const string &str);
  Value *decode_from_file(const string &path);
}
