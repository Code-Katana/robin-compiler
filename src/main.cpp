#include "fa_scanner.h"
#include <iostream>
using namespace std;

int main()
{
  FAScanner sc("var s: string = \"hello\"");
  sc.display_tokens();
  system("pause");

  return 0;
}