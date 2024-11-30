#include "debugger.h"

string Debugger::DEBUGGING_FOLDER = "./debug";
string Debugger::PROGRAM_FILE = "main.wren";

string read_program(string path)
{
  ifstream file(path);

  if (!file.is_open())
  {
    cout << "Program file " << path << " was not found." << endl;
    system("pause");
    exit(1);
  }

  string program;
  string line;
  while (getline(file, line))
  {
    program += line + '\n';
  }

  return program;
}

int Debugger::run()
{
  string program = read_program(DEBUGGING_FOLDER + "/" + PROGRAM_FILE);

  // HandCodedScanner *scanner1 = new HandCodedScanner(program);
  // FAScanner *scanner2 = new FAScanner(program);

  // vector<Token> tokens_stream1 = scanner1->get_tokens_stream();
  // vector<Token> tokens_stream2 = scanner2->get_tokens_stream();

  // for (int i = 0; i < tokens_stream1.size(); ++i)
  // {
  //   cout << tokens_stream1[i].value << "... ";
  //   if (
  //       tokens_stream1[i].line == tokens_stream2[i].line &&
  //       tokens_stream1[i].start == tokens_stream2[i].start &&
  //       tokens_stream1[i].end == tokens_stream2[i].end)
  //   {
  //     cout << "matches (line: " << tokens_stream2[i].line << ", start: " << tokens_stream2[i].start << ", end: " << tokens_stream2[i].end << ")" << endl;
  //   }
  //   else
  //   {
  //     cout << "mismatch" << endl;
  //   }
  // }

  // JSON::debug_file(DEBUGGING_FOLDER + "/hand_coded_scanner.json", JSON::stringify_tokens_stream(tokens_stream1));
  // JSON::debug_file(DEBUGGING_FOLDER + "/fa_scanner.json", JSON::stringify_tokens_stream(tokens_stream2));

  // system("pause");
  WrenCompiler wc(program, ScannerOptions::HandCoded, ParserOptions::RecursiveDecent);

  AstNode *tree = wc.parser->parse_ast();

  cout << "start line = " << tree->start_line << endl;
  cout << "end line = " << tree->end_line << endl;
  cout << "node start = " << tree->node_start << endl;
  cout << "node end = " << tree->node_end << endl;

  JSON::debug_file(DEBUGGING_FOLDER + "/parse_tree.json", JSON::stringify_node(tree));

  system("pause");
  return 0;
}