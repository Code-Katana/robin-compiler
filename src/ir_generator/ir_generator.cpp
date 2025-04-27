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
    return PointerType::getUnqual(Type::getInt8Ty(context));
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
  IRGenerator::printSymbolTable();
}

Value *IRGenerator::codegen(AstNode *node)
{
  if (auto decl = dynamic_cast<VariableDeclaration *>(node))
    return codegenVariableDeclaration(decl);
  if (auto init = dynamic_cast<VariableInitialization *>(node))
    return codegenVariableInitialization(init);
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
  pushScope();
  // Process program body
  for (auto stmt : program->body)
  {
    codegen(stmt);
  }

  builder.CreateRet(ConstantInt::get(context, APInt(32, 0)));
  popScope();
  return mainFunc;
}

void IRGenerator::codegenGlobalVariable(VariableDefinition *dif)
{
  llvm::Type *ty = nullptr;
  llvm::Constant *init = nullptr;
  std::string name;

  // Handle VariableDeclaration (multiple variables)
  if (auto varDecl = dynamic_cast<VariableDeclaration *>(dif->def))
  {
    ty = getLLVMType(
        Symbol::get_datatype(varDecl->datatype),
        Symbol::get_dimension(varDecl->datatype));

    init = llvm::Constant::getNullValue(ty);

    // Create global for each variable in declaration
    for (auto var : varDecl->variables)
    {
      llvm::GlobalVariable *gVar = new llvm::GlobalVariable(
          *module,
          ty,
          false, // isConstant
          llvm::GlobalValue::ExternalLinkage,
          init,
          var->name);

      // Insert into symbol table
      symbolTable.top()[var->name] = SymbolEntry{ty, gVar};
    }
  }
  // Handle VariableInitialization (single variable with initializer)
  else if (auto varInit = dynamic_cast<VariableInitialization *>(dif->def))
  {
    ty = getLLVMType(
        Symbol::get_datatype(varInit->datatype),
        Symbol::get_dimension(varInit->datatype));
    name = varInit->name->name;

    // Get initializer value (must be constant)
    llvm::Value *initVal = codegenExpression(varInit->initializer);
    if (auto *constant = llvm::dyn_cast<llvm::Constant>(initVal))
    {
      init = constant;
    }
    else
    {
      module->getContext().emitError("Global initializer must be constant");
      return;
    }

    llvm::GlobalVariable *gVar = new llvm::GlobalVariable(
        *module,
        ty,
        false, // isConstant
        llvm::GlobalValue::ExternalLinkage,
        init,
        name);

    // Insert into symbol table
    symbolTable.top()[name] = SymbolEntry{ty, gVar};
  }
}
Value *IRGenerator::codegenFunction(FunctionDefinition *func)
{
  // Get return type
  llvm::Type *retType = getLLVMType(
      Symbol::get_datatype(func->return_type->return_type),
      Symbol::get_dimension(func->return_type->return_type));

  // Get parameter types
  std::vector<llvm::Type *> paramTypes;
  for (auto param : func->parameters)
  {
    if (auto decl = dynamic_cast<VariableDeclaration *>(param->def))
    {
      llvm::Type *ty = getLLVMType(
          Symbol::get_datatype(decl->datatype),
          Symbol::get_dimension(decl->datatype));
      paramTypes.push_back(ty);
    }
  }

  // Create function type
  llvm::FunctionType *funcType = llvm::FunctionType::get(retType, paramTypes, false);
  llvm::Function *llvmFunc = llvm::Function::Create(
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
      llvm::Type *paramType = getLLVMType(
          Symbol::get_datatype(decl->datatype),
          Symbol::get_dimension(decl->datatype));

      llvm::AllocaInst *alloca = builder.CreateAlloca(paramType, nullptr, decl->variables[0]->name);
      builder.CreateStore(&arg, alloca);

      // Insert into symbol table
      symbolTable.top()[decl->variables[0]->name] = SymbolEntry{paramType, alloca};
    }
  }

  // Process function body
  for (auto stmt : func->body)
  {
    codegen(stmt);
  }

  // If function has no explicit return, add return void
  if (retType->isVoidTy() && !builder.GetInsertBlock()->getTerminator())
  {
    builder.CreateRetVoid();
  }

  popScope();
  return llvmFunc;
}
Value *IRGenerator::codegenVariableInitialization(VariableInitialization *init)
{
  llvm::Type *ty = getLLVMType(
      Symbol::get_datatype(init->datatype),
      Symbol::get_dimension(init->datatype));

  llvm::AllocaInst *alloca = builder.CreateAlloca(ty, nullptr, init->name->name);
  symbolTable.top()[init->name->name] = SymbolEntry{ty, alloca};

  if (init->initializer)
  {
    Value *initVal = codegenExpression(init->initializer);
    builder.CreateStore(initVal, alloca);
  }
  else
  {
    Value *defaultVal = llvm::Constant::getNullValue(ty);
    builder.CreateStore(defaultVal, alloca);
  }

  return alloca;
}
Value *IRGenerator::codegenStatement(Statement *stmt)
{
  if (auto varDecl = dynamic_cast<VariableDeclaration *>(stmt))
    return codegenVariableDeclaration(varDecl);
  if (auto varInit = dynamic_cast<VariableInitialization *>(stmt))
    return codegenVariableInitialization(varInit);
  if (auto assign = dynamic_cast<AssignmentExpression *>(stmt))
    return codegenAssignment(assign);
  if (auto ifStmt = dynamic_cast<IfStatement *>(stmt))
    return codegenIfStatement(ifStmt);
  if (auto whileLoop = dynamic_cast<WhileLoop *>(stmt))
    return codegenWhileLoop(whileLoop);
  if (auto forLoop = dynamic_cast<ForLoop *>(stmt))
    return codegenForLoop(forLoop);
  if (auto writeStmt = dynamic_cast<WriteStatement *>(stmt))
    return codegenWriteStatement(writeStmt);
  if (auto readStmt = dynamic_cast<ReadStatement *>(stmt))
    return codegenReadStatement(readStmt);
  if (auto retStmt = dynamic_cast<ReturnStatement *>(stmt))
    return codegenReturnStatement(retStmt);
  if (auto skip = dynamic_cast<SkipStatement *>(stmt))
    return codegenSkipStatement(skip);
  if (auto stop = dynamic_cast<StopStatement *>(stmt))
    return codegenStopStatement(stop);

  // Fallback
  return codegenExpression(dynamic_cast<Expression *>(stmt));
}
Value *IRGenerator::codegenSkipStatement(SkipStatement *stmt)
{
  // No operation
  return nullptr;
}
Value *IRGenerator::codegenStopStatement(StopStatement *stmt)
{
  // Call exit(0)
  llvm::Function *exitFunc = module->getFunction("exit");
  if (!exitFunc)
  {
    llvm::FunctionType *exitType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(context),
        {llvm::Type::getInt32Ty(context)},
        false);
    exitFunc = llvm::Function::Create(
        exitType,
        llvm::Function::ExternalLinkage,
        "exit",
        module.get());
  }
  builder.CreateCall(exitFunc, {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0)});
  builder.CreateUnreachable();
  return nullptr;
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
      if (!findSymbol(id->name))
      {
        // Create implicit declaration
        Value *initVal = codegenExpression(assign->value);
        llvm::Type *varType = initVal->getType();

        llvm::AllocaInst *alloca = builder.CreateAlloca(varType, nullptr, id->name);
        symbolTable.top()[id->name] = SymbolEntry{varType, alloca};

        // Store the initial value
        builder.CreateStore(initVal, alloca);
      }
      else
      {
        // Variable already exists, just assign
        codegenAssignment(assign);
      }
    }
    else
    {
      // Not an identifier? Just generate the assignment normally
      codegenAssignment(assign);
    }
  }

  llvm::Function *func = builder.GetInsertBlock()->getParent();

  // Create basic blocks
  llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "for.cond", func);
  llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context, "for.body");
  llvm::BasicBlock *updateBB = llvm::BasicBlock::Create(context, "for.update");
  llvm::BasicBlock *exitBB = llvm::BasicBlock::Create(context, "for.exit");

  // Jump to condition
  builder.CreateBr(condBB);

  // Condition block
  builder.SetInsertPoint(condBB);
  Value *condVal = castToBoolean(codegenExpression(forLoop->condition));
  builder.CreateCondBr(condVal, bodyBB, exitBB);

  // Body block
  bodyBB->insertInto(func);
  builder.SetInsertPoint(bodyBB);
  pushScope(); // New scope inside body
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

  popScope(); // Pop the loop variable scope
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
  // Check if printf is already declared
  Function *printfFunc = module->getFunction("printf");

  if (!printfFunc)
  {
    FunctionType *printfType = FunctionType::get(
        Type::getInt32Ty(context),
        {PointerType::getUnqual(Type::getInt8Ty(context))},
        true); // variadic
    printfFunc = Function::Create(
        printfType,
        Function::ExternalLinkage,
        "printf",
        module.get());
  }

  std::vector<Value *> args;
  std::string formatStr;

  for (auto expr : write->args)
  {
    Value *val = codegenExpression(expr);
    if (!val)
      return nullptr;

    llvm::Type *valType = val->getType();

    // If the expression is an identifier, lookup its declared type
    if (auto id = dynamic_cast<Identifier *>(expr))
    {
      auto entryOpt = findSymbol(id->name);
      if (!entryOpt)
      {
        module->getContext().emitError("Undefined variable in write: " + id->name);
        return nullptr;
      }

      SymbolEntry entry = *entryOpt;
      llvm::Type *declaredType = entry.llvmType;

      if (declaredType->isIntegerTy(32))
      {
        formatStr += "%d ";
      }
      else if (declaredType->isFloatTy())
      {
        // Promote float to double for printf
        val = builder.CreateFPExt(val, llvm::Type::getDoubleTy(context));
        formatStr += "%f ";
      }
      else if (declaredType->isIntegerTy(1))
      {
        // Boolean: extend to i32
        val = builder.CreateZExt(val, llvm::Type::getInt32Ty(context));
        formatStr += "%d ";
      }
      else if (declaredType->isPointerTy())
      {
        // Assume pointer to i8 (string)
        formatStr += "%s ";
      }
      else
      {
        module->getContext().emitError("Unsupported declared type in write");
        return nullptr;
      }
    }
    else
    {
      // If it's not an identifier (e.g., a literal or expression result), use the value type
      if (valType->isIntegerTy(32))
      {
        formatStr += "%d ";
      }
      else if (valType->isFloatTy())
      {
        // Promote float to double for printf
        val = builder.CreateFPExt(val, llvm::Type::getDoubleTy(context));
        formatStr += "%f ";
      }
      else if (valType->isIntegerTy(1))
      {
        val = builder.CreateZExt(val, llvm::Type::getInt32Ty(context));
        formatStr += "%d ";
      }
      else if (valType->isPointerTy())
      {
        // Assume pointer to i8 (string)
        formatStr += "%s ";
      }
      else
      {
        module->getContext().emitError("Unsupported value type in write");
        return nullptr;
      }
    }

    args.push_back(val);
  }

  formatStr += "\n"; // Add newline

  // Create format string constant
  Constant *formatConst = ConstantDataArray::getString(context, formatStr, true);
  GlobalVariable *fmtVar = new GlobalVariable(
      *module,
      formatConst->getType(),
      true,
      GlobalValue::PrivateLinkage,
      formatConst,
      "printf_fmt");

  fmtVar->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
  fmtVar->setAlignment(llvm::Align(1));

  Value *fmtPtr = builder.CreateInBoundsGEP(
      formatConst->getType(),
      fmtVar,
      {builder.getInt32(0), builder.getInt32(0)});

  args.insert(args.begin(), fmtPtr);

  return builder.CreateCall(printfFunc, args);
}

