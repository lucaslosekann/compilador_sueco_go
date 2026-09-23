#include "SymbolType.h"
#include "SemanticError.h"

SymbolType symbolTypeFromToken(const GalsGenerated::Token &token) {
    using namespace GalsGenerated;

    switch (token.getId()) {
    case t_KW_INT:
        return SymbolType::INT;
    case t_KW_FLOAT:
        return SymbolType::FLOAT;
    case t_KW_CHAR:
        return SymbolType::CHAR;
    case t_KW_STRING:
        return SymbolType::STRING;
    case t_KW_BOOL:
        return SymbolType::BOOL;
    default:
        throw SemanticError("Token não representa um tipo", token.getPosition());
    }
}

const char *symbolTypeToString(SymbolType type) {
    switch (type) {
    case SymbolType::INT:
        return "inteiro";
    case SymbolType::CHAR:
        return "caractere";
    case SymbolType::STRING:
        return "texto";
    case SymbolType::FLOAT:
        return "real";
    case SymbolType::BOOL:
        return "booleano";
    case SymbolType::VOID:
        return "sem retorno";
    }

    return "desconhecido";
}
