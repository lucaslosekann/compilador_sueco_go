#pragma once

#include "Token.h"

enum class SymbolType : int { INT, CHAR, STRING, FLOAT, BOOL, VOID };

SymbolType symbolTypeFromToken(const GalsGenerated::Token &token);
const char *symbolTypeToString(SymbolType type);
