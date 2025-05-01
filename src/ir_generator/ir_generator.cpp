#include "ir_generator.h"

LLVMContext IRGenerator::context;

IRGenerator::IRGenerator() : builder(context)
{
  // InitializeNativeTarget();
  // InitializeNativeTargetAsmPrinter();
  module = make_unique<Module>("main_module", context);
  pushScope();
}

Type* IRGenerator::getLLVMType(SymbolType type, int dim, bool hasLiteralInit) {
  Type* baseType = nullptr;
  switch (type) {
    case SymbolType::Integer: baseType = Type::getInt32Ty(context); break;
    case SymbolType::Float:   baseType = Type::getFloatTy(context); break;
    case SymbolType::Boolean: baseType = Type::getInt1Ty(context); break;
    case SymbolType::String:  baseType = PointerType::getUnqual(Type::getInt8Ty(context)); break;
    case SymbolType::Void:    baseType = Type::getVoidTy(context); break;
    default:                  baseType = Type::getVoidTy(context); break;
  }

  // Use pointer types for arrays and jagged arrays
  for (int i = 0; i < dim; i++) {
    baseType = PointerType::getUnqual(baseType);
  }

  return baseType;
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

  // Value* IRGenerator::codegen(AstNode* node) {
  //   if (auto expr = dynamic_cast<Expression*>(node))
  //     return codegenExpression(expr);
  //   if (auto stmt = dynamic_cast<Statement*>(node)) {
  //     if (auto varDecl = dynamic_cast<VariableDeclaration*>(stmt))
  //       return codegenVariableDeclaration(varDecl);
  //     if (auto varInit = dynamic_cast<VariableInitialization*>(stmt))  // Fixed declaration
  //       return codegenVariableInitialization(varInit);
  //     // Handle other statement types
  //     if (auto write = dynamic_cast<WriteStatement*>(stmt))
  //       return codegenWriteStatement(write);
  //     if (auto read = dynamic_cast<ReadStatement*>(stmt))
  //       return codegenReadStatement(read);
  //     // Add other statement handlers...
  //   }
  //   return nullptr;
  // }

 // In codegenProgram function:
void IRGenerator::codegenProgram(ProgramDefinition* program) {
  // Create main function
  FunctionType* funcType = FunctionType::get(Type::getInt32Ty(context), false);
  Function* mainFunc = Function::Create(funcType, Function::ExternalLinkage, "main", module.get());
  
  BasicBlock* entry = BasicBlock::Create(context, "entry", mainFunc);
  builder.SetInsertPoint(entry);

  // Process global variables
  for (auto global : program->globals) {
    if (auto varDef = dynamic_cast<VariableDefinition*>(global)) {
      if (auto varDecl = dynamic_cast<VariableDeclaration*>(varDef->def)) {
        // Handle uninitialized declarations
        codegenVariableDeclaration(varDecl);
      } else if (auto varInit = dynamic_cast<VariableInitialization*>(varDef->def)) {
        // Handle initialized variables
        codegenVariableInitialization(varInit);
      }
    }
  }

  // Process program body (assignments)
  for (auto stmt : program->body) {
    codegen(stmt);
  }

  builder.CreateRet(ConstantInt::get(context, APInt(32, 0)));
}

// New helper function for initialized variables
// Value* IRGenerator::codegenVariableInitialization(VariableInitialization* init) {
//   Type* ty = getLLVMType(
//     Symbol::get_datatype(init->datatype),
//     Symbol::get_dimension(init->datatype),
//     true
//   );

//   AllocaInst* alloca = builder.CreateAlloca(ty, nullptr, init->name->name);
//   symbolTable.top()[init->name->name] = alloca;

//   Value* initVal = codegen(init->initializer);
//   if (!initVal) return nullptr;

//   if (auto arrayLit = dynamic_cast<ArrayLiteral*>(init->initializer)) {
//     if (isUniformArray(arrayLit)) {
//       ArrayType* arrType = inferArrayType(arrayLit);
//       Constant* constArray = createNestedArray(arrayLit, arrType);
//       builder.CreateStore(constArray, alloca);
//     } else {
//       Function* mallocFn = declareMalloc();
//       Value* jagged = createJaggedArray(arrayLit, mallocFn);
//       builder.CreateStore(jagged, alloca);
//     }
//   } else {
//     builder.CreateStore(initVal, alloca);
//   }

//   return alloca;
// }

// Updated variable declaration handler
// Value* IRGenerator::codegenVariableDeclaration(VariableDeclaration* decl) {
//   for (auto var : decl->variables) {
//     Type* ty = getLLVMType(
//       Symbol::get_datatype(decl->datatype),
//       Symbol::get_dimension(decl->datatype),
//       false  // No literal initialization
//     );

//     AllocaInst* alloca = builder.CreateAlloca(ty, nullptr, var->name);
//     symbolTable.top()[var->name] = alloca;
//   }
//   return nullptr;
// }
  Value *IRGenerator::codegenFunction(FunctionDefinition *func) {
    // Get return type
    Type *retType = getLLVMType(
      Symbol::get_datatype(func->return_type->return_type),
      Symbol::get_dimension(func->return_type->return_type),
      false  // Not a literal initialization
    );

    // Get parameter types
    std::vector<Type *> paramTypes;
  for (auto param : func->parameters) {
    if (auto decl = dynamic_cast<VariableDeclaration *>(param->def)) {
      Type *ty = getLLVMType(
        Symbol::get_datatype(decl->datatype),
        Symbol::get_dimension(decl->datatype),
        false  // Not a literal initialization
      );
      paramTypes.push_back(ty);
    }
  }

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
  for (auto &arg : llvmFunc->args()) {
    if (auto decl = dynamic_cast<VariableDeclaration *>(func->parameters[idx++]->def)) {
      llvm::AllocaInst *alloca = builder.CreateAlloca(arg.getType());
      builder.CreateStore(&arg, alloca);
      symbolTable.top()[decl->variables[0]->name] = alloca;
    }
  }

  // Process function body
  for (auto stmt : func->body) {
    codegen(stmt);
  }

  if (retType->isVoidTy()) {
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
// Value *IRGenerator::codegenAssignableExpr(AssignableExpression *expr)
// {
//   // Implement address calculation for variables/array elements
//   return nullptr; // Simplified exampleA
// }

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
  if (auto index = dynamic_cast<IndexExpression *>(expr))
    return codegenIndexExpression(index);
  return nullptr;
}


// Implement index expression
Value* IRGenerator::codegenIndexExpression(IndexExpression* expr) {
  Value* base = codegen(expr->base);
  std::vector<Value*> indices;

  // First index is always 0 for the outermost array
  indices.push_back(llvm::ConstantInt::get(context, llvm::APInt(32, 0)));

  // Process indices (x[i][j][k] becomes [0, i, j, k])
  IndexExpression* current = expr;
  while (current) {
    indices.push_back(codegen(current->index));
    current = dynamic_cast<IndexExpression*>(current->base);
  }

  // Reverse to get correct nesting order
  std::reverse(indices.begin() + 1, indices.end());

  // Opaque pointer GEP (LLVM 15+)
  return builder.CreateInBoundsGEP(
      base->getType()->getScalarType(),  // Base pointer type
      base,
      indices,
      "arrayidx"
  );
}





Value *IRGenerator::createArrayAllocation(Type *elementType, Value *size) {
  // Calculate total bytes needed
DataLayout dl = module->getDataLayout();
  uint64_t typeSize = dl.getTypeAllocSize(elementType);
  Value *allocSize = builder.CreateMul(
      size, 
      ConstantInt::get(context, APInt(32, typeSize)),
      "total_size");

  // Create malloc call
  Function *mallocFunc = module->getFunction("malloc");
  if (!mallocFunc) {
    FunctionType *mallocType = FunctionType::get(
        PointerType::getInt8Ty(context), 
        {Type::getInt64Ty(context)}, 
        false);
    mallocFunc = Function::Create(
        mallocType, 
        Function::ExternalLinkage, 
        "malloc", 
        module.get());
  }

  Value *rawPtr = builder.CreateCall(
      mallocFunc, 
      {builder.CreateIntCast(allocSize, Type::getInt64Ty(context), false)},
      "malloc");

  // Cast to appropriate pointer type
  return builder.CreateBitCast(
      rawPtr, 
      PointerType::get(elementType, 0), 
      "array_ptr");
}


ArrayType* IRGenerator::inferArrayTypeFromLiteral(ArrayLiteral* lit) {
  std::vector<uint64_t> dims;
  ArrayLiteral* current = lit;

  // Traverse nested literals to get dimensions
  while (!current->elements.empty() && 
         dynamic_cast<ArrayLiteral*>(current->elements[0])) {
    dims.push_back(current->elements.size());
    current = dynamic_cast<ArrayLiteral*>(current->elements[0]);
  }
  dims.push_back(current->elements.size());

  // Infer element type from the innermost literal
  Type* elementType = codegen(current->elements[0])->getType();

  // Build nested ArrayType (e.g., [2 x [3 x float]])
  ArrayType* arrType = ArrayType::get(elementType, dims.back());
  for (int i = dims.size() - 2; i >= 0; i--) {
    arrType = ArrayType::get(arrType, dims[i]);
  }

  return arrType; // Now returns ArrayType*
}
// In codegenVariableDeclaration function
Value* IRGenerator::codegenVariableDeclaration(VariableDeclaration* decl) {
  for (auto var : decl->variables) {
    Type* ty = getLLVMType(
      Symbol::get_datatype(decl->datatype),
      Symbol::get_dimension(decl->datatype),
      false  // No literal initialization
    );
    AllocaInst* alloca = builder.CreateAlloca(ty, nullptr, var->name);
    symbolTable.top()[var->name] = alloca;
  }
  return nullptr;
}

Value* IRGenerator::codegenVariableInitialization(VariableInitialization* init) {
  Type* ty = getLLVMType(
      Symbol::get_datatype(init->datatype),
      Symbol::get_dimension(init->datatype),
      true
  );

  AllocaInst* alloca = builder.CreateAlloca(ty, nullptr, Twine(init->name->name));
  symbolTable.top()[init->name->name] = alloca;

  Expression* initializer = init->initializer;
  if (auto arrayLit = dynamic_cast<ArrayLiteral*>(initializer)) {
      Function* mallocFn = declareMalloc();

      if (!isUniformArray(arrayLit)) {
          // ✅ Jagged — handle with malloc
          Value* jagged = createJaggedArray(arrayLit, mallocFn);
          builder.CreateStore(jagged, alloca);
      } else {
          // ✅ Uniform — handle with constant array
          ArrayType* arrType = inferArrayType(arrayLit);
          Constant* constArray = createNestedArray(arrayLit, arrType);
          builder.CreateStore(constArray, alloca);
      }

      return alloca;
  }

  // Handle scalar case (int, float, etc.)
  Value* initVal = codegen(initializer);
  builder.CreateStore(initVal, alloca);
  return alloca;
}


Value* IRGenerator::createJaggedArray(ArrayLiteral* lit, Function* mallocFn) {
  size_t numElements = lit->elements.size();
  Type* elementType = getElementType(lit); // base type, e.g. int/float
  PointerType* elementPtrTy = PointerType::get(elementType, 0);
  PointerType* ptrToPtr = PointerType::get(elementPtrTy, 0);

  DataLayout dl = module->getDataLayout();
  uint64_t ptrSize = dl.getPointerSize();
  Value* totalSize = ConstantInt::get(Type::getInt64Ty(context), ptrSize * numElements);
  Value* rawMem = builder.CreateCall(mallocFn, { totalSize }, "malloc.outer");
  Value* outerPtr = builder.CreateBitCast(rawMem, ptrToPtr, "jagged");

  for (size_t i = 0; i < numElements; i++) {
      Value* index = ConstantInt::get(Type::getInt32Ty(context), i);
      Value* gep = builder.CreateInBoundsGEP(elementPtrTy, outerPtr, index);

      Expression* item = lit->elements[i];

      if (auto subLit = dynamic_cast<ArrayLiteral*>(item)) {
          Value* nested = createJaggedArray(subLit, mallocFn);
          builder.CreateStore(nested, gep);
      } else {
          Value* scalarVal = codegen(item);
          uint64_t scalarSize = dl.getTypeAllocSize(elementType);
          Value* allocSize = ConstantInt::get(Type::getInt64Ty(context), scalarSize);
          Value* scalarMem = builder.CreateCall(mallocFn, { allocSize }, "malloc.elem");
          Value* typedPtr = builder.CreateBitCast(scalarMem, elementPtrTy);
          builder.CreateStore(scalarVal, typedPtr);
          builder.CreateStore(typedPtr, gep);
      }
  }

  return outerPtr;
}

Type* IRGenerator::getElementType(ArrayLiteral* lit) {
  ArrayLiteral* current = lit;
  while (auto sub = dynamic_cast<ArrayLiteral*>(current->elements[0])) {
    current = sub;
  }

  return codegen(current->elements[0])->getType();
}

// Handle array assignments in codegenAssignment
Value* IRGenerator::codegenAssignment(AssignmentExpression* assign) {
  Value* target = codegen(assign->assignee);
  Value* value = codegen(assign->value);

  if (!target || !value)
    return nullptr;

  if (!target->getType()->isPointerTy()) {
    module->getContext().emitError("Assignment target is not a pointer.");
    return nullptr;
  }

  Type* valueType = value->getType();

  // Handle uniform arrays via memcpy
  if (valueType->isArrayTy()) {
    // Create temporary alloca to hold the array value
    AllocaInst* tempArray = builder.CreateAlloca(valueType, nullptr, "tmp.array");
    builder.CreateStore(value, tempArray);

    // Compute size of the array
    DataLayout dl = module->getDataLayout();
    uint64_t size = dl.getTypeAllocSize(valueType);

    // Cast both pointers to i8* (LLVM IR opaque pointers)
    Value* srcPtr = builder.CreateBitCast(tempArray, builder.getPtrTy(), "src.cast");
    Value* destPtr = builder.CreateBitCast(target, builder.getPtrTy(), "dest.cast");

    // Use CreateMemCpy instead of manual @llvm.memcpy
    builder.CreateMemCpy(
      destPtr,
      Align(4),                 // Destination alignment
      srcPtr,
      Align(4),                 // Source alignment
      ConstantInt::get(Type::getInt64Ty(context), size)
    );

    return target;
  }

  // Scalars (int, float, bool, pointer/string)
  return builder.CreateStore(value, target);
}

Value* IRGenerator::codegen(AstNode* node) {
  if (!node) return nullptr;

  // Handle expressions
  if (auto expr = dynamic_cast<Expression*>(node)) {
    return codegenExpression(expr);
  }

  // Handle statements
  if (auto stmt = dynamic_cast<Statement*>(node)) {

    if (auto varDecl = dynamic_cast<VariableDeclaration*>(stmt)) {
      return codegenVariableDeclaration(varDecl);
    }

    if (auto varInit = dynamic_cast<VariableInitialization*>(stmt)) {
      return codegenVariableInitialization(varInit);
    }

    if (auto write = dynamic_cast<WriteStatement*>(stmt)) {
      return codegenWriteStatement(write);
    }

    if (auto read = dynamic_cast<ReadStatement*>(stmt)) {
      return codegenReadStatement(read);
    }

    if (auto ifStmt = dynamic_cast<IfStatement*>(stmt)) {
      return codegenConditional(ifStmt);
    }

    if (auto whileLoop = dynamic_cast<WhileLoop*>(stmt)) {
      return codegenLoop(whileLoop);
    }

    if (auto forLoop = dynamic_cast<ForLoop*>(stmt)) {
      return codegenForLoop(forLoop);
    }

    if (auto returnStmt = dynamic_cast<ReturnStatement*>(stmt)) {
      if (returnStmt->returnValue)
          return builder.CreateRet(codegenExpression(returnStmt->returnValue));
      else
          return builder.CreateRetVoid(); // handle void return
    }

    if (dynamic_cast<SkipStatement*>(stmt)) {
      return nullptr;
    }

    if (dynamic_cast<StopStatement*>(stmt)) {
      Function* exitFunc = module->getFunction("exit");
      if (!exitFunc) {
        FunctionType* exitType = FunctionType::get(Type::getVoidTy(context), { Type::getInt32Ty(context) }, false);
        exitFunc = Function::Create(exitType, Function::ExternalLinkage, "exit", module.get());
      }
      builder.CreateCall(exitFunc, ConstantInt::get(Type::getInt32Ty(context), 0));
      return nullptr;
    }
  }

  errs() << "Unknown AST node in codegen: " << AstNode::get_node_name(node) << "\n";
  return nullptr;
}

Value *IRGenerator::codegenIdentifier(Identifier *id)
{
  Value *val = findValue(id->name);
  if (!val)
    return nullptr;
  return builder.CreateLoad(val->getType(), val);
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
  if (auto arrayLit = dynamic_cast<ArrayLiteral *>(lit)) {
    return createNestedArray(arrayLit, nullptr);
  }
  return nullptr;
}


Constant* IRGenerator::createNestedArray(ArrayLiteral* lit, ArrayType* arrType) {
  std::vector<Constant*> elements;

  // ⚠️ Infer array type if not passed
  if (!arrType) {
      arrType = inferArrayType(lit);
  }

  Type* elementType = arrType->getElementType();

  for (auto elem : lit->elements) {
      if (auto subLit = dynamic_cast<ArrayLiteral*>(elem)) {
          // Recurse
          ArrayType* subArrType = cast<ArrayType>(elementType);
          elements.push_back(createNestedArray(subLit, subArrType));
      } else {
          Constant* c = cast<Constant>(codegen(elem));
          elements.push_back(c);
      }
  }

  return ConstantArray::get(arrType, elements);
}


Value *IRGenerator::createArrayAllocation(Type *baseType, const std::vector<Value*> &dims) {
  // Create nested array type
  Type *arrayType = baseType;
  for (auto it = dims.rbegin(); it != dims.rend(); ++it) {
    arrayType = ArrayType::get(arrayType, cast<ConstantInt>(*it)->getZExtValue());
  }
  
  // Allocate on stack
  AllocaInst *alloca = builder.CreateAlloca(arrayType);
  return builder.CreateBitCast(alloca, PointerType::get(arrayType, 0));
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
  Value *L = codegen(expr->left);
  Value *R = codegen(expr->right);
  if (!L || !R)
    return nullptr;

  if (L->getType()->isIntegerTy())
    return builder.CreateICmpSGT(L, R, "gttmp");
  if (L->getType()->isFloatingPointTy())
    return builder.CreateFCmpOGT(L, R, "gttmp");
  return nullptr;
}

Value *IRGenerator::codegenEqualityExpr(EqualityExpression *expr)
{
  Value *L = codegen(expr->left);
  Value *R = codegen(expr->right);
  if (!L || !R)
    return nullptr;
  return builder.CreateICmpEQ(L, R, "eqtmp");
}

Value *IRGenerator::codegenAdditiveExpr(AdditiveExpression *expr)
{
  Value *L = codegen(expr->left);
  Value *R = codegen(expr->right);
  if (!L || !R)
    return nullptr;

  if (expr->optr == "+")
  {
    if (L->getType()->isIntegerTy())
      return builder.CreateAdd(L, R, "addtmp");
    return builder.CreateFAdd(L, R, "addtmp");
  }
  else
  {
    if (L->getType()->isIntegerTy())
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
Value *IRGenerator::codegenConditional(IfStatement *ifStmt)
{
  // Implement if-else logic, e.g., create basic blocks, condition evaluation, etc.
  return nullptr;
}

Value *IRGenerator::codegenLoop(WhileLoop *loop)
{
  // Implement while loop logic (condition, loop body, branches)
  return nullptr;
}

Value *IRGenerator::codegenForLoop(ForLoop *forLoop)
{
  // Implement for loop logic (initialization, condition, increment, body)
  return nullptr;
}

Value *IRGenerator::codegenCall(CallFunctionExpression *call)
{
  // Implement function call logic (lookup function, prepare args, create call instruction)
  return nullptr;
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



bool IRGenerator::isUniformArray(ArrayLiteral* lit) {
  if (lit->elements.empty()) return true;

  bool isNested = dynamic_cast<ArrayLiteral*>(lit->elements[0]) != nullptr;
  size_t expectedSize = isNested ? 
    dynamic_cast<ArrayLiteral*>(lit->elements[0])->elements.size() : 0;

  Type* expectedType = codegen(lit->elements[0])->getType();

  for (auto elem : lit->elements) {
      auto sub = dynamic_cast<ArrayLiteral*>(elem);

      if ((sub != nullptr) != isNested) return false;

      if (isNested && sub) {
          if (sub->elements.size() != expectedSize) return false;
          if (!isUniformArray(sub)) return false;
      }

      if (!isNested) {
          if (codegen(elem)->getType() != expectedType) return false;
      }
  }

  return true;
}


ArrayType* IRGenerator::inferArrayType(ArrayLiteral* lit) {
  std::vector<uint64_t> dims;
  ArrayLiteral* current = lit;
  while (dynamic_cast<ArrayLiteral*>(current->elements[0])) {
    dims.push_back(current->elements.size());
    current = dynamic_cast<ArrayLiteral*>(current->elements[0]);
  }
  dims.push_back(current->elements.size());

  Type* elementType = codegen(current->elements[0])->getType();
  ArrayType* arrType = ArrayType::get(elementType, dims.back());
  for (int i = dims.size()-2; i >= 0; --i) {
    arrType = ArrayType::get(arrType, dims[i]);
  }
  return arrType;
}

Function* IRGenerator::declareMalloc() {
  Function* mallocFn = module->getFunction("malloc");

  if (!mallocFn) {
    FunctionType* mallocType = FunctionType::get(
      PointerType::getInt8Ty(context),
      { Type::getInt64Ty(context) },
      false
    );

    mallocFn = Function::Create(
      mallocType,
      Function::ExternalLinkage,
      "malloc",
      module.get());
  }
  return mallocFn;
}