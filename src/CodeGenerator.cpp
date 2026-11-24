#include "CodeGenerator.h"

#include <sstream>

namespace {

// Identificadores com å, ä, ö (Latin-1) viram a, a, o no assembly
std::string asciiName(const std::string &name) {
    std::string out;
    for (unsigned char c : name) {
        switch (c) {
        case 0xE5: // å
        case 0xE4: // ä
            out += 'a';
            break;
        case 0xC5: // Å
        case 0xC4: // Ä
            out += 'A';
            break;
        case 0xF6: // ö
            out += 'o';
            break;
        case 0xD6: // Ö
            out += 'O';
            break;
        default:
            out += static_cast<char>(c);
        }
    }
    return out;
}

// Operacoes que tem instrucao direta no BIP
const std::map<std::string, std::string> BINARY_INSTRUCTIONS = {
    {"plus", "ADD"}, {"minus", "SUB"}, {"bitoch", "AND"}, {"biteller", "OR"}, {"bitxor", "XOR"},
};

// Desvio quando a comparacao e VERDADEIRA (o BIP olha o resultado da ultima subtracao)
const std::map<std::string, std::string> JUMP_IF_TRUE = {
    {"gt", "BGT"}, {"ge", "BGE"}, {"lt", "BLT"}, {"le", "BLE"}, {"eq", "BEQ"}, {"ne", "BNE"},
};

// Desvio quando a comparacao e FALSA (condicao invertida)
const std::map<std::string, std::string> JUMP_IF_FALSE = {
    {"gt", "BLE"}, {"ge", "BLT"}, {"lt", "BGE"}, {"le", "BGT"}, {"eq", "BNE"}, {"ne", "BEQ"},
};

bool isCommutative(const std::string &instruction) { return instruction != "SUB"; }

} // namespace

AsmOperand AsmOperand::immediate(int value) {
    AsmOperand operand;
    operand.kind = IMMEDIATE;
    operand.value = value;
    return operand;
}

AsmOperand AsmOperand::memory(const std::string &address) {
    AsmOperand operand;
    operand.kind = MEMORY;
    operand.address = address;
    return operand;
}

AsmOperand AsmOperand::computed(std::vector<std::string> code) {
    AsmOperand operand;
    operand.kind = COMPUTED;
    operand.code = std::move(code);
    return operand;
}

AsmOperand AsmOperand::unsupported(const std::string &reason) {
    AsmOperand operand;
    operand.kind = UNSUPPORTED;
    operand.reason = reason;
    return operand;
}

CodeGenerator::CodeGenerator(WarningCallback warn) : warn(std::move(warn)), buffers(1) {}

// Temporarios do programa principal: _temp1, _temp2...; de uma sub-rotina: _soma_temp1...
// (assim uma chamada no meio de uma expressao nao estraga os temporarios de quem chamou)
std::string CodeGenerator::newTemporary() {
    const std::string prefix = functionNames.empty() ? "_temp" : "_" + functionNames.back() + "_temp";
    temporaryCount++;
    if (temporaryCount > temporaryTotals[prefix]) temporaryTotals[prefix] = temporaryCount;
    return prefix + std::to_string(temporaryCount);
}

// Temporarios podem ser reaproveitados a cada novo comando
void CodeGenerator::endStatement() { temporaryCount = 0; }

// Codigo que coloca o operando no acumulador
std::vector<std::string> CodeGenerator::load(const AsmOperand &operand) const {
    switch (operand.kind) {
    case AsmOperand::IMMEDIATE:
        return {"LDI " + std::to_string(operand.value)};
    case AsmOperand::MEMORY:
        return {"LD " + operand.address};
    case AsmOperand::COMPUTED:
        return operand.code;
    default:
        return {};
    }
}

void CodeGenerator::emit(const std::vector<std::string> &code) {
    auto &text = buffers.back();
    text.insert(text.end(), code.begin(), code.end());
}

