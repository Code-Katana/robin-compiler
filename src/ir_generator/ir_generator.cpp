#include "ir_generator.h"

LLVMContext IRGenerator::context;

IRGenerator::IRGenerator(SemanticAnalyzer *semantic) : builder(context)
{
  source = semantic->analyze();
  module = make_unique<Module>(source->program->program_name->name + "_module", context);
  pushScope();
}

Type *IRGenerator::getLLVMType(SymbolType type, int dim)
{
  // 1) map SymbolType to a primitive LLVM type
  Type *baseType = nullptr;
  switch (type)
  {
  case SymbolType::Integer:
    baseType = Type::getInt32Ty(context);
    break;
  case SymbolType::Float:
    baseType = Type::getFloatTy(context);
    break;
  case SymbolType::Boolean:
    baseType = Type::getInt1Ty(context);
    break;
  case SymbolType::String:
    baseType = PointerType::getUnqual(Type::getInt8Ty(context));
    break;
  case SymbolType::Void:
    baseType = Type::getVoidTy(context);
    break;
  default:
    baseType = Type::getVoidTy(context);
    break;
  }

  // 2) if no array dimension, just return the primitive
  if (dim == 0)
    return baseType;

  // 3) otherwise return a pointer to the N-dimensional array struct
  StructType *arrStruct = getOrCreateArrayStruct(baseType, dim, "array");
  return PointerType::getUnqual(arrStruct);
}

