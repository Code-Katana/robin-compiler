#include <iostream>
#include <vector>

#include "wren_compiler.h"
#include "json.h"

using namespace std;

int main()
{
  // EXAMPLE PROGRAM

  // program katana is
  // begin
  //    names[0] = "samir";
  // end

  // parsing the program above...
  Identifier *names = new Identifier("names");
  IntegerLiteral *idx = new IntegerLiteral(0);
  StringLiteral *val = new StringLiteral("samir");

  IndexExpression *lhs = new IndexExpression(names, idx);
  Expression *rhs = new PrimaryExpression(val);

  AssignmentExpression *assExpr = new AssignmentExpression(lhs, rhs);

  // the parse tree components
  // empty function array
  vector<Function *> funcs = {};
  // the program node
  vector<VariableDefinition *> globals = {};
  Identifier *program_name = new Identifier("katana");
  vector<Statement *> program_body = {assExpr};

  Program *program = new Program(program_name, globals, program_body);

  // the root node (i.e. Source)
  AstNode *source = new Source(program, funcs);

  // stringify to json
  cout << JSON::stringify_node(source) << endl;

  system("pause");
  return 0;
}