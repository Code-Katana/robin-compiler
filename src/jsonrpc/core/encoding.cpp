#include "jsonrpc/core/encoding.h"

namespace json
{
  class Encoder
  {
  public:
    static string stringify(Json *val);

  private:
    Encoder();

    // decoding functions
    static string stringify_value(Value *arr);
    static string stringify_object(Object *obj);
    static string stringify_array(Array *arr);

    // helper functions
    static string quote(string s);
  };

  // decoding functions
  string Encoder::stringify(Json *val)
  {
    return stringify_value(val);
  }

  string Encoder::stringify_value(Value *value)
  {
    switch (value->type)
    {
    case ValueType::OBJECT:
      return stringify_object(static_cast<Object *>(value));

    case ValueType::ARRAY:
      return stringify_array(static_cast<Array *>(value));

    case ValueType::STRING:
      return quote(static_cast<String *>(value)->value);

    case ValueType::BOOLEAN:
      return static_cast<Boolean *>(value)->value ? keywords::TRUE_KEYWORD : keywords::FALSE_KEYWORD;

    case ValueType::NUMBER:
      return to_string(static_cast<Number *>(value)->value);

    case ValueType::INTEGER:
      return to_string(static_cast<Integer *>(value)->value);

    case ValueType::NULL_VALUE:
      return keywords::NULL_KEYWORD;
    default:
      throw invalid_argument("Invalid value type.");
    }
  }

  string Encoder::stringify_object(Object *obj)
  {
    string stringified_json = "{";
    vector<string> keys = obj->keys();

    for (auto key : keys)
    {
      stringified_json += quote(key) + ": " + stringify_value(obj->props.at(key));

      if (key != keys.back())
      {
        stringified_json += ',';
      }
    }

    stringified_json += "}";
    return stringified_json;
  }

  string Encoder::stringify_array(Array *arr)
  {
    string stringified_json = "[";

    for (auto value : arr->values)
    {
      stringified_json += stringify_value(value);

      if (value != arr->values.back())
      {
        stringified_json += ", ";
      }
    }

    stringified_json += ']';
    return stringified_json;
  }

  // helper functions
  string Encoder::quote(string s)
  {
    return "\"" + s + "\"";
  }

  // the public interface
  string encode(Json *value)
  {
    return Encoder::stringify(value);
  }

  void encode_to_file(Json *value, string path)
  {
    ofstream file(path);
    file << encode(value);
    file.close();
  }
}
