#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace GalsGenerated {

enum TokenId 
{
    EPSILON  = 0,
    DOLLAR   = 1,
    t_KW_READ_STDIN = 2,
    t_KW_WRITE_STDOUT = 3,
    t_KW_INT = 4,
    t_KW_FLOAT = 5,
    t_KW_CHAR = 6,
    t_KW_STRING = 7,
    t_KW_BOOL = 8,
    t_KW_TRUE = 9,
    t_KW_FALSE = 10,
    t_KW_TYPE = 11,
    t_KW_VAR = 12,
    t_KW_CONST = 13,
    t_KW_STRUCT = 14,
    t_KW_INTERFACE = 15,
    t_KW_IF = 16,
    t_KW_ELSE = 17,
    t_KW_SWITCH = 18,
    t_KW_CASE = 19,
    t_KW_DEFAULT = 20,
    t_KW_FALLTHROUGH = 21,
    t_KW_FOR = 22,
    t_KW_WHILE = 23,
    t_KW_DO = 24,
    t_KW_BREAK = 25,
    t_KW_CONTINUE = 26,
    t_KW_GOTO = 27,
    t_KW_FUNCTION = 28,
    t_KW_RETURN = 29,
    t_KW_PACKAGE = 30,
    t_KW_IMPORT = 31,
    t_KW_GO = 32,
    t_KW_CHAN = 33,
    t_KW_SELECT = 34,
    t_KW_DEFER = 35,
    t_KW_RANGE = 36,
    t_KW_MAP = 37,
    t_ARTM_OP_PLUS = 38,
    t_ARTM_OP_MINUS = 39,
    t_ARTM_OP_MUL = 40,
    t_ARTM_OP_DIV = 41,
    t_ARTM_OP_MOD = 42,
    t_REL_OP_GT_EQ = 43,
    t_REL_OP_LT_EQ = 44,
    t_REL_OP_EQ = 45,
    t_REL_OP_NOT_EQ = 46,
    t_REL_OP_GT = 47,
    t_REL_OP_LT = 48,
    t_LOG_OP_AND = 49,
    t_LOG_OP_OR = 50,
    t_LOG_OP_NOT = 51,
    t_BIT_OP_SH_LFT = 52,
    t_BIT_OP_SH_RHT = 53,
    t_BIT_OP_AND = 54,
    t_BIT_OP_OR = 55,
    t_BIT_OP_NOT = 56,
    t_BIT_OP_XOR = 57,
    t_ATTR_OP_EQ = 58,
    t_ATTR_INCR = 59,
    t_ATTR_DECR = 60,
    t_DEL_DOT = 61,
    t_DEL_SEMICOLON = 62,
    t_DEL_COMMA = 63,
    t_DEL_COLON = 64,
    t_DEL_PAREN_LFT = 65,
    t_DEL_PAREN_RHT = 66,
    t_DEL_SQ_BRACKET_LFT = 67,
    t_DEL_SQ_BRACKET_RHT = 68,
    t_DEL_CR_BRACKET_LFT = 69,
    t_DEL_CR_BRACKET_RHT = 70,
    t_COMMENT = 71,
    t_COMMENT_MULTI_LINE = 72,
    t_LIT_INT_HEX = 73,
    t_LIT_INT_BIN = 74,
    t_LIT_FLOAT = 75,
    t_LIT_INT_DEC = 76,
    t_LIT_CHAR = 77,
    t_LIT_STRING = 78,
    t_ID = 79
};

const int STATES_COUNT = 281;

extern int SCANNER_TABLE[STATES_COUNT][256];

extern int TOKEN_STATE[STATES_COUNT];

extern const char *SCANNER_ERROR[STATES_COUNT];

const int FIRST_SEMANTIC_ACTION = 128;

const int SHIFT  = 0;
const int REDUCE = 1;
const int ACTION = 2;
const int ACCEPT = 3;
const int GO_TO  = 4;
const int ERROR  = 5;

extern const int PARSER_TABLE[186][128][2];

extern const int PRODUCTIONS[115][2];

extern const char *PARSER_ERROR[186];

} //namespace GalsGenerated

#endif
