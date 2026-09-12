#include "file_handler.h"

int read_file_contents(const std::string file_path, std::string *contents) {
    std::ifstream file(file_path);

    if (!file.is_open()) {
        std::cerr << "Erro ao abrir arquivo" << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    *contents = buffer.str();
    return 0;
}