Value *IRGenerator::codegenReadStatement(ReadStatement *read)
{
  Function *scanfFunc = module->getFunction("scanf");
  if (!scanfFunc)
  {
    // Declare scanf if not found
    FunctionType *scanfType = FunctionType::get(
        Type::getInt32Ty(context),
        {PointerType::getUnqual(context)}, // i8* for format string
        true);                             // variadic function

    scanfFunc = Function::Create(
        scanfType,
        Function::ExternalLinkage,
        "scanf",
        module.get());
  }

  std::vector<Value *> args;
  std::string formatStr;

  for (auto var : read->variables)
  {
    Type *baseType;
    // Get address of the variable
    Value *addr = codegenIdentifierAddress(dynamic_cast<Identifier *>(var));
    if (!addr)
    {
      module->getContext().emitError("Invalid variable in read statement");
      return nullptr;
    }
    if (auto global = dyn_cast<GlobalVariable>(addr))
    {
      // builder.CreateLoad(global->getInitializer()->getType(), global);
      baseType = global->getInitializer()->getType();
    }
    else if (auto local = dyn_cast<AllocaInst>(addr))
    {
      // builder.CreateLoad(local->getAllocatedType(), local);
      baseType = local->getAllocatedType();
    }
    // Now: instead of asking getPointerElementType()
    // just check the address TYPE directly

    if (addr->getType()->isPointerTy())
    {
      // Assume it is pointer to int32
      // Type *baseType = Type::getFloatTy(context);

      if (baseType->isIntegerTy(32))
      {
        formatStr += "%d";
      }
      else if (baseType->isFloatTy())
      {
        formatStr += "%f";
      }
      else if (baseType->isIntegerTy(1))
      {
        formatStr += "%d";
        addr = builder.CreateBitCast(addr, PointerType::get(Type::getInt32Ty(context), 0));
      }
      else
      {
        module->getContext().emitError("Unsupported type in read statement");
        return nullptr;
      }
    }
    else
    {
      module->getContext().emitError("Expected pointer type");
      return nullptr;
    }

    formatStr += " ";
    args.push_back(addr);
  }

  // Finalize format string
  if (!formatStr.empty())
    formatStr.pop_back();
  formatStr += "\n";

  // Create global format string
  Constant *formatConst = ConstantDataArray::getString(context, formatStr);
  GlobalVariable *fmtVar = new GlobalVariable(
      *module,
      formatConst->getType(),
      true,
      GlobalValue::PrivateLinkage,
      formatConst,
      "readfmt");

  Value *fmtPtr = builder.CreateInBoundsGEP(
      formatConst->getType(),
      fmtVar,
      {ConstantInt::get(Type::getInt32Ty(context), 0), ConstantInt::get(Type::getInt32Ty(context), 0)});

  args.insert(args.begin(), fmtPtr);
  return builder.CreateCall(scanfFunc, args);
}

