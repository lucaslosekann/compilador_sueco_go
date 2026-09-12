#ifndef SEMANTIC_ERROR_H
#define SEMANTIC_ERROR_H

#include "AnalysisError.h"

#include <string>

namespace GalsGenerated {

class SemanticError : public AnalysisError
{
public:

    SemanticError(const std::string &msg, int position = -1)
      : AnalysisError(msg, position) { }
};

} //namespace GalsGenerated

#endif
