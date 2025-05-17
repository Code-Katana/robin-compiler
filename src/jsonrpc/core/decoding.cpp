#include "jsonrpc/core/decoding.h"

namespace json
{
  // Decoder Class
  class Decoder
  {
  public:
    static Json *parse(const string &str);

  private:
    Decoder();

    // parse functions
    static Value *parse_value();
    static Json *parse_json();
    static Object *parse_object();
    static Array *parse_array();
    static String *parse_string();
    static Value *parse_numeric();
    static Value *parse_keyword();

    // helper functions
    static char peek();
    static char consume();
    static string slice(int s, int e);
    static bool is_keyword(const string &str);
    static int get_length();
    static void skip_whitespace();

    static string *source;
    static int current_index;
  };

  // members
  int Decoder::current_index = 0;
  string *Decoder::source = new string();

  // helpers
  char Decoder::peek()
  {
    return (*source)[current_index];
  }

  char Decoder::consume()
  {
    return (*source)[current_index++];
  }

  string Decoder::slice(int s, int e)
  {
    return (*source).substr(s, e);
  }

  bool Decoder::is_keyword(const string &str)
  {
    return str == keywords::TRUE_KEYWORD || str == keywords::FALSE_KEYWORD || str == keywords::NULL_KEYWORD;
  }

  int Decoder::get_length()
  {
    return (*source).length();
  }

  void Decoder::skip_whitespace()
  {
    while (isspace(peek()) && current_index < get_length())
    {
      consume();
    }
  }

  // parsing
  Json *Decoder::parse(const string &str)
  {
    *source = str;
    current_index = 0;

    return parse_json();
  }

  Value *Decoder::parse_value()
  {
    skip_whitespace();

    if (peek() == '{')
    {
      return parse_object();
    }
    else if (peek() == '[')
    {
      return parse_array();
    }
    else if (peek() == '\"' || peek() == '\'')
    {
      return parse_string();
    }
    else if (isdigit(peek()) || peek() == '-' || peek() == '+')
    {
      return parse_numeric();
    }
    else if (is_keyword(slice(current_index, current_index + 4)))
    {
      return parse_keyword();
    }

    return nullptr;
  }

  Json *Decoder::parse_json()
  {
    if (peek() != '{' && peek() != '[')
    {
      return nullptr;
    }

    return (Json *)parse_value();
  }

  Object *Decoder::parse_object()
  {
    if (peek() != '{')
    {
      return nullptr;
    }

    consume();
    skip_whitespace();

    Object *object = new Object();

    while (peek() != '}')
    {
      if (peek() == ',')
      {
        consume();
        skip_whitespace();
      }

      string key = parse_string()->value;
      skip_whitespace();

      if (peek() != ':')
      {
        return nullptr;
      }

      consume();
      skip_whitespace();

      Value *value = parse_value();

      if (!value)
      {
        return nullptr;
      }

      object->add(key, value);
      skip_whitespace();
    }

    if (peek() != '}')
    {
      return nullptr;
    }

    consume();
    return object;
  }

  Array *Decoder::parse_array()
  {
    if (peek() != '[')
    {
      return nullptr;
    }

    consume();
    skip_whitespace();

    Array *array = new Array();

    while (peek() != ']')
    {
      Value *item = parse_value();

      if (!item)
      {
        return nullptr;
      }

      array->add(item);
      skip_whitespace();

      if (peek() == ',')
      {
        consume();
      }

      skip_whitespace();
    }

    if (peek() != ']')
    {
      return nullptr;
    }

    consume();
    return array;
  }

  String *Decoder::parse_string()
  {
    string str = "";
    bool is_single_quote = false;

    if (peek() == '\'')
    {
      is_single_quote = true;
      consume();
    }
    else if (peek() == '\"')
    {
      consume();
    }

    while (peek() != (is_single_quote ? '\'' : '\"') && current_index < get_length())
    {
      str += consume();
    }

    if (peek() != (is_single_quote ? '\'' : '\"'))
    {
      return nullptr;
    }

    consume();
    return new String(str);
  }

  Value *Decoder::parse_numeric()
  {
    string num = "";
    bool is_real = false;

    while (isdigit(peek()) || peek() == '.')
    {
      if (peek() == '.')
      {
        if (is_real)
        {
          return nullptr;
        }

        is_real = true;
      }

      num += consume();
    }

    if (!is_real)
    {
      return new Integer(stoi(num));
    }

    return new Number(stod(num));
  }

  Value *Decoder::parse_keyword()
  {
    string str = slice(current_index, current_index + 4);
    current_index += 4;

    if (str == keywords::TRUE_KEYWORD || str == keywords::FALSE_KEYWORD)
    {
      return new Boolean(str);
    }
    else if (str == keywords::NULL_KEYWORD)
    {
      return new Null();
    }

    return nullptr;
  }

  Value *decode(const string &str)
  {
    return Decoder::parse(str);
  }

  Value *decode_from_file(const string &path)
  {
    ifstream file(path);
    string str((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    return Decoder::parse(str);
  }
}
