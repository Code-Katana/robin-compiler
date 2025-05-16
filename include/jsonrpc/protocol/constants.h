#pragma once

#include <string>

namespace rpc
{
  const std::string JSON_RPC_VERSION = "2.0";

  enum class ErrorCode
  {
    TEST_ERROR = 123456,
  };

  namespace Headers
  {
    const std::string CONTENT_LENGTH = "Content-Length";
    const std::string CONTENT_TYPE = "Content-Type";
  };
}
