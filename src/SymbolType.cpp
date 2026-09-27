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
        return "heltal";
    case SymbolType::FLOAT:
        return "flyttal";
    case SymbolType::CHAR:
        return "tecken";
    case SymbolType::STRING:
        return "text";
    case SymbolType::BOOL:
        return "intelagom";
    case SymbolType::VOID:
        return "sem retorno";
    case SymbolType::ANY:
        return "qualquer";
    }

    return "desconhecido";
}