void IRGenerator::generate(const string &filename)
{
  for (auto func : source->functions)
  {
    functionTable[func->funcname->name] = func;
    generate_function(func);
  }

  generate_program(source->program);

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

Value *IRGenerator::generate_node(AstNode *node)
{
  if (auto decl = dynamic_cast<VariableDefinition *>(node))
    return generate_variable_definition(decl);
  if (auto expr = dynamic_cast<Expression *>(node))
    return generate_expression(expr);
  if (auto stmt = dynamic_cast<Statement *>(node))
    return generate_statement(stmt);
  return nullptr;
}

Value *IRGenerator::generate_program(ProgramDefinition *program)
{

  FunctionType *funcType = FunctionType::get(Type::getInt32Ty(context), false);
  Function *mainFunc = Function::Create(funcType, Function::ExternalLinkage, "main", module.get());
  FunctionCallee pauseFunc = module->getOrInsertFunction(
      "waitForKeypress",
      FunctionType::get(Type::getVoidTy(context), {}, false));
  BasicBlock *entry = BasicBlock::Create(context, "entry", mainFunc);
  builder.SetInsertPoint(entry);

  for (auto global : program->globals)
  {

    generate_global_variable(global);
  }
  pushScope();

  for (auto stmt : program->body)
  {
    generate_node(stmt);
  }

  builder.CreateCall(pauseFunc);
  builder.CreateRet(ConstantInt::get(context, APInt(32, 0)));

  popScope();
  return mainFunc;
}

Value *IRGenerator::generate_function(FunctionDefinition *func)
{
  std::vector<llvm::Type *> paramTypes;
  std::vector<VariableDeclaration *> paramDecls;
  std::vector<VariableInitialization *> paramInits;

  for (auto param : func->parameters)
  {
    if (auto decl = dynamic_cast<VariableDeclaration *>(param->def))
    {
      llvm::Type *ty = getLLVMType(
          Symbol::get_datatype(decl->datatype),
          Symbol::get_dimension(decl->datatype));
      paramTypes.push_back(ty);
      paramDecls.push_back(decl);
      paramInits.push_back(nullptr);
    }
    else if (auto init = dynamic_cast<VariableInitialization *>(param->def))
    {
      llvm::Type *ty = getLLVMType(
          Symbol::get_datatype(init->datatype),
          Symbol::get_dimension(init->datatype));
      paramTypes.push_back(ty);
      paramDecls.push_back(nullptr);
      paramInits.push_back(init);
    }
  }

  llvm::Type *retType = getLLVMType(
      Symbol::get_datatype(func->return_type->return_type),
      Symbol::get_dimension(func->return_type->return_type));
  llvm::FunctionType *funcType = llvm::FunctionType::get(retType, paramTypes, false);
  llvm::Function *llvmFunc = llvm::Function::Create(
      funcType,
      llvm::Function::ExternalLinkage,
      func->funcname->name,
      module.get());

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", llvmFunc);
  builder.SetInsertPoint(entry);
  pushScope();

  unsigned idx = 0;
  for (auto &arg : llvmFunc->args())
  {
    std::string paramName;
    SymbolType baseType;
    int dimensions = 0;
    size_t defaultLength = 10;

    if (paramDecls[idx])
    {
      auto varName = paramDecls[idx]->variables[0]->name;
      paramName = varName;
      baseType = Symbol::get_datatype(paramDecls[idx]->datatype);
      dimensions = Symbol::get_dimension(paramDecls[idx]->datatype);

      Type *ty = getLLVMType(baseType, dimensions);
      AllocaInst *alloca = builder.CreateAlloca(ty, nullptr, varName);
      builder.CreateStore(&arg, alloca);

      symbolTable.top()[varName] = {
          ty, alloca, baseType, dimensions, defaultLength, nullptr};
    }
    else if (paramInits[idx])
    {

      auto varName = paramInits[idx]->name->name;
      paramName = varName;
      baseType = Symbol::get_datatype(paramInits[idx]->datatype);
      dimensions = Symbol::get_dimension(paramInits[idx]->datatype);

      Type *ty = getLLVMType(baseType, dimensions);
      AllocaInst *alloca = builder.CreateAlloca(ty, nullptr, varName);
      builder.CreateStore(&arg, alloca);

      symbolTable.top()[varName] = {
          ty, alloca, baseType, dimensions, defaultLength, nullptr};
    }

    if (dimensions > 0)
    {

      AllocaInst *lengthAlloca = builder.CreateAlloca(
          Type::getInt32Ty(context), nullptr, paramName + "_len");

      builder.CreateStore(builder.getInt32(defaultLength), lengthAlloca);

      auto &entry = symbolTable.top()[paramName];
      entry.lengthAlloca = lengthAlloca;
      entry.arrayLength = defaultLength;

      symbolTable.top()[paramName + "_len"] = {
          Type::getInt32Ty(context), lengthAlloca, SymbolType::Integer, 0, 0, nullptr};
    }

    ++idx;
  }

  for (auto stmt : func->body)
  {
    generate_node(stmt);
  }

  if (retType->isVoidTy() && !builder.GetInsertBlock()->getTerminator())
  {
    builder.CreateRetVoid();
  }

  popScope();
  return llvmFunc;
}

Value *IRGenerator::generate_statement(Statement *stmt)
{
  if (auto decl = dynamic_cast<VariableDefinition *>(stmt))
    return generate_variable_definition(decl);
  if (auto assign = dynamic_cast<AssignmentExpression *>(stmt))
    return generate_assignment(assign);
  if (auto ifStmt = dynamic_cast<IfStatement *>(stmt))
    return generate_if_statement(ifStmt);
  if (auto whileLoop = dynamic_cast<WhileLoop *>(stmt))
    return generate_while_loop(whileLoop);
  if (auto forLoop = dynamic_cast<ForLoop *>(stmt))
    return generate_for_loop(forLoop);
  if (auto writeStmt = dynamic_cast<WriteStatement *>(stmt))
    return generate_write_statement(writeStmt);
  if (auto readStmt = dynamic_cast<ReadStatement *>(stmt))
    return generate_read_statement(readStmt);
  if (auto retStmt = dynamic_cast<ReturnStatement *>(stmt))
    return generate_return_statement(retStmt);
  if (auto skip = dynamic_cast<SkipStatement *>(stmt))
    return generate_skip_statement(skip);
  if (auto stop = dynamic_cast<StopStatement *>(stmt))
    return generate_stop_statement(stop);

  return generate_expression(dynamic_cast<Expression *>(stmt));
}

Value *IRGenerator::generate_skip_statement(SkipStatement *stmt)
{
  return nullptr;
}

Value *IRGenerator::generate_stop_statement(StopStatement *stmt)
{
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

Value *IRGenerator::generate_if_statement(IfStatement *ifStmt)
{
  Value *condVal = castToBoolean(generate_expression(ifStmt->condition));
  Function *func = builder.GetInsertBlock()->getParent();

  BasicBlock *thenBB = BasicBlock::Create(context, "if.then", func);
  BasicBlock *elseBB = ifStmt->alternate.empty() ? nullptr
                                                 : BasicBlock::Create(context, "if.else");
  BasicBlock *mergeBB = BasicBlock::Create(context, "if.merge");

  builder.CreateCondBr(condVal, thenBB, elseBB ? elseBB : mergeBB);

  builder.SetInsertPoint(thenBB);
  pushScope();
  for (Statement *stmt : ifStmt->consequent)
    generate_node(stmt);
  popScope();
  builder.CreateBr(mergeBB);

  if (elseBB)
  {
    elseBB->insertInto(func);
    builder.SetInsertPoint(elseBB);
    pushScope();
    for (Statement *stmt : ifStmt->alternate)
      generate_node(stmt);
    popScope();
    builder.CreateBr(mergeBB);
  }

  mergeBB->insertInto(func);
  builder.SetInsertPoint(mergeBB);

  return mergeBB;
}

Value *IRGenerator::generate_while_loop(WhileLoop *loop)
{
  Function *func = builder.GetInsertBlock()->getParent();

  BasicBlock *condBB = BasicBlock::Create(context, "while.cond", func);
  BasicBlock *bodyBB = BasicBlock::Create(context, "while.body");
  BasicBlock *exitBB = BasicBlock::Create(context, "while.exit");

  builder.CreateBr(condBB);

  builder.SetInsertPoint(condBB);
  Value *condVal = castToBoolean(generate_expression(loop->condition));
  builder.CreateCondBr(condVal, bodyBB, exitBB);

  bodyBB->insertInto(func);
  builder.SetInsertPoint(bodyBB);
  pushScope();
  for (Statement *stmt : loop->body)
    generate_node(stmt);
  popScope();
  builder.CreateBr(condBB);

  exitBB->insertInto(func);
  builder.SetInsertPoint(exitBB);

  return exitBB;
}

Value *IRGenerator::generate_for_loop(ForLoop *forLoop)
{
  pushScope();

  if (auto assign = dynamic_cast<AssignmentExpression *>(forLoop->init))
  {
    if (auto id = dynamic_cast<Identifier *>(assign->assignee))
    {
      if (!findSymbol(id->name))
      {
        Value *initVal = generate_expression(assign->value);
        llvm::Type *varType = initVal->getType();

        llvm::AllocaInst *alloca = builder.CreateAlloca(varType, nullptr, id->name);
        symbolTable.top()[id->name] = SymbolEntry{varType, alloca};

        builder.CreateStore(initVal, alloca);
      }
      else
      {
        generate_assignment(assign);
      }
    }
    else
    {
      generate_assignment(assign);
    }
  }

  llvm::Function *func = builder.GetInsertBlock()->getParent();

  llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "for.cond", func);
  llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context, "for.body");
  llvm::BasicBlock *updateBB = llvm::BasicBlock::Create(context, "for.update");
  llvm::BasicBlock *exitBB = llvm::BasicBlock::Create(context, "for.exit");

  builder.CreateBr(condBB);

  builder.SetInsertPoint(condBB);
  Value *condVal = castToBoolean(generate_expression(forLoop->condition));
  builder.CreateCondBr(condVal, bodyBB, exitBB);

  bodyBB->insertInto(func);
  builder.SetInsertPoint(bodyBB);
  pushScope();
  for (Statement *stmt : forLoop->body)
    generate_node(stmt);
  popScope();
  builder.CreateBr(updateBB);

  updateBB->insertInto(func);
  builder.SetInsertPoint(updateBB);
  if (forLoop->update)
    generate_expression(forLoop->update);
  builder.CreateBr(condBB);

  exitBB->insertInto(func);
  builder.SetInsertPoint(exitBB);

  popScope();
  return exitBB;
}

Value *IRGenerator::generate_return_statement(ReturnStatement *retStmt)
{
  if (retStmt->returnValue)
  {
    Value *retVal = generate_expression(retStmt->returnValue);
    return builder.CreateRet(retVal);
  }
  return builder.CreateRetVoid();
}

