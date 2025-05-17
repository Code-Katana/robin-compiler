#include "lsp/methods/tokenize.h"

namespace lsp::methods
{
  struct TokenizeParams
  {
    string text_document;
    rbn::options::ScannerOptions scanner_option;
  };

  TokenizeParams *get_token_params(json::Object *params)
  {
    if (!params->has("textDocument") || params->get("textDocument")->type != json::ValueType::STRING)
    {
      return nullptr;
    }

    string text_document = params->get_string("textDocument")->value;
    rbn::options::ScannerOptions scanner_option = rbn::options::ScannerOptions::FiniteAutomaton;

    if (params->has("scannerOption") && params->get("scannerOption")->type == json::ValueType::NUMBER)
    {
      scanner_option = (rbn::options::ScannerOptions)params->get_number("scannerOption")->as_integer();
    }

    return new TokenizeParams{text_document, scanner_option};
  }

  rpc::ResponseMessage *compiler_action_tokenize(rpc::RequestMessage *request)
  {
    TokenizeParams *params = get_token_params((json::Object *)request->params);

    if (params == nullptr)
    {
      return nullptr;
    }

    rbn::lexical::ScannerBase *scanner = nullptr;
    string program = read_file(params->text_document);

    switch (params->scanner_option)
    {
    case rbn::options::ScannerOptions::HandCoded:
      scanner = new rbn::lexical::HandCodedScanner(program);
      break;
    case rbn::options::ScannerOptions::FiniteAutomaton:
      scanner = new rbn::lexical::FAScanner(program);
      break;
    }

    vector<rbn::core::Token> tokens = scanner->get_tokens_stream();

    json::Array *stream = new json::Array();

    for (const auto &token : tokens)
    {
      json::Object *obj = new json::Object();

      obj->add("type", new json::String(rbn::core::Token::get_token_name(token.type)));
      obj->add("value", new json::String(token.value));
      obj->add("line", new json::String(to_string(token.line)));
      obj->add("start", new json::String(to_string(token.start)));
      obj->add("end", new json::String(to_string(token.end)));

      stream->add(obj);
    }

    json::Object *result = new json::Object();
    result->add("tokens", stream);
    result->add("tokenCount", new json::Number(tokens.size()));

    return new rpc::ResponseMessage(request->id, result);
  }
}