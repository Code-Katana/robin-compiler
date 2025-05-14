#pragma once

#include "robin/core/symbol.h"

namespace rbn::semantic
{
  class TypeChecker
  {
  private:
    TypeChecker();
    // helper
    static bool is_number(core::SymbolType t);

  public:
    // checkers
    static core::SymbolType is_valid_assign(core::SymbolType left, core::SymbolType right, int dim_left, int dim_right);
    static core::SymbolType is_valid_or_and(core::SymbolType left, core::SymbolType right);
    static core::SymbolType is_valid_equality(core::SymbolType left, core::SymbolType right);
    static core::SymbolType is_valid_relational(core::SymbolType left, core::SymbolType right);
    static core::SymbolType is_valid_addition(core::SymbolType left, core::SymbolType right, string op);
    static core::SymbolType is_valid_multiplicative(core::SymbolType left, core::SymbolType right, string op);
    static core::SymbolType is_valid_Unary(core::SymbolType operand, string op, int dim);
  };
}