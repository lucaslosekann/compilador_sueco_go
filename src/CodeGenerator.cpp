#include "CodeGenerator.h"
#include "Semantico.h"

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

// Instrucao do BIP de cada operacao (texto vazio se a operacao nao tem instrucao direta)
std::string instructionFor(const std::string &op) {
    if (op == "plus") return "ADD";
    if (op == "minus") return "SUB";
    if (op == "bitoch") return "AND";
    if (op == "biteller") return "OR";
    if (op == "bitxor") return "XOR";
    return "";
}

bool isRelational(const std::string &op) {
    return op == "gt" || op == "ge" || op == "lt" || op == "le" || op == "eq" || op == "ne";
}

// Desvio quando a comparacao e VERDADEIRA (o BIP olha o resultado da ultima subtracao)
std::string jumpWhenTrue(const std::string &relop) {
    if (relop == "gt") return "BGT";
    if (relop == "ge") return "BGE";
    if (relop == "lt") return "BLT";
    if (relop == "le") return "BLE";
    if (relop == "eq") return "BEQ";
    return "BNE";
}

// Desvio quando a comparacao e FALSA (condicao invertida)
std::string jumpWhenFalse(const std::string &relop) {
    if (relop == "gt") return "BLE";
    if (relop == "ge") return "BLT";
    if (relop == "lt") return "BGE";
    if (relop == "le") return "BGT";
    if (relop == "eq") return "BNE";
    return "BEQ";
}

bool isCommutative(const std::string &instruction) { return instruction != "SUB"; }

// O codigo da expressao tem uma chamada (CALL) dentro?
bool hasCall(const AsmOperand &value) {
    for (const std::string &line : value.code) {
        if (line.substr(0, 5) == "CALL ") return true;
    }
    return false;
}

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

AsmOperand AsmOperand::computed(const std::vector<std::string> &code) {
    AsmOperand operand;
    operand.kind = COMPUTED;
    operand.code = code;
    return operand;
}

AsmOperand AsmOperand::unsupported(const std::string &reason) {
    AsmOperand operand;
    operand.kind = UNSUPPORTED;
    operand.reason = reason;
    return operand;
}

CodeGenerator::CodeGenerator(GalsGenerated::Semantico *semantico) : semantico(semantico), buffers(1) {}

void CodeGenerator::warn(const std::string &message) { semantico->addWarning(message); }

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

