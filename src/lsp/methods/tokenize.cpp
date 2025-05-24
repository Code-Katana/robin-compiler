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
      scanner_option = (rbn::options::ScannerOptions)params->get_integer("scannerOption")->value;
    }

    return new TokenizeParams{text_document, scanner_option};
  }

  json::Object *get_token_result(const TokenizeParams *params)
  {
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

    json::Object *result = new json::Object();
    result->add("tokens", json::utils::convert_token_stream(&tokens));
    result->add("tokenCount", new json::Integer(tokens.size()));
    result->add("textDocument", new json::String(params->text_document));
  }

  rpc::ResponseMessage *compiler_action_tokenize(rpc::RequestMessage *request)
  {
    TokenizeParams *params = get_token_params((json::Object *)request->params);

    if (params == nullptr)
    {
      return nullptr;
    }

    json::Object *result = get_token_result(params);
    return new rpc::ResponseMessage(request->id, result);
  }
}