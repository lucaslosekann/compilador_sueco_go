#ifndef LEXICAL_ERROR_H
#define LEXICAL_ERROR_H

#include "AnalysisError.h"

#include <string>

namespace GalsGenerated {

class LexicalError : public AnalysisError
{
public:

    LexicalError(const std::string &msg, int position = -1)
      : AnalysisError(msg, position) { }
};

} //namespace GalsGenerated

#endif
