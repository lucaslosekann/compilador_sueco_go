#include "Semantico.h"
#include "Constants.h"
#include "SymbolType.h"

#include <limits>
#include <stdexcept>
#include <string>

namespace {

int parseArraySize(const GalsGenerated::Token &token) {
    std::string digits = token.getLexeme();
    int base = 10;

    switch (token.getId()) {
    case GalsGenerated::t_LIT_INT_HEX:
        digits = digits.substr(2);
        base = 16;
        break;
    case GalsGenerated::t_LIT_INT_BIN:
        digits = digits.substr(2);
        base = 2;
        break;
    case GalsGenerated::t_LIT_INT_DEC:
        break;
    default:
        throw GalsGenerated::SemanticError("Tamanho de vetor deve ser um literal inteiro", token.getPosition());
    }

    try {
        std::size_t parsedCharacters = 0;
        const long long size = std::stoll(digits, &parsedCharacters, base);

        if (parsedCharacters != digits.size() || size <= 0 || size > std::numeric_limits<int>::max()) {
            throw GalsGenerated::SemanticError("Tamanho de vetor invalido", token.getPosition());
        }

        return static_cast<int>(size);
    } catch (const std::invalid_argument &) {
        throw GalsGenerated::SemanticError("Tamanho de vetor invalido", token.getPosition());
    } catch (const std::out_of_range &) {
        throw GalsGenerated::SemanticError("Tamanho de vetor fora do limite", token.getPosition());
    }
}

} // namespace

namespace GalsGenerated {

Semantico::Semantico() { scopeStack.push(0); }

void Semantico::executeAction(int action, const Token *token) {
    if (token == nullptr) throw SemanticError("Acao semantica executada sem token");

    switch (action) {
    case 1: { // ID em declaração de variável
        pendingIdentifiers.push_back(token->getLexeme());
        break;
    }
    case 2: { // Tipo em declaração de variável
        const auto type = symbolTypeFromToken(*token);
        const auto modality = pendingVectorSize.has_value() ? SymbolModality::VECTOR : SymbolModality::VARIABLE;

        for (const auto &identifier : pendingIdentifiers) {
            symbolTable.declare(identifier, type, modality, scopeStack.top(), pendingVectorSize);
        }

        pendingIdentifiers.clear();
        pendingVectorSize.reset();
        break;
    }
    case 3: { // Tamanho de vetor
        pendingVectorSize = parseArraySize(*token);
        break;
    }
    case 4: { // ID de função
        pendingFunctionIdentifier = token->getLexeme();
        break;
    }
    case 5: { // ID de parâmetro
        pendingParameterIdentifier = token->getLexeme();
        pendingVectorSize.reset();
        break;
    }
    case 6: { // Tipo de parâmetro
        if (!pendingParameterIdentifier.has_value()) {
            throw SemanticError("Parametro sem identificador", token->getPosition());
        }

        symbolTable.declare(*pendingParameterIdentifier, symbolTypeFromToken(*token), SymbolModality::PARAMETER,
                            scopeStack.top(), pendingVectorSize);
        pendingParameterIdentifier.reset();
        pendingVectorSize.reset();
        break;
    }
    case 7: { // Tipo de retorno de função
        if (!pendingFunctionIdentifier.has_value()) {
            throw SemanticError("Funcao sem identificador", token->getPosition());
        }

        symbolTable.declare(*pendingFunctionIdentifier, symbolTypeFromToken(*token), SymbolModality::FUNCTION,
                            scopeStack.top(), pendingVectorSize);
        pendingFunctionIdentifier.reset();
        pendingVectorSize.reset();
        break;
    }
    case 8: { // Função sem tipo de retorno
        if (!pendingFunctionIdentifier.has_value()) {
            throw SemanticError("Funcao sem identificador", token->getPosition());
        }

        symbolTable.declare(*pendingFunctionIdentifier, SymbolType::VOID, SymbolModality::FUNCTION, scopeStack.top());
        pendingFunctionIdentifier.reset();
        pendingVectorSize.reset();
        break;
    }
    default:
        throw SemanticError("Acao sem tratamento", token->getPosition());
    }
}

const SymbolTable &Semantico::getSymbolTable() const { return symbolTable; }

} // namespace GalsGenerated