Value *IRGenerator::codegenVariableDefinition(VariableDefinition *def)
{
  if (auto decl = dynamic_cast<VariableDeclaration *>(def))
    return codegenVariableDeclaration(decl);
  if (auto init = dynamic_cast<VariableInitialization *>(def))
    return codegenVariableInitialization(init);
  return nullptr;
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
  // unary
  if (auto unary = dynamic_cast<UnaryExpression *>(expr))
  {
    return codegenUnaryExpr(unary);
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
    symbolTable.top()[var->name] = SymbolEntry{ty, alloca};
  }
  return nullptr;
}

Value *IRGenerator::codegenAssignment(AssignmentExpression *assign)
{
  auto *id = dynamic_cast<Identifier *>(assign->assignee);
  auto entryOpt = findSymbol(id->name);
  if (!entryOpt)
    return nullptr;

  SymbolEntry entry = *entryOpt;
  llvm::Value *target = entry.llvmValue;
  llvm::Type *varType = entry.llvmType;
  llvm::Value *value = codegen(assign->value);

  if (!target || !value)
    return nullptr;

  // If value is a pointer (e.g., from another alloca), load it first
  if (value->getType()->isPointerTy())
  {
    value = builder.CreateLoad(varType, value);
  }

  return builder.CreateStore(value, target);
}
Value *IRGenerator::codegenIdentifier(Identifier *id)
{
  auto entryOpt = findSymbol(id->name);
  if (!entryOpt)
    return nullptr;

  SymbolEntry entry = *entryOpt;
  llvm::Value *val = entry.llvmValue;
  llvm::Type *ty = entry.llvmType;

  return builder.CreateLoad(ty, val, id->name);
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
  if (auto stringLit = dynamic_cast<::StringLiteral *>(lit))
  {
    // Create global string constant
    Constant *strConst = ConstantDataArray::getString(
        context,
        StringRef(stringLit->value.c_str()),
        true // Add null terminator
    );

    GlobalVariable *gvar = new GlobalVariable(
        *module,
        strConst->getType(),
        true, // isConstant
        GlobalValue::PrivateLinkage,
        strConst,
        ".str");

    // Cast to i8* (char pointer) for string usage
    return builder.CreateInBoundsGEP(
        strConst->getType(),
        gvar,
        {builder.getInt32(0), builder.getInt32(0)});
  }

  module->getContext().emitError("Unknown literal type");
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

  Type *LTy = L->getType();
  Type *RTy = R->getType();

  // Type conversion: match types
  if (LTy != RTy)
  {
    if (LTy->isFloatingPointTy() && RTy->isIntegerTy())
    {
      R = builder.CreateSIToFP(R, LTy, "int2fp");
      RTy = LTy;
    }
    else if (LTy->isIntegerTy() && RTy->isFloatingPointTy())
    {
      L = builder.CreateSIToFP(L, RTy, "int2fp");
      LTy = RTy;
    }
    else
    {
      module->getContext().emitError("Incompatible types in relational expression");
      return nullptr;
    }
  }

  if (LTy->isIntegerTy())
  {
    if (expr->optr == "<")
      return builder.CreateICmpSLT(L, R, "cmptmp");
    if (expr->optr == "<=")
      return builder.CreateICmpSLE(L, R, "cmptmp");
    if (expr->optr == ">")
      return builder.CreateICmpSGT(L, R, "cmptmp");
    if (expr->optr == ">=")
      return builder.CreateICmpSGE(L, R, "cmptmp");
  }
  else if (LTy->isFloatingPointTy())
  {
    if (expr->optr == "<")
      return builder.CreateFCmpOLT(L, R, "cmptmp");
    if (expr->optr == "<=")
      return builder.CreateFCmpOLE(L, R, "cmptmp");
    if (expr->optr == ">")
      return builder.CreateFCmpOGT(L, R, "cmptmp");
    if (expr->optr == ">=")
      return builder.CreateFCmpOGE(L, R, "cmptmp");
  }

  module->getContext().emitError("Invalid relational operator or operand types");
  return nullptr;
}
Value *IRGenerator::codegenEqualityExpr(EqualityExpression *expr)
{
  Value *L = codegen(expr->left);
  Value *R = codegen(expr->right);
  if (!L || !R)
    return nullptr;

  Type *LTy = L->getType();
  Type *RTy = R->getType();

  if (LTy != RTy)
  {
    if (LTy->isFloatingPointTy() && RTy->isIntegerTy())
    {
      R = builder.CreateSIToFP(R, LTy, "int2fp");
      RTy = LTy;
    }
    else if (LTy->isIntegerTy() && RTy->isFloatingPointTy())
    {
      L = builder.CreateSIToFP(L, RTy, "int2fp");
      LTy = RTy;
    }
    else
    {
      module->getContext().emitError("Incompatible types in equality expression");
      return nullptr;
    }
  }

  if (LTy->isFloatingPointTy())
  {
    if (expr->optr == "==")
      return builder.CreateFCmpOEQ(L, R, "eqtmp");
    else
      return builder.CreateFCmpONE(L, R, "netmp");
  }
  else
  {
    if (expr->optr == "==")
      return builder.CreateICmpEQ(L, R, "eqtmp");
    else
      return builder.CreateICmpNE(L, R, "netmp");
  }
}

