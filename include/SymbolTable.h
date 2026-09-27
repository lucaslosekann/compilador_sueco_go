#pragma once
#include "Symbol.h"
#include <optional>
#include <string>
#include <vector>

// Um escopo: nome (ex.: global, somar, somar1), escopo pai e, se for escopo de funcao, o indice da funcao
struct Scope {
    std::string name;
    int parent;
    std::optional<std::size_t> function;
};

class SymbolTable {
  public:
    SymbolTable();

    int createScope(const std::string &name, int parent, std::optional<std::size_t> function = std::nullopt);
    const Scope &getScope(int scope) const;

    std::size_t declare(const Symbol &symbol);
    std::optional<std::size_t> findInScope(const std::string &name, int scope) const;

    Symbol &at(std::size_t index);
    const Symbol &at(std::size_t index) const;
    const std::vector<Symbol> &getSymbols() const;

  private:
    std::vector<Symbol> symbols;
    std::vector<Scope> scopes;
};
