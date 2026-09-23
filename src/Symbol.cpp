#include "Symbol.h"

Symbol::Symbol(const std::string &id, SymbolType type, SymbolModality modality, int scope, std::optional<int> vectorSize)
    : id(id), type(type), modality(modality), scope(scope), vectorSize(vectorSize) {}

const std::string &Symbol::getId() const { return id; }

SymbolType Symbol::getType() const { return type; }

SymbolModality Symbol::getModality() const { return modality; }

int Symbol::getScope() const { return scope; }

std::optional<int> Symbol::getVectorSize() const { return vectorSize; }
