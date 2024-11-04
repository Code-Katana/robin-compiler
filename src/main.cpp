#include <iostream>
#include "handcoded_scanner.h"
#include "fa_scanner.h"
#include "json.h"

using namespace std;

int main()
{
  // string program = "x ++ = 13 ; lam program   */*zamlma*/ ahmed ali int x = 2; if ^ x > 3 then end if //akjakmakma\n msakmka -- * >= lma <> akj - ++ /*spkossxs";
  string program = "x #";

      // FAScanner sc1(program);
      FAScanner sc2(program);

  // vector<Token> tks1 = sc1.get_tokens_stream();
  vector<Token> tks2 = sc2.get_tokens_stream();

  int token_count = 0;
  string s1 = "", s2 = "";

  // sc1.get_token();
  // sc1.get_token();
  cout << JSON::stringify_token(sc2.get_token()) << endl;

  for (int i = 0; i < tks2.size(); ++i)
  {
    // s1 = JSON::stringify_token(tks2[i]);
    // s2 = JSON::stringify_token(tks1[i]);

    // if (s1 == s2)
    // {
    //   cout << "pass..." << endl;
    //   ++token_count;
    // }
    // else
    // {
    //   cout << "mismatch in token:" << endl;

    cout << "HandCoded Output: " << endl
         << JSON::stringify_token(tks2[i]) << endl;
    //   cout << "FAScanner Output: " << endl
    //        << JSON::stringify_token(tks1[i]) << endl;

    //   exit(1);
    // }
  }

  // system("cls");
  cout << JSON::stringify_token(sc2.get_token()) << endl;
  cout << "end" << endl;
  // cout << "Scanner test Passed: " << token_count << " tokens were detected." << endl;

  system("pause");
  return 0;
}
