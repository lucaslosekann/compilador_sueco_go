#include "SemanticTable.h"

namespace SemanticTable {

namespace {

constexpr int INT = static_cast<int>(SymbolType::INT);
constexpr int FLO = static_cast<int>(SymbolType::FLOAT);
constexpr int STR = static_cast<int>(SymbolType::STRING);
constexpr int BOO = static_cast<int>(SymbolType::BOOL);
constexpr int TYPE_COUNT = 5;

// expTable[T1][T2][OP]
// OP:        SUM  SUB  MUL  DIV  MOD  REL  EQ   LOG  BIT
const int expTable[TYPE_COUNT][TYPE_COUNT][OPERATION_COUNT] = {
    /* INT */ {
        /* INT */ {INT, INT, INT, FLO, INT, BOO, BOO, ERR, INT},
        /* FLO */ {FLO, FLO, FLO, FLO, ERR, BOO, BOO, ERR, ERR},
        /* CHA */ {ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
        /* STR */ {ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
        /* BOO */ {ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
    },
    /* FLO */ {
        /* INT */ {FLO, FLO, FLO, FLO, ERR, BOO, BOO, ERR, ERR},
        /* FLO */ {FLO, FLO, FLO, FLO, ERR, BOO, BOO, ERR, ERR},
        /* CHA */ {ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
        /* STR */ {ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
        /* BOO */ {ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
    },
    /* CHA */ {
        /* INT */ {ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
        /* FLO */ {ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
        /* CHA */ {ERR, ERR, ERR, ERR, ERR, ERR, BOO, ERR, ERR},
        /* STR */ {STR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
        /* BOO */ {ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
    },
    /* STR */ {
        /* INT */ {ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
        /* FLO */ {ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
        /* CHA */ {STR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
        /* STR */ {STR, ERR, ERR, ERR, ERR, ERR, BOO, ERR, ERR},
        /* BOO */ {ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
    },
    /* BOO */ {
        /* INT */ {ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
        /* FLO */ {ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
        /* CHA */ {ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
        /* STR */ {ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
        /* BOO */ {ERR, ERR, ERR, ERR, ERR, ERR, BOO, BOO, ERR},
    },
};

// unaryTable[T][OP]
// OP:                  NEG  LOG_NOT BIT_NOT
const int unaryTable[TYPE_COUNT][3] = {
    /* INT */ {INT, ERR, INT},
    /* FLO */ {FLO, ERR, ERR},
    /* CHA */ {ERR, ERR, ERR},
    /* STR */ {ERR, ERR, ERR},
    /* BOO */ {ERR, BOO, ERR},
};

// atribTable[ESQ][DIR]
// DIR:                 INT  FLO  CHA  STR  BOO
const int atribTable[TYPE_COUNT][TYPE_COUNT] = {
    /* INT */ {OK_, WAR, ERR, ERR, ERR},
    /* FLO */ {OK_, OK_, ERR, ERR, ERR},
    /* CHA */ {ERR, ERR, OK_, ERR, ERR},
    /* STR */ {ERR, ERR, ERR, OK_, ERR},
    /* BOO */ {ERR, ERR, ERR, ERR, OK_},
};

bool isTableType(SymbolType type) { return static_cast<int>(type) >= 0 && static_cast<int>(type) < TYPE_COUNT; }

} // namespace

int resultType(SymbolType left, SymbolType right, Operation operation) {
    // ANY (valor de std_läsa) se adapta ao outro operando
    if (left == SymbolType::ANY && right == SymbolType::ANY) {
        return (operation == REL || operation == EQ || operation == LOG) ? BOO : static_cast<int>(SymbolType::ANY);
    }
    if (left == SymbolType::ANY) left = right;
    if (right == SymbolType::ANY) right = left;

    if (!isTableType(left) || !isTableType(right)) return ERR;
    return expTable[static_cast<int>(left)][static_cast<int>(right)][operation];
}

int unaryResultType(SymbolType type, UnaryOperation operation) {
    if (type == SymbolType::ANY) return static_cast<int>(SymbolType::ANY);
    if (!isTableType(type)) return ERR;
    return unaryTable[static_cast<int>(type)][operation];
}

int atribType(SymbolType left, SymbolType right) {
    if (right == SymbolType::ANY) return OK_;
    if (!isTableType(left) || !isTableType(right)) return ERR;
    return atribTable[static_cast<int>(left)][static_cast<int>(right)];
}

} // namespace SemanticTable
