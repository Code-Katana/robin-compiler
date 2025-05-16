#pragma once

#include <string>
#include <variant>
#include <optional>

#include "jsonrpc/protocol/constants.h"
#include "jsonrpc/core/encoding.h"
#include "lsp/core/methods.h"

using namespace std;

namespace rpc
{
  // plain message
  class Message
  {
  public:
    Message();
    Message(string json_rpc);
    virtual ~Message() = default;

    virtual json::Object *get_message();
    virtual string get_json();

    string json_rpc;
  };

  // notification message
  class NotificationMessage : public Message
  {
  public:
    NotificationMessage(lsp::MethodType method, json::Json *params);
    NotificationMessage(string json_rpc, lsp::MethodType method, json::Json *params);
    virtual ~NotificationMessage() = default;

    json::Object *get_message();
    string get_json();

    lsp::MethodType method;
    json::Json *params;
  };

  // request message
  class RequestMessage : public Message
  {
  public:
    RequestMessage(int id, lsp::MethodType method, json::Json *params);
    RequestMessage(string json_rpc, int id, lsp::MethodType method, json::Json *params);
    virtual ~RequestMessage() = default;

    json::Number *get_id();
    json::Object *get_message();
    string get_json();

    int id;
    lsp::MethodType method;
    json::Json *params;
  };

  // response error
  class ResponseError
  {
  public:
    ResponseError(ErrorCode code, string message);
    ResponseError(ErrorCode code, string message, json::Value *data);
    virtual ~ResponseError() = default;

    json::Object *get_error();
    string get_json();

    ErrorCode code;
    string message;
    optional<json::Value *> data;
  };

  // response message
  class ResponseMessage : public Message
  {
  public:
    ResponseMessage(json::Value *result);
    ResponseMessage(optional<int> id, optional<json::Value *> result);
    ResponseMessage(optional<int> id, optional<json::Value *> result, optional<ResponseError *> error);
    ResponseMessage(string json_rpc, optional<int> id, optional<json::Value *> result, optional<ResponseError *> error);
    virtual ~ResponseMessage() = default;

    json::Value *get_id();
    json::Object *get_message();
    string get_json();

    optional<int> id; // number | null
    optional<json::Value *> result;
    optional<ResponseError *> error;
  };
}
