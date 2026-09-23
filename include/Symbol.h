#pragma once

#include "SymbolModality.h"
#include "SymbolType.h"
#include <optional>
#include <string>

class Symbol {
  private:
    std::string id;
    SymbolType type;
    SymbolModality modality;
    int scope;
    std::optional<int> vectorSize;

  public:
    Symbol(const std::string &id, SymbolType type, SymbolModality modality, int scope,
           std::optional<int> vectorSize = std::nullopt);

    const std::string &getId() const;
    SymbolType getType() const;
    SymbolModality getModality() const;
    int getScope() const;
    std::optional<int> getVectorSize() const;
};
