#include "Lexico.h"
#include "Semantico.h"
#include "Sintatico.h"
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
        std::cout << "Compilado com sucesso!" << std::endl;
    } catch (GalsGenerated::LexicalError err) {
        std::cerr << "Problema lexico: " << err.getMessage() << std::endl;

        std::cerr << "Posição: " << err.getPosition() << " | Caractere: " << fileContents[err.getPosition()] << std::endl;
        return EXIT_FAILURE;
    } catch (GalsGenerated::SyntacticError err) {
        std::cerr << "Problema sintatico: " << err.getMessage() << std::endl;
        return EXIT_FAILURE;
    } catch (GalsGenerated::SemanticError err) {
        std::cerr << "Problema semantico: " << err.getMessage() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
