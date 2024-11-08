#include <iostream>
#include <vector>

#include "wren_compiler.h"
#include "json.h"
#include "ast.h"

using namespace std;

int main()
{
  Identifier *program_name = new Identifier("say_hello");
  vector<VariableDefinition *> globals = {};
  vector<Statement *> body = {
      new WriteStatement(
          vector<Expression *>{
              new PrimaryExpression(
                  new StringLiteral("Hello Word!!!")),
          }),
  };

  Program *program = new Program(program_name, globals, body);

  vector<Function *> functions = {};

  AstNode *source = new Source(program, functions);

  cout << JSON::stringify_node(source);

  system("pause");
  return 0;
}