#include "utils/debugger.h"

/**
 * Example of WASM Integration
 * ***************************
 *
 * string tokenize(string js_input, int sc = 0, int pr = 0) {
 *    WrenCompiler wc(js_input, (ScannerOptions)sc, (ParserOptions)pr);
 *    return JSON::stringify_tokens_stream(wc.scanner->get_tokens_stream());
 * }
 *
 * string generateAst(string js_input, int sc = 0, int pr = 0) {
 *    WrenCompiler wc(js_input, (ScannerOptions)sc, (ParserOptions)pr);
 *    return JSON::stringify_tokens_stream(wc.parser->parse_ast());
 * }
 *
 */

int main()
{
  return Debugger::run();
}
