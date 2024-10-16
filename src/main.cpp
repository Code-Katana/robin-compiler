#include "handcoded_scanner.h"
#include <iostream>
using namespace std;

int main()
{
  HandCodedScanner sc("var message: string = \"Hello \"; write message + \"Wren!\"; 8.");
  sc.display_tokens();
  system("pause");
  return 0;
}