#include "ir_generator.h"

LLVMContext IRGenerator::context;

IRGenerator::IRGenerator() : builder(context)
{
  // InitializeNativeTarget();
  // InitializeNativeTargetAsmPrinter();
  module = make_unique<Module>("main_module", context);
  pushScope();
}

Type *IRGenerator::getLLVMType(SymbolType type, int dim)
{
  if (dim > 0)
    return PointerType::getUnqual(getLLVMType(type, dim - 1));

  switch (type)
  {
  case SymbolType::Integer:
    return Type::getInt32Ty(context);
  case SymbolType::Float:
    return Type::getFloatTy(context);
  case SymbolType::Boolean:
    return Type::getInt1Ty(context);
  case SymbolType::String:
    return PointerType::getInt8Ty(context);
  case SymbolType::Void:
    return Type::getVoidTy(context);
  default:
    return Type::getVoidTy(context);
  }
}

void IRGenerator::generate(Source *source, const string &filename)
{
  // Generate code for program (main function)
  codegenProgram(source->program);

  // Generate code for all functions
  for (auto func : source->functions)
  {
    codegenFunction(func);
  }
  std::error_code EC;
  raw_fd_ostream outfile(filename, EC, llvm::sys::fs::OF_None);

  if (EC)
  {
    errs() << "Could not open file: " << EC.message() << "\n";
    return;
  }

  module->print(outfile, nullptr);
  outfile.flush();

  outs() << "LLVM IR written to " << filename << "\n";
}

Value *IRGenerator::codegen(AstNode *node)
{
  if (auto expr = dynamic_cast<Expression *>(node))
    return codegenExpression(expr);
  if (auto stmt = dynamic_cast<Statement *>(node))
    return codegenStatement(stmt);
  return nullptr;
}
Value *IRGenerator::codegenProgram(ProgramDefinition *program)
{
  // Create main function
  FunctionType *funcType = FunctionType::get(Type::getInt32Ty(context), false);
  llvm::Function *mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, program->program_name->name, module.get());

  BasicBlock *entry = BasicBlock::Create(context, "entry", mainFunc);
  builder.SetInsertPoint(entry);

  // Process global variables
  for (auto global : program->globals)
  {
    codegen(global->def);
  }

  // Process program body
  for (auto stmt : program->body)
  {
    codegen(stmt);
  }

  builder.CreateRet(ConstantInt::get(context, APInt(32, 0)));
  return mainFunc;
}

Value *IRGenerator::codegenFunction(FunctionDefinition *func)
{
  // Get return type
  Type *retType = getLLVMType(Symbol::get_datatype(func->return_type->return_type),
                              Symbol::get_dimension(func->return_type->return_type));

  // Get parameter types
  std::vector<Type *> paramTypes;
  for (auto param : func->parameters)
  {
    if (auto decl = dynamic_cast<VariableDeclaration *>(param->def))
    {
      Type *ty = getLLVMType(Symbol::get_datatype(decl->datatype),
                             Symbol::get_dimension(decl->datatype));
      paramTypes.push_back(ty);
    }
  }

  // Explicitly qualify LLVM components with llvm::
  llvm::FunctionType *funcType = llvm::FunctionType::get(retType, paramTypes, false);
  llvm::Function *llvmFunc = llvm::Function::Create( // <-- Fully qualified
      funcType,
      llvm::Function::ExternalLinkage,
      func->funcname->name,
      module.get());

  // Create entry block
  llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", llvmFunc);
  builder.SetInsertPoint(entry);
  pushScope();

  // Process parameters
  unsigned idx = 0;
  for (auto &arg : llvmFunc->args())
  {
    if (auto decl = dynamic_cast<VariableDeclaration *>(func->parameters[idx++]->def))
    {
      llvm::AllocaInst *alloca = builder.CreateAlloca(arg.getType());
      builder.CreateStore(&arg, alloca);
      symbolTable.top()[decl->variables[0]->name] = alloca;
    }
  }

  // Process function body
  for (auto stmt : func->body)
  {
    codegen(stmt);
  }

  if (retType->isVoidTy())
  {
    builder.CreateRetVoid();
  }

  popScope();
  return llvmFunc;
}

