#pragma once
#include "Symbol.h"
#include <optional>
#include <string>
#include <vector>

class SymbolTable {
  public:
    void declare(const std::string &identifier, SymbolType type, SymbolModality modality, int scope,
                 std::optional<int> vectorSize = std::nullopt);

    const Symbol *find(const std::string &name, int scope) const;
    const std::vector<Symbol> &getSymbols() const;

  private:
    std::vector<Symbol> symbols;
};
