#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "CodeGenerator.h"
#include "SemanticError.h"
#include "SemanticTable.h"
#include "Symbol.h"
#include "SymbolTable.h"
#include "Token.h"
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace GalsGenerated {

// Aviso semantico (nao interrompe a compilacao)
struct SemanticWarning {
    std::string message;
    int position;
};

// Tipo de uma expressao na pilha de tipos
struct ExprType {
    SymbolType type;
    bool isVector;    // vetor usado sem indice (ex.: passado como argumento)
    std::string name; // nome do identificador, quando houver (para mensagens)
    AsmOperand asmOperand; // como gerar o valor no BIP (T4)

    ExprType(SymbolType type = SymbolType::VOID, bool isVector = false, std::string name = "",
             AsmOperand asmOperand = AsmOperand())
        : type(type), isVector(isVector), name(std::move(name)), asmOperand(std::move(asmOperand)) {}
};

class Semantico {
  private:
    struct PendingId {
        std::string name;
        int position;
    };

    struct AttrTarget {
        std::size_t symbol;
        bool indexed;
        int position;
        AsmOperand index;
    };

    struct CallFrame {
        std::size_t function;
        std::size_t typeStackMark;
        int position;
    };

    struct PendingOperator {
        TokenId token;
        std::string lexeme;
        int position;
    };

    SymbolTable symbolTable;
    std::vector<int> scopeStack;
    std::vector<std::size_t> functionStack;
    std::map<std::string, int> blockCounter;

    // declaracoes
    std::vector<PendingId> pendingIdentifiers;
    std::optional<PendingId> pendingParameter;
    std::optional<int> pendingVectorSize;
    std::vector<std::size_t> lastDeclared;

    // expressoes
    std::vector<ExprType> typeStack;
    std::vector<PendingOperator> operatorStack;
    std::vector<std::size_t> valueListMarks;
    std::vector<AttrTarget> attrTargets;
    std::vector<CallFrame> callStack;
    ExprType lastCallResult{SymbolType::VOID};
    ExprType lastIncrDecrResult{SymbolType::INT};

    // geracao de codigo (T4)
    CodeGenerator codeGenerator;
    int currentPosition = 0;
    std::optional<std::size_t> pendingIncrDecr;

    std::vector<SemanticWarning> warnings;
    std::set<std::size_t> warnedUninitialized;

    int currentScope() const;
    std::string currentScopeName() const;
    void openScope(const std::string &name, std::optional<std::size_t> function = std::nullopt);
    void openBlockScope();
    void closeScope();

    std::size_t declare(const PendingId &id, SymbolType type, SymbolModality modality,
                        std::optional<int> vectorSize = std::nullopt);
    std::optional<std::size_t> findVisible(const std::string &name) const;
    std::size_t findVariableOrThrow(const Token &token);

    ExprType popType(int position);
    std::vector<ExprType> popValueList(int position);
    void checkAssignment(SymbolType leftType, bool leftIsVector, const ExprType &right, const std::string &target,
                         int position);
    void checkIndex(const ExprType &index, const std::string &vectorName, int position);
    void warnIfUninitialized(std::size_t symbol, int position);
    void warn(const std::string &message, int position);

    std::string asmName(std::size_t symbol) const;
    void updateCodeGeneration();

    void binaryOperation(int position);
    void unaryOperation(int position);

  public:
    Semantico();

    void executeAction(int action, const Token *token);

    // Chamado depois da analise sintatica terminar (avisos de nao usados)
    void finish();

    const SymbolTable &getSymbolTable() const;
    const std::vector<SemanticWarning> &getWarnings() const;
    std::string scopeName(int scope) const;

    // Codigo assembly BIP do programa (T4)
    std::string generateProgram() const;
};

} // namespace GalsGenerated

#endif
