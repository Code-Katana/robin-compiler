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
  Literal *f = new Identifier("x");

  vector<Literal *> elements = {a, b, c, d, e};

  Literal *arr = new ArrayLiteral(elements);

  AstNode *expr = new AdditiveExpression(
      new PrimaryExpression(f), new PrimaryExpression(b), "+");

  cout << JSON::stringify_node(expr) << endl;

  system("pause");
  return 0;
}