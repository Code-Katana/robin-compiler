#include "ir_generator.h"

IRGenerator::IRGenerator()
    : module(new Module("rbn_module", context)), builder(context) {}

void IRGenerator::generate(const string &filename)
{
  temp_test_method(filename);
}

void IRGenerator::temp_test_method(const string &filename)
{
  // Create a function type returning int32 and taking no parameters
  FunctionType *funcType = FunctionType::get(builder.getInt32Ty(), false);

  // Create a function with the given type inside the module
  Function *mainFunc = Function::Create(funcType, Function::ExternalLinkage, "main", module);

  // Create a basic block and set the insertion point
  BasicBlock *entry = BasicBlock::Create(context, "entry", mainFunc);
  builder.SetInsertPoint(entry);

  // Return 69
  Value *returnValue = builder.getInt32(69);
  builder.CreateRet(returnValue);

  // Verify the function
  verifyFunction(*mainFunc);

  // Open a file and write the LLVM IR
  std::error_code EC;
  raw_fd_ostream outFile(filename, EC, sys::fs::OF_Text);
  if (EC)
  {
    errs() << "Error opening file: " << EC.message() << "\n";
    return;
  }

  module->print(outFile, nullptr);
}
