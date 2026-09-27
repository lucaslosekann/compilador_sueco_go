#include "Symbol.h"

Symbol::Symbol(const std::string &id, SymbolType type, SymbolModality modality, int scope, int sourcePosition,
               std::optional<int> vectorSize)
    : id(id), type(type), modality(modality), scope(scope), sourcePosition(sourcePosition), vectorSize(vectorSize) {}

const std::string &Symbol::getId() const { return id; }

SymbolType Symbol::getType() const { return type; }

SymbolModality Symbol::getModality() const { return modality; }

int Symbol::getScope() const { return scope; }

int Symbol::getSourcePosition() const { return sourcePosition; }

std::optional<int> Symbol::getVectorSize() const { return vectorSize; }

bool Symbol::isVector() const { return vectorSize.has_value(); }

bool Symbol::isFunction() const { return modality == SymbolModality::FUNCTION; }

bool Symbol::isParameter() const { return modality == SymbolModality::PARAMETER; }

bool Symbol::isInitialized() const { return initialized; }

bool Symbol::isUsed() const { return used; }

int Symbol::getParameterPosition() const { return parameterPosition; }

const std::vector<ParameterInfo> &Symbol::getParameters() const { return parameters; }

void Symbol::setType(SymbolType newType) { type = newType; }

void Symbol::setVectorSize(std::optional<int> size) { vectorSize = size; }

void Symbol::markInitialized() { initialized = true; }

void Symbol::markUsed() { used = true; }

void Symbol::setParameterPosition(int position) { parameterPosition = position; }

void Symbol::addParameter(ParameterInfo parameter) { parameters.push_back(parameter); }