Value *IRGenerator::codegenAdditiveExpr(AdditiveExpression *expr)
{
  Value *L = codegenExpression(expr->left);
  Value *R = codegenExpression(expr->right);

  L->getType()->print(llvm::errs());
  errs() << "\n";
  R->getType()->print(llvm::errs());
  errs() << "\n";
  if (!L || !R)
    return nullptr;

  // Check types to determine which operation to use
  if (L->getType()->isIntOrPtrTy() && R->getType()->isIntOrPtrTy())
  {
    // Integer addition/subtraction
    if (expr->optr == "+")
    {
      return builder.CreateAdd(L, R, "addtmp");
    }
    else
    { // "-"
      return builder.CreateSub(L, R, "subtmp");
    }
  }
  else if (L->getType()->isFloatTy() && R->getType()->isFloatTy())
  {
    // Float addition/subtraction
    if (expr->optr == "+")
    {
      return builder.CreateFAdd(L, R, "faddtmp");
    }
    else
    { // "-"
      return builder.CreateFSub(L, R, "fsubtmp");
    }
  }
  else
  {
    // Handle type conversion if needed
    if (L->getType()->isIntegerTy() && R->getType()->isFloatTy())
    {
      L = builder.CreateSIToFP(L, Type::getFloatTy(context), "int2fp");
    }
    else if (L->getType()->isFloatTy() && R->getType()->isIntegerTy())
    {
      R = builder.CreateSIToFP(R, Type::getFloatTy(context), "int2fp");
    }

    // Now both should be float
    if (expr->optr == "+")
    {
      return builder.CreateFAdd(L, R, "faddtmp");
    }
    else
    { // "-"
      return builder.CreateFSub(L, R, "fsubtmp");
    }
  }
}

