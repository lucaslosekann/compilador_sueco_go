#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "SemanticError.h"
#include "Token.h"

namespace GalsGenerated {

class Semantico {
  public:
    void executeAction(int action, const Token *token);
};

} // namespace GalsGenerated

#endif
