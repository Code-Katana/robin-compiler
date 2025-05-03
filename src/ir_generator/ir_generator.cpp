#include "ir_generator.h"

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

  // Create pointer types for array dimensions
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
    codegenFunction(func);
  }
  // Generate code for program (main function)
  codegenProgram(source->program);

  // Generate code for all functions

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
  if (auto decl = dynamic_cast<VariableDefinition *>(node))
    return codegenVariableDefinition(decl);
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
  FunctionCallee pauseFunc = module->getOrInsertFunction(
      "waitForKeypress",
      FunctionType::get(Type::getVoidTy(context), {}, false));
  BasicBlock *entry = BasicBlock::Create(context, "entry", mainFunc);
  builder.SetInsertPoint(entry);

  // Process global variables (as actual globals)
  for (auto global : program->globals)
  {

    codegenGlobalVariable(global);
  }
  pushScope();

  for (auto stmt : program->body)
  {
    if (auto assign = dynamic_cast<AssignmentExpression *>(stmt))
    {
      if (auto arrayLit = dynamic_cast<ArrayLiteral *>(assign->value))
      {
        Function *mallocFn = declareMalloc();
        SymbolType baseType;
        int dimensions;

        Value *array = createJaggedArray(arrayLit, mallocFn, &baseType, &dimensions);

        // Get the target variable
        if (auto id = dynamic_cast<Identifier *>(assign->assignee))
        {
          Value *target = codegenIdentifierAddress(id);
          builder.CreateStore(array, target);

          // Update symbol table with array type information
          auto entry = findSymbol(id->name);
          if (entry)
          {
            // Update the array length in the symbol table
            size_t arraySize = arrayLit->elements.size();

            // Look for a length variable
            auto lenEntry = findSymbol(id->name + "_len");
            if (lenEntry && lenEntry->llvmValue)
            {
              builder.CreateStore(
                  builder.getInt32(arraySize),
                  lenEntry->llvmValue);

              // Update the symbol table entry
              symbolTable.top()[id->name] = {
                  entry->llvmType,
                  entry->llvmValue,
                  baseType,
                  dimensions,
                  arraySize,
                  lenEntry->llvmValue};
            }
          }
        }
        else
        {
          // Handle other assignee types (like array elements)
          Value *target = codegenLValue(assign->assignee);
          builder.CreateStore(array, target);
        }
        continue;
      }
    }
    codegen(stmt);
  }

  builder.CreateCall(pauseFunc);
  builder.CreateRet(ConstantInt::get(context, APInt(32, 0)));

  popScope();
  return mainFunc;
}

Value *IRGenerator::codegenFunction(FunctionDefinition *func)
{
  // 1. Collect parameter types
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

  // 2. Create function type
  llvm::Type *retType = getLLVMType(
      Symbol::get_datatype(func->return_type->return_type),
      Symbol::get_dimension(func->return_type->return_type));
  llvm::FunctionType *funcType = llvm::FunctionType::get(retType, paramTypes, false);
  llvm::Function *llvmFunc = llvm::Function::Create(
      funcType,
      llvm::Function::ExternalLinkage,
      func->funcname->name,
      module.get());

  // 3. Create entry block
  llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", llvmFunc);
  builder.SetInsertPoint(entry);
  pushScope();

  // 4. Process parameters: allocate and store incoming values
  unsigned idx = 0;
  for (auto &arg : llvmFunc->args())
  {
    std::string paramName;
    SymbolType baseType;
    int dimensions = 0;
    size_t defaultLength = 10; // Default length for arrays

    if (paramDecls[idx])
    {
      // Handle parameter declaration
      auto varName = paramDecls[idx]->variables[0]->name;
      paramName = varName;
      baseType = Symbol::get_datatype(paramDecls[idx]->datatype);
      dimensions = Symbol::get_dimension(paramDecls[idx]->datatype);

      // Create local variable
      Type *ty = getLLVMType(baseType, dimensions);
      AllocaInst *alloca = builder.CreateAlloca(ty, nullptr, varName);
      builder.CreateStore(&arg, alloca);

      // Store in symbol table with default array length
      symbolTable.top()[varName] = {
          ty, alloca, baseType, dimensions, defaultLength, nullptr};
    }
    else if (paramInits[idx])
    {
      // Handle parameter with initialization
      auto varName = paramInits[idx]->name->name;
      paramName = varName;
      baseType = Symbol::get_datatype(paramInits[idx]->datatype);
      dimensions = Symbol::get_dimension(paramInits[idx]->datatype);

      // Create local variable
      Type *ty = getLLVMType(baseType, dimensions);
      AllocaInst *alloca = builder.CreateAlloca(ty, nullptr, varName);
      builder.CreateStore(&arg, alloca);

      // Store in symbol table with default array length
      symbolTable.top()[varName] = {
          ty, alloca, baseType, dimensions, defaultLength, nullptr};
    }

    // For array parameters, create a length variable
    if (dimensions > 0)
    {
      // Create length variable
      AllocaInst *lengthAlloca = builder.CreateAlloca(
          Type::getInt32Ty(context), nullptr, paramName + "_len");

      // Initialize with a non-zero default value
      builder.CreateStore(builder.getInt32(defaultLength), lengthAlloca);

      // Debug output
      errs() << "Setting default length " << defaultLength << " for array parameter " << paramName << "\n";

      // Update symbol table
      auto &entry = symbolTable.top()[paramName];
      entry.lengthAlloca = lengthAlloca;
      entry.arrayLength = defaultLength;

      // Add length variable to symbol table
      symbolTable.top()[paramName + "_len"] = {
          Type::getInt32Ty(context), lengthAlloca, SymbolType::Integer, 0, 0, nullptr};
    }

    ++idx;
  }

  // 5. Process function body
  for (auto stmt : func->body)
  {
    codegen(stmt);
  }

  // 6. If function has no explicit return, add return void
  if (retType->isVoidTy() && !builder.GetInsertBlock()->getTerminator())
  {
    builder.CreateRetVoid();
  }

  popScope();
  return llvmFunc;
}

