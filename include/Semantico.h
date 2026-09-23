#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "SemanticError.h"
#include "Symbol.h"
#include "SymbolTable.h"
#include "Token.h"
#include <optional>
#include <stack>
#include <vector>

namespace GalsGenerated {

class Semantico {
  private:
    std::vector<std::string> pendingIdentifiers;
    std::optional<std::string> pendingParameterIdentifier;
    std::optional<std::string> pendingFunctionIdentifier;
    std::optional<int> pendingVectorSize;
    std::stack<int> scopeStack;
    SymbolTable symbolTable;

  public:
    Semantico();

    void executeAction(int action, const Token *token);
    const SymbolTable &getSymbolTable() const;
};

} // namespace GalsGenerated

#endif
