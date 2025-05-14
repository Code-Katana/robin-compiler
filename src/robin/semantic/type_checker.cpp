#include "robin/semantic/type_checker.h"

namespace rbn::semantic
{
  TypeChecker::TypeChecker() {}

  core::SymbolType TypeChecker::is_valid_assign(core::SymbolType left, core::SymbolType right, int dim_left, int dim_right)
  {
    if (dim_left != dim_right)
    {
      return core::SymbolType::Undefined;
    }

    if ((left == core::SymbolType::Boolean || right == core::SymbolType::Boolean) && left != right)
    {
      return core::SymbolType::Undefined;
    }
    else if ((left == core::SymbolType::String || right == core::SymbolType::String) && left != right)
    {
      return core::SymbolType::Undefined;
    }
    if (right == core::SymbolType::Float)
    {
      return right;
    }

    return left;
  }

  core::SymbolType TypeChecker::is_valid_or_and(core::SymbolType left, core::SymbolType right)
  {
    if (left != core::SymbolType::Boolean || right != core::SymbolType::Boolean)
    {
      return core::SymbolType::Undefined;
    }

    return core::SymbolType::Boolean;
  }

  core::SymbolType TypeChecker::is_valid_equality(core::SymbolType left, core::SymbolType right)
  {
    if ((left == core::SymbolType::Boolean || right == core::SymbolType::Boolean) && left != right)
    {
      return core::SymbolType::Undefined;
    }
    else if ((left == core::SymbolType::String || right == core::SymbolType::String) && left != right)
    {
      return core::SymbolType::Undefined;
    }

    return core::SymbolType::Boolean;
  }

  core::SymbolType TypeChecker::is_valid_relational(core::SymbolType left, core::SymbolType right)
  {
    if (is_number(left) && is_number(right))
    {
      return core::SymbolType::Boolean;
    }

    return core::SymbolType::Undefined;
  }

  core::SymbolType TypeChecker::is_valid_addition(core::SymbolType left, core::SymbolType right, string op)
  {
    if (op == "+")
    {
      if ((left == core::SymbolType::String || right == core::SymbolType::String))
      {
        if (left != right)
        {
          return core::SymbolType::Undefined;
        }
      }
      else if (!is_number(left) || !is_number(right))
      {
        return core::SymbolType::Undefined;
      }
    }
    else
    {
      if (!is_number(left) || !is_number(right))
      {
        return core::SymbolType::Undefined;
      }
    }

    if (left == right)
    {
      return left;
    }

    return core::SymbolType::Float;
  }

  core::SymbolType TypeChecker::is_valid_multiplicative(core::SymbolType left, core::SymbolType right, string op)
  {
    if (op == "%")
    {
      if (left != core::SymbolType::Integer || right != core::SymbolType::Integer)
      {
        return core::SymbolType::Undefined;
      }
      else
      {
        return core::SymbolType::Integer;
      }
    }
    else
    {
      if (!is_number(left) || !is_number(right))
      {
        return core::SymbolType::Undefined;
      }
    }

    if (left == right)
    {
      return left;
    }

    return core::SymbolType::Float;
  }

  core::SymbolType TypeChecker::is_valid_Unary(core::SymbolType operand, string op, int dim)
  {
    if (op == "-")
    {
      if (!is_number(operand))
      {
        return core::SymbolType::Undefined;
      }

      return operand;
    }
    else if (op == "not")
    {
      if (operand != core::SymbolType::Boolean)
      {
        return core::SymbolType::Undefined;
      }
      return operand;
    }
    else if (op == "++" || op == "--")
    {
      if (!is_number(operand))
      {
        return core::SymbolType::Undefined;
      }

      return operand;
    }
    else if (op == "$")
    {
      return core::SymbolType::String;
    }
    else if (op == "?")
    {
      return core::SymbolType::Boolean;
    }
    else if (op == "@")
    {
      if (!is_number(operand) && operand != core::SymbolType::Boolean)
      {
        return core::SymbolType::Undefined;
      }
      return core::SymbolType::Integer;
    }
    else if (op == "#")
    {
      if (dim == 0)
      {
        if (operand != core::SymbolType::String)
        {
          return core::SymbolType::Undefined;
        }
      }
      return core::SymbolType::Integer;
    }
    return core::SymbolType::Undefined;
  }

  bool TypeChecker::is_number(core::SymbolType t)
  {
    return t == core::SymbolType::Integer || t == core::SymbolType::Float;
  }
}