// Confere se o operando pode virar codigo. Se nao puder, avisa e o comando e ignorado.
bool CodeGenerator::canEmit(const AsmOperand &operand, const std::string &command) {
    if (operand.kind == AsmOperand::UNSUPPORTED || operand.kind == AsmOperand::CONDITION) {
        std::string reason = operand.reason;
        if (operand.kind == AsmOperand::CONDITION) {
            reason = "condicao (comparacao, och, eller, inte) so pode ser usada em om, medan, gor-medan e for";
        }
        warn("Geracao de codigo: " + reason + " (" + command + " ignorado)");
        endStatement();
        return false;
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
    if (isRelational(op)) {
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

    const std::string instruction = instructionFor(op);
    if (instruction == "") {
        return AsmOperand::unsupported("operacao '" + op + "' nao existe no BIP");
    }

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

// Copia um argumento para o parametro (vetor: posicao por posicao, o mesmo $indr serve para os dois)
void CodeGenerator::copyArgument(std::vector<std::string> &code, const CallArgument &argument) const {
    if (argument.vectorSize > 0) {
        for (int i = 0; i < argument.vectorSize; i++) {
            code.push_back("LDI " + std::to_string(i));
            code.push_back("STO $indr");
            code.push_back("LDV " + argument.value.address);
            code.push_back("STOV " + argument.parameter);
        }
    } else {
        for (const std::string &line : load(argument.value)) code.push_back(line);
        code.push_back("STO " + argument.parameter);
    }
}

// f(a, b)  ->  copia cada argumento no seu parametro / CALL _f   (o retorno fica no ACC)
//
// Os parametros ficam em posicoes fixas da memoria. Se um argumento tambem chama uma sub-rotina,
// como em f(1, f(2, 3)), essa chamada de dentro reescreve os parametros que ja tinham sido copiados.
// Por isso, quando existe chamada dentro dos argumentos, os argumentos anteriores a ela sao calculados
// em temporarios e so copiados para os parametros depois que a chamada de dentro terminou.
AsmOperand CodeGenerator::call(const std::string &label, const std::vector<CallArgument> &arguments) {
    std::vector<std::string> code;
    int total = arguments.size();

    for (int i = 0; i < total; i++) {
        if (arguments[i].value.kind == AsmOperand::UNSUPPORTED) return arguments[i].value;
        if (arguments[i].value.kind == AsmOperand::CONDITION) {
            return AsmOperand::unsupported("comparacao como argumento");
        }
    }

    // posicao do ultimo argumento que tambem faz uma chamada (-1 se nenhum faz)
    int lastCall = -1;
    for (int i = 0; i < total; i++) {
        if (hasCall(arguments[i].value)) lastCall = i;
    }

    if (lastCall == -1) {
        // caso comum: copia direto
        for (int i = 0; i < total; i++) copyArgument(code, arguments[i]);
    } else {
        // 1) os argumentos antes da ultima chamada vao para temporarios (vetores nao mudam, ficam para o passo 3)
        std::vector<std::string> temporaries(total);
        for (int i = 0; i < lastCall; i++) {
            if (arguments[i].vectorSize > 0) continue;

            temporaries[i] = newTemporary();
            for (const std::string &line : load(arguments[i].value)) code.push_back(line);
            code.push_back("STO " + temporaries[i]);
        }

        // 2) o argumento da ultima chamada ja vai para o parametro (o resultado esta no ACC)
        copyArgument(code, arguments[lastCall]);

        // 3) agora os temporarios e os vetores vao para os parametros
        for (int i = 0; i < lastCall; i++) {
            if (arguments[i].vectorSize > 0) {
                copyArgument(code, arguments[i]);
            } else {
                code.push_back("LD " + temporaries[i]);
                code.push_back("STO " + arguments[i].parameter);
            }
        }

        // 4) os argumentos depois da ultima chamada nao chamam nada: copia direto
        for (int i = lastCall + 1; i < total; i++) copyArgument(code, arguments[i]);
    }

    code.push_back("CALL " + label);
    return AsmOperand::computed(code);
}

// ---------------------------------------------------------------------------
// Comandos
// ---------------------------------------------------------------------------

// x = <valor>  ->  (valor) / STO x
void CodeGenerator::assign(const std::string &variable, const AsmOperand &value) {
    if (!canEmit(value, "atribuicao a '" + variable + "'")) return;

    emit(load(value));
    emit({"STO " + variable});
    endStatement();
}

// v[i] = <valor>  ->  (i) / STO $indr / (valor) / STOV v
void CodeGenerator::assignVector(const std::string &vector, const AsmOperand &index, const AsmOperand &value) {
    const std::string command = "atribuicao ao vetor '" + vector + "'";
    if (!canEmit(index, command) || !canEmit(value, command)) return;

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
    if (!canEmit(value, "std_skriva")) return;

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
    if (!canEmit(call, "chamada")) return;

    emit(call.code);
    endStatement();
}

// ge <valor>;  ->  (valor) / RETURN 0   (o valor de retorno fica no ACC)
void CodeGenerator::returnValue(const AsmOperand &value) {
    if (canEmit(value, "ge")) emit(load(value));
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

// Gera o desvio de uma condicao: pula para `target` quando a condicao for igual a `jumpWhen`
// (true ou false). Se nao for, o programa segue para a proxima linha.
void CodeGenerator::branch(const AsmOperand &condition, const std::string &target, bool jumpWhen) {
    if (condition.logic == "not") {
        // inte: pula quando o valor for o contrario
        branch(condition.parts[0], target, !jumpWhen);
    } else if (condition.logic == "and") {
        if (jumpWhen == false) {
            // pula se qualquer um dos dois for falso
            branch(condition.parts[0], target, false);
            branch(condition.parts[1], target, false);
        } else {
            // pula so se os dois forem verdadeiros: se o primeiro for falso, passa direto
            const std::string skip = "CURTO" + std::to_string(newLabelNumber());
            branch(condition.parts[0], skip, false);
            branch(condition.parts[1], target, true);
            label(skip);
        }
    } else if (condition.logic == "or") {
        if (jumpWhen == true) {
            // pula se qualquer um dos dois for verdadeiro
            branch(condition.parts[0], target, true);
            branch(condition.parts[1], target, true);
        } else {
            // pula so se os dois forem falsos: se o primeiro for verdadeiro, passa direto
            const std::string skip = "CURTO" + std::to_string(newLabelNumber());
            branch(condition.parts[0], skip, true);
            branch(condition.parts[1], target, false);
            label(skip);
        }
    } else {
        // comparacao simples: o codigo da subtracao e o desvio
        emit(condition.code);
        if (jumpWhen) {
            emit({jumpWhenTrue(condition.relop) + " " + target});
        } else {
            emit({jumpWhenFalse(condition.relop) + " " + target});
        }
    }
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

std::string CodeGenerator::asmName(const SymbolTable &table, std::size_t symbol) const {
    const Symbol &s = table.at(symbol);

    // Se ja existe um simbolo ANTERIOR com o mesmo nome, este recebe o nome do escopo
    // (ex.: idade_global2, x_maior). Olhar so os anteriores deixa o nome sempre igual.
    bool repeated = false;
    for (std::size_t i = 0; i < symbol; i++) {
        if (table.at(i).getId() == s.getId()) repeated = true;
    }

    std::string name = s.getId();
    if (repeated) name += "_" + table.getScope(s.getScope()).name;
    return asciiName(name);
}

std::string CodeGenerator::program(const SymbolTable &table) const {
    std::ostringstream out;

    // .data: variaveis, vetores e parametros (passagem por copia) + temporarios
    out << ".data\n";
    const auto &symbols = table.getSymbols();
    for (std::size_t i = 0; i < symbols.size(); i++) {
        const Symbol &symbol = symbols[i];
        if (symbol.isFunction()) continue;

        out << '\t' << asmName(table, i) << " : 0";
        if (symbol.isVector()) {
            for (int k = 1; k < *symbol.getVectorSize(); k++) out << ", 0";
        }
        out << '\n';
    }
    for (std::map<std::string, int>::const_iterator it = temporaryTotals.begin(); it != temporaryTotals.end(); it++) {
        for (int t = 1; t <= it->second; t++) out << '\t' << it->first << t << " : 0\n";
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
