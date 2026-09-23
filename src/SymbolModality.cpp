#include "SymbolModality.h"

const char *symbolModalityToString(SymbolModality modality) {
    switch (modality) {
    case SymbolModality::VARIABLE:
        return "variavel";
    case SymbolModality::VECTOR:
        return "vetor";
    case SymbolModality::PARAMETER:
        return "parametro";
    case SymbolModality::FUNCTION:
        return "funcao";
    }

    return "desconhecida";
}
