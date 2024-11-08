#include <iostream>
#include <vector>

#include "wren_compiler.h"
#include "json.h"
#include "ast.h"

using namespace std;

int main()
{
  Literal *a = new StringLiteral("Hello World");
  Literal *b = new IntegerLiteral(69);
  Literal *c = new FloatLiteral(3.14);
  Literal *d = new BooleanLiteral(true);
  Literal *e = new IntegerLiteral(88);
  Identifier *x = new Identifier("x");
  Identifier *y = new Identifier("y");
  Identifier *z = new Identifier("z");

  vector<Identifier *> vars = {x, y, z};

  // Literal *arr = new ArrayLiteral(elements);

  // AstNode *expr = new AdditiveExpression(
  //     new PrimaryExpression(f), new PrimaryExpression(b), "+");

  AstNode *stmt = new ReadStatement(vars);

  cout << JSON::stringify_node(stmt) << endl;

  system("pause");
  return 0;
}