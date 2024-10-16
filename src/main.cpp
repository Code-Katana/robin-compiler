#include "handcoded_scanner.h"
#include "fa_scanner.h"
#include <iostream>
using namespace std;

int main()
{
  cout << "Hand Coded Scanner:" << endl;
  HandCodedScanner sc1("\"Wren!\"; 898. //skasmka\n ++ * / /*sodojs:jaisj*****@#$%^&*(*saojs*hhh + --");
  sc1.display_tokens();
  cout << "**************************************************************" << endl;
  cout << "FA Scanner:" << endl;
  FAScanner sc2("\"Wren!\"; 898. //skasmka\n ++ * / /*sodojs:jaisj*****@#$%^&*(*saojs*hhh + --");
  sc2.display_tokens();
  system("pause");
  return 0;
}