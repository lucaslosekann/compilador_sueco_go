#pragma once

#include "Token.h"

// A ordem de INT..BOOL e usada como indice nas tabelas de compatibilidade (SemanticTable).
// VOID e ANY sao tipos internos: VOID = sub-rotina sem retorno, ANY = valor lido com std_läsa().
enum class SymbolType : int { INT = 0, FLOAT = 1, CHAR = 2, STRING = 3, BOOL = 4, VOID, ANY };

SymbolType symbolTypeFromToken(const GalsGenerated::Token &token);
const char *symbolTypeToString(SymbolType type);
