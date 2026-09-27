#include "SymbolTable.h"

SymbolTable::SymbolTable() { scopes.push_back({"global", -1, std::nullopt}); }

int SymbolTable::createScope(const std::string &name, int parent, std::optional<std::size_t> function) {
    scopes.push_back({name, parent, function});
    return static_cast<int>(scopes.size() - 1);
}

const Scope &SymbolTable::getScope(int scope) const { return scopes.at(scope); }

std::size_t SymbolTable::declare(const Symbol &symbol) {
    symbols.push_back(symbol);
    return symbols.size() - 1;
}

std::optional<std::size_t> SymbolTable::findInScope(const std::string &name, int scope) const {
    for (std::size_t i = 0; i < symbols.size(); i++) {
        if (symbols[i].getId() == name && symbols[i].getScope() == scope) return i;
    }

    return std::nullopt;
}

Symbol &SymbolTable::at(std::size_t index) { return symbols.at(index); }

const Symbol &SymbolTable::at(std::size_t index) const { return symbols.at(index); }

const std::vector<Symbol> &SymbolTable::getSymbols() const { return symbols; }