Value *IRGenerator::codegenStatement(Statement *stmt)
{
  if (auto decl = dynamic_cast<VariableDefinition *>(stmt))
    return codegenVariableDefinition(decl);
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
  if (auto decl = dynamic_cast<VariableDeclaration *>(def->def))
    return codegenVariableDeclaration(decl);
  if (auto init = dynamic_cast<VariableInitialization *>(def->def))
    return codegenVariableInitialization(init);
  return nullptr;
}

void IRGenerator::codegenGlobalVariable(VariableDefinition *dif)
{
  // Handle VariableDeclaration (multiple variables without initializers)
  if (auto varDecl = dynamic_cast<VariableDeclaration *>(dif->def))
  {
    SymbolType baseType = Symbol::get_datatype(varDecl->datatype);
    int dimensions = Symbol::get_dimension(varDecl->datatype);
    Type *ty = getLLVMType(baseType, dimensions);

    // Create null initializer for arrays/pointers
    Constant *init = Constant::getNullValue(ty);

    for (auto var : varDecl->variables)
    {
      std::string name = var->name;

      GlobalVariable *gVar = new GlobalVariable(
          *module,
          ty,
          false, // isConstant
          GlobalValue::ExternalLinkage,
          init,
          name);

      // Store complete type information
      symbolTable.top()[name] = {
          ty,
          gVar,
          baseType,
          dimensions,
          0,
          nullptr};

      // Create length variable for arrays
      if (dimensions > 0)
      {
        GlobalVariable *lengthGlobal = new GlobalVariable(
            *module,
            Type::getInt32Ty(context),
            false,
            GlobalValue::ExternalLinkage,
            ConstantInt::get(Type::getInt32Ty(context), 0), // Initial size 0
            name + "_len");

        // Add length variable to symbol table
        symbolTable.top()[name + "_len"] = {
            Type::getInt32Ty(context),
            lengthGlobal,
            SymbolType::Integer,
            0, // Not an array itself
            0, // No length for the length variable
            nullptr};
      }
    }
  }
  // Handle VariableInitialization (single variable with initializer)
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
      // Handle 1D global arrays as constant arrays
      if (dimensions == 1 && isUniformArray(arrayLit))
      {
        std::vector<Constant *> elements;
        Type *elementType = getLLVMType(baseType, 0);

        for (auto elem : arrayLit->elements)
        {
          elements.push_back(cast<Constant>(codegen(elem)));
        }

        ArrayType *arrType = ArrayType::get(elementType, elements.size());
        init = ConstantArray::get(arrType, elements);
        ty = arrType; // Store as array type, not pointer
        arrayLength = elements.size();
      }
      else
      {
        // For dynamic/jagged arrays, initialize as null pointer
        // The actual array will be created at runtime
        ty = getLLVMType(baseType, dimensions); // pointer type
        init = ConstantPointerNull::get(cast<PointerType>(ty));
        arrayLength = arrayLit->elements.size();
      }
    }
    else
    {
      Value *initVal = codegenExpression(varInit->initializer);
      init = dyn_cast<Constant>(initVal);
      if (!init)
      {
        // If not a constant, use a default value
        init = Constant::getNullValue(ty);
      }
    }

    GlobalVariable *gVar = new GlobalVariable(
        *module, ty, false, GlobalValue::ExternalLinkage, init, name);

    // Store type info and static length (if known)
    symbolTable.top()[name] = {
        ty,
        gVar,
        baseType,
        dimensions,
        arrayLength,
        nullptr // lengthAlloca not used for globals
    };

    // Create length variable for arrays
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