bool CodeGenerator::canEmit(std::initializer_list<const AsmOperand *> operands, const std::string &command) {
    for (const AsmOperand *operand : operands) {
        if (operand->kind == AsmOperand::UNSUPPORTED || operand->kind == AsmOperand::CONDITION) {
            const std::string reason = operand->kind == AsmOperand::CONDITION
                                           ? "condicao (comparacao, och, eller, inte) so pode ser usada em om, medan, gor-medan e for"
                                           : operand->reason;
            warn("Geracao de codigo: " + reason + " (" + command + " ignorado)");
            endStatement();
            return false;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// Expressoes
// ---------------------------------------------------------------------------

// Qualquer operando vira condicao: comparacao e usada como esta; um valor intelagom
// sozinho (ex.: ok, ja, f()) e verdadeiro quando diferente de 0
AsmOperand CodeGenerator::asCondition(const AsmOperand &operand) const {
    if (operand.kind == AsmOperand::CONDITION) return operand;

    AsmOperand condition;
    condition.kind = AsmOperand::CONDITION;
    condition.code = load(operand);
    condition.code.push_back("SUBI 0");
    condition.relop = "ne";
    return condition;
}

AsmOperand CodeGenerator::binary(const AsmOperand &left, const std::string &op, const AsmOperand &right) {
    if (left.kind == AsmOperand::UNSUPPORTED) return left;
    if (right.kind == AsmOperand::UNSUPPORTED) return right;

    // och / eller: combinam condicoes; os desvios em curto-circuito sao gerados em jumpIf
    if (op == "och" || op == "eller") {
        AsmOperand combined;
        combined.kind = AsmOperand::CONDITION;
        combined.logic = op == "och" ? "and" : "or";
        combined.parts = {asCondition(left), asCondition(right)};
        return combined;
    }

    if (left.kind == AsmOperand::CONDITION || right.kind == AsmOperand::CONDITION) {
        return AsmOperand::unsupported("comparacao dentro de outra operacao nao e gerada");
    }

    // Relacionais: calcula esquerda - direita; o desvio (BGT, BEQ...) decide depois
    if (JUMP_IF_TRUE.count(op)) {
        AsmOperand condition = binary(left, "minus", right);
        condition.kind = AsmOperand::CONDITION;
        condition.relop = op;
        return condition;
    }

    // Deslocamentos: no BIP a quantidade e sempre uma constante (SLL/SRL)
    if (op == "vanster" || op == "hoger") {
        if (right.kind != AsmOperand::IMMEDIATE) {
            return AsmOperand::unsupported("deslocamento so pode ser feito por uma constante");
        }

        auto code = load(left);
        code.push_back((op == "vanster" ? "SLL " : "SRL ") + std::to_string(right.value));
        return AsmOperand::computed(code);
    }

    const auto found = BINARY_INSTRUCTIONS.find(op);
    if (found == BINARY_INSTRUCTIONS.end()) {
        return AsmOperand::unsupported("operacao '" + op + "' nao existe no BIP");
    }

    const std::string &instruction = found->second;
    std::vector<std::string> code;

    switch (right.kind) {
    case AsmOperand::IMMEDIATE: // a + 5  ->  LD a / ADDI 5
        code = load(left);
        code.push_back(instruction + "I " + std::to_string(right.value));
        break;
    case AsmOperand::MEMORY: // a + b  ->  LD a / ADD b
        code = load(left);
        code.push_back(instruction + " " + right.address);
        break;
    default: // lado direito precisa do ACC (ex.: vetor, chamada, subexpressao)
        if (isCommutative(instruction) && left.kind != AsmOperand::COMPUTED) {
            // b + v[i]  ->  (v[i]) / ADD b
            code = right.code;
            code.push_back(left.kind == AsmOperand::IMMEDIATE ? instruction + "I " + std::to_string(left.value)
                                                              : instruction + " " + left.address);
        } else {
            // guarda o lado esquerdo, calcula o direito e opera
            const std::string leftTemp = newTemporary();
            const std::string rightTemp = newTemporary();
            code = load(left);
            code.push_back("STO " + leftTemp);
            code.insert(code.end(), right.code.begin(), right.code.end());
            code.push_back("STO " + rightTemp);
            code.push_back("LD " + leftTemp);
            code.push_back(instruction + " " + rightTemp);
        }
        break;
    }

    return AsmOperand::computed(code);
}

AsmOperand CodeGenerator::unary(const std::string &op, const AsmOperand &operand) {
    if (operand.kind == AsmOperand::UNSUPPORTED) return operand;

    if (op == "inte") {
        AsmOperand negated;
        negated.kind = AsmOperand::CONDITION;
        negated.logic = "not";
        negated.parts = {asCondition(operand)};
        return negated;
    }

    if (operand.kind == AsmOperand::CONDITION) return AsmOperand::unsupported("operacao com comparacao nao e gerada");

    if (op == "minus") {
        switch (operand.kind) {
        case AsmOperand::IMMEDIATE:
            return AsmOperand::immediate(-operand.value);
        case AsmOperand::MEMORY: // -a  ->  LDI 0 / SUB a
            return AsmOperand::computed({"LDI 0", "SUB " + operand.address});
        default: {
            const std::string temp = newTemporary();
            auto code = operand.code;
            code.push_back("STO " + temp);
            code.push_back("LDI 0");
            code.push_back("SUB " + temp);
            return AsmOperand::computed(code);
        }
        }
    }

    if (op == "bitinte") {
        auto code = load(operand);
        code.push_back("NOT 0");
        return AsmOperand::computed(code);
    }

    return AsmOperand::unsupported("operacao '" + op + "' nao existe no BIP");
}

// v[i]  ->  (i) / STO $indr / LDV v
AsmOperand CodeGenerator::vectorElement(const std::string &vector, const AsmOperand &index) {
    if (index.kind == AsmOperand::UNSUPPORTED || index.kind == AsmOperand::CONDITION) {
        return AsmOperand::unsupported("indice de vetor invalido para o BIP");
    }

    auto code = load(index);
    code.push_back("STO $indr");
    code.push_back("LDV " + vector);
    return AsmOperand::computed(code);
}

// f(a, b)  ->  copia cada argumento no seu parametro / CALL _f   (o retorno fica no ACC)
//
// Os parametros ficam em posicoes fixas da memoria. Se um argumento tambem chama uma sub-rotina
// (ex.: f(1, f(2, 3))), essa chamada reescreve os parametros que ja tinham sido copiados.
// Entao, quando algum argumento contem um CALL, os argumentos anteriores a ele sao avaliados
// em temporarios e so copiados para os parametros depois que a ultima chamada interna terminou.
AsmOperand CodeGenerator::call(const std::string &label, const std::vector<CallArgument> &arguments) {
    std::vector<std::string> code;

    for (const auto &argument : arguments) {
        const AsmOperand &value = argument.value;
        if (value.kind == AsmOperand::UNSUPPORTED) return value;
        if (value.kind == AsmOperand::CONDITION) return AsmOperand::unsupported("comparacao como argumento");
    }

    auto containsCall = [](const AsmOperand &value) {
        for (const auto &line : value.code) {
            if (line.rfind("CALL ", 0) == 0) return true;
        }
        return false;
    };

    // indice do ultimo argumento que chama uma sub-rotina (-1 se nenhum)
    int lastCall = -1;
    for (std::size_t i = 0; i < arguments.size(); i++) {
        if (containsCall(arguments[i].value)) lastCall = static_cast<int>(i);
    }

    auto copyArgument = [&](const CallArgument &argument) {
        if (argument.vectorSize > 0) {
            // vetor por copia: copia posicao por posicao (o mesmo $indr serve para os dois vetores)
            for (int i = 0; i < argument.vectorSize; i++) {
                code.push_back("LDI " + std::to_string(i));
                code.push_back("STO $indr");
                code.push_back("LDV " + argument.value.address);
                code.push_back("STOV " + argument.parameter);
            }
        } else {
            const auto valueCode = load(argument.value);
            code.insert(code.end(), valueCode.begin(), valueCode.end());
            code.push_back("STO " + argument.parameter);
        }
    };

    std::vector<std::string> temporaries(arguments.size());
    const std::size_t firstDirect = lastCall < 0 ? 0 : static_cast<std::size_t>(lastCall);

    // fase 1: argumentos que vem antes da ultima chamada interna (vetores nao mudam, ficam para a fase 2)
    for (std::size_t i = 0; i < firstDirect; i++) {
        if (arguments[i].vectorSize > 0) continue;
        temporaries[i] = newTemporary();
        const auto valueCode = load(arguments[i].value);
        code.insert(code.end(), valueCode.begin(), valueCode.end());
        code.push_back("STO " + temporaries[i]);
    }

    // o argumento da ultima chamada interna e copiado logo, antes de qualquer outra coisa mexer no ACC
    if (lastCall >= 0) copyArgument(arguments[lastCall]);

    // fase 2: copia os temporarios (e vetores) para os parametros
    for (std::size_t i = 0; i < firstDirect; i++) {
        if (arguments[i].vectorSize > 0) {
            copyArgument(arguments[i]);
        } else {
            code.push_back("LD " + temporaries[i]);
            code.push_back("STO " + arguments[i].parameter);
        }
    }

    // argumentos restantes (sem chamadas): copia direta
    for (std::size_t i = lastCall < 0 ? 0 : firstDirect + 1; i < arguments.size(); i++) copyArgument(arguments[i]);

    code.push_back("CALL " + label);
    return AsmOperand::computed(code);
}

// ---------------------------------------------------------------------------
// Comandos
// ---------------------------------------------------------------------------

// x = <valor>  ->  (valor) / STO x
void CodeGenerator::assign(const std::string &variable, const AsmOperand &value) {
    if (!canEmit({&value}, "atribuicao a '" + variable + "'")) return;

    emit(load(value));
    emit({"STO " + variable});
    endStatement();
}

// v[i] = <valor>  ->  (i) / STO $indr / (valor) / STOV v
void CodeGenerator::assignVector(const std::string &vector, const AsmOperand &index, const AsmOperand &value) {
    if (!canEmit({&index, &value}, "atribuicao ao vetor '" + vector + "'")) return;

    if (value.kind == AsmOperand::COMPUTED) {
        // o valor usa o ACC (e talvez o $indr): calcula antes e guarda num temporario
        const std::string temp = newTemporary();
        emit(value.code);
        emit({"STO " + temp});
        emit(load(index));
        emit({"STO $indr", "LD " + temp, "STOV " + vector});
    } else {
        emit(load(index));
        emit({"STO $indr"});
        emit(load(value));
        emit({"STOV " + vector});
    }
    endStatement();
}

// std_skriva(<valor>)  ->  (valor) / STO $out_port
void CodeGenerator::write(const AsmOperand &value) {
    if (!canEmit({&value}, "std_skriva")) return;

    emit(load(value));
    emit({"STO $out_port"});
    endStatement();
}

// x++ / x--  ->  LD x / ADDI 1 / STO x
void CodeGenerator::increment(const std::string &variable, int delta) {
    emit({"LD " + variable, (delta > 0 ? "ADDI " : "SUBI ") + std::to_string(delta > 0 ? delta : -delta),
          "STO " + variable});
}

// f(x); como comando: so executa a chamada
void CodeGenerator::callStatement(const AsmOperand &call) {
    if (!canEmit({&call}, "chamada")) return;

    emit(call.code);
    endStatement();
}

// ge <valor>;  ->  (valor) / RETURN 0   (o valor de retorno fica no ACC)
void CodeGenerator::returnValue(const AsmOperand &value) {
    if (canEmit({&value}, "ge")) emit(load(value));
    emit({"RETURN 0"});
    endStatement();
}

void CodeGenerator::returnVoid() { emit({"RETURN 0"}); }

// ---------------------------------------------------------------------------
// Desvios
// ---------------------------------------------------------------------------

int CodeGenerator::newLabelNumber() { return ++labelCount; }

void CodeGenerator::label(const std::string &name) { emit({name + ":"}); }

void CodeGenerator::jump(const std::string &label) { emit({"JMP " + label}); }

void CodeGenerator::jumpIfFalse(const AsmOperand &condition, const std::string &label) {
    jumpIf(condition, label, false);
}

void CodeGenerator::jumpIfTrue(const AsmOperand &condition, const std::string &label) {
    jumpIf(condition, label, true);
}

// a > b  ->  LD a / SUB b / BLE label   (desvio pela condicao invertida quando "se falso")
void CodeGenerator::jumpIf(const AsmOperand &condition, const std::string &label, bool whenTrue) {
    if (condition.kind == AsmOperand::UNSUPPORTED) {
        warn("Geracao de codigo: " + condition.reason + " (desvio para " + label + " ignorado)");
        endStatement();
        return;
    }

    branch(asCondition(condition), label, whenTrue);
    endStatement();
}

// Gera o codigo que desvia para `target` quando a condicao vale `jumpWhen` e segue em frente caso contrario.
// och / eller usam curto-circuito: o segundo operando so e avaliado se o primeiro nao decidiu o resultado.
void CodeGenerator::branch(const AsmOperand &condition, const std::string &target, bool jumpWhen) {
    if (condition.logic == "not") {
        branch(condition.parts[0], target, !jumpWhen);
        return;
    }

    if (!condition.logic.empty()) {
        // valor que decide sozinho: falso para "and", verdadeiro para "or"
        const bool decisive = condition.logic == "or";

        if (jumpWhen == decisive) {
            branch(condition.parts[0], target, decisive);
            branch(condition.parts[1], target, decisive);
        } else {
            const std::string skip = "CURTO" + std::to_string(newLabelNumber());
            branch(condition.parts[0], skip, decisive);
            branch(condition.parts[1], target, jumpWhen);
            label(skip);
        }
        return;
    }

    emit(condition.code);
    const auto &jumps = jumpWhen ? JUMP_IF_TRUE : JUMP_IF_FALSE;
    emit({jumps.at(condition.relop) + " " + target});
}

void CodeGenerator::beginCapture() { buffers.emplace_back(); }

std::vector<std::string> CodeGenerator::endCapture() {
    std::vector<std::string> captured = buffers.back();
    buffers.pop_back();
    return captured;
}

void CodeGenerator::emitCaptured(const std::vector<std::string> &code) { emit(code); }

// ---------------------------------------------------------------------------
// Sub-rotinas
// ---------------------------------------------------------------------------

// O codigo da sub-rotina vai para um trecho separado, que fica antes do programa principal
void CodeGenerator::beginFunction(const std::string &label, const std::string &name) {
    buffers.push_back({label + ":"});
    functionNames.push_back(name);
}

void CodeGenerator::endFunction() {
    // procedimento ou funcao sem 'ge' no final
    if (buffers.back().back() != "RETURN 0") emit({"RETURN 0"});
    functionsText.insert(functionsText.end(), buffers.back().begin(), buffers.back().end());
    buffers.pop_back();
    functionNames.pop_back();
}

// ---------------------------------------------------------------------------
// Programa
// ---------------------------------------------------------------------------

std::string CodeGenerator::asmName(const SymbolTable &table, std::size_t symbol,
                                   const std::function<std::string(int)> &scopeName) const {
    const Symbol &s = table.at(symbol);

    // Se ja existe um simbolo ANTERIOR com o mesmo nome, este recebe o nome do escopo
    // (ex.: idade_global2, x_maior). Olhar so os anteriores deixa o nome sempre igual.
    bool repeated = false;
    for (std::size_t i = 0; i < symbol; i++) {
        if (table.at(i).getId() == s.getId()) repeated = true;
    }

    std::string name = s.getId();
    if (repeated) name += "_" + scopeName(s.getScope());
    return asciiName(name);
}

std::string CodeGenerator::program(const SymbolTable &table, const std::function<std::string(int)> &scopeName) const {
    std::ostringstream out;

    // .data: variaveis, vetores e parametros (passagem por copia) + temporarios
    out << ".data\n";
    const auto &symbols = table.getSymbols();
    for (std::size_t i = 0; i < symbols.size(); i++) {
        const Symbol &symbol = symbols[i];
        if (symbol.isFunction()) continue;

        out << '\t' << asmName(table, i, scopeName) << " : 0";
        if (symbol.isVector()) {
            for (int k = 1; k < *symbol.getVectorSize(); k++) out << ", 0";
        }
        out << '\n';
    }
    for (const auto &[prefix, total] : temporaryTotals) {
        for (int t = 1; t <= total; t++) out << '\t' << prefix << t << " : 0\n";
    }

    // .text: sub-rotinas primeiro, depois o programa principal
    std::vector<std::string> text;
    if (!functionsText.empty()) {
        text.push_back("JMP _PRINCIPAL");
        text.insert(text.end(), functionsText.begin(), functionsText.end());
        text.push_back("_PRINCIPAL:");
    }
    text.insert(text.end(), buffers.front().begin(), buffers.front().end());
    text.push_back("HLT 0");

    // Formato pedido pelo professor: rotulos na margem,
    // mnemonico a um tab da margem e operando a um tab do mnemonico (tres colunas)
    out << ".text\n";
    for (const auto &line : text) {
        if (line.back() == ':') {
            out << line << '\n';
            continue;
        }
        const auto space = line.find(' ');
        out << '\t' << line.substr(0, space) << '\t' << line.substr(space + 1) << '\n';
    }

    return out.str();
}
