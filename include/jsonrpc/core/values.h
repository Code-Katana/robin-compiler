#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <map>

#include "jsonrpc/core/keywords.h"

using namespace std;

namespace json
{
  enum class ValueType
  {
    OBJECT,
    ARRAY,
    STRING,
    NUMBER,
    INTEGER,
    BOOLEAN,
    NULL_VALUE,
  };

  // Base Value class
  class Value
  {
  public:
    Value(ValueType type);
    virtual ~Value() = default;

    ValueType type;
  };

  // Json Value class (e.g. array | object)
  class Json : public Value
  {
  public:
    Json(ValueType type);
    virtual ~Json() = default;
  };

  // Null Value class
  class Null : public Value
  {
  public:
    Null();
    virtual ~Null() = default;

    const string value = keywords::NULL_KEYWORD;
  };

  // Boolean Value class
  class Boolean : public Value
  {
  public:
    Boolean(const bool &value);
    Boolean(const string &value);
    virtual ~Boolean() = default;

    bool value;
  };

  // Number Value class
  class Number : public Value
  {
  public:
    Number(const double &value);
    Number(const string &value);
    virtual ~Number() = default;

    int as_integer();

    double value;
  };

  // Integer values class
  class Integer : public Value
  {
  public:
    Integer(const int &value);
    Integer(const string &value);
    virtual ~Integer() = default;

    int value;
  };

  // String Value class
  class String : public Value
  {
  public:
    String(const string &value);
    virtual ~String() = default;

    string value;
  };

  // Array Value class (inherits from Json)
  class Array : public Json
  {
  public:
    Array();
    Array(const Array &other);
    virtual ~Array() override;

    Array &operator=(const Array &other);

    Value *add(Value *value);
    Value *get(int index);
    Value *remove(int index);

    vector<Value *> values;
    int length;
  };

  // Object Value class (inherits from Json)
  class Object : public Json
  {
  public:
    Object();
    Object(const Object &other);
    virtual ~Object() override;

    Object &operator=(const Object &other);

    void add(const string &key, Value *value);
    void remove(const string &key);

    Value *get(const string &key);
    Value *set(const string &key, Value *value);
    bool has(const string &key);
    vector<string> keys();

    String *get_string(const string &key);
    Number *get_number(const string &key);
    Integer *get_integer(const string &key);
    Boolean *get_boolean(const string &key);
    Array *get_array(const string &key);
    Object *get_object(const string &key);
    Null *get_null(const string &key);

    map<string, Value *> props;
  };
}