#include <iostream>
#include <vector>

#include "wren_compiler.h"
#include "json.h"
#include "ast.h"

using namespace std;

int main()
{
  Identifier *x = new Identifier("x");
  Identifier *y = new Identifier("y");
  Identifier *z = new Identifier("z");

  vector<Identifier *> vars = {x, y, z};

  AstNode *stmt = new ReadStatement(vars);

  cout << JSON::stringify_node(stmt) << endl;

  system("pause");
  return 0;
}