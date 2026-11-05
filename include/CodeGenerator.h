#pragma once

#include "SymbolTable.h"
#include <functional>
#include <string>
#include <vector>

// Operando de uma expressao durante a geracao de codigo BIP.
// IMMEDIATE: constante (LDI/ADDI...), MEMORY: endereco (LD/ADD...),
// COMPUTED: codigo que deixa o resultado no ACC, UNSUPPORTED: nao gera codigo no T4.
struct AsmOperand {
    enum Kind { NONE, IMMEDIATE, MEMORY, COMPUTED, UNSUPPORTED };

    Kind kind = NONE;
    int value = 0;
    std::string address;
    std::vector<std::string> code;
    std::string reason; // motivo quando UNSUPPORTED

    static AsmOperand immediate(int value);
    static AsmOperand memory(const std::string &address);
    static AsmOperand computed(std::vector<std::string> code);
    static AsmOperand unsupported(const std::string &reason);
};

// Gera codigo assembly do processador BIP (Bipide 3.0)
class CodeGenerator {
  public:
    using WarningCallback = std::function<void(const std::string &)>;

    explicit CodeGenerator(WarningCallback warn);

    // Expressoes
    AsmOperand binary(const AsmOperand &left, const std::string &op, const AsmOperand &right);
    AsmOperand unary(const std::string &op, const AsmOperand &operand);
    AsmOperand vectorElement(const std::string &vector, const AsmOperand &index);

    // Comandos (escrevem no .text)
    void assign(const std::string &variable, const AsmOperand &value);
    void assignVector(const std::string &vector, const AsmOperand &index, const AsmOperand &value);
    void write(const AsmOperand &value);
    void read();
    void increment(const std::string &variable, int delta);

    // Descarta temporarios de uma expressao que nao gerou comando (ex.: condicao, retorno)
    void endStatement();

    // Liga/desliga a geracao (ex.: dentro de sub-rotinas, que ficam para o T6)
    void setEnabled(bool enabled);
    bool isEnabled() const;

    // Nome usado no assembly para um simbolo da tabela
    std::string asmName(const SymbolTable &table, std::size_t symbol,
                        const std::function<std::string(int)> &scopeName) const;

    // Programa completo: .data (tabela de simbolos + temporarios) e .text
    std::string program(const SymbolTable &table, const std::function<std::string(int)> &scopeName) const;

  private:
    WarningCallback warn;
    bool enabled = true;
    std::vector<std::string> text;
    int temporaryCount = 0;    // temporarios usados no comando atual
    int maxTemporaryCount = 0; // total declarado no .data

    std::string newTemporary();
    std::vector<std::string> load(const AsmOperand &operand) const;
    bool canEmit(std::initializer_list<const AsmOperand *> operands, const std::string &command);
    void emit(const std::vector<std::string> &code);
};