Value *IRGenerator::codegenVariableInitialization(VariableInitialization *init)
{
  SymbolType baseType = Symbol::get_datatype(init->datatype);
  int dimensions = Symbol::get_dimension(init->datatype);
  Type *ty = getLLVMType(baseType, dimensions);
  std::string varName = init->name->name;

  AllocaInst *alloca = builder.CreateAlloca(ty, nullptr, varName);
  AllocaInst *lengthAlloca = nullptr;
  size_t arraySize = 0;

  // Create length variable for arrays
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

    // Verify type matches declaration
    if (actualBaseType != baseType || actualDimensions != dimensions)
    {
      module->getContext().emitError("Array literal type doesn't match declaration");
      return nullptr;
    }

    builder.CreateStore(jagged, alloca);

    // Store the length for dynamic arrays
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
    Value *initVal = codegen(init->initializer);
    builder.CreateStore(initVal, alloca);

    // Initialize length to 0 for empty arrays
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
    // Initialize length to 0 for empty arrays
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

Value *IRGenerator::codegenVariableDeclaration(VariableDeclaration *decl)
{
  SymbolType baseType = Symbol::get_datatype(decl->datatype);
  int dimensions = Symbol::get_dimension(decl->datatype);
  Type *ty = getLLVMType(baseType, dimensions);

  for (auto var : decl->variables)
  {
    AllocaInst *alloca = builder.CreateAlloca(ty, nullptr, var->name);
    // Store type metadata in symbol table
    symbolTable.top()[var->name] = {ty, alloca, baseType, dimensions};
  }
  return nullptr;
}

Value *IRGenerator::codegenExpression(Expression *expr)
{
  // Handle index expressions first with type tracking
  if (auto index = dynamic_cast<IndexExpression *>(expr))
  {
    Type *elementType = nullptr;
    Value *gep = codegenIndexExpression(index, &elementType);
    return builder.CreateLoad(elementType, gep, "loadidx");
  }

  // Existing other cases with adjustments for opaque pointers
  if (auto decl = dynamic_cast<VariableDefinition *>(expr))
    return codegenVariableDefinition(decl);

  if (auto lit = dynamic_cast<Literal *>(expr))
    return codegenLiteral(lit);

  if (auto id = dynamic_cast<Identifier *>(expr))
  {
    return codegenIdentifier(id);
  }

  if (auto call = dynamic_cast<CallFunctionExpression *>(expr))
    return codegenCall(call);

  // Binary operations (unchanged but included for completeness)
  if (auto add = dynamic_cast<AdditiveExpression *>(expr))
    return codegenAdditiveExpr(add);
  if (auto mult = dynamic_cast<MultiplicativeExpression *>(expr))
    return codegenMultiplicativeExpr(mult);
  if (auto assign = dynamic_cast<AssignmentExpression *>(expr))
    return codegenAssignment(assign);

  // Unary operations
  if (auto unary = dynamic_cast<UnaryExpression *>(expr))
    return codegenUnaryExpr(unary);

  // Boolean operations
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

Value *IRGenerator::codegenAssignment(AssignmentExpression *assign)
{
  Value *target = codegenLValue(assign->assignee);
  Value *value = codegenExpression(assign->value);

  if (!target || !value)
    return nullptr;

  // Get the variable name (if possible)
  std::string varName;
  if (auto id = dynamic_cast<Identifier *>(assign->assignee))
  {
    varName = id->name;
  }
  else if (auto index = dynamic_cast<IndexExpression *>(assign->assignee))
  {
    // For array element assignments, we don't update the length
    builder.CreateStore(value, target);
    return target;
  }

  // Handle array assignments
  if (value->getType()->isPointerTy() && !varName.empty())
  {
    // Store the array pointer in the variable
    builder.CreateStore(value, target);

    // Update the length variable if it exists
    size_t arraySize = 0;

    // If the right-hand side is an ArrayLiteral, get its size
    if (auto arrayLit = dynamic_cast<ArrayLiteral *>(assign->value))
    {
      arraySize = arrayLit->elements.size();

      // Look for a length variable
      auto lenEntry = findSymbol(varName + "_len");
      if (lenEntry && lenEntry->llvmValue)
      {
        builder.CreateStore(
            builder.getInt32(arraySize),
            lenEntry->llvmValue);

        // Also update the symbol table entry
        auto varEntry = findSymbol(varName);
        if (varEntry)
        {
          // Create a new entry with updated length
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

  // Scalar assignment
  builder.CreateStore(value, target);
  return target;
}

Value *IRGenerator::codegenIdentifier(Identifier *id)
{
  auto entryOpt = findSymbol(id->name);
  if (!entryOpt)
    return nullptr;

  SymbolEntry entry = *entryOpt;

  // Return pointer directly for arrays
  if (entry.dimensions > 0)
  {
    return entry.llvmValue;
  }

  // Load scalar values
  return builder.CreateLoad(entry.llvmType, entry.llvmValue, id->name);
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
    auto id = dynamic_cast<Identifier *>(expr->operand);
    if (!id)
    {
      module->getContext().emitError("# operator requires an array variable");
      return nullptr;
    }

    auto entryOpt = findSymbol(id->name);
    if (!entryOpt)
    {
      module->getContext().emitError("Undefined variable: " + id->name);
      return nullptr;
    }

    const SymbolEntry &entry = *entryOpt;

    // First check if we have a length variable
    auto lenEntryOpt = findSymbol(id->name + "_len");
    if (lenEntryOpt && lenEntryOpt->llvmValue)
    {
      // Load the length from the length variable
      Value *lengthVal = builder.CreateLoad(Type::getInt32Ty(context),
                                            lenEntryOpt->llvmValue,
                                            id->name + "_len");

      // Make sure it's an integer type
      if (!lengthVal->getType()->isIntegerTy(32))
      {
        lengthVal = builder.CreateIntCast(lengthVal, Type::getInt32Ty(context), false);
      }

      return lengthVal;
    }

    // If no length variable but we have a static length in the symbol table
    if (entry.arrayLength > 0)
    {
      return ConstantInt::get(Type::getInt32Ty(context), entry.arrayLength);
    }

    // For array types with fixed size in LLVM IR
    if (entry.llvmType->isArrayTy())
    {
      ArrayType *arrTy = cast<ArrayType>(entry.llvmType);
      uint64_t size = arrTy->getNumElements();
      return ConstantInt::get(Type::getInt32Ty(context), size);
    }

    // If we can't determine the size, return 0 with a warning
    module->getContext().emitError("Cannot determine array size for variable: " + id->name);
    return ConstantInt::get(Type::getInt32Ty(context), 0);
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
  // 1. Lookup the function in the module
  llvm::Function *callee = module->getFunction(call->function->name);
  if (!callee)
  {
    module->getContext().emitError("Unknown function: " + call->function->name);
    return nullptr;
  }

  // 2. Lookup the function definition in your AST (needed for default values)
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
      // User provided this argument
      argVal = codegenExpression(call->arguments[i]);
    }
    else
    {
      // User did not provide this argument, use default from AST
      auto paramDef = funcDef->parameters[i]->def;
      if (auto init = dynamic_cast<VariableInitialization *>(paramDef))
      {
        argVal = codegenExpression(init->initializer);
      }
      else
      {
        module->getContext().emitError("Missing argument and no default for parameter " + std::to_string(i));
        return nullptr;
      }
    }

    if (!argVal)
      return nullptr;

    // Type conversion (as before)
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

  // 3. Create the call instruction
  return builder.CreateCall(callee, args, "calltmp");
}

bool IRGenerator::isUniformArray(ArrayLiteral *lit)
{

  if (lit->elements.empty())
    return true;

  bool isNested = dynamic_cast<ArrayLiteral *>(lit->elements[0]) != nullptr;
  size_t expectedSize = isNested ? dynamic_cast<ArrayLiteral *>(lit->elements[0])->elements.size() : 0;
  Type *expectedType = codegen(lit->elements[0])->getType();

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

    if (!isNested && codegen(elem)->getType() != expectedType)
      return false;
  }

  return true;
}

Type *IRGenerator::getElementType(ArrayLiteral *lit, int *outDim)
{
  int dimensions = 0;
  ArrayLiteral *current = lit;

  // Calculate dimensions and find base type
  while (true)
  {
    dimensions++;
    if (current->elements.empty())
    {
      *outDim = dimensions;
      return Type::getInt32Ty(context); // Default to integer if empty
    }

    auto first = current->elements[0];
    if (auto sub = dynamic_cast<ArrayLiteral *>(first))
    {
      current = sub;
    }
    else
    {
      *outDim = dimensions;
      return codegen(first)->getType();
    }
  }
}

Value *IRGenerator::createJaggedArray(ArrayLiteral *lit, Function *mallocFn,
                                      SymbolType *outBaseType, int *outDimensions)
{
  // Determine array structure
  int totalDimensions = 0;
  Type *elementType = getElementType(lit, &totalDimensions);
  SymbolType baseType;

  // Map LLVM type to our symbol type
  if (elementType->isIntegerTy(32))
    baseType = SymbolType::Integer;
  else if (elementType->isFloatTy())
    baseType = SymbolType::Float;
  else if (elementType->isIntegerTy(1))
    baseType = SymbolType::Boolean;
  else
    baseType = SymbolType::Integer; // Default

  // Set output parameters
  if (outBaseType)
    *outBaseType = baseType;
  if (outDimensions)
    *outDimensions = totalDimensions;

  // Create nested allocations
  return createJaggedArrayHelper(lit, mallocFn, totalDimensions, elementType);
}
Constant *IRGenerator::createNestedArray(ArrayLiteral *lit, ArrayType *arrType)
{
  std::vector<Constant *> elements;
  if (!arrType)
    arrType = inferArrayType(lit);

  Type *elementType = arrType->getElementType();

  for (auto elem : lit->elements)
  {
    if (auto sub = dynamic_cast<ArrayLiteral *>(elem))
    {
      ArrayType *subArrType = cast<ArrayType>(elementType);
      elements.push_back(createNestedArray(sub, subArrType));
    }
    else
    {
      Constant *c = cast<Constant>(codegen(elem));
      elements.push_back(c);
    }
  }

  return ConstantArray::get(arrType, elements);
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

  Type *elementType = codegen(current->elements[0])->getType();
  ArrayType *arrType = ArrayType::get(elementType, dims.back());

  for (int i = dims.size() - 2; i >= 0; i--)
  {
    arrType = ArrayType::get(arrType, dims[i]);
  }

  return arrType;
}

Value *IRGenerator::codegenIndexExpression(IndexExpression *expr, Type **outElementType)
{
  std::vector<Value *> indices;
  const Identifier *rootId = nullptr;
  SymbolType baseType;
  int totalDims;

  // Traverse to find root Identifier and collect indices
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
      indices.push_back(codegen(current->index));
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

  // Collect remaining indices from the root's IndexExpressions
  while (current)
  {
    indices.push_back(codegen(current->index));
    current = dynamic_cast<IndexExpression *>(current->base);
  }
  std::reverse(indices.begin(), indices.end());

  // Get array metadata from root symbol
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

  // Load root array pointer
  Value *currentPtr = builder.CreateLoad(getLLVMType(baseType, totalDims), entryOpt->llvmValue, "array_ptr");

  // Process each index
  for (size_t i = 0; i < indices.size(); ++i)
  {
    int remainingDims = totalDims - i - 1;
    Type *elementType = getLLVMType(baseType, remainingDims);

    // Bounds check if we have length information
    if (i == 0)
    {
      auto lenEntryOpt = findSymbol(rootId->name + "_len");
      if (lenEntryOpt && lenEntryOpt->llvmValue)
      {
        Value *lengthVal = builder.CreateLoad(Type::getInt32Ty(context),
                                              lenEntryOpt->llvmValue,
                                              "len_load");

        // Create comparison: index < length
        Value *cmp = builder.CreateICmpULT(indices[i], lengthVal, "bounds_check");

        // Create basic blocks for the bounds check
        Function *currentFunc = builder.GetInsertBlock()->getParent();
        BasicBlock *thenBB = BasicBlock::Create(context, "in_bounds", currentFunc);
        BasicBlock *elseBB = BasicBlock::Create(context, "out_of_bounds", currentFunc);
        BasicBlock *mergeBB = BasicBlock::Create(context, "after_bounds_check", currentFunc);

        builder.CreateCondBr(cmp, thenBB, elseBB);

        // Out-of-bounds case: print error and use index 0
        builder.SetInsertPoint(elseBB);
        // Call a runtime error function or just use a default index
        Value *defaultIndex = ConstantInt::get(Type::getInt32Ty(context), 0);
        builder.CreateBr(mergeBB);

        // In-bounds case: use the actual index
        builder.SetInsertPoint(thenBB);
        builder.CreateBr(mergeBB);

        // Merge point: phi node to select the right index
        builder.SetInsertPoint(mergeBB);
        PHINode *indexPhi = builder.CreatePHI(Type::getInt32Ty(context), 2, "safe_index");
        indexPhi->addIncoming(defaultIndex, elseBB);
        indexPhi->addIncoming(indices[i], thenBB);

        // Use the safe index
        indices[i] = indexPhi;
      }
    }

    // Use a named value instead of a numbered temporary
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

Function *IRGenerator::declareMalloc()
{
  Function *mallocFn = module->getFunction("malloc");
  if (!mallocFn)
  {
    FunctionType *mallocType = FunctionType::get(
        builder.getPtrTy(), // returns 'ptr' (opaque pointer)
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

Value *IRGenerator::codegenLValue(Expression *expr)
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
    return codegenIndexExpression(index, &dummy);
  }
  return nullptr;
}

Value *IRGenerator::createJaggedArrayHelper(ArrayLiteral *lit, Function *mallocFn,
                                            int remainingDims, Type *elementType)
{
  DataLayout dl = module->getDataLayout();

  // Handle 1D arrays as contiguous memory blocks
  if (remainingDims == 1)
  {
    size_t elementCount = lit->elements.size();
    Value *allocSize = ConstantInt::get(Type::getInt64Ty(context),
                                        dl.getTypeAllocSize(elementType) * elementCount);

    // Allocate contiguous memory
    Value *buffer = builder.CreateCall(mallocFn, {allocSize}, "contiguous.array");
    buffer = builder.CreateBitCast(buffer, PointerType::getUnqual(elementType));

    // Store elements directly
    for (size_t i = 0; i < elementCount; ++i)
    {
      Value *elementPtr = builder.CreateInBoundsGEP(elementType, buffer,
                                                    ConstantInt::get(Type::getInt32Ty(context), i));
      Value *val = codegen(lit->elements[i]);
      builder.CreateStore(val, elementPtr);
    }
    return buffer;
  }

  // Original jagged array handling for multi-dimensional arrays
  const size_t ptrSize = dl.getPointerSize();
  Value *allocSize = ConstantInt::get(Type::getInt64Ty(context),
                                      lit->elements.size() * ptrSize);

  Value *rawMem = builder.CreateCall(mallocFn, {allocSize}, "malloc.array");
  Value *ptrArray = builder.CreateBitCast(rawMem,
                                          PointerType::getUnqual(PointerType::getUnqual(elementType)),
                                          "ptr.array");

  for (size_t i = 0; i < lit->elements.size(); ++i)
  {
    Value *elementPtr = builder.CreateInBoundsGEP(
        PointerType::getUnqual(elementType),
        ptrArray,
        ConstantInt::get(Type::getInt32Ty(context), i),
        "element.ptr");

    auto subLit = dynamic_cast<ArrayLiteral *>(lit->elements[i]);
    if (!subLit)
    {
      module->getContext().emitError("Invalid jagged array structure");
      return nullptr;
    }

    Value *subArray = createJaggedArrayHelper(subLit, mallocFn,
                                              remainingDims - 1, elementType);
    builder.CreateStore(subArray, elementPtr);
  }
  return ptrArray;
}

bool IRGenerator::isSingleDimensionArray(const SymbolEntry &entry)
{
  return entry.dimensions == 1 &&
         entry.baseType != SymbolType::String;
}