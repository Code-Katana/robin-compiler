#include "type_checker.h"

TypeChecker::TypeChecker() {}

SymbolType TypeChecker::is_valid_assign(SymbolType left, SymbolType right, int dim_left, int dim_right)
{
  if (dim_left != dim_right)
  {
    return SymbolType::Undefined;
  }

  if ((left == SymbolType::Boolean || right == SymbolType::Boolean) && left != right)
  {
    return SymbolType::Undefined;
  }
  else if ((left == SymbolType::String || right == SymbolType::String) && left != right)
  {
    return SymbolType::Undefined;
  }
  if (right == SymbolType::Float)
  {
    return right;
  }

  return left;
}

SymbolType TypeChecker::is_valid_or_and(SymbolType left, SymbolType right)
{
  if (left != SymbolType::Boolean || right != SymbolType::Boolean)
  {
    return SymbolType::Undefined;
  }

  return SymbolType::Boolean;
}

SymbolType TypeChecker::is_valid_equality(SymbolType left, SymbolType right)
{
  if ((left == SymbolType::Boolean || right == SymbolType::Boolean) && left != right)
  {
    return SymbolType::Undefined;
  }
  else if ((left == SymbolType::String || right == SymbolType::String) && left != right)
  {
    return SymbolType::Undefined;
  }

  return SymbolType::Boolean;
}

SymbolType TypeChecker::is_valid_relational(SymbolType left, SymbolType right)
{
  if (is_number(left) && is_number(right))
  {
    return SymbolType::Boolean;
  }

  return SymbolType::Undefined;
}

SymbolType TypeChecker::is_valid_addition(SymbolType left, SymbolType right, string op)
{
  if (op == "+")
  {
    if ((left == SymbolType::String || right == SymbolType::String))
    {
      if (left != right)
      {
        return SymbolType::Undefined;
      }
    }
    else if (!is_number(left) || !is_number(right))
    {
      return SymbolType::Undefined;
    }
  }
  else
  {
    if (!is_number(left) || !is_number(right))
    {
      return SymbolType::Undefined;
    }
  }

  if (left == right)
  {
    return left;
  }

  return SymbolType::Float;
}

SymbolType TypeChecker::is_valid_multiplicative(SymbolType left, SymbolType right, string op)
{
  if (op == "%")
  {
    if (left != SymbolType::Integer || right != SymbolType::Integer)
    {
      return SymbolType::Undefined;
    }
    else
    {
      return SymbolType::Integer;
    }
  }
  else
  {
    if (!is_number(left) || !is_number(right))
    {
      return SymbolType::Undefined;
    }
  }

  if (left == right)
  {
    return left;
  }

  return SymbolType::Float;
}

SymbolType TypeChecker::is_valid_Unary(SymbolType operand, string op, int dim)
{
  if (op == "-")
  {
    if (!is_number(operand))
    {
      return SymbolType::Undefined;
    }

    return operand;
  }
  else if (op == "not")
  {
    if (operand != SymbolType::Boolean)
    {
      return SymbolType::Undefined;
    }
    return operand;
  }
  else if (op == "++" || op == "--")
  {
    if (!is_number(operand))
    {
      return SymbolType::Undefined;
    }

    return operand;
  }
  else if (op == "$")
  {
    return SymbolType::String;
  }
  else if (op == "?")
  {
    return SymbolType::Boolean;
  }
  else if (op == "@")
  {
    if (!is_number(operand) && operand != SymbolType::Boolean)
    {
      return SymbolType::Undefined;
    }
    return SymbolType::Integer;
  }
  else if (op == "#")
  {
    if (dim == 0)
    {
      if (operand != SymbolType::String)
      {
        return SymbolType::Undefined;
      }
    }
    return SymbolType::Integer;
  }
  return SymbolType::Undefined;
}

bool TypeChecker::is_number(SymbolType t)
{
  return t == SymbolType::Integer || t == SymbolType::Float;
}