Value *IRGenerator::generate_write_statement(WriteStatement *write)
{
  Function *printfFunc = module->getFunction("printf");

  if (!printfFunc)
  {
    FunctionType *printfType = FunctionType::get(
        Type::getInt32Ty(context),
        {PointerType::getUnqual(Type::getInt8Ty(context))},
        true);
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
    Value *val = generate_expression(expr);
    if (!val)
      return nullptr;

    llvm::Type *valType = val->getType();

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
        val = builder.CreateFPExt(val, llvm::Type::getDoubleTy(context));
        formatStr += "%f ";
      }
      else if (declaredType->isIntegerTy(1))
      {
        val = builder.CreateZExt(val, llvm::Type::getInt32Ty(context));
        formatStr += "%d ";
      }
      else if (declaredType->isPointerTy())
      {
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
      if (valType->isIntegerTy(32))
      {
        formatStr += "%d ";
      }
      else if (valType->isFloatTy())
      {
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

  formatStr += "\n";

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

Value *IRGenerator::generate_read_statement(ReadStatement *read)
{
  Function *scanfFunc = module->getFunction("scanf");
  if (!scanfFunc)
  {
    FunctionType *scanfType = FunctionType::get(
        Type::getInt32Ty(context),
        {PointerType::getUnqual(context)},
        true);

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
    Value *addr = generate_identifier_address(dynamic_cast<Identifier *>(var));
    if (!addr)
    {
      module->getContext().emitError("Invalid variable in read statement");
      return nullptr;
    }
    if (auto global = dyn_cast<GlobalVariable>(addr))
    {

      baseType = global->getInitializer()->getType();
    }
    else if (auto local = dyn_cast<AllocaInst>(addr))
    {

      baseType = local->getAllocatedType();
    }

    if (addr->getType()->isPointerTy())
    {

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

  if (!formatStr.empty())
    formatStr.pop_back();
  formatStr += "\n";

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

Value *IRGenerator::generate_variable_definition(VariableDefinition *def)
{
  if (auto decl = dynamic_cast<VariableDeclaration *>(def->def))
    return generate_variable_declaration(decl);
  if (auto init = dynamic_cast<VariableInitialization *>(def->def))
    return generate_variable_initialization(init);
  return nullptr;
}

void IRGenerator::generate_global_variable(VariableDefinition *def)
{
  if (!def || !def->def)
  {
    module->getContext().emitError("Invalid global variable definition");
    return;
  }

  // --- Case A: Declaration only ---
  if (auto *varDecl = dynamic_cast<VariableDeclaration *>(def->def))
  {
    SymbolType baseType = Symbol::get_datatype(varDecl->datatype);
    int dims = Symbol::get_dimension(varDecl->datatype);
    Type *llvmType = getLLVMType(baseType, dims);

    for (auto *var : varDecl->variables)
    {
      const std::string &name = var->name;

      if (dims == 0)
      {
        // Scalar: zero-initialize with default value
        Constant *init = Constant::getNullValue(llvmType);
        auto *gVar = new GlobalVariable(
            *module,
            llvmType,
            /*isConstant=*/false,
            GlobalValue::ExternalLinkage,
            init,
            name);

        symbolTable.top()[name] = {
            llvmType, gVar, baseType, dims, 0, nullptr};
      }
      else
      {
        // Array: allocate and zero-init a struct { data, length }
        Type *elemType = getLLVMType(baseType, 0);
        StructType *arrayStruct = getOrCreateArrayStruct(elemType, dims, "array");
        Constant *zeroStruct = Constant::getNullValue(arrayStruct);

        auto *gVar = new GlobalVariable(
            *module,
            arrayStruct,
            /*isConstant=*/false,
            GlobalValue::ExternalLinkage,
            zeroStruct,
            name);

        symbolTable.top()[name] = {
            PointerType::getUnqual(arrayStruct), gVar, baseType, dims, 0, nullptr};
      }
    }
    return;
  }

  // --- Case B: Declaration with initializer ---
  if (auto *varInit = dynamic_cast<VariableInitialization *>(def->def))
  {
    SymbolType baseType = Symbol::get_datatype(varInit->datatype);
    int dims = Symbol::get_dimension(varInit->datatype);
    const std::string &name = varInit->name->name;

    if (dims == 0)
    {
      // Scalar initializer — try to evaluate as constant
      Type *ty = getLLVMType(baseType, 0);
      Constant *init = nullptr;

      if (auto *lit = dynamic_cast<Literal *>(varInit->initializer))
        init = dyn_cast<Constant>(generate_node(lit));

      if (!init)
        init = Constant::getNullValue(ty); // fallback

      auto *gVar = new GlobalVariable(
          *module,
          ty,
          /*isConstant=*/false,
          GlobalValue::ExternalLinkage,
          init,
          name);

      symbolTable.top()[name] = {ty, gVar, baseType, 0, 0, nullptr};
    }
    else
    {
      // Arrays: currently always zero-initialized
      Type *elemType = getLLVMType(baseType, 0);
      StructType *arrayStruct = getOrCreateArrayStruct(elemType, dims, "array");
      Constant *zeroStruct = Constant::getNullValue(arrayStruct);

      auto *gVar = new GlobalVariable(
          *module,
          arrayStruct,
          /*isConstant=*/false,
          GlobalValue::ExternalLinkage,
          zeroStruct,
          name);

      symbolTable.top()[name] = {
          PointerType::getUnqual(arrayStruct), gVar, baseType, dims, 0, nullptr};
    }
    return;
  }

  // --- Fallback: unrecognized global form ---
  module->getContext().emitError("Unknown global-variable form");
}

Value *IRGenerator::generate_variable_initialization(VariableInitialization *init)
{
  SymbolType baseType = Symbol::get_datatype(init->datatype);
  int dimensions = Symbol::get_dimension(init->datatype);
  const std::string varName = init->name->name;

  // ARRAY case: build & fill the ArrayN struct via your helper
  if (dimensions > 0)
  {
    auto *lit = dynamic_cast<ArrayLiteral *>(init->initializer);
    if (!lit)
    {
      module->getContext().emitError(
          "Array initializer expected for " + varName);
      return nullptr;
    }

    SymbolType actualBase;
    int actualDim;
    // this returns an AllocaInst* of ArrayN on the stack
    Value *arrAlloca =
        createJaggedArrayStruct(lit,
                                declareMalloc(),
                                &actualBase,
                                &actualDim);

    // sanity check
    if (!arrAlloca ||
        actualBase != baseType ||
        actualDim != dimensions)
    {
      module->getContext().emitError(
          "Array literal mismatch for " + varName);
      return nullptr;
    }

    // register in symbol‐table
    StructType *arrStruct =
        getOrCreateArrayStruct(getLLVMType(baseType, 0),
                               dimensions,
                               "array");
    PointerType *arrPtrTy = PointerType::getUnqual(arrStruct);

    symbolTable.top()[varName] = {
        /* llvmType    */ arrPtrTy,
        /* llvmValue   */ arrAlloca,
        /* baseType    */ baseType,
        /* dimensions  */ dimensions,
        /* arrayLength */ lit->elements.size(),
        /* lengthAlloca*/ nullptr};
    return arrAlloca;
  }

  // SCALAR case: same as before
  Type *ty = getLLVMType(baseType, dimensions);
  AllocaInst *alloca =
      builder.CreateAlloca(ty, nullptr, varName);

  if (init->initializer)
  {
    Value *val = generate_node(init->initializer);
    builder.CreateStore(val, alloca);
  }
  else
  {
    // default‐zero
    builder.CreateStore(
        Constant::getNullValue(ty),
        alloca);
  }

  symbolTable.top()[varName] = {
      ty, alloca, baseType, dimensions, 0, nullptr};
  return alloca;
}

Value *IRGenerator::generate_variable_declaration(VariableDeclaration *decl)
{
  SymbolType baseType = Symbol::get_datatype(decl->datatype);
  int dimensions = Symbol::get_dimension(decl->datatype);

  // ARRAY case: emit "struct ArrayN { T* data; i32 length; }" on the stack
  if (dimensions > 0)
  {
    // element‐type = baseType @ dim=0
    Type *elementTy = getLLVMType(baseType, /*dim*/ 0);
    // get-or-create the ArrayN struct
    StructType *arrStruct =
        getOrCreateArrayStruct(elementTy, dimensions, "array");
    PointerType *arrPtrTy = PointerType::getUnqual(arrStruct);

    for (auto *var : decl->variables)
    {
      const std::string &name = var->name;
      // allocate the ArrayN struct on the stack
      AllocaInst *all = builder.CreateAlloca(arrStruct, nullptr, name);

      // zero‐init .data (field 0)
      Value *dataGEP =
          builder.CreateStructGEP(arrStruct, all, 0, name + ".data_gep");
      // pointer‐type of the .data field:
      Type *dataFieldTy =
          (dimensions == 1)
              ? elementTy->getPointerTo()
              : getOrCreateArrayStruct(elementTy, dimensions - 1, "array")
                    ->getPointerTo();
      builder.CreateStore(
          ConstantPointerNull::get(cast<PointerType>(dataFieldTy)),
          dataGEP);

      // zero‐init .length (field 1)
      Value *lenGEP =
          builder.CreateStructGEP(arrStruct, all, 1, name + ".len_gep");
      builder.CreateStore(
          builder.getInt32(0),
          lenGEP);

      // register in symbol‐table
      symbolTable.top()[name] = {
          /* llvmType    */ arrPtrTy,
          /* llvmValue   */ all,
          /* baseType    */ baseType,
          /* dimensions  */ dimensions,
          /* arrayLength */ 0,
          /* lengthAlloca*/ nullptr};
    }
    return nullptr;
  }

  // SCALAR case: unchanged
  Type *ty = getLLVMType(baseType, dimensions);
  for (auto *var : decl->variables)
  {
    AllocaInst *alloca =
        builder.CreateAlloca(ty, nullptr, var->name);
    symbolTable.top()[var->name] = {
        ty, alloca, baseType, dimensions, 0, nullptr};
  }
  return nullptr;
}

Value *IRGenerator::generate_expression(Expression *expr)
{
  if (auto index = dynamic_cast<IndexExpression *>(expr))
  {
    Type *elemTy = nullptr;
    Value *addr = generate_index_expression(index, &elemTy);
    return builder.CreateLoad(elemTy, addr, "loadidx");
  }
  if (auto decl = dynamic_cast<VariableDefinition *>(expr))
    return generate_variable_definition(decl);

  if (auto lit = dynamic_cast<Literal *>(expr))
    return generate_literal(lit);

  if (auto id = dynamic_cast<Identifier *>(expr))
  {
    return generate_identifier(id);
  }

  if (auto call = dynamic_cast<CallFunctionExpression *>(expr))
    return generate_call(call);

  if (auto add = dynamic_cast<AdditiveExpression *>(expr))
    return generate_additive_expr(add);
  if (auto mult = dynamic_cast<MultiplicativeExpression *>(expr))
    return generate_multiplicative_expr(mult);
  if (auto assign = dynamic_cast<AssignmentExpression *>(expr))
    return generate_assignment(assign);

  // Unary operations
  if (auto unary = dynamic_cast<UnaryExpression *>(expr))
    return generate_unary_expr(unary);

  // Boolean operations
  if (auto OR = dynamic_cast<OrExpression *>(expr))
    return generate_or_expr(OR);
  if (auto AND = dynamic_cast<AndExpression *>(expr))
    return generate_and_expr(AND);
  if (auto eq = dynamic_cast<EqualityExpression *>(expr))
    return generate_equality_expr(eq);
  if (auto re = dynamic_cast<RelationalExpression *>(expr))
    return generate_relational_expr(re);

  return nullptr;
}

Value *IRGenerator::generate_assignment(AssignmentExpression *assign)
{
  // 1) Get the address of the LHS and the RHS value
  Value *lhsAddr = generate_l_value(assign->assignee);
  Value *rhsVal = generate_expression(assign->value);
  if (!lhsAddr || !rhsVal)
    return nullptr;

  // 2) If LHS is a top-level identifier, check if it's an array
  if (auto *id = dynamic_cast<Identifier *>(assign->assignee))
  {
    auto entryOpt = findSymbol(id->name);
    if (entryOpt)
    {
      SymbolEntry entry = *entryOpt;

      // 2a) ARRAY case
      if (entry.dimensions > 0)
      {
        // reconstruct your ArrayN struct type
        Type *eltTy = getLLVMType(entry.baseType, /*dim=*/0);
        StructType *arrStruct =
            getOrCreateArrayStruct(eltTy, entry.dimensions, "array");

        // --- unpack RHS struct fields ---
        Value *rhsStructPtr = rhsVal; // this is ArrayN*
        // .data
        Value *rhsDataGEP = builder.CreateStructGEP(
            arrStruct, rhsStructPtr,
            /*fieldNo=*/0,
            id->name + ".rhs.data.gep");
        Type *dataFieldTy = arrStruct->getElementType(0);
        Value *rhsData = builder.CreateLoad(
            dataFieldTy,
            rhsDataGEP,
            id->name + ".rhs.data");
        // .length
        Value *rhsLenGEP = builder.CreateStructGEP(
            arrStruct, rhsStructPtr,
            /*fieldNo=*/1,
            id->name + ".rhs.len.gep");
        Type *lenFieldTy = arrStruct->getElementType(1);
        Value *rhsLen = builder.CreateLoad(
            lenFieldTy,
            rhsLenGEP,
            id->name + ".rhs.len");

        // --- overwrite LHS struct fields in place ---
        Value *lhsStructPtr = entry.llvmValue; // this is the AllocaInst* from decl/init
        // store .data
        Value *lhsDataGEP = builder.CreateStructGEP(
            arrStruct, lhsStructPtr,
            /*fieldNo=*/0,
            id->name + ".data.gep");
        builder.CreateStore(rhsData, lhsDataGEP);
        // store .length
        Value *lhsLenGEP = builder.CreateStructGEP(
            arrStruct, lhsStructPtr,
            /*fieldNo=*/1,
            id->name + ".len.gep");
        builder.CreateStore(rhsLen, lhsLenGEP);

        // update in-memory arrayLength if you like
        if (auto *CI = dyn_cast<ConstantInt>(rhsLen))
          symbolTable.top()[id->name].arrayLength = CI->getZExtValue();

        return lhsAddr;
      }
    }
  }

  // 2b) indexed or scalar: just a normal store
  builder.CreateStore(rhsVal, lhsAddr);
  return lhsAddr;
}

Value *IRGenerator::generate_identifier(Identifier *id)
{
  auto entryOpt = findSymbol(id->name);
  if (!entryOpt)
    return nullptr;

  SymbolEntry entry = *entryOpt;

  return builder.CreateLoad(entry.llvmType, entry.llvmValue, id->name);
}

Value *IRGenerator::generate_literal(Literal *lit)
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

    Constant *strConst = ConstantDataArray::getString(
        context,
        StringRef(stringLit->value.c_str()),
        true);

    GlobalVariable *gvar = new GlobalVariable(
        *module,
        strConst->getType(),
        true,
        GlobalValue::PrivateLinkage,
        strConst,
        ".str");

    return builder.CreateInBoundsGEP(
        strConst->getType(),
        gvar,
        {builder.getInt32(0), builder.getInt32(0)});
  }

  module->getContext().emitError("Unknown literal type");
  return nullptr;
}

Value *IRGenerator::generate_or_expr(OrExpression *expr)
{
  Value *L = generate_node(expr->left);
  Value *R = generate_node(expr->right);
  if (!L || !R)
    return nullptr;
  return builder.CreateOr(L, R, "ortmp");
}

Value *IRGenerator::generate_and_expr(AndExpression *expr)
{
  Value *L = generate_node(expr->left);
  Value *R = generate_node(expr->right);
  if (!L || !R)
    return nullptr;
  return builder.CreateAnd(L, R, "andtmp");
}

Value *IRGenerator::generate_relational_expr(RelationalExpression *expr)
{
  Value *L = generate_expression(expr->left);
  Value *R = generate_expression(expr->right);

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

Value *IRGenerator::generate_equality_expr(EqualityExpression *expr)
{
  Value *L = generate_node(expr->left);
  Value *R = generate_node(expr->right);
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

Value *IRGenerator::generate_additive_expr(AdditiveExpression *expr)
{
  Value *L = generate_expression(expr->left);
  Value *R = generate_expression(expr->right);

  L->getType()->print(llvm::errs());
  errs() << "\n";
  R->getType()->print(llvm::errs());
  errs() << "\n";
  if (!L || !R)
    return nullptr;

  Type *i8Ty = Type::getInt8Ty(context);
  PointerType *i8PtrTy = PointerType::getUnqual(context);

  bool bothStrings = L->getType() == i8PtrTy && R->getType() == i8PtrTy;

  if (bothStrings && expr->optr == "+")
  {

    FunctionType *strlenTy = FunctionType::get(
        Type::getInt64Ty(context), {i8PtrTy}, false);
    FunctionCallee strlenFunc = module->getOrInsertFunction("strlen", strlenTy);

    FunctionType *memcpyTy = FunctionType::get(
        Type::getVoidTy(context), {i8PtrTy, i8PtrTy, Type::getInt64Ty(context), Type::getInt1Ty(context)}, false);
    FunctionCallee memcpyFunc = module->getOrInsertFunction("llvm.memcpy.p0.p0.i64", memcpyTy);

    Value *len1 = builder.CreateCall(strlenFunc, {L}, "strlen1");
    Value *len2 = builder.CreateCall(strlenFunc, {R}, "strlen2");

    Value *totalLen = builder.CreateAdd(len1, len2, "concat_len");
    Value *totalLenWithNull = builder.CreateAdd(totalLen,
                                                ConstantInt::get(Type::getInt64Ty(context), 1), "total_with_null");

    Function *mallocFunc = declareMalloc();
    Value *buffer = builder.CreateCall(mallocFunc, {totalLen}, "concat_buf");
    buffer = builder.CreateBitCast(buffer, i8PtrTy);

    builder.CreateCall(memcpyFunc,
                       {buffer,
                        L,
                        len1,
                        ConstantInt::get(Type::getInt1Ty(context), 0)});

    Value *destPtr = builder.CreateGEP(i8Ty, buffer, {len1}, "dest_ptr");
    builder.CreateCall(memcpyFunc,
                       {destPtr,
                        R,
                        len2,
                        ConstantInt::get(Type::getInt1Ty(context), 0)});

    Value *totalLenBytes = builder.CreateAdd(len1, len2, "total_len_bytes");
    Value *nullTermPtr = builder.CreateGEP(i8Ty, buffer, {totalLenBytes});
    builder.CreateStore(ConstantInt::get(i8Ty, 0), nullTermPtr);

    return buffer;
  }

  if (L->getType()->isIntOrPtrTy() && R->getType()->isIntOrPtrTy())
  {

    if (expr->optr == "+")
    {
      return builder.CreateAdd(L, R, "addtmp");
    }
    else
    {
      return builder.CreateSub(L, R, "subtmp");
    }
  }
  else if (L->getType()->isFloatTy() && R->getType()->isFloatTy())
  {

    if (expr->optr == "+")
    {
      return builder.CreateFAdd(L, R, "faddtmp");
    }
    else
    {
      return builder.CreateFSub(L, R, "fsubtmp");
    }
  }
  else
  {

    if (L->getType()->isIntegerTy() && R->getType()->isFloatTy())
    {
      L = builder.CreateSIToFP(L, Type::getFloatTy(context), "int2fp");
    }
    else if (L->getType()->isFloatTy() && R->getType()->isIntegerTy())
    {
      R = builder.CreateSIToFP(R, Type::getFloatTy(context), "int2fp");
    }

    if (expr->optr == "+")
    {
      return builder.CreateFAdd(L, R, "faddtmp");
    }
    else
    {
      return builder.CreateFSub(L, R, "fsubtmp");
    }
  }
}

Value *IRGenerator::generate_multiplicative_expr(MultiplicativeExpression *expr)
{
  Value *L = generate_expression(expr->left);
  Value *R = generate_expression(expr->right);
  L->getType()->print(llvm::errs());
  errs() << "\n";
  R->getType()->print(llvm::errs());
  errs() << "\n";

  if (!L || !R)
    return nullptr;

  if (L->getType()->isIntegerTy() && R->getType()->isIntegerTy())
  {

    if (expr->optr == "*")
    {
      return builder.CreateMul(L, R, "multmp");
    }
    else if (expr->optr == "/")
    {
      return builder.CreateSDiv(L, R, "sdivtmp");
    }
    else if (expr->optr == "%")
    {
      return builder.CreateSRem(L, R, "sremtmp");
    }
  }
  else if (L->getType()->isFloatTy() && R->getType()->isFloatTy())
  {

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
      return builder.CreateFRem(L, R, "fremtmp");
    }
  }
  else
  {

    if (L->getType()->isIntegerTy() && R->getType()->isFloatTy())
    {
      L = builder.CreateSIToFP(L, Type::getFloatTy(context), "int2fp");
    }
    else if (L->getType()->isFloatTy() && R->getType()->isIntegerTy())
    {
      R = builder.CreateSIToFP(R, Type::getFloatTy(context), "int2fp");
    }

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
      return builder.CreateFRem(L, R, "fremtmp");
    }
  }

  module->getContext().emitError("Unsupported multiplicative operator: " + expr->optr);

  return nullptr;
}

Value *IRGenerator::generate_unary_expr(UnaryExpression *expr)
{
  Value *operand = generate_expression(expr->operand);
  if (!operand)
    return nullptr;

  llvm::Type *ty = operand->getType();

  if (expr->optr == "NOT()")
  {
    if (!ty->isIntegerTy(1))
    {
      operand = builder.CreateICmpNE(operand, ConstantInt::get(ty, 0), "boolcast");
    }
    return builder.CreateNot(operand, "nottmp");
  }

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

  if (expr->optr == "#")
  {
    Value *arrStructPtr = nullptr;
    SymbolEntry info;

    // 1) Determine if we're doing #arr or #arr[i]
    if (auto *id = dynamic_cast<Identifier *>(expr->operand))
    {
      // #arr
      auto entryOpt = findSymbol(id->name);
      if (!entryOpt)
      {
        module->getContext().emitError("Undefined variable: " + id->name);
        return nullptr;
      }
      info = *entryOpt;

      // load the ArrayN* from its alloca
      arrStructPtr = builder.CreateLoad(
          info.llvmType,  // should be ArrayN*
          info.llvmValue, // the alloca of type ArrayN*
          id->name + ".struct.load");
    }
    else if (auto *ix = dynamic_cast<IndexExpression *>(expr->operand))
    {
      // #arr[i]  → we want the sub‐array struct, not the element
      // First find the root identifier and how many inds
      int count = 0;
      Expression *cur = ix;
      while (auto *sub = dynamic_cast<IndexExpression *>(cur))
      {
        count++;
        cur = sub->base;
      }
      auto *rootId = dynamic_cast<Identifier *>(cur);
      if (!rootId)
      {
        module->getContext().emitError(
            "# operator requires arr or arr[i]...");
        return nullptr;
      }
      auto entryOpt = findSymbol(rootId->name);
      if (!entryOpt)
      {
        module->getContext().emitError("Undefined variable: " + rootId->name);
        return nullptr;
      }
      info = *entryOpt;

      // We need a little helper that returns the address of the sub‐array struct:
      //   e.g. from ArrayN* data pointer to Array(N−1)* alloca
      // Let’s write it inline here:

      //  a) generate the raw element GEP + load chain for sub‐array
      Type *dummyTy = nullptr;
      // This returns the *address* of the sub‐array or element slot
      Value *gepAddr = generate_index_expression(ix, &dummyTy);

      //  b) since there are still sub‐arrays left (count < dims), this gepAddr
      //     is actually a pointer to a Array(N-count)*, not to a scalar.
      //     So we can treat gepAddr itself as our struct pointer:
      arrStructPtr = gepAddr;
      //  c) adjust the dimensions for our struct type below:
      info.dimensions = info.dimensions - count;
    }
    else
    {
      module->getContext().emitError("# operator requires variable or indexed array");
      return nullptr;
    }

    // At this point arrStructPtr is an ArrayM* (for M>=1)
    if (info.dimensions < 1)
    {
      module->getContext().emitError("# operator on non-array");
      return nullptr;
    }

    // 2) Reconstruct the ArrayM struct type
    Type *elt0Ty = getLLVMType(info.baseType, /*dim*/ 0);
    StructType *arrSt = getOrCreateArrayStruct(elt0Ty, info.dimensions, "array");

    // 3) GEP into the .length field and load it
    Value *lenGEP = builder.CreateStructGEP(
        arrSt,
        arrStructPtr,
        /*fieldNo=*/1,
        "arr.len.gep");
    Value *lenVal = builder.CreateLoad(
        arrSt->getElementType(1), // i32
        lenGEP,
        "arr.len");
    // final result
    return builder.CreateIntCast(lenVal,
                                 Type::getInt32Ty(context),
                                 /*isSigned=*/false,
                                 "len32");
  }

  if (expr->optr == "++" || expr->optr == "--")
  {
    auto id = dynamic_cast<Identifier *>(expr->operand);
    Value *addr = generate_identifier_address(id);
    if (!addr)
      return nullptr;

    Value *current = builder.CreateLoad(ty, addr, id->name);
    Value *one = ty->isIntegerTy() ? ConstantInt::get(ty, 1) : ConstantFP::get(ty, 1.0);
    Value *result = (expr->optr == "++") ? (ty->isIntegerTy() ? builder.CreateAdd(current, one) : builder.CreateFAdd(current, one)) : (ty->isIntegerTy() ? builder.CreateSub(current, one) : builder.CreateFSub(current, one));
    builder.CreateStore(result, addr);
    return expr->postfix ? current : result;
  }

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

  if (expr->optr == "$")
  {
    FunctionCallee convertFunc;

    if (ty->isIntegerTy(1))
    {

      convertFunc = module->getOrInsertFunction(
          "boolToString",
          FunctionType::get(PointerType::getUnqual(Type::getInt8Ty(context)), {ty}, false));
    }
    else if (ty->isIntegerTy(32))
    {

      convertFunc = module->getOrInsertFunction(
          "intToString",
          FunctionType::get(PointerType::getUnqual(Type::getInt8Ty(context)), {ty}, false));
    }
    else if (ty->isFloatingPointTy())
    {

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

Value *IRGenerator::generate_call(CallFunctionExpression *call)
{

  llvm::Function *callee = module->getFunction(call->function->name);
  if (!callee)
  {
    module->getContext().emitError("Unknown function: " + call->function->name);
    return nullptr;
  }

  FunctionDefinition *funcDef = nullptr;
  auto it = functionTable.find(call->function->name);
  if (it != functionTable.end())
  {
    funcDef = it->second;
  }
  else
  {
    module->getContext().emitError("No AST for function: " + call->function->name);
    return nullptr;
  }

  size_t numParams = callee->arg_size();
  size_t numArgs = call->arguments.size();

  if (numArgs > numParams)
  {
    module->getContext().emitError("Too many arguments for function: " + call->function->name);
    return nullptr;
  }

  std::vector<Value *> args;
  auto paramIt = callee->arg_begin();

  for (size_t i = 0; i < numParams; ++i)
  {
    Value *argVal = nullptr;

    if (i < numArgs)
    {

      argVal = generate_expression(call->arguments[i]);
    }
    else
    {

      auto paramDef = funcDef->parameters[i]->def;
      if (auto init = dynamic_cast<VariableInitialization *>(paramDef))
      {
        argVal = generate_expression(init->initializer);
      }
      else
      {
        module->getContext().emitError("Missing argument and no default for parameter " + std::to_string(i));
        return nullptr;
      }
    }

    if (!argVal)
      return nullptr;

    if (paramIt != callee->arg_end())
    {
      llvm::Type *expectedType = paramIt->getType();
      llvm::Type *actualType = argVal->getType();

      if (expectedType->isDoubleTy() && actualType->isFloatTy())
      {
        argVal = builder.CreateFPExt(argVal, expectedType);
      }
      else if (expectedType->isFloatTy() && actualType->isDoubleTy())
      {
        argVal = builder.CreateFPTrunc(argVal, expectedType);
      }
      else if (expectedType->isIntegerTy(32) && actualType->isIntegerTy(1))
      {
        argVal = builder.CreateZExt(argVal, expectedType);
      }

      ++paramIt;
    }

    args.push_back(argVal);
  }

  return builder.CreateCall(callee, args, "calltmp");
}

Value *IRGenerator::generate_index_expression(IndexExpression *expr, Type **outElementType)
{
  // 1) peel off root-id + collect all index values
  const Identifier *rootId = nullptr;
  std::vector<Value *> indices;
  {
    IndexExpression *cur = expr;
    while (cur)
    {
      indices.push_back(generate_node(cur->index));
      if (auto *id = dynamic_cast<Identifier *>(cur->base))
      {
        rootId = id;
        break;
      }
      cur = dynamic_cast<IndexExpression *>(cur->base);
    }
  }
  if (!rootId)
  {
    module->getContext().emitError("Bad base in index expression");
    return nullptr;
  }
  std::reverse(indices.begin(), indices.end());

  // 2) lookup symbol
  auto entryOpt = findSymbol(rootId->name);
  if (!entryOpt)
  {
    module->getContext().emitError("Undefined array: " + rootId->name);
    return nullptr;
  }
  SymbolEntry entry = *entryOpt;
  SymbolType baseTy = entry.baseType;
  int totalD = entry.dimensions;
  if (indices.size() > (size_t)totalD)
  {
    module->getContext()
        .emitError("Too many indices for array " + rootId->name);
    return nullptr;
  }

  // 3) rebuild ArrayN struct type { T* data; i32 len; }
  Type *elt0Ty = getLLVMType(baseTy, /*dim=*/0);
  StructType *arrSt = getOrCreateArrayStruct(elt0Ty, totalD, "array");

  // 4) load the top‐level .data pointer
  Value *dataGEP = builder.CreateStructGEP(
      arrSt,
      entry.llvmValue, // the AllocaInst* holding ArrayN
      /*field=*/0,
      rootId->name + ".data_gep");
  Value *currentPtr = builder.CreateLoad(
      arrSt->getElementType(0), // the T* or Array(N-1)*
      dataGEP,
      rootId->name + ".data_load");

  // 5) load the top‐level length (for the dim0 bounds check)
  Value *lenGEP = builder.CreateStructGEP(
      arrSt,
      entry.llvmValue,
      /*field=*/1,
      rootId->name + ".len_gep");
  Value *lengthVal = builder.CreateLoad(
      arrSt->getElementType(1), // i32
      lenGEP,
      rootId->name + ".len_load");

  // 6) walk each ‘[i]’
  for (size_t i = 0; i < indices.size(); ++i)
  {
    int remDims = totalD - int(i) - 1;

    // 6a) first‐dim bounds‐check
    if (i == 0)
    {
      Value *ok = builder.CreateICmpULT(
          indices[i], lengthVal,
          rootId->name + ".in_bounds");
      Function *F = builder.GetInsertBlock()->getParent();
      BasicBlock *thenBB = BasicBlock::Create(context, "in_bounds", F);
      BasicBlock *elseBB = BasicBlock::Create(context, "out_of_bounds", F);
      BasicBlock *merge = BasicBlock::Create(context, "after_bounds", F);

      builder.CreateCondBr(ok, thenBB, elseBB);
      builder.SetInsertPoint(elseBB);
      builder.CreateBr(merge);
      builder.SetInsertPoint(thenBB);
      builder.CreateBr(merge);
      builder.SetInsertPoint(merge);

      PHINode *phi = builder.CreatePHI(
          Type::getInt32Ty(context), 2,
          rootId->name + ".idx_safe");
      phi->addIncoming(ConstantInt::get(context, APInt(32, 0)), elseBB);
      phi->addIncoming(indices[i], thenBB);
      indices[i] = phi;
    }

    // 6b) load nested .data pointer before indexing if remDims > 0
    if (remDims > 0)
    {
      StructType *subArrStruct = getOrCreateArrayStruct(elt0Ty, remDims, "array");
      Value *subDataGEP = builder.CreateStructGEP(subArrStruct, currentPtr, 0);
      currentPtr = builder.CreateLoad(subArrStruct->getElementType(0), subDataGEP);
    }

    // 6c) final access: GEP to the element
    if (remDims == 0)
    {
      Value *elemGEP = builder.CreateInBoundsGEP(
          Type::getInt32Ty(context),
          currentPtr,
          {indices[i]},
          rootId->name + ".elem_gep");
      if (outElementType)
        *outElementType = Type::getInt32Ty(context);
      return elemGEP;
    }

    // 6d) GEP to next level struct pointer
    Type *ptrTy = getLLVMType(baseTy, remDims);
    Value *slot = builder.CreateInBoundsGEP(
        ptrTy,
        currentPtr,
        {indices[i]},
        rootId->name + ".idx" + std::to_string(i));

    currentPtr = builder.CreateLoad(ptrTy, slot, rootId->name + ".nested_load" + std::to_string(i));
  }

  // 7) no indices? return the raw data ptr
  if (outElementType)
    *outElementType = getLLVMType(baseTy, totalD);
  return currentPtr;
}

Function *IRGenerator::declareMalloc()
{
  Function *mallocFn = module->getFunction("malloc");
  if (!mallocFn)
  {
    FunctionType *mallocType = FunctionType::get(
        builder.getPtrTy(),
        {Type::getInt64Ty(context)},
        false);
    mallocFn = Function::Create(
        mallocType,
        Function::ExternalLinkage,
        "malloc",
        module.get());
  }
  return mallocFn;
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

Value *IRGenerator::castToBoolean(Value *value)
{
  if (value->getType()->isIntegerTy(1))
    return value;

  return builder.CreateICmpNE(
      value,
      ConstantInt::get(value->getType(), 0),
      "castbool");
}

Value *IRGenerator::generate_identifier_address(Identifier *id)
{

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
      return it->second;
    }
    tempStack.pop();
  }
  return std::nullopt;
}

Value *IRGenerator::generate_l_value(Expression *expr)
{
  if (auto id = dynamic_cast<Identifier *>(expr))
  {
    auto entry = findSymbol(id->name);
    if (!entry)
      return nullptr;
    return entry->llvmValue;
  }
  if (auto index = dynamic_cast<IndexExpression *>(expr))
  {
    Type *dummy;
    return generate_index_expression(index, &dummy);
  }
  return nullptr;
}

llvm::StructType *IRGenerator::getOrCreateArrayStruct(llvm::Type *elementType, unsigned dimension, const std::string &baseName)
{
  // Unique key per elementType+dimension
  std::string key = baseName + "_" + std::to_string(dimension) + "_" + std::to_string((uintptr_t)elementType);
  auto it = arrayStructCache.find(key);
  if (it != arrayStructCache.end())
    return it->second;

  // Decide the data field type
  llvm::Type *dataType;
  if (dimension == 1)
  {
    // 1D: data is T*
    dataType = llvm::PointerType::getUnqual(elementType);
  }
  else
  {
    // ND>1: data is a pointer to the (D-1)-dim struct
    auto innerStruct = getOrCreateArrayStruct(elementType, dimension - 1, baseName);
    dataType = llvm::PointerType::getUnqual(innerStruct);
  }

  // Create struct { dataType data; i32 length; }
  auto st = llvm::StructType::create(context, baseName + std::to_string(dimension));
  st->setBody({dataType, llvm::Type::getInt32Ty(context)});
  arrayStructCache[key] = st;
  return st;
}

Value *IRGenerator::createJaggedArrayStruct(ArrayLiteral *lit, Function *mallocFn, SymbolType *outBaseType, int *outDimensions)
{
  // 1) Determine dimensions and base element type
  int dims = 0;
  ArrayLiteral *cur = lit;
  // Drill down to primitive literal
  while (auto sub = dynamic_cast<ArrayLiteral *>(cur->elements[0]))
  {
    dims++;
    cur = sub;
  }
  dims++; // count last level
  // Get element type from first non‐array literal
  auto firstElem = cur->elements[0];
  Type *llvmEltType = nullptr;
  if (dynamic_cast<IntegerLiteral *>(firstElem))
    llvmEltType = Type::getInt32Ty(context), *outBaseType = SymbolType::Integer;
  else if (dynamic_cast<FloatLiteral *>(firstElem))
    llvmEltType = Type::getFloatTy(context), *outBaseType = SymbolType::Float;
  else if (dynamic_cast<BooleanLiteral *>(firstElem))
    llvmEltType = Type::getInt1Ty(context), *outBaseType = SymbolType::Boolean;
  else
    llvmEltType = PointerType::getUnqual(Type::getInt8Ty(context)), *outBaseType = SymbolType::String;

  *outDimensions = dims;

  // 2) Get the struct type for this dimension
  auto arrStructTy = getOrCreateArrayStruct(llvmEltType, dims, "Array");

  // 3) Allocate the struct on the stack (or heap if global)
  auto arrAlloca = builder.CreateAlloca(arrStructTy, nullptr, "array_d" + std::to_string(dims));

  // 4) Set length = lit->elements.size()
  int len = lit->elements.size();
  auto lenGEP = builder.CreateStructGEP(arrStructTy, arrAlloca, 1, "lenGEP");
  builder.CreateStore(builder.getInt32(len), lenGEP);

  // 5) Allocate data buffer: element type = pointer to dim−1 struct (or primitive*)
  Type *dataEltTy = (dims == 1)
                        ? llvmEltType
                        : PointerType::getUnqual(getOrCreateArrayStruct(llvmEltType, dims - 1, "Array"));

  // total bytes = len * sizeof(dataEltTy)
  auto dl = module->getDataLayout();
  uint64_t eltSize = dl.getTypeAllocSize(dataEltTy);
  auto dataSize = builder.CreateMul(
      builder.getInt64(len), ConstantInt::get(builder.getInt64Ty(), eltSize));
  auto raw = builder.CreateCall(mallocFn, {dataSize}, "malloc_d" + std::to_string(dims));
  auto dataPtr = builder.CreateBitCast(raw, PointerType::getUnqual(dataEltTy), "dataPtr");

  // store data pointer
  auto dataGEP = builder.CreateStructGEP(arrStructTy, arrAlloca, 0, "dataGEP");
  builder.CreateStore(dataPtr, dataGEP);

  // 6) Recursively initialize each element if dimension>1
  if (dims > 1)
  {
    for (int i = 0; i < len; ++i)
    {
      // sub‐literal
      auto subLit = dynamic_cast<ArrayLiteral *>(lit->elements[i]);
      // create subarray
      SymbolType dummyBase;
      int dummyDim;
      auto subArr = createJaggedArrayStruct(subLit, mallocFn, &dummyBase, &dummyDim);
      // store pointer into dataPtr[i]
      auto idx = builder.getInt32(i);
      auto slot = builder.CreateGEP(dataEltTy, dataPtr, idx, "slot" + std::to_string(i));
      builder.CreateStore(subArr, slot);
    }
  }
  else
  {
    for (int i = 0; i < len; ++i)
    {
      auto litElem = dynamic_cast<IntegerLiteral *>(lit->elements[i]);
      Value *v = ConstantInt::get(Type::getInt32Ty(context), litElem->value);
      auto slot = builder.CreateGEP(dataEltTy, dataPtr, builder.getInt32(i));
      builder.CreateStore(v, slot);
    }
  }

  return arrAlloca;
}

StructType *IRGenerator::getArrayStruct(const SymbolEntry &entry)
{
  // element‐type = primitive @ dim=0
  Type *elt0 = getLLVMType(entry.baseType, /*dim*/ 0);
  return getOrCreateArrayStruct(elt0, entry.dimensions, "array");
}

Value *IRGenerator::loadArrayLength(Value *arrayAlloca, StructType *arrSt, const Twine &name)
{
  // GEP to field #1 (.length) then load i32
  Value *lenGEP = builder.CreateStructGEP(
      arrSt,
      arrayAlloca,
      /*idx=*/1,
      name + ".len.gep");
  return builder.CreateLoad(
      arrSt->getElementType(1), // i32
      lenGEP,
      name + ".len");
}

Value *IRGenerator::callStrlen(Value *strPtr, const Twine &name)
{
  // declare or lookup strlen(i8*)->i64
  auto *i8ptr = PointerType::getUnqual(Type::getInt8Ty(context));
  FunctionCallee strlenFn = module->getOrInsertFunction(
      "strlen",
      FunctionType::get(Type::getInt64Ty(context), {i8ptr}, false));
  // call + truncate
  Value *len64 = builder.CreateCall(strlenFn, {strPtr}, name + ".strlen");
  return builder.CreateTrunc(len64,
                             Type::getInt32Ty(context),
                             name + ".len32");
}