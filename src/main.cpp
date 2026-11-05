#include "Lexico.h"
#include "Semantico.h"
#include "Sintatico.h"
#include "Symbol.h"
#include "SymbolModality.h"
#include "SymbolType.h"
#include "file_handler.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

// O codigo fonte e lido em Latin-1 (a IDE salva com toLatin1), mas o terminal/IDE exibem UTF-8.
// Converte textos que contem lexemas (ex.: 'gånger', 'std_läsa') para UTF-8 antes de imprimir.
std::string toUtf8(const std::string &latin1) {
    std::string out;
    for (unsigned char c : latin1) {
        if (c < 0x80) {
            out += static_cast<char>(c);
        } else {
            out += static_cast<char>(0xC0 | (c >> 6));
            out += static_cast<char>(0x80 | (c & 0x3F));
        }
    }
    return out;
}

// Converte a posicao (indice no arquivo) em "linha L, coluna C"
std::string lineColumn(const std::string &source, int position) {
    if (position < 0) return "posicao desconhecida";

    int line = 1, column = 1;
    for (int i = 0; i < position && static_cast<std::size_t>(i) < source.size(); i++) {
        if (source[i] == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
    }

    return "linha " + std::to_string(line) + ", coluna " + std::to_string(column);
}

const char *boolText(bool value) { return value ? "V" : "F"; }

std::vector<std::string> symbolRow(const GalsGenerated::Semantico &sem, const Symbol &symbol) {
    std::string type = symbolTypeToString(symbol.getType());
    if (symbol.isVector()) type += "[" + std::to_string(*symbol.getVectorSize()) + "]";

    return {symbol.getId(),
            type,
            symbolModalityToString(symbol.getModality()),
            boolText(symbol.isInitialized()),
            boolText(symbol.isUsed()),
            sem.scopeName(symbol.getScope()),
            boolText(symbol.isParameter()),
            std::to_string(symbol.getParameterPosition()),
            boolText(symbol.isVector()),
            boolText(false), // matriz: a linguagem nao tem matrizes
            boolText(false), // ref: passagem por referencia nao suportada
            boolText(symbol.isFunction())};
}

const std::vector<std::string> HEADER = {"id",     "tipo",  "modalidade", "ini", "usada",  "escopo",
                                         "param",  "pos",   "vet",        "matriz", "ref", "func"};

// Tabela legivel no terminal
void printTable(const GalsGenerated::Semantico &sem) {
    std::vector<std::vector<std::string>> rows = {HEADER};
    for (const auto &symbol : sem.getSymbolTable().getSymbols()) rows.push_back(symbolRow(sem, symbol));
    for (auto &row : rows)
        for (auto &field : row) field = toUtf8(field);

    std::vector<std::size_t> widths(HEADER.size(), 0);
    for (const auto &row : rows)
        for (std::size_t c = 0; c < row.size(); c++) widths[c] = std::max(widths[c], row[c].size());

    std::cout << "\nTabela de simbolos:\n";
    for (const auto &row : rows) {
        for (std::size_t c = 0; c < row.size(); c++) std::cout << std::left << std::setw(widths[c] + 2) << row[c];
        std::cout << '\n';
    }
}

// Formato lido pela IDE: uma linha por simbolo, campos separados por '|'
void printTableForIde(const GalsGenerated::Semantico &sem) {
    for (const auto &symbol : sem.getSymbolTable().getSymbols()) {
        std::cout << "@TS";
        for (const auto &field : symbolRow(sem, symbol)) std::cout << '|' << toUtf8(field);
        std::cout << '\n';
    }
}

void printWarnings(const GalsGenerated::Semantico &sem, const std::string &source) {
    for (const auto &warning : sem.getWarnings()) {
        std::cout << "Aviso (" << lineColumn(source, warning.position) << "): " << toUtf8(warning.message) << '\n';
    }
}

// Codigo BIP: no terminal imprime e salva <arquivo>.asm; na IDE envia linhas "@ASM|..."
void printProgram(const GalsGenerated::Semantico &sem, const std::string &sourcePath, bool ideMode) {
    const std::string program = sem.generateProgram();

    if (ideMode) {
        std::istringstream lines(program);
        std::string line;
        while (std::getline(lines, line)) std::cout << "@ASM|" << line << '\n';
        return;
    }

    std::string asmPath = sourcePath;
    const auto dot = asmPath.find_last_of('.');
    const auto slash = asmPath.find_last_of("/\\");
    if (dot != std::string::npos && (slash == std::string::npos || dot > slash)) asmPath.erase(dot);
    asmPath += ".asm";

    std::ofstream file(asmPath);
    file << program;

    std::cout << "\nCodigo BIP (salvo em " << asmPath << "):\n" << program;
}

} // namespace

int main(int argc, char *argv[]) {
    bool ideMode = false;
    std::string path;

    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if (arg == "--ide") {
            ideMode = true;
        } else {
            path = arg;
        }
    }

    if (path.empty()) {
        std::cerr << "Uso: " << argv[0] << " [--ide] <caminho para o arquivo a ser compilado>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string fileContents;
    int file_status = read_file_contents(path, &fileContents);
    if (file_status != EXIT_SUCCESS) return file_status;

    GalsGenerated::Lexico lex;
    GalsGenerated::Sintatico sint;
    GalsGenerated::Semantico sem;

    auto printSemanticResult = [&]() {
        printWarnings(sem, fileContents);
        if (ideMode) {
            printTableForIde(sem);
        } else {
            printTable(sem);
        }
    };

    lex.setInput(fileContents.c_str());
    try {
        sint.parse(&lex, &sem);
        sem.finish();

        printSemanticResult();
        printProgram(sem, path, ideMode);
        std::cout << "\nCompilado com sucesso!" << std::endl;
    } catch (const GalsGenerated::LexicalError &err) {
        const int position = err.getPosition();
        std::cout << "Erro lexico (" << lineColumn(fileContents, position) << "): " << toUtf8(err.getMessage());

        if (position >= 0 && static_cast<std::size_t>(position) < fileContents.size()) {
            std::cout << " | Caractere: " << toUtf8(std::string(1, fileContents[position]));
        }

        std::cout << std::endl;
        return EXIT_FAILURE;
    } catch (const GalsGenerated::SyntacticError &err) {
        std::cout << "Erro sintatico (" << lineColumn(fileContents, err.getPosition()) << "): " << toUtf8(err.getMessage())
                  << std::endl;
        return EXIT_FAILURE;
    } catch (const GalsGenerated::SemanticError &err) {
        printSemanticResult();
        std::cout << "\nErro semantico (" << lineColumn(fileContents, err.getPosition()) << "): " << toUtf8(err.getMessage())
                  << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
