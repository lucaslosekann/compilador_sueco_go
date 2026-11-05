#include "CodeGenerator.h"

#include <map>
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

// Operacoes com instrucao direta no BIP
const std::map<std::string, std::string> BINARY_INSTRUCTIONS = {
    {"plus", "ADD"},    {"minus", "SUB"},    {"bitoch", "AND"},
    {"biteller", "OR"}, {"bitxor", "XOR"},
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

CodeGenerator::CodeGenerator(WarningCallback warn) : warn(std::move(warn)) {}

void CodeGenerator::setEnabled(bool value) { enabled = value; }

bool CodeGenerator::isEnabled() const { return enabled; }

std::string CodeGenerator::newTemporary() {
    temporaryCount++;
    if (temporaryCount > maxTemporaryCount) maxTemporaryCount = temporaryCount;
    return "_temp" + std::to_string(temporaryCount);
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

void CodeGenerator::emit(const std::vector<std::string> &code) { text.insert(text.end(), code.begin(), code.end()); }

bool CodeGenerator::canEmit(std::initializer_list<const AsmOperand *> operands, const std::string &command) {
    if (!enabled) {
        endStatement();
        return false;
    }

    for (const AsmOperand *operand : operands) {
        if (operand->kind == AsmOperand::UNSUPPORTED) {
            warn("Geracao de codigo: " + operand->reason + " (" + command + " ignorado)");
            endStatement();
            return false;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// Expressoes
// ---------------------------------------------------------------------------

AsmOperand CodeGenerator::binary(const AsmOperand &left, const std::string &op, const AsmOperand &right) {
    if (left.kind == AsmOperand::UNSUPPORTED) return left;
    if (right.kind == AsmOperand::UNSUPPORTED) return right;

    // Deslocamentos: no BIP a quantidade e sempre uma constante (SLL/SRL)
    if (op == "vänster" || op == "höger" || op == "v\xE4nster" || op == "h\xF6ger") {
        if (right.kind != AsmOperand::IMMEDIATE) {
            return AsmOperand::unsupported("deslocamento so pode ser feito por uma constante");
        }

        auto code = load(left);
        code.push_back(std::string(op[0] == 'v' ? "SLL " : "SRL ") + std::to_string(right.value));
        return AsmOperand::computed(code);
    }

    const auto found = BINARY_INSTRUCTIONS.find(op);
    if (found == BINARY_INSTRUCTIONS.end()) {
        return AsmOperand::unsupported("operacao '" + op + "' nao e gerada no T4 (so plus, minus e bit a bit)");
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
    default: // lado direito precisa do ACC (ex.: vetor, subexpressao)
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

    return AsmOperand::unsupported("operacao '" + op + "' nao e gerada no T4");
}

// v[i]  ->  (i) / STO $indr / LDV v
AsmOperand CodeGenerator::vectorElement(const std::string &vector, const AsmOperand &index) {
    if (index.kind == AsmOperand::UNSUPPORTED) return index;

    auto code = load(index);
    code.push_back("STO $indr");
    code.push_back("LDV " + vector);
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

// std_läsa(); sozinho: le e descarta
void CodeGenerator::read() {
    if (!enabled) return;
    emit({"LD $in_port"});
}

// x++ / x--  ->  LD x / ADDI 1 / STO x
void CodeGenerator::increment(const std::string &variable, int delta) {
    if (!enabled) return;
    emit({"LD " + variable, (delta > 0 ? "ADDI " : "SUBI ") + std::to_string(delta > 0 ? delta : -delta),
          "STO " + variable});
}

// ---------------------------------------------------------------------------
// Programa
// ---------------------------------------------------------------------------

std::string CodeGenerator::asmName(const SymbolTable &table, std::size_t symbol,
                                   const std::function<std::string(int)> &scopeName) const {
    const Symbol &s = table.at(symbol);

    // Nomes repetidos em escopos diferentes recebem o nome do escopo (ex.: idade_global2)
    int sameName = 0;
    for (const auto &other : table.getSymbols()) {
        if (other.getId() == s.getId() && !other.isFunction() && !other.isParameter()) sameName++;
    }

    std::string name = s.getId();
    if (sameName > 1 && s.getScope() != 0) name += "_" + scopeName(s.getScope());
    return asciiName(name);
}

std::string CodeGenerator::program(const SymbolTable &table, const std::function<std::string(int)> &scopeName) const {
    std::ostringstream out;

    out << ".data\n";
    const auto &symbols = table.getSymbols();
    for (std::size_t i = 0; i < symbols.size(); i++) {
        const Symbol &symbol = symbols[i];
        if (symbol.isFunction() || symbol.isParameter()) continue; // sub-rotinas: T6

        out << '\t' << asmName(table, i, scopeName) << " : 0";
        if (symbol.isVector()) {
            for (int k = 1; k < *symbol.getVectorSize(); k++) out << ", 0";
        }
        out << '\n';
    }
    for (int t = 1; t <= maxTemporaryCount; t++) out << "\t_temp" << t << " : 0\n";

    // Formato pedido pelo professor: diretivas e rotulos na margem,
    // mnemonico a um tab da margem e operando a um tab do mnemonico (tres colunas)
    out << ".text\n";
    for (const auto &line : text) {
        const auto space = line.find(' ');
        out << '\t' << line.substr(0, space) << '\t' << line.substr(space + 1) << '\n';
    }
    out << "\tHLT\t0\n";

    return out.str();
}