Value *IRGenerator::codegenStatement(Statement *stmt)
{
  if (auto varDecl = dynamic_cast<VariableDeclaration *>(stmt))
  {
    return codegenVariableDeclaration(varDecl);
  }
  if (auto assign = dynamic_cast<AssignmentExpression *>(stmt))
  {
    return codegenAssignment(assign);
  }
  if (auto write = dynamic_cast<WriteStatement *>(stmt))
  {
    return codegenWriteStatement(write);
  }
  if (auto read = dynamic_cast<ReadStatement *>(stmt))
  {
    return codegenReadStatement(read);
  }
  if (auto ifStmt = dynamic_cast<IfStatement *>(stmt))
  {
    return codegenConditional(ifStmt);
  }
  if (auto loop = dynamic_cast<WhileLoop *>(stmt))
  {
    return codegenLoop(loop);
  }
  if (auto forLoop = dynamic_cast<ForLoop *>(stmt))
  {
    return codegenForLoop(forLoop);
  }
  return codegenExpression(dynamic_cast<Expression *>(stmt));
}
Value *IRGenerator::codegenWriteStatement(WriteStatement *write)
{
  FunctionType *printfType = FunctionType::get(
      Type::getInt32Ty(context),
      {PointerType::get(Type::getInt8Ty(context), 0)},
      true);
  Function *printfFunc = Function::Create(
      printfType,
      Function::ExternalLinkage,
      "printf",
      module.get());

  std::vector<Value *> args;
  std::string formatStr;

  for (auto expr : write->args)
  {
    Value *val = codegenExpression(expr);
    Type *ty = val->getType();

    if (ty->isIntegerTy(32))
    {
      formatStr += "%d ";
    }
    else if (ty->isFloatTy())
    {
      formatStr += "%f ";
    }
    else if (ty == Type::getInt1Ty(context))
    {
      formatStr += "%d ";
      val = builder.CreateZExt(val, Type::getInt32Ty(context));
    }
    else if (ty->isPointerTy())
    {
      formatStr += "%s ";
    }
    else
    {
      module->getContext().emitError("Unsupported write type");
      return nullptr;
    }
    args.push_back(val);
  }
  formatStr += "\n";

  Constant *formatConst = ConstantDataArray::getString(context, formatStr);
  GlobalVariable *fmtVar = new GlobalVariable(
      *module,
      formatConst->getType(),
      true,
      GlobalValue::PrivateLinkage,
      formatConst,
      "fmt.str");

  Value *fmtPtr = builder.CreateConstInBoundsGEP2_32(
      formatConst->getType(),
      fmtVar,
      0, 0);

  args.insert(args.begin(), fmtPtr);
  return builder.CreateCall(printfFunc, args);
}

Value *IRGenerator::codegenReadStatement(ReadStatement *read)
{
  FunctionType *scanfType = FunctionType::get(
      Type::getInt32Ty(context),
      {PointerType::get(Type::getInt8Ty(context), 0)},
      true);
  Function *scanfFunc = Function::Create(
      scanfType,
      Function::ExternalLinkage,
      "scanf",
      module.get());

  std::vector<Value *> args;
  std::string formatStr;

  for (auto var : read->variables)
  {
    auto id = dynamic_cast<Identifier *>(var);
    Value *addr = codegenIdentifier(id);
    Type *ty = cast<AllocaInst>(addr)->getAllocatedType();

    if (ty->isIntegerTy(32))
    {
      formatStr += "%d";
    }
    else if (ty->isFloatTy())
    {
      formatStr += "%f";
    }
    else
    {
      module->getContext().emitError("Unsupported read type");
      return nullptr;
    }
    formatStr += " ";
    args.push_back(addr);
  }

  Constant *formatConst = ConstantDataArray::getString(context, formatStr);
  GlobalVariable *fmtVar = new GlobalVariable(
      *module,
      formatConst->getType(),
      true,
      GlobalValue::PrivateLinkage,
      formatConst,
      "scan.fmt");

  Value *fmtPtr = builder.CreateConstInBoundsGEP2_32(
      formatConst->getType(),
      fmtVar,
      0, 0);

  args.insert(args.begin(), fmtPtr);
  return builder.CreateCall(scanfFunc, args);
}
Value *IRGenerator::findValue(const string &name)
{
  for (auto scope = symbolTable.top(); !symbolTable.empty(); symbolTable.pop())
  {
    if (scope.find(name) != scope.end())
    {
      return scope[name];
    }
  }
  return nullptr;
}