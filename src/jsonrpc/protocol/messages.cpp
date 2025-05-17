#include "jsonrpc/protocol/messages.h"

namespace rpc
{
  // plain message implementation
  Message::Message() : json_rpc(JSON_RPC_VERSION) {}
  Message::Message(string json_rpc) : json_rpc(json_rpc) {}

  json::Object *Message::get_message()
  {
    json::Object *msg = new json::Object();
    msg->add("jsonrpc", new json::String(JSON_RPC_VERSION));
    return msg;
  }

  string Message::get_json()
  {
    json::Object *msg = get_message();
    return json::encode(msg);
  }

  // notification message implementation
  NotificationMessage::NotificationMessage(lsp::MethodType method, json::Json *params) : Message(), method(method), params(params) {}
  NotificationMessage::NotificationMessage(string json_rpc, lsp::MethodType method, json::Json *params) : Message(json_rpc), method(method), params(params) {}

  json::Object *NotificationMessage::get_message()
  {
    json::Object *msg = new json::Object();

    msg->add("jsonrpc", new json::String(json_rpc));
    msg->add("method", new json::String(lsp::get_method_name(method)));
    msg->add("params", params);

    return msg;
  }

  string NotificationMessage::get_json()
  {
    json::Object *msg = get_message();
    return json::encode(msg);
  }

  // request message implementation
  RequestMessage::RequestMessage(int id, lsp::MethodType method, json::Json *params) : Message(), id(id), method(method), params(params) {}
  RequestMessage::RequestMessage(string json_rpc, int id, lsp::MethodType method, json::Json *params) : Message(json_rpc), id(id), method(method), params(params) {}

  json::Integer *RequestMessage::get_id()
  {
    return new json::Integer(id);
  }

  json::Object *RequestMessage::get_message()
  {
    json::Object *msg = new json::Object();

    msg->add("jsonrpc", new json::String(json_rpc));
    msg->add("id", get_id());
    msg->add("method", new json::String(lsp::get_method_name(method)));
    msg->add("params", params);

    return msg;
  }

  string RequestMessage::get_json()
  {
    json::Object *msg = get_message();
    return json::encode(msg);
  }

  // response error implementation
  ResponseError::ResponseError(ErrorCode code, string message) : code(code), message(message), data(nullopt) {}
  ResponseError::ResponseError(ErrorCode code, string message, json::Value *data) : code(code), message(message), data(data) {}

  json::Object *ResponseError::get_error()
  {
    json::Object *msg = new json::Object();

    msg->add("code", new json::Integer((int)code));
    msg->add("message", new json::String(message));

    if (data.has_value())
    {
      msg->add("data", data.value());
    }

    return msg;
  }

  string ResponseError::get_json()
  {
    json::Object *msg = get_error();
    return json::encode(msg);
  }

  // response message implementation
  ResponseMessage::ResponseMessage(json::Value *result) : Message(), id(nullopt), result(result), error(nullopt) {}
  ResponseMessage::ResponseMessage(optional<int> id, optional<json::Value *> result) : Message(), id(id), result(result), error(nullopt) {}
  ResponseMessage::ResponseMessage(optional<int> id, optional<json::Value *> result, optional<ResponseError *> error) : Message(), id(id), result(result), error(error) {}
  ResponseMessage::ResponseMessage(string json_rpc, optional<int> id, optional<json::Value *> result, optional<ResponseError *> error) : Message(json_rpc), id(id), result(result), error(error) {}

  json::Value *ResponseMessage::get_id()
  {
    return (id.has_value())
               ? (json::Value *)new json::Integer(id.value())
               : (json::Value *)new json::Null();
  }

  json::Object *ResponseMessage::get_message()
  {
    json::Object *msg = new json::Object();

    msg->add("jsonrpc", new json::String(json_rpc));
    msg->add("id", get_id());

    if (result.has_value() && result.value())
    {
      msg->add("result", result.value());
    }

    if (error.has_value() && error.value())
    {
      msg->add("error", error.value()->get_error());
    }

    return msg;
  }

  string ResponseMessage::get_json()
  {
    json::Object *msg = get_message();
    return json::encode(msg);
  }
}
