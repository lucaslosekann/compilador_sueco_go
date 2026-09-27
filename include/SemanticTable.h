#pragma once

#include "SymbolType.h"

// Tabelas de compatibilidade de tipos (modelo SemanticTable do professor, traduzido para C++).
// Tipos: INT, FLO, CHA, STR, BOO (mesma ordem de SymbolType).
namespace SemanticTable {

constexpr int ERR = -1;
constexpr int OK_ = 0;
constexpr int WAR = 1;

enum Operation { SUM, SUB, MUL, DIV, MOD, REL, EQ, LOG, BIT, OPERATION_COUNT };
enum UnaryOperation { NEG, LOG_NOT, BIT_NOT };

// Tipo resultante de T1 <op> T2, ou ERR
int resultType(SymbolType left, SymbolType right, Operation operation);

// Tipo resultante de <op> T, ou ERR
int unaryResultType(SymbolType type, UnaryOperation operation);

// OK_, WAR ou ERR para ESQ = DIR
int atribType(SymbolType left, SymbolType right);

} // namespace SemanticTable
