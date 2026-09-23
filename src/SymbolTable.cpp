#include "SymbolTable.h"

void SymbolTable::declare(const std::string &identifier, SymbolType type, SymbolModality modality, int scope,
                          std::optional<int> vectorSize) {
    symbols.emplace_back(identifier, type, modality, scope, vectorSize);
}

const Symbol *SymbolTable::find(const std::string &name, int scope) const {
    for (const auto &symbol : symbols) {
        if (symbol.getId() == name && symbol.getScope() == scope) return &symbol;
    }

    return nullptr;
}

const std::vector<Symbol> &SymbolTable::getSymbols() const { return symbols; }
