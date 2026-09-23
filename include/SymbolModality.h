#pragma once

enum class SymbolModality { VARIABLE, VECTOR, PARAMETER, FUNCTION };

const char *symbolModalityToString(SymbolModality modality);
