#pragma once

#include "compiler/core/symbol.h"

class TypeChecker
{
private:
  TypeChecker();
  // helper
  static bool is_number(SymbolType t);

public:
  static SymbolType is_valid_assign(SymbolType left, SymbolType right, int dim_left, int dim_right);
  static SymbolType is_valid_or_and(SymbolType left, SymbolType right);
  static SymbolType is_valid_equality(SymbolType left, SymbolType right);
  static SymbolType is_valid_relational(SymbolType left, SymbolType right);
  static SymbolType is_valid_addition(SymbolType left, SymbolType right, string op);
  static SymbolType is_valid_multiplicative(SymbolType left, SymbolType right, string op);
  static SymbolType is_valid_Unary(SymbolType operand, string op, int dim);
};