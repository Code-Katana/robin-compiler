#include "symbol.h"

Symbol::Symbol(string n, SymbolType t, SymbolKind k, vector<SymbolType> p)
{
  name = n;
  type = t;
  kind = k;
  parameters = p;
}