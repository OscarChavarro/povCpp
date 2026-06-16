#ifndef __SPHERE_PARSER_H__
#define __SPHERE_PARSER_H__

#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class TranslatedBody;

class SphereParser {
  public:
    static TranslatedBody *parseSphere();
    static TranslatedBody *parseSphere(ParserContext &ctx);
};

#endif