Value *IRGenerator::codegenMultiplicativeExpr(MultiplicativeExpression *expr)
{
  Value *L = codegenExpression(expr->left);
  Value *R = codegenExpression(expr->right);
  L->getType()->print(llvm::errs());
  errs() << "\n";
  R->getType()->print(llvm::errs());
  errs() << "\n";

  if (!L || !R)
    return nullptr;

  // Check types to determine which operation to use
  if (L->getType()->isIntegerTy() && R->getType()->isIntegerTy())
  {
    // Integer multiplication/division
    if (expr->optr == "*")
    {
      return builder.CreateMul(L, R, "multmp");
    }
    else if (expr->optr == "/")
    {
      return builder.CreateSDiv(L, R, "sdivtmp"); // Signed division
    }
    else if (expr->optr == "%")
    {
      return builder.CreateSRem(L, R, "sremtmp"); // Remainder
    }
  }
  else if (L->getType()->isFloatTy() && R->getType()->isFloatTy())
  {
    // Float multiplication/division
    if (expr->optr == "*")
    {
      return builder.CreateFMul(L, R, "fmultmp");
    }
    else if (expr->optr == "/")
    {
      return builder.CreateFDiv(L, R, "fdivtmp");
    }
    else if (expr->optr == "%")
    {
      return builder.CreateFRem(L, R, "fremtmp"); // Float remainder
    }
  }
  else
  {
    // Handle type conversion
    if (L->getType()->isIntegerTy() && R->getType()->isFloatTy())
    {
      L = builder.CreateSIToFP(L, Type::getFloatTy(context), "int2fp");
    }
    else if (L->getType()->isFloatTy() && R->getType()->isIntegerTy())
    {
      R = builder.CreateSIToFP(R, Type::getFloatTy(context), "int2fp");
    }

    // Now both should be float
    if (expr->optr == "*")
    {
      return builder.CreateFMul(L, R, "fmultmp");
    }
    else if (expr->optr == "/")
    {
      return builder.CreateFDiv(L, R, "fdivtmp");
    }
    else if (expr->optr == "%")
    {
      return builder.CreateFRem(L, R, "fremtmp"); // Float remainder
    }
  }

  // Shouldn't reach here, but just in case
  module->getContext().emitError("Unsupported multiplicative operator: " + expr->optr);
  return nullptr;
}
Value *IRGenerator::codegenUnaryExpr(UnaryExpression *expr)
{
  Value *operand = codegenExpression(expr->operand);
  if (!operand)
    return nullptr;

  llvm::Type *ty = operand->getType();

  // 1. NOT Operator
  if (expr->optr == "NOT()")
  {
    if (!ty->isIntegerTy(1))
    {
      operand = builder.CreateICmpNE(operand, ConstantInt::get(ty, 0), "boolcast");
    }
    return builder.CreateNot(operand, "nottmp");
  }

  // 2. Boolean Conversion (?)
  if (expr->optr == "?")
  {
    if (ty->isIntegerTy())
    {
      return builder.CreateICmpNE(operand, ConstantInt::get(ty, 0), "boolconv");
    }
    else if (ty->isFloatingPointTy())
    {
      return builder.CreateFCmpONE(operand, ConstantFP::get(ty, 0.0), "boolconv");
    }
    else if (ty->isPointerTy())
    {
      return builder.CreateICmpNE(operand, ConstantPointerNull::get(cast<PointerType>(ty)), "ptrbool");
    }
    module->getContext().emitError("Invalid type for ? operator");
    return nullptr;
  }

  // 3. Array Size (#)
  if (expr->optr == "#")
  {
    if (!ty->isPointerTy())
    {
      module->getContext().emitError("# requires array pointer");
      return nullptr;
    }
    Value *sizePtr = builder.CreateStructGEP(nullptr, operand, 0);
    return builder.CreateLoad(builder.getInt32Ty(), sizePtr, "arraysize");
  }

  // 4. Increment/Decrement (++/--)
  if (expr->optr == "++" || expr->optr == "--")
  {
    Value *addr = codegenIdentifierAddress(dynamic_cast<Identifier *>(expr->operand));
    if (!addr)
      return nullptr;

    Value *current = builder.CreateLoad(ty, addr);
    Value *one = ty->isIntegerTy() ? ConstantInt::get(ty, 1) : ConstantFP::get(ty, 1.0);
    Value *result = (expr->optr == "++") ? (ty->isIntegerTy() ? builder.CreateAdd(current, one) : builder.CreateFAdd(current, one)) : (ty->isIntegerTy() ? builder.CreateSub(current, one) : builder.CreateFSub(current, one));
    builder.CreateStore(result, addr);
    return expr->postfix ? current : result;
  }

  // 5. Rounding/Boolean to Int (@)
  if (expr->optr == "@")
  {
    if (ty->isIntegerTy(1))
    {
      return builder.CreateZExt(operand, builder.getInt32Ty(), "booltoint");
    }
    else if (ty->isFloatingPointTy())
    {
      FunctionCallee roundFunc = Intrinsic::getOrInsertDeclaration(module.get(), Intrinsic::round, {ty});
      return builder.CreateCall(roundFunc, {operand}, "round");
    }
    module->getContext().emitError("Invalid type for @ operator");
    return nullptr;
  }

  // 6. Negation (-)
  if (expr->optr == "-")
  {
    if (ty->isIntegerTy())
    {
      return builder.CreateNeg(operand, "negtmp");
    }
    else if (ty->isFloatingPointTy())
    {
      return builder.CreateFNeg(operand, "fnegtmp");
    }
    module->getContext().emitError("Invalid type for - operator");
    return nullptr;
  }

  // 7. Stringify ($)
  if (expr->optr == "$")
  {
    FunctionCallee convertFunc;

    if (ty->isIntegerTy(1))
    {
      // Boolean: call boolToString
      convertFunc = module->getOrInsertFunction(
          "boolToString",
          FunctionType::get(PointerType::getUnqual(Type::getInt8Ty(context)), {ty}, false));
    }
    else if (ty->isIntegerTy(32))
    {
      // Integer: call intToString
      convertFunc = module->getOrInsertFunction(
          "intToString",
          FunctionType::get(PointerType::getUnqual(Type::getInt8Ty(context)), {ty}, false));
    }
    else if (ty->isFloatingPointTy())
    {
      // Float: call floatToString
      convertFunc = module->getOrInsertFunction(
          "floatToString",
          FunctionType::get(PointerType::getUnqual(Type::getInt8Ty(context)), {ty}, false));
    }
    else
    {
      module->getContext().emitError("Cannot stringify type");
      return nullptr;
    }

    return builder.CreateCall(convertFunc, {operand}, "stringify");
  }

  module->getContext().emitError("Unknown unary operator");
  return nullptr;
}
Value *IRGenerator::codegenCall(CallFunctionExpression *call)
{
  llvm::Function *callee = module->getFunction(call->function->name);
  if (!callee)
  {
    module->getContext().emitError("Unknown function: " + call->function->name);
    return nullptr;
  }

  std::vector<Value *> args;
  for (auto argExpr : call->arguments)
  {
    Value *argVal = codegenExpression(argExpr);
    if (!argVal)
      return nullptr;
    args.push_back(argVal);
  }

  return builder.CreateCall(callee, args, "calltmp");
}
Value *IRGenerator::findValue(const std::string &name)
{
  auto tempStack = symbolTable;
  while (!tempStack.empty())
  {
    auto &scope = tempStack.top();
    if (scope.find(name) != scope.end())
    {
      return scope[name].llvmValue;
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
Value *IRGenerator::codegenIdentifierAddress(Identifier *id)
{
  // Directly return the pointer (for assignments/scanf)
  Value *addr = findValue(id->name);

  if (!addr)
  {
    module->getContext().emitError("Undefined variable: " + id->name);
    return nullptr;
  }

  if (!addr->getType()->isPointerTy())
  {
    module->getContext().emitError("Variable is not addressable");
    return nullptr;
  }

  return addr;
}
void IRGenerator::printSymbolTable()
{
  llvm::errs() << "=== Symbol Table ===\n";

  // Make a copy of the symbol table stack
  auto tempStack = symbolTable;

  int scopeLevel = tempStack.size();
  while (!tempStack.empty())
  {
    auto &scope = tempStack.top();
    llvm::errs() << "Scope Level " << (scopeLevel--) << ":\n";

    for (const auto &entry : scope)
    {
      const std::string &name = entry.first;
      const SymbolEntry &symbol = entry.second;

      llvm::errs() << "  Variable: " << name << " -> ";
      if (symbol.llvmValue)
        symbol.llvmValue->print(llvm::errs());
      else
        llvm::errs() << "(null)";
      llvm::errs() << " : ";
      if (symbol.llvmType)
        symbol.llvmType->print(llvm::errs());
      else
        llvm::errs() << "(null)";
      llvm::errs() << "\n";
    }

    tempStack.pop();
  }

  llvm::errs() << "====================\n";
}
optional<SymbolEntry> IRGenerator::findSymbol(const std::string &name)
{
  auto tempStack = symbolTable;
  while (!tempStack.empty())
  {
    auto &scope = tempStack.top();
    auto it = scope.find(name);
    if (it != scope.end())
    {
      return it->second; // Return a copy
    }
    tempStack.pop();
  }
  return std::nullopt;
}