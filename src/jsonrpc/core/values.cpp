#include "jsonrpc/core/values.h"

namespace json
{
  // Base Value implementation
  Value::Value(ValueType type) : type(type) {}

  // Json Value implementation
  Json::Json(ValueType type) : Value(type) {}

  // Null Value implementation
  Null::Null() : Value(ValueType::NULL_VALUE) {}

  // Boolean Value implementation
  Boolean::Boolean(const bool &value) : Value(ValueType::BOOLEAN), value(value) {}

  Boolean::Boolean(const string &value) : Value(ValueType::BOOLEAN)
  {
    this->value = value == keywords::TRUE_KEYWORD;
  }

  // Number Value implementation
  Number::Number(const double &value) : Value(ValueType::NUMBER), value(value) {}
  Number::Number(const string &value) : Value(ValueType::NUMBER), value(stod(value)) {}

  int Number::as_integer()
  {
    return (int)this->value;
  }

  // String Value implementation
  String::String(const string &value) : Value(ValueType::STRING), value(value) {}

  // Array Value implementation
  Array::Array() : Json(ValueType::ARRAY), length(0) {}

  Array::~Array()
  {
    for (auto value : values)
    {
      delete value;
    }
  }

  Array::Array(const Array &other) : Json(ValueType::ARRAY)
  {
    for (const auto &value : other.values)
    {
      this->values.push_back(value);
    }
    this->length = other.length;
  }

  Array &Array::operator=(const Array &other)
  {
    if (this != &other)
    {
      this->values = other.values;
    }
    this->length = other.length;
    return *this;
  }

  Value *Array::add(Value *value)
  {
    this->values.push_back(value);
    this->length++;
    return value;
  }

  Value *Array::get(int index)
  {
    return this->values[index];
  }

  Value *Array::remove(int index)
  {
    Value *value = this->values[index];
    this->values.erase(this->values.begin() + index);
    this->length--;
    return value;
  }

  // Object Value implementation
  Object::Object() : Json(ValueType::OBJECT) {}

  Object::~Object()
  {
    for (auto pair : props)
    {
      delete pair.second;
    }
  }

  Object::Object(const Object &other) : Json(ValueType::OBJECT)
  {
    for (const auto &pair : other.props)
    {
      this->props[pair.first] = pair.second;
    }
  }

  Object &Object::operator=(const Object &other)
  {
    for (const auto &pair : other.props)
    {
      this->props[pair.first] = pair.second;
    }
    return *this;
  }

  void Object::add(const string &key, Value *value)
  {
    this->props[key] = value;
  }

  Value *Object::get(const string &key)
  {
    return this->props[key];
  }

  String *Object::get_string(const string &key)
  {
    Value *value = this->props[key];
    if (value && value->type == ValueType::STRING)
    {
      return static_cast<String *>(value);
    }
    return nullptr;
  }

  Number *Object::get_number(const string &key)
  {
    Value *value = this->props[key];
    if (value && value->type == ValueType::NUMBER)
    {
      return static_cast<Number *>(value);
    }
    return nullptr;
  }

  Boolean *Object::get_boolean(const string &key)
  {
    Value *value = this->props[key];
    if (value && value->type == ValueType::BOOLEAN)
    {
      return static_cast<Boolean *>(value);
    }
    return nullptr;
  }

  Array *Object::get_array(const string &key)
  {
    Value *value = this->props[key];
    if (value && value->type == ValueType::ARRAY)
    {
      return static_cast<Array *>(value);
    }
    return nullptr;
  }

  Object *Object::get_object(const string &key)
  {
    Value *value = this->props[key];
    if (value && value->type == ValueType::OBJECT)
    {
      return static_cast<Object *>(value);
    }
    return nullptr;
  }

  Null *Object::get_null(const string &key)
  {
    Value *value = this->props[key];
    if (value && value->type == ValueType::NULL_VALUE)
    {
      return static_cast<Null *>(value);
    }
    return nullptr;
  }

  Value *Object::set(const string &key, Value *value)
  {
    this->props[key] = value;
    return value;
  }

  void Object::remove(const string &key)
  {
    this->props.erase(key);
  }

  bool Object::has(const string &key)
  {
    return this->props.find(key) != this->props.end();
  }

  vector<string> Object::keys()
  {
    vector<string> keys;
    for (const auto &pair : this->props)
    {
      keys.push_back(pair.first);
    }
    return keys;
  }
}
