#include "ir_generator.h"

#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/Support/raw_ostream.h"

LLVMContext IRGenerator::context;

IRGenerator::IRGenerator(SemanticAnalyzer *semantic) : builder(context)
{
  source = semantic->analyze();
  module = make_unique<Module>("main_module", context);
  pushScope();
}

Type *IRGenerator::getLLVMType(SymbolType type, int dim)
{
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

  for (int i = 0; i < dim; i++)
  {
    baseType = PointerType::getUnqual(baseType);
  }
  return baseType;
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

Value *IRGenerator::generate_program(ProgramDefinition *program) {
  FunctionType *FT = FunctionType::get(Type::getInt32Ty(context), false);
  Function *Main = Function::Create(FT, Function::ExternalLinkage,
                                    "main", module.get());
  BasicBlock *EntryBB = BasicBlock::Create(context, "entry", Main);
  builder.SetInsertPoint(EntryBB);

  for (auto *G : program->globals)
    generate_global_variable(G);

  for (auto *G : program->globals) {
    auto *initStmt = dynamic_cast<VariableInitialization*>(G->def);
    if (!initStmt) continue;

    const string &name = initStmt->name->name;
    if (auto *lit = dynamic_cast<ArrayLiteral*>(initStmt->initializer)) {
      auto sym = findSymbol(name);
      assert(sym && "global var missing");
      GlobalVariable *gvar = cast<GlobalVariable>(sym->llvmValue);

      SymbolType baseType;
      int dimensions;
      Function *mallocFn = declareMalloc();
      Value *jaggedPtr = createJaggedArray(lit,
                                           mallocFn,
                                           &baseType,
                                           &dimensions);
      builder.CreateStore(jaggedPtr, gvar);

      auto lenSym = findSymbol(name + "_len");
      if (lenSym && lenSym->llvmValue) {
        builder.CreateStore(
          builder.getInt32(lit->elements.size()),
          lenSym->llvmValue);
      }
    }
  }

  pushScope();
  for (auto *stmt : program->body)
    generate_node(stmt);

  auto pauseFn = module->getOrInsertFunction(
      "waitForKeypress", FunctionType::get(Type::getVoidTy(context), {}, false));
  builder.CreateCall(pauseFn);
  builder.CreateRet(ConstantInt::get(context, APInt(32, 0)));

  popScope();
  return Main;
}

Value* IRGenerator::generate_function(FunctionDefinition *func) {
  SmallVector<Type*,8> paramTypes;
  SmallVector<VariableDeclaration*,8> paramDecls;
  SmallVector<VariableInitialization*,8> paramInits;

  for (auto *paramDef : func->parameters) {
    if (auto *decl = dynamic_cast<VariableDeclaration*>(paramDef->def)) {
      Type *ty = getLLVMType(
          Symbol::get_datatype(decl->datatype),
          Symbol::get_dimension(decl->datatype));
      paramTypes.push_back(ty);
      paramDecls.push_back(decl);
      paramInits.push_back(nullptr);
    }
    else if (auto *init = dynamic_cast<VariableInitialization*>(paramDef->def)) {
      Type *ty = getLLVMType(
          Symbol::get_datatype(init->datatype),
          Symbol::get_dimension(init->datatype));
      paramTypes.push_back(ty);
      paramDecls.push_back(nullptr);
      paramInits.push_back(init);
    }
  }

  Type  *retTy = getLLVMType(
      Symbol::get_datatype(func->return_type->return_type),
      Symbol::get_dimension(func->return_type->return_type));
  FunctionType *ft = FunctionType::get(retTy, paramTypes, false);
  Function     *F = Function::Create(
                       ft,
                       Function::ExternalLinkage,
                       func->funcname->name,
                       module.get());
  BasicBlock *entryBB = BasicBlock::Create(context, "entry", F);
  builder.SetInsertPoint(entryBB);

  pushScope();
  unsigned idx = 0;
  for (auto &Arg : F->args()) {
    std::string name;
    SymbolType baseType;
    int        dimensions;
    if (paramDecls[idx]) {
      name       = paramDecls[idx]->variables[0]->name;
      baseType   = Symbol::get_datatype(paramDecls[idx]->datatype);
      dimensions = Symbol::get_dimension(paramDecls[idx]->datatype);
    } else {
      auto *init = paramInits[idx];
      name       = init->name->name;
      baseType   = Symbol::get_datatype(init->datatype);
      dimensions = Symbol::get_dimension(init->datatype);
    }

    Type *ParamTy = getLLVMType(baseType, dimensions);
    AllocaInst *paramAlloca = builder.CreateAlloca(ParamTy, nullptr, name);
    builder.CreateStore(&Arg, paramAlloca);

    symbolTable.top()[name] = {
        ParamTy,    
        paramAlloca,
        baseType,
        dimensions,
        0,          
        nullptr     
    };

    if (dimensions > 0) {
      AllocaInst *lenAlloca =
         builder.CreateAlloca(Type::getInt32Ty(context),
                              nullptr,
                              name + "_len");

      
      Value *loadedPtr = builder.CreateLoad(
          ParamTy,          
          paramAlloca,
          name + ".ptr");

      Type *slotElemTy = (dimensions > 1)
         ? getLLVMType(baseType, dimensions - 1)
         : getLLVMType(baseType, 0);
      uint64_t slotSize = module
          ->getDataLayout()
          .getTypeAllocSize(slotElemTy);

      auto *i8p = PointerType::getUnqual(Type::getInt8Ty(context));
      Value *asI8 = builder.CreateBitCast(loadedPtr, i8p, name + ".hdr.cast");
      Value *off   = ConstantInt::get(
                         Type::getInt64Ty(context),
                         -int64_t(slotSize));
      Value *hdrGep = builder.CreateInBoundsGEP(
                         Type::getInt8Ty(context),
                         asI8,
                         off,
                         name + ".hdr.gep");
      Value *lenPtr = builder.CreateBitCast(
                         hdrGep,
                         PointerType::getUnqual(Type::getInt32Ty(context)),
                         name + ".len.ptr");
      Value *trueLen = builder.CreateLoad(
                         Type::getInt32Ty(context),
                         lenPtr,
                         name + ".len.load");

      builder.CreateStore(trueLen, lenAlloca);

      auto &entry = symbolTable.top()[name];
      entry.lengthAlloca = lenAlloca;
      entry.arrayLength =  
                         0;
      symbolTable.top()[name + "_len"] = {
        Type::getInt32Ty(context),
        lenAlloca,
        SymbolType::Integer,
        0,
        0,
        nullptr
      };
    }

    ++idx;
  }

  
  for (auto *stmt : func->body)
    generate_node(stmt);

  
  if (retTy->isVoidTy() && !builder.GetInsertBlock()->getTerminator())
    builder.CreateRetVoid();

  popScope();
  return F;
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

void IRGenerator::generate_global_variable(VariableDefinition *dif)
{
  if (auto varDecl = dynamic_cast<VariableDeclaration *>(dif->def))
  {
    SymbolType baseType = Symbol::get_datatype(varDecl->datatype);
    int dimensions = Symbol::get_dimension(varDecl->datatype);
    Type *ty = getLLVMType(baseType, dimensions);

    Constant *init = Constant::getNullValue(ty);

    for (auto var : varDecl->variables)
    {
      std::string name = var->name;

      GlobalVariable *gVar = new GlobalVariable(
          *module,
          ty,
          false,
          GlobalValue::ExternalLinkage,
          init,
          name);

      symbolTable.top()[name] = {
          ty,
          gVar,
          baseType,
          dimensions,
          0,
          nullptr};

      if (dimensions > 0)
      {
        GlobalVariable *lengthGlobal = new GlobalVariable(
            *module,
            Type::getInt32Ty(context),
            false,
            GlobalValue::ExternalLinkage,
            ConstantInt::get(Type::getInt32Ty(context), 0),
            name + "_len");

        symbolTable.top()[name + "_len"] = {
            Type::getInt32Ty(context),
            lengthGlobal,
            SymbolType::Integer,
            0,
            0,
            nullptr};
      }
    }
  }
  else if (auto varInit = dynamic_cast<VariableInitialization *>(dif->def))
  {
    SymbolType baseType = Symbol::get_datatype(varInit->datatype);
    int dimensions = Symbol::get_dimension(varInit->datatype);
    Type *ty = getLLVMType(baseType, dimensions);
    std::string name = varInit->name->name;

    Constant *init = nullptr;
    size_t arrayLength = 0;

    if (auto arrayLit = dynamic_cast<ArrayLiteral *>(varInit->initializer))
    {
      if (dimensions == 1 && isUniformArray(arrayLit))
      {
        std::vector<Constant *> elements;
        Type *elementType = getLLVMType(baseType, 0);

        for (auto elem : arrayLit->elements)
        {
          elements.push_back(cast<Constant>(generate_node(elem)));
        }

        ArrayType *arrType = ArrayType::get(elementType, elements.size());
        init = ConstantArray::get(arrType, elements);
        ty = arrType;
        arrayLength = elements.size();
      }
      else
      {
        ty = getLLVMType(baseType, dimensions);
        init = ConstantPointerNull::get(cast<PointerType>(ty));
        arrayLength = arrayLit->elements.size();
      }
    }
    else
    {
      Value *initVal = generate_expression(varInit->initializer);
      init = dyn_cast<Constant>(initVal);
      if (!init)
      {
        init = Constant::getNullValue(ty);
      }
    }

    GlobalVariable *gVar = new GlobalVariable(
        *module, ty, false, GlobalValue::ExternalLinkage, init, name);

    symbolTable.top()[name] = {
        ty,
        gVar,
        baseType,
        dimensions,
        arrayLength,
        nullptr};

    if (dimensions > 0)
    {
      GlobalVariable *lengthGlobal = new GlobalVariable(
          *module,
          Type::getInt32Ty(context),
          false,
          GlobalValue::ExternalLinkage,
          ConstantInt::get(Type::getInt32Ty(context), arrayLength),
          name + "_len");

      symbolTable.top()[name + "_len"] = {
          Type::getInt32Ty(context),
          lengthGlobal,
          SymbolType::Integer,
          0,
          0,
          nullptr};
    }
  }
  else
  {
    module->getContext().emitError("Unknown variable definition type");
  }
}

Value *IRGenerator::generate_variable_initialization(VariableInitialization *init)
{
  SymbolType baseType = Symbol::get_datatype(init->datatype);
  int dimensions = Symbol::get_dimension(init->datatype);
  Type *ty = getLLVMType(baseType, dimensions);
  std::string varName = init->name->name;

  AllocaInst *alloca = builder.CreateAlloca(ty, nullptr, varName);
  AllocaInst *lengthAlloca = nullptr;
  size_t arraySize = 0;

  if (dimensions > 0)
  {
    lengthAlloca = builder.CreateAlloca(Type::getInt32Ty(context), nullptr, varName + "_len");
  }

  if (auto arrayLit = dynamic_cast<ArrayLiteral *>(init->initializer))
  {
    SymbolType actualBaseType;
    int actualDimensions;
    arraySize = arrayLit->elements.size();

    Value *jagged = createJaggedArray(arrayLit, declareMalloc(), &actualBaseType, &actualDimensions);

    if (actualBaseType != baseType || actualDimensions != dimensions)
    {
      module->getContext().emitError("Array literal type doesn't match declaration");
      return nullptr;
    }

    builder.CreateStore(jagged, alloca);

    if (lengthAlloca)
    {
      builder.CreateStore(builder.getInt32(arraySize), lengthAlloca);
    }

    symbolTable.top()[varName] = {
        ty, alloca, baseType, dimensions, arraySize, lengthAlloca};

    if (lengthAlloca)
    {
      symbolTable.top()[varName + "_len"] = {
          Type::getInt32Ty(context), lengthAlloca, SymbolType::Integer, 0, 0, nullptr};
    }
  }
  else if (init->initializer)
  {
    Value *initVal = generate_node(init->initializer);
    builder.CreateStore(initVal, alloca);

    if (lengthAlloca)
    {
      builder.CreateStore(builder.getInt32(0), lengthAlloca);
    }

    symbolTable.top()[varName] = {
        ty, alloca, baseType, dimensions, arraySize, lengthAlloca};

    if (lengthAlloca)
    {
      symbolTable.top()[varName + "_len"] = {
          Type::getInt32Ty(context), lengthAlloca, SymbolType::Integer, 0, 0, nullptr};
    }
  }
  else
  {
    if (lengthAlloca)
    {
      builder.CreateStore(builder.getInt32(0), lengthAlloca);
    }

    symbolTable.top()[varName] = {
        ty, alloca, baseType, dimensions, arraySize, lengthAlloca};

    if (lengthAlloca)
    {
      symbolTable.top()[varName + "_len"] = {
          Type::getInt32Ty(context), lengthAlloca, SymbolType::Integer, 0, 0, nullptr};
    }
  }

  return alloca;
}

Value *IRGenerator::generate_variable_declaration(VariableDeclaration *decl)
{
  SymbolType baseType = Symbol::get_datatype(decl->datatype);
  int dimensions = Symbol::get_dimension(decl->datatype);
  Type *ty = getLLVMType(baseType, dimensions);

  for (auto var : decl->variables)
  {
    AllocaInst *alloca = builder.CreateAlloca(ty, nullptr, var->name);
    symbolTable.top()[var->name] = {ty, alloca, baseType, dimensions};
  }
  return nullptr;
}

Value *IRGenerator::generate_expression(Expression *expr)
{
  if (auto index = dynamic_cast<IndexExpression *>(expr))
  {
    Type *elementType = nullptr;
    Value *gep = generate_index_expression(index, &elementType);
    return builder.CreateLoad(elementType, gep, "loadidx");
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
  Value *target = generate_l_value(assign->assignee);
  Value *value = generate_expression(assign->value);

  if (!target || !value)
    return nullptr;

  std::string varName;
  if (auto id = dynamic_cast<Identifier *>(assign->assignee))
  {
    varName = id->name;
  }
  else if (auto index = dynamic_cast<IndexExpression *>(assign->assignee))
  {

    builder.CreateStore(value, target);
    return target;
  }

  if (value->getType()->isPointerTy() && !varName.empty())
  {

    builder.CreateStore(value, target);

    size_t arraySize = 0;

    if (auto arrayLit = dynamic_cast<ArrayLiteral *>(assign->value))
    {
      arraySize = arrayLit->elements.size();

      auto lenEntry = findSymbol(varName + "_len");
      if (lenEntry && lenEntry->llvmValue)
      {
        builder.CreateStore(
            builder.getInt32(arraySize),
            lenEntry->llvmValue);

        auto varEntry = findSymbol(varName);
        if (varEntry)
        {

          symbolTable.top()[varName] = {
              varEntry->llvmType,
              varEntry->llvmValue,
              varEntry->baseType,
              varEntry->dimensions,
              arraySize,
              lenEntry->llvmValue};
        }
      }
    }
    return target;
  }

  builder.CreateStore(value, target);
  return target;
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

  if (expr->optr == "#") {
     if (auto *id = dynamic_cast<Identifier*>(expr->operand)) {
      auto symOpt = findSymbol(id->name);
      if (symOpt &&
          symOpt->baseType   == SymbolType::String &&
          symOpt->dimensions == 0)
      {
        Value *strPtr = generate_expression(id);
        if (!strPtr) return nullptr;

        auto &C    = context;
        Type *i8Ty   = Type::getInt8Ty(C);
        Type *i8Ptr  = PointerType::getUnqual(i8Ty);
        Type *i64Ty  = Type::getInt64Ty(C);
        Type *i32Ty  = Type::getInt32Ty(C);

        FunctionCallee strlenFn = module->getOrInsertFunction(
          "strlen",
          FunctionType::get(i64Ty, { i8Ptr },false));

        Value *len64 = builder.CreateCall(strlenFn,{ strPtr },"strlen.call");
        return builder.CreateTrunc(len64,i32Ty,"strlen.i32");
      }
    }
    int numIndices = 0;
    Identifier *rootId = nullptr;
    Expression *cur = expr->operand;
    while (auto *ix = dynamic_cast<IndexExpression*>(cur)) {
      numIndices++;
      cur = ix->base;
    }
    rootId = dynamic_cast<Identifier*>(cur);
    if (!rootId) {
      module->getContext().emitError(
        "# operator requires an identifier or index expression");
      return nullptr;
    }
  
    Value *slotAddr = nullptr;
    Type  *dummyTy = nullptr;
    if (numIndices > 0) {
      slotAddr = generate_index_expression(static_cast<IndexExpression*>(expr->operand),&dummyTy);
    } else {
      auto sym = findSymbol(rootId->name);
      if (!sym) return nullptr;
      slotAddr = sym->llvmValue;           
    }
  
   
    Value *dataPtr = nullptr;
    if (numIndices > 0 ||                       
        dynamic_cast<IndexExpression*>(expr->operand)) {
      auto sym = *findSymbol(rootId->name);
      int remDims = sym.dimensions - numIndices;
      Type *loadTy = getLLVMType(sym.baseType, remDims);
      dataPtr = builder.CreateLoad(loadTy, slotAddr, "slot.load");
    }
    else {
      dataPtr = builder.CreateLoad(
                  getLLVMType(findSymbol(rootId->name)->baseType,
                               findSymbol(rootId->name)->dimensions),
                  slotAddr,
                  rootId->name + ".load");
    }
    if (!dataPtr) return nullptr;
  
    auto sym = *findSymbol(rootId->name);
    int declaredDims = sym.dimensions;
    int remDims = declaredDims - numIndices;
    if (remDims < 1) {
      module->getContext().emitError(
        "# applied to a non-array or too many indices");
      return nullptr;
    }
  
    Type *slotElemTy = (remDims > 1)
         ? getLLVMType(sym.baseType, remDims - 1)
         : getLLVMType(sym.baseType, 0);
    uint64_t slotSize = module
        ->getDataLayout()
        .getTypeAllocSize(slotElemTy);
  
    auto *i8p = PointerType::getUnqual(Type::getInt8Ty(context));
    Value *asI8 = builder.CreateBitCast(dataPtr, i8p, "hdr.cast");
    Value *off   = ConstantInt::get(Type::getInt64Ty(context),
                                    -int64_t(slotSize));
    Value *hdr = builder.CreateInBoundsGEP(
                   Type::getInt8Ty(context),
                   asI8,
                   off,
                   "hdr.ptr");
  
    Value *lenPtr = builder.CreateBitCast(
                       hdr,
                       PointerType::getUnqual(Type::getInt32Ty(context)),
                       "len.ptr");
    return builder.CreateLoad(
             Type::getInt32Ty(context),
             lenPtr,
             "len.load");
  }

  if (expr->optr == "++" || expr->optr == "--")
  {
    auto id = dynamic_cast<Identifier *>(expr->operand);
    Value *addr = generate_identifier_address(id);
    if (!addr)
      return nullptr;

    Value *current = builder.CreateLoad(ty, addr, id->name);
    Value *one = ty->isIntegerTy() ? ConstantInt::get(ty, 1) : ConstantFP::get(ty, 1.0);
    Value *result = (expr->optr == "++") ? (ty->isIntegerTy() ? builder.CreateAdd(current, one, "addtmp") : builder.CreateFAdd(current, one, "faddtmp")) : (ty->isIntegerTy() ? builder.CreateSub(current, one, "subtmp") : builder.CreateFSub(current, one, "fsubtmp"));
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
  std::vector<Value *> indices;
  const Identifier *rootId = nullptr;
  SymbolType baseType;
  int totalDims;

  IndexExpression *current = expr;
  while (current)
  {
    if (auto id = dynamic_cast<Identifier *>(current->base))
    {
      rootId = id;
      break;
    }
    else if (auto innerExpr = dynamic_cast<IndexExpression *>(current->base))
    {
      indices.push_back(generate_node(current->index));
      current = innerExpr;
    }
    else
    {
      module->getContext().emitError("Unsupported base expression in index");
      return nullptr;
    }
  }

  if (!rootId)
  {
    module->getContext().emitError("Could not find root array in index expression");
    return nullptr;
  }

  while (current)
  {
    indices.push_back(generate_node(current->index));
    current = dynamic_cast<IndexExpression *>(current->base);
  }
  std::reverse(indices.begin(), indices.end());

  auto entryOpt = findSymbol(rootId->name);
  if (!entryOpt)
  {
    module->getContext().emitError("Undefined variable: " + rootId->name);
    return nullptr;
  }
  baseType = entryOpt->baseType;
  totalDims = entryOpt->dimensions;

  if (indices.size() > totalDims)
  {
    module->getContext().emitError("Too many array indices for '" + rootId->name + "'");
    return nullptr;
  }

  Value *currentPtr = builder.CreateLoad(getLLVMType(baseType, totalDims), entryOpt->llvmValue, "array_ptr");

  for (size_t i = 0; i < indices.size(); ++i)
  {
    int remainingDims = totalDims - i - 1;
    Type *elementType = getLLVMType(baseType, remainingDims);

    if (i == 0)
    {
      auto lenEntryOpt = findSymbol(rootId->name + "_len");
      if (lenEntryOpt && lenEntryOpt->llvmValue)
      {
        Value *lengthVal = builder.CreateLoad(Type::getInt32Ty(context),
                                              lenEntryOpt->llvmValue,
                                              "len_load");

        Value *cmp = builder.CreateICmpULT(indices[i], lengthVal, "bounds_check");

        Function *currentFunc = builder.GetInsertBlock()->getParent();
        BasicBlock *thenBB = BasicBlock::Create(context, "in_bounds", currentFunc);
        BasicBlock *elseBB = BasicBlock::Create(context, "out_of_bounds", currentFunc);
        BasicBlock *mergeBB = BasicBlock::Create(context, "after_bounds_check", currentFunc);

        builder.CreateCondBr(cmp, thenBB, elseBB);

        builder.SetInsertPoint(elseBB);

        Value *defaultIndex = ConstantInt::get(Type::getInt32Ty(context), 0);
        builder.CreateBr(mergeBB);

        builder.SetInsertPoint(thenBB);
        builder.CreateBr(mergeBB);

        builder.SetInsertPoint(mergeBB);
        PHINode *indexPhi = builder.CreatePHI(Type::getInt32Ty(context), 2, "safe_index");
        indexPhi->addIncoming(defaultIndex, elseBB);
        indexPhi->addIncoming(indices[i], thenBB);

        indices[i] = indexPhi;
      }
    }

    currentPtr = builder.CreateInBoundsGEP(
        elementType,
        currentPtr,
        {indices[i]},
        "dim" + std::to_string(i) + "_idx");

    if (i < indices.size() - 1)
    {
      currentPtr = builder.CreateLoad(elementType, currentPtr, "elem_ptr");
    }
  }

  if (outElementType)
  {
    *outElementType = getLLVMType(baseType, totalDims - indices.size());
  }

  return currentPtr;
}

bool IRGenerator::isUniformArray(ArrayLiteral *lit)
{

  if (lit->elements.empty())
    return true;

  bool isNested = dynamic_cast<ArrayLiteral *>(lit->elements[0]) != nullptr;
  size_t expectedSize = isNested ? dynamic_cast<ArrayLiteral *>(lit->elements[0])->elements.size() : 0;
  Type *expectedType = generate_node(lit->elements[0])->getType();

  for (auto elem : lit->elements)
  {
    auto sub = dynamic_cast<ArrayLiteral *>(elem);
    if ((sub != nullptr) != isNested)
      return false;

    if (isNested && sub)
    {
      if (sub->elements.size() != expectedSize)
        return false;
      if (!isUniformArray(sub))
        return false;
    }

    if (!isNested && generate_node(elem)->getType() != expectedType)
      return false;
  }

  return true;
}

Type *IRGenerator::getElementType(ArrayLiteral *lit, int *outDim)
{
  int dimensions = 0;
  ArrayLiteral *current = lit;

  while (true)
  {
    dimensions++;
    if (current->elements.empty())
    {
      *outDim = dimensions;
      return Type::getInt32Ty(context);
    }

    auto first = current->elements[0];
    if (auto sub = dynamic_cast<ArrayLiteral *>(first))
    {
      current = sub;
    }
    else
    {
      *outDim = dimensions;
      return generate_node(first)->getType();
    }
  }
}

Value *IRGenerator::createJaggedArray(ArrayLiteral *lit, Function *mallocFn,
                                      SymbolType *outBaseType, int *outDimensions)
{

  int totalDimensions = 0;
  Type *elementType = getElementType(lit, &totalDimensions);
  SymbolType baseType;

  if (elementType->isIntegerTy(32))
    baseType = SymbolType::Integer;
  else if (elementType->isFloatTy())
    baseType = SymbolType::Float;
  else if (elementType->isIntegerTy(1))
    baseType = SymbolType::Boolean;
  else
    baseType = SymbolType::Integer;

  if (outBaseType)
    *outBaseType = baseType;
  if (outDimensions)
    *outDimensions = totalDimensions;

  return createJaggedArrayHelper(lit, mallocFn, totalDimensions, elementType);
}
Constant* IRGenerator::createNestedArray(ArrayLiteral *lit,
  ArrayType     *arrType) {
std::vector<Constant*> elems;
if (!arrType)
arrType = inferArrayType(lit);

Type *eltType = arrType->getElementType();
for (auto *e : lit->elements) {
if (auto *subLit = dynamic_cast<ArrayLiteral*>(e)) {
auto *subArrTy = cast<ArrayType>(eltType);
elems.push_back(createNestedArray(subLit, subArrTy));
}
else {
auto *val = generate_node(e);
auto *c   = dyn_cast<Constant>(val);
assert(c && "expected a Constant for nested array literal");
elems.push_back(c);
}
}
return ConstantArray::get(arrType, elems);
}

ArrayType *IRGenerator::inferArrayType(ArrayLiteral *lit)
{
  std::vector<uint64_t> dims;
  ArrayLiteral *current = lit;
  while (dynamic_cast<ArrayLiteral *>(current->elements[0]))
  {
    dims.push_back(current->elements.size());
    current = dynamic_cast<ArrayLiteral *>(current->elements[0]);
  }
  dims.push_back(current->elements.size());

  Type *elementType = generate_node(current->elements[0])->getType();
  ArrayType *arrType = ArrayType::get(elementType, dims.back());

  for (int i = dims.size() - 2; i >= 0; i--)
  {
    arrType = ArrayType::get(arrType, dims[i]);
  }

  return arrType;
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

Value *IRGenerator::createArrayAllocation(Type *elementType, Value *size)
{
  DataLayout dl = module->getDataLayout();
  uint64_t typeSize = dl.getTypeAllocSize(elementType);
  Value *allocSize = builder.CreateMul(
      size,
      ConstantInt::get(context, APInt(32, typeSize)),
      "total_size");

  Value *rawPtr = builder.CreateCall(
      declareMalloc(),
      {builder.CreateIntCast(allocSize, Type::getInt64Ty(context), false)},
      "malloc");

  return builder.CreateBitCast(rawPtr, PointerType::get(elementType, 0), "array_ptr");
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

Value* IRGenerator::createJaggedArrayHelper(ArrayLiteral *lit,
  Function    *mallocFn,
  int          remainingDims,
  Type        *elementType) {
uint64_t count = lit->elements.size();

Type *slotTy = (remainingDims > 1)
? PointerType::getUnqual(elementType)
: elementType;

auto &DL = module->getDataLayout();
uint64_t slotSize = DL.getTypeAllocSize(slotTy);

Value *allocBytes = ConstantInt::get(
Type::getInt64Ty(context),
slotSize * (count + 1));
Value *raw = builder.CreateCall(mallocFn, {allocBytes}, "raw.mem");

{
Value *hdrPtr = builder.CreateBitCast(
raw,
PointerType::getUnqual(Type::getInt32Ty(context)),
"hdr.ptr");
builder.CreateStore(
ConstantInt::get(Type::getInt32Ty(context), count),
hdrPtr);
}

Value *dataStart = builder.CreateInBoundsGEP(
Type::getInt8Ty(context),
builder.CreateBitCast(raw,
PointerType::getUnqual(Type::getInt8Ty(context)),
"raw.i8"),
ConstantInt::get(Type::getInt64Ty(context), slotSize),
"data.start");

Value *arrayPtr = builder.CreateBitCast(
dataStart,
PointerType::getUnqual(slotTy),
"arr.ptr");

for (unsigned i = 0; i < count; ++i) {
Value *eltAddr = builder.CreateInBoundsGEP(
slotTy,
arrayPtr,
ConstantInt::get(Type::getInt32Ty(context), i),
"elt.addr");

if (remainingDims == 1) {
Value *v = generate_node(lit->elements[i]);
builder.CreateStore(v, eltAddr);
} else {
auto *subLit = dynamic_cast<ArrayLiteral*>(lit->elements[i]);
assert(subLit && "Expected ArrayLiteral here");
Value *subArr = createJaggedArrayHelper(
subLit, mallocFn,
remainingDims - 1,
elementType);
builder.CreateStore(subArr, eltAddr);
}
}

return arrayPtr;
}

bool IRGenerator::isSingleDimensionArray(const SymbolEntry &entry)
{
  return entry.dimensions == 1 &&
         entry.baseType != SymbolType::String;
}