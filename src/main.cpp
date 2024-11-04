#include <iostream>

#include "handcoded_scanner.h"
#include "fa_scanner.h"
#include "json.h"

using namespace std;

int main()
{
  string program = "for i = 0; i < 5; ++i do write arr[i + 1]; end for";

  HandCodedScanner sc1(program);
  FAScanner sc2(program);

  vector<Token> tks1 = sc1.get_tokens_stream();
  vector<Token> tks2 = sc2.get_tokens_stream();

  string s1 = "", s2 = "";
  int token_count = 0;

  for (int i = 0; i < min(tks1.size(), tks2.size()); ++i)
  {
    s1 = JSON::stringify_token(tks1[i]);
    s2 = JSON::stringify_token(tks2[i]);
    if (s1 == s2)
    {
      cout << "pass..." << endl;
      ++token_count;
    }
    else
    {
      cout << "mismatch in token:" << endl;
      cout << "HandCoded Output: " << endl
           << JSON::stringify_token(tks1[i]) << endl;
      cout << "FAScanner Output: " << endl
           << JSON::stringify_token(tks2[i]) << endl;
      exit(1);
    }
  }

  system("cls");
  cout << "all test passed, " << token_count << " token were scanned." << endl;

  system("pause");
  return 0;
}
