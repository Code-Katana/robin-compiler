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
    return Type::getInt8Ty(context);
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
  Function *mainFunc = Function::Create(funcType, Function::ExternalLinkage, "main", module.get());

  BasicBlock *entry = BasicBlock::Create(context, "entry", mainFunc);
  builder.SetInsertPoint(entry);

  // Process global variables (as actual globals)
  for (auto global : program->globals)
  {

    codegenGlobalVariable(global);
  }

  // Process program body
  for (auto stmt : program->body)
  {
    codegen(stmt);
  }

  builder.CreateRet(ConstantInt::get(context, APInt(32, 0)));
  return mainFunc;
}

void IRGenerator::codegenGlobalVariable(VariableDefinition *dif)
{
  Type *ty = nullptr;
  Constant *init = nullptr;
  std::string name;

  // Handle VariableDeclaration (multiple variables)
  if (auto varDecl = dynamic_cast<VariableDeclaration *>(dif->def))
  {
    ty = getLLVMType(Symbol::get_datatype(varDecl->datatype),
                     Symbol::get_dimension(varDecl->datatype));
    init = Constant::getNullValue(ty);

    // Create global for each variable in declaration
    for (auto var : varDecl->variables)
    {
      GlobalVariable *gVar = new GlobalVariable(
          *module,
          ty,
          false,
          GlobalValue::ExternalLinkage,
          init,
          var->name);
      symbolTable.top()[var->name] = gVar;
    }
  }
  // Handle VariableInitialization (single variable with initializer)
  else if (auto varInit = dynamic_cast<VariableInitialization *>(dif->def))
  {
    ty = getLLVMType(Symbol::get_datatype(varInit->datatype),
                     Symbol::get_dimension(varInit->datatype));
    name = varInit->name->name;

    // Get initializer value (must be constant)
    Value *initVal = codegenExpression(varInit->initializer);
    if (auto *constant = dyn_cast<Constant>(initVal))
    {
      init = constant;
    }
    else
    {
      module->getContext().emitError("Global initializer must be constant");
      return;
    }

    GlobalVariable *gVar = new GlobalVariable(
        *module,
        ty,
        false,
        GlobalValue::ExternalLinkage,
        init,
        name);
    symbolTable.top()[name] = gVar;
  }
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
Value *IRGenerator::codegenVariableInitialization(VariableInitialization *init)
{

  Type *ty = getLLVMType(
      Symbol::get_datatype(init->datatype),
      Symbol::get_dimension(init->datatype));

  AllocaInst *alloca = builder.CreateAlloca(ty, nullptr, init->name->name);

  symbolTable.top()[init->name->name] = alloca;

  if (init->initializer)
  {
    Value *initVal = codegenExpression(init->initializer);

    builder.CreateStore(initVal, alloca);
  }
  else
  {

    Value *defaultVal = Constant::getNullValue(ty);
    builder.CreateStore(defaultVal, alloca);
  }

  return alloca;
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
  if (auto ifStmt = dynamic_cast<IfStatement *>(stmt))
  {
    return codegenIfStatement(ifStmt);
  }
  if (auto whileLoop = dynamic_cast<WhileLoop *>(stmt))
  {
    return codegenWhileLoop(whileLoop);
  }
  if (auto forLoop = dynamic_cast<ForLoop *>(stmt))
  {
    return codegenForLoop(forLoop);
  }
  if (auto writeStmt = dynamic_cast<WriteStatement *>(stmt))
  {
    return codegenWriteStatement(writeStmt);
  }
  if (auto readStmt = dynamic_cast<ReadStatement *>(stmt))
  {
    return codegenReadStatement(readStmt);
  }
  if (auto retStmt = dynamic_cast<ReturnStatement *>(stmt))
  {
    return codegenReturnStatement(retStmt);
  }
  return codegenExpression(dynamic_cast<Expression *>(stmt));
}

Value *IRGenerator::codegenIfStatement(IfStatement *ifStmt)
{
  Value *condVal = castToBoolean(codegenExpression(ifStmt->condition));
  Function *func = builder.GetInsertBlock()->getParent();

  BasicBlock *thenBB = BasicBlock::Create(context, "if.then", func);
  BasicBlock *elseBB = ifStmt->alternate.empty() ? nullptr
                                                 : BasicBlock::Create(context, "if.else");
  BasicBlock *mergeBB = BasicBlock::Create(context, "if.merge");

  builder.CreateCondBr(condVal, thenBB, elseBB ? elseBB : mergeBB);

  // Then block
  builder.SetInsertPoint(thenBB);
  pushScope();
  for (Statement *stmt : ifStmt->consequent)
    codegen(stmt);
  popScope();
  builder.CreateBr(mergeBB);

  // Else block
  if (elseBB)
  {
    elseBB->insertInto(func);
    builder.SetInsertPoint(elseBB);
    pushScope();
    for (Statement *stmt : ifStmt->alternate)
      codegen(stmt);
    popScope();
    builder.CreateBr(mergeBB);
  }

  // Merge block
  mergeBB->insertInto(func);
  builder.SetInsertPoint(mergeBB);

  return mergeBB;
}
Value *IRGenerator::codegenWhileLoop(WhileLoop *loop)
{
  Function *func = builder.GetInsertBlock()->getParent();

  BasicBlock *condBB = BasicBlock::Create(context, "while.cond", func);
  BasicBlock *bodyBB = BasicBlock::Create(context, "while.body");
  BasicBlock *exitBB = BasicBlock::Create(context, "while.exit");

  // Initial jump to condition
  builder.CreateBr(condBB);

  // Condition block
  builder.SetInsertPoint(condBB);
  Value *condVal = castToBoolean(codegenExpression(loop->condition));
  builder.CreateCondBr(condVal, bodyBB, exitBB);

  // Body block
  bodyBB->insertInto(func);
  builder.SetInsertPoint(bodyBB);
  pushScope();
  for (Statement *stmt : loop->body)
    codegen(stmt);
  popScope();
  builder.CreateBr(condBB); // Loop back

  // Exit block
  exitBB->insertInto(func);
  builder.SetInsertPoint(exitBB);

  return exitBB;
}
Value *IRGenerator::codegenForLoop(ForLoop *forLoop)
{
  pushScope(); // New scope for loop variables

  // Handle implicit declaration of loop variable
  if (auto assign = dynamic_cast<AssignmentExpression *>(forLoop->init))
  {
    if (auto id = dynamic_cast<Identifier *>(assign->assignee))
    {
      // Check if variable doesn't exist
      if (!findValue(id->name))
      {
        // Create implicit declaration
        Value *initVal = codegenExpression(assign->value);
        AllocaInst *alloca = builder.CreateAlloca(
            initVal->getType(),
            nullptr,
            id->name);
        symbolTable.top()[id->name] = alloca;
      }
    }
    codegenAssignment(assign); // Process initial assignment
  }

  Function *func = builder.GetInsertBlock()->getParent();

  // Create basic blocks
  BasicBlock *condBB = BasicBlock::Create(context, "for.cond", func);
  BasicBlock *bodyBB = BasicBlock::Create(context, "for.body");
  BasicBlock *updateBB = BasicBlock::Create(context, "for.update");
  BasicBlock *exitBB = BasicBlock::Create(context, "for.exit");

  // Jump to condition
  builder.CreateBr(condBB);

  // Condition block
  builder.SetInsertPoint(condBB);
  Value *condVal = castToBoolean(codegenExpression(forLoop->condition));
  builder.CreateCondBr(condVal, bodyBB, exitBB);

  // Body block
  bodyBB->insertInto(func);
  builder.SetInsertPoint(bodyBB);
  pushScope();
  for (Statement *stmt : forLoop->body)
    codegen(stmt);
  popScope();
  builder.CreateBr(updateBB);

  // Update block
  updateBB->insertInto(func);
  builder.SetInsertPoint(updateBB);
  if (forLoop->update)
    codegenExpression(forLoop->update);
  builder.CreateBr(condBB);

  // Exit block
  exitBB->insertInto(func);
  builder.SetInsertPoint(exitBB);
  popScope();

  return exitBB;
}
Value *IRGenerator::codegenReturnStatement(ReturnStatement *retStmt)
{
  if (retStmt->returnValue)
  {
    Value *retVal = codegenExpression(retStmt->returnValue);
    return builder.CreateRet(retVal);
  }
  return builder.CreateRetVoid();
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
Value *IRGenerator::codegenExpression(Expression *expr)
{
  if (auto lit = dynamic_cast<Literal *>(expr))
    return codegenLiteral(lit);
  if (auto id = dynamic_cast<Identifier *>(expr))
    return codegenIdentifier(id);
  if (auto call = dynamic_cast<CallFunctionExpression *>(expr))
    return codegenCall(call);

  // Handle binary operations
  if (auto add = dynamic_cast<AdditiveExpression *>(expr))
    return codegenAdditiveExpr(add);
  if (auto mult = dynamic_cast<MultiplicativeExpression *>(expr))
    return codegenMultiplicativeExpr(mult);

  if (auto assign = dynamic_cast<AssignmentExpression *>(expr))
  {
    return codegenAssignment(assign);
  }
  // boolean operation
  if (auto OR = dynamic_cast<OrExpression *>(expr))
    return codegenOrExpr(OR);
  if (auto AND = dynamic_cast<AndExpression *>(expr))
    return codegenAndExpr(AND);
  if (auto eq = dynamic_cast<EqualityExpression *>(expr))
    return codegenEqualityExpr(eq);
  if (auto re = dynamic_cast<RelationalExpression *>(expr))
    return codegenRelationalExpr(re);
  return nullptr;
}

Value *IRGenerator::codegenVariableDeclaration(VariableDeclaration *decl)
{
  Type *ty = getLLVMType(Symbol::get_datatype(decl->datatype),
                         Symbol::get_dimension(decl->datatype));

  for (auto var : decl->variables)
  {
    AllocaInst *alloca = builder.CreateAlloca(ty, nullptr, var->name);
    symbolTable.top()[var->name] = alloca;
  }
  return nullptr;
}

Value *IRGenerator::codegenAssignment(AssignmentExpression *assign)
{
  Value *target = codegen(assign->assignee);
  Value *value = codegenExpression(assign->value);

  if (!target || !value)
    return nullptr;

  return builder.CreateStore(value, target);
}
Value *IRGenerator::codegenIdentifier(Identifier *id)
{
  Value *val = findValue(id->name);
  if (!val)
    return nullptr;
  return builder.CreateLoad(val->getType(), val);
  // codegenLiteral(dynamic_cast<Literal *>(id));
}

Value *IRGenerator::codegenLiteral(Literal *lit)
{
  if (auto intLit = dynamic_cast<IntegerLiteral *>(lit))
  {
    return ConstantInt::get(context, APInt(32, intLit->value));
  }
  if (auto floatLit = dynamic_cast<FloatLiteral *>(lit))
  {
    return ConstantFP::get(context, APFloat(floatLit->value));
  }
  if (auto boolLit = dynamic_cast<BooleanLiteral *>(lit))
  {
    return ConstantInt::get(context, APInt(1, boolLit->value));
  }
  return nullptr;
}

Value *IRGenerator::codegenOrExpr(OrExpression *expr)
{
  Value *L = codegen(expr->left);
  Value *R = codegen(expr->right);
  if (!L || !R)
    return nullptr;
  return builder.CreateOr(L, R, "ortmp");
}

Value *IRGenerator::codegenAndExpr(AndExpression *expr)
{
  Value *L = codegen(expr->left);
  Value *R = codegen(expr->right);
  if (!L || !R)
    return nullptr;
  return builder.CreateAnd(L, R, "andtmp");
}

Value *IRGenerator::codegenRelationalExpr(RelationalExpression *expr)
{
  Value *L = codegenExpression(expr->left);
  Value *R = codegenExpression(expr->right);

  if (!L || !R)
    return nullptr;

  // Ensure types are compatible
  // if (L->getType() != R->getType()) {
  //     // Try to cast R to L's type
  //     R = castValue(R, L->getType());
  //     if (!R) return nullptr;
  // }

  // Map operator to comparison predicate
  std::map<std::string, CmpInst::Predicate> intPredicates = {
      {"<", CmpInst::ICMP_SLT},
      {"<=", CmpInst::ICMP_SLE},
      {">", CmpInst::ICMP_SGT},
      {">=", CmpInst::ICMP_SGE}};

  std::map<std::string, CmpInst::Predicate> floatPredicates = {
      {"<", CmpInst::FCMP_OLT},
      {"<=", CmpInst::FCMP_OLE},
      {">", CmpInst::FCMP_OGT},
      {">=", CmpInst::FCMP_OGE}};

  Type *ty = L->getType();

  if (ty->isIntegerTy())
  {
    auto it = intPredicates.find(expr->optr);
    if (it != intPredicates.end())
    {
      return builder.CreateICmp(it->second, L, R, "cmptmp");
    }
  }
  else if (ty->isFloatingPointTy())
  {
    auto it = floatPredicates.find(expr->optr);
    if (it != floatPredicates.end())
    {
      return builder.CreateFCmp(it->second, L, R, "cmptmp");
    }
  }

  // Error handling for unsupported types/operators
  module->getContext().emitError("Invalid relational operator or types");
  return nullptr;
}
Value *IRGenerator::codegenEqualityExpr(EqualityExpression *expr)
{
  Value *L = codegen(expr->left);
  Value *R = codegen(expr->right);
  if (!L || !R)
    return nullptr;

  if (expr->optr == "==")
  {
    return builder.CreateICmpEQ(L, R, "eqtmp");
  }
  else
  {
    return builder.CreateICmpNE(L, R, "netmp");
  }
  return nullptr;
}

Value *IRGenerator::codegenAdditiveExpr(AdditiveExpression *expr)
{
  Value *L = codegen(expr->left);
  Value *R = codegen(expr->right);
  if (!L || !R)
    return nullptr;
  // todo string +
  if (expr->optr == "+")
  {
    if (L->getType()->isIntegerTy() || L->getType()->isFloatTy())
      return builder.CreateAdd(L, R, "addtmp");
    return builder.CreateFAdd(L, R, "addtmp");
  }
  else
  {
    if (L->getType()->isIntegerTy() || L->getType()->isFloatTy())
      return builder.CreateSub(L, R, "subtmp");
    return builder.CreateFSub(L, R, "subtmp");
  }
}

Value *IRGenerator::codegenMultiplicativeExpr(MultiplicativeExpression *expr)
{
  Value *L = codegen(expr->left);
  Value *R = codegen(expr->right);
  if (!L || !R)
    return nullptr;
  // todo %
  if (expr->optr == "*")
  {
    if (L->getType()->isIntegerTy())
      return builder.CreateMul(L, R, "multmp");
    return builder.CreateFMul(L, R, "multmp");
  }
  else
  {
    if (L->getType()->isIntegerTy())
      return builder.CreateSDiv(L, R, "divtmp");
    return builder.CreateFDiv(L, R, "divtmp");
  }
}

Value *IRGenerator::codegenCall(CallFunctionExpression *call)
{
  // Implement function call logic (lookup function, prepare args, create call instruction)
  return nullptr;
}
Value *IRGenerator::findValue(const std::string &name)
{
  auto tempStack = symbolTable;
  while (!tempStack.empty())
  {
    auto &scope = tempStack.top();
    if (scope.find(name) != scope.end())
    {
      return scope[name];
    }
    tempStack.pop();
  }
  return nullptr;
}
//!! to boolean unary opr
Value *IRGenerator::castToBoolean(Value *value)
{
  if (value->getType()->isIntegerTy(1))
    return value; // Already bool

  // Convert to boolean: 0 = false, non-zero = true
  return builder.CreateICmpNE(
      value,
      ConstantInt::get(value->getType(), 0),
      "castbool");
}