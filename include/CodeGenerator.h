#pragma once

#include "SymbolTable.h"
#include <functional>
#include <map>
#include <string>
#include <vector>

// Operando de uma expressao durante a geracao de codigo BIP.
// IMMEDIATE: constante (LDI/ADDI...), MEMORY: endereco (LD/ADD...),
// COMPUTED: codigo que deixa o resultado no ACC,
// CONDITION: condicao usada nos desvios. Folha = comparacao (codigo da subtracao + operador relacional);
//            ou combinacao de condicoes (logic = "and", "or" ou "not", com as partes em `parts`),
// UNSUPPORTED: nao gera codigo.
struct AsmOperand {
    enum Kind { NONE, IMMEDIATE, MEMORY, COMPUTED, CONDITION, UNSUPPORTED };

    Kind kind = NONE;
    int value = 0;
    std::string address;
    std::vector<std::string> code;
    std::string relop;  // gt, ge, lt, le, eq, ne (so em CONDITION)
    std::string logic;  // "and", "or", "not" (so em CONDITION combinada; vazio nas folhas)
    std::vector<AsmOperand> parts;
    std::string reason; // motivo quando UNSUPPORTED

    static AsmOperand immediate(int value);
    static AsmOperand memory(const std::string &address);
    static AsmOperand computed(std::vector<std::string> code);
    static AsmOperand unsupported(const std::string &reason);
};

// Argumento de uma chamada: valor e parametro que recebe a copia
struct CallArgument {
    AsmOperand value;
    std::string parameter; // nome do parametro no .data
    int vectorSize;        // 0 = escalar; > 0 = vetor copiado posicao por posicao
};

// Gera codigo assembly do processador BIP (Bipide 3.0)
class CodeGenerator {
  public:
    using WarningCallback = std::function<void(const std::string &)>;

    explicit CodeGenerator(WarningCallback warn);

    // Expressoes (op: plus, minus, bitoch, biteller, bitxor, vanster, hoger, gt, ge, lt, le, eq, ne, ...)
    AsmOperand binary(const AsmOperand &left, const std::string &op, const AsmOperand &right);
    AsmOperand unary(const std::string &op, const AsmOperand &operand);
    AsmOperand vectorElement(const std::string &vector, const AsmOperand &index);
    AsmOperand call(const std::string &label, const std::vector<CallArgument> &arguments);

    // Comandos
    void assign(const std::string &variable, const AsmOperand &value);
    void assignVector(const std::string &vector, const AsmOperand &index, const AsmOperand &value);
    void write(const AsmOperand &value);
    void increment(const std::string &variable, int delta);
    void callStatement(const AsmOperand &call);
    void returnValue(const AsmOperand &value);
    void returnVoid();

    // Desvios (T5)
    int newLabelNumber();
    void label(const std::string &name);
    void jump(const std::string &label);
    void jumpIfFalse(const AsmOperand &condition, const std::string &label);
    void jumpIfTrue(const AsmOperand &condition, const std::string &label);

    // Guarda o codigo gerado num trecho (usado no incremento do for, que vai para o fim do laco)
    void beginCapture();
    std::vector<std::string> endCapture();
    void emitCaptured(const std::vector<std::string> &code);

    // Sub-rotinas (T6)
    void beginFunction(const std::string &label, const std::string &name);
    void endFunction();

    // Descarta temporarios de uma expressao terminada
    void endStatement();

    // Nome usado no assembly para um simbolo da tabela
    std::string asmName(const SymbolTable &table, std::size_t symbol,
                        const std::function<std::string(int)> &scopeName) const;

    // Programa completo: .data (tabela de simbolos + temporarios) e .text
    std::string program(const SymbolTable &table, const std::function<std::string(int)> &scopeName) const;

  private:
    WarningCallback warn;

    std::vector<std::vector<std::string>> buffers; // buffers[0] = programa principal
    std::vector<std::string> functionsText;        // codigo das sub-rotinas
    std::vector<std::string> functionNames;        // sub-rotinas sendo geradas (para nomear temporarios)

    int labelCount = 0;
    int temporaryCount = 0;                     // temporarios usados no comando atual
    std::map<std::string, int> temporaryTotals; // prefixo -> quantidade declarada no .data

    std::string newTemporary();
    std::vector<std::string> load(const AsmOperand &operand) const;
    bool canEmit(std::initializer_list<const AsmOperand *> operands, const std::string &command);
    void emit(const std::vector<std::string> &code);
    void jumpIf(const AsmOperand &condition, const std::string &label, bool whenTrue);
    void branch(const AsmOperand &condition, const std::string &target, bool jumpWhen);
    AsmOperand asCondition(const AsmOperand &operand) const;
};
