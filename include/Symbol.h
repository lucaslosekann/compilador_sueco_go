#pragma once

#include "SymbolModality.h"
#include "SymbolType.h"
#include <optional>
#include <string>
#include <vector>

// Informacao de um parametro guardada na funcao (tipo, se e vetor)
struct ParameterInfo {
    SymbolType type;
    bool isVector;
};

// Uma linha da tabela de simbolos.
// Colunas do modelo do professor: id, tipo, ini, usada, escopo, param, pos, vet, matriz, ref, func
class Symbol {
  private:
    std::string id;
    SymbolType type;
    SymbolModality modality;
    int scope;
    int sourcePosition;
    std::optional<int> vectorSize;
    bool initialized = false;
    bool used = false;
    int parameterPosition = 0; // pos: 0 = nao e parametro, 1.. = ordem do parametro
    std::vector<ParameterInfo> parameters; // so para funcoes

  public:
    Symbol(const std::string &id, SymbolType type, SymbolModality modality, int scope, int sourcePosition,
           std::optional<int> vectorSize = std::nullopt);

    const std::string &getId() const;
    SymbolType getType() const;
    SymbolModality getModality() const;
    int getScope() const;
    int getSourcePosition() const;
    std::optional<int> getVectorSize() const;
    bool isVector() const;
    bool isFunction() const;
    bool isParameter() const;
    bool isInitialized() const;
    bool isUsed() const;
    int getParameterPosition() const;
    const std::vector<ParameterInfo> &getParameters() const;

    void setType(SymbolType newType);
    void setVectorSize(std::optional<int> size);
    void markInitialized();
    void markUsed();
    void setParameterPosition(int position);
    void addParameter(ParameterInfo parameter);
};
