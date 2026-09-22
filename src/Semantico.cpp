#include "Semantico.h"
#include "Constants.h"

#include <iostream>

namespace GalsGenerated {

void Semantico::executeAction(int action, const Token *token) {
    std::cout << "Ação: " << action << ", Token: " << token->getId() << ", Lexema: " << token->getLexeme() << std::endl;
}

} // namespace GalsGenerated
