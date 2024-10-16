#include "handcoded_scanner.h"
#include <iostream>
using namespace std;

int main()
{
  HandCodedScanner sc("\"Wren!\"; 898. //skasmka\n ++ * / /*sodojs:jaisj*****@#$%^&*(*saojs*hhh + --");
  sc.display_tokens();
  system("pause");
  return 0;
}