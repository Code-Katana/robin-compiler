#include "parser_base.h"

#include "json.h"

ParserBase::ParserBase(ScannerBase *scanner, vector<SymbolTable *> *vector)
{
  sc = scanner;
  env = vector;
  current_token = sc->get_token();
  previous_token = Token();
  tokens_stream = {};
}

Token ParserBase::match(TokenType type)
{
  previous_token = current_token;

  if (!tokens_stream.empty())
  {
    current_token = tokens_stream.front();
    if (current_token.type != type)
    {
      syntax_error("expecting " + Token::get_token_name(type) +
                   " instead of " + Token::get_token_name(current_token.type) +
                   " at line " + to_string(current_token.line));
    }
    tokens_stream.erase(tokens_stream.begin());
    current_token = tokens_stream.front();
    return previous_token;
  }

  if (current_token.type != type)
  {
    syntax_error("expecting " + Token::get_token_name(type) +
                 " instead of " + Token::get_token_name(current_token.type) +
                 " at line " + to_string(current_token.line));
  }

  current_token = sc->get_token();
  return previous_token;
}

void ParserBase::set_token_stream(vector<Token> stream)
{
  tokens_stream = stream;
}

void ParserBase::syntax_error(string message)
{
  current_token = Token("$", TokenType::END_OF_FILE, 0, 0, 0);
  cerr << message << endl;
  system("pause");
  throw runtime_error(message);
}

bool ParserBase::lookahead(TokenType type)
{
  return current_token.type == type;
}

SymbolTable *ParserBase::create_scope()
{
  SymbolTable *scope;

  if (env->empty())
  {
    scope = new SymbolTable();
  }

  scope = new SymbolTable(env->back());
  env->push_back(scope);

  return scope;
}

SymbolTable *ParserBase::delete_scope()
{
  SymbolTable *last_scope = env->back();
  env->pop_back();

  return last_scope;
}

// SymbolTable *ParserBase::init_global_scope()
// {
//   env->insert(env->begin(), new SymbolTable());
// }