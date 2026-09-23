#include "Lexico.h"
#include "Semantico.h"
#include "Sintatico.h"
#include "Symbol.h"
#include "SymbolModality.h"
#include "SymbolType.h"
#include "file_handler.h"
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " " << "<caminho para o arquivo a ser compilado>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string fileContents;
    int file_status = read_file_contents(argv[1], &fileContents);
    if (file_status != EXIT_SUCCESS) return file_status;

    GalsGenerated::Lexico lex;
    GalsGenerated::Sintatico sint;
    GalsGenerated::Semantico sem;

    lex.setInput(fileContents.c_str());
    try {
        sint.parse(&lex, &sem);

        for (const auto &symbol : sem.getSymbolTable().getSymbols()) {
            std::cout << "Id: " << symbol.getId() << " | Tipo: " << symbolTypeToString(symbol.getType())
                      << " | Modalidade: " << symbolModalityToString(symbol.getModality())
                      << " | Escopo: " << symbol.getScope();

            if (symbol.getVectorSize().has_value()) {
                std::cout << " | Tamanho do vetor: " << *symbol.getVectorSize();
            }

            std::cout << '\n';
        }
        std::cout << "Compilado com sucesso!" << std::endl;
    } catch (const GalsGenerated::LexicalError &err) {
        std::cerr << "Problema lexico: " << err.getMessage() << std::endl;

        const int position = err.getPosition();

        std::cerr << "Posição: " << position << " | Caractere: ";

        if (position >= 0 && static_cast<std::size_t>(position) < fileContents.size()) {
            std::cerr << fileContents[position];
        } else {
            std::cerr << "N/A";
        }

        std::cerr << '\n';
        return EXIT_FAILURE;

    } catch (const GalsGenerated::SyntacticError &err) {
        std::cerr << "Problema sintatico: " << err.getMessage() << std::endl;
        return EXIT_FAILURE;
    } catch (const GalsGenerated::SemanticError &err) {
        std::cerr << "Problema semantico: " << err.getMessage() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
