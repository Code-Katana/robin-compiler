#include "jsonrpc/transport/receiving.h"

namespace rpc
{
  // constants for the finit automaton state
  const int start_state = 0;
  const int final_state = 9;
  const int error_state = 8;

  // finit automaton scanner
  map<string, string> parse_headers(const string &headers)
  {
    map<string, string> parsed_headers;
    string header_name = "", header_value = "";
    int i = 0, current_state = start_state;

    while (current_state >= start_state && current_state < final_state)
    {
      switch (current_state)
      {
      case 0:
        if (isalpha(headers[i]))
          current_state = 1;
        else
          current_state = final_state;
        break;
      case 1:
        if (isalpha(headers[i]) || headers[i] == '-')
          header_name += headers[i++];
        else if (headers[i] == ':' && headers[i + 1] == ' ')
          current_state = 2;
        else
          current_state = error_state;
        break;
      case 2:
        i += 2;
        current_state = 3;
        break;
      case 3:
        if (isdigit(headers[i]))
          current_state = 4;
        else if (headers[i] == '\"')
          current_state = 5;
        break;
      case 4:
        if (isdigit(headers[i]))
          header_value += headers[i++];
        else if (headers[i] == '\r' && headers[i + 1] == '\n')
          current_state = 7;
        else
          current_state = error_state;
        break;
      case 5:
        if (headers[i] == '\"')
          current_state = 6;
        else
          header_value += headers[i++];
        break;
      case 6:
        if (headers[i] == '\r' && headers[i + 1] == '\n')
          current_state = 7;
        else
          header_value += headers[i++];
        break;
      case 7:
        parsed_headers.insert(pair<string, string>(header_name, header_value));
        current_state = 0;
        break;
      case 8: // error state
        return map<string, string>();
      }
    }

    return parsed_headers;
  }

  // public interfaces
  variant<RequestMessage *, NotificationMessage *> receive(const string &message)
  {
    json::Object *obj = (json::Object *)json::decode(message);

    if (obj->has("id"))
    {
      return new rpc::RequestMessage(
          obj->get_string("jsonrpc")->value,
          obj->get_number("id")->as_integer(),
          lsp::get_method(obj->get_string("method")->value),
          (json::Json *)obj->get("params"));
    }
    else
    {
      return new rpc::NotificationMessage(
          obj->get_string("jsonrpc")->value,
          lsp::get_method(obj->get_string("method")->value),
          (json::Json *)obj->get("params"));
    }
  }

  int get_content_length(const string &headers)
  {
    map<string, string> parsed_headers = parse_headers(headers);

    if (parsed_headers.find(Headers::CONTENT_LENGTH) == parsed_headers.end())
    {
      return -1;
    }

    return stoi(parsed_headers.at(Headers::CONTENT_LENGTH));
  }
}
