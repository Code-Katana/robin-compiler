#include <iostream>

#include "handcoded_scanner.h"
#include "fa_scanner.h"
#include "json.h"

using namespace std;

int main()
{
  // for  i = 0; i < size(); i++ do
  string program = "for  i = 0; i < size(); i++ do";

  // HandCodedScanner sc(program);
  FAScanner sc1(program);
  HandCodedScanner sc2(program);

  vector<Token> tks1 = sc1.get_tokens_stream();
  vector<Token> tks2 = sc2.get_tokens_stream();

  for (int i = 0; i < tks1.size(); ++i)
  {
    cout << "HandCoded: " << JSON::stringify_token(tks2[i]) << endl;
    cout << "FAScanner: " << JSON::stringify_token(tks1[i]) << endl;
  }

  system("pause");
  return 0;
}
