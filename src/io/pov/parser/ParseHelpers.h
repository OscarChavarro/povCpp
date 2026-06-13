#ifndef __PARSE_HELPERS_H__
#define __PARSE_HELPERS_H__

#include "environment/light/Light.h"
#include "environment/geometry/SimpleBody.h"
#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class ParseHelpers {
  public:
    static void linkShapes(
        Light *newObject, Light **field, Light **oldObjectList);
    static void postProcessObject(SimpleBody *object);
    static void postProcessShape(Geometry *shape);
    static void getExpectedToken(int tokenId);
    static void getExpectedToken(int tokenId, ParserContext &ctx);
    static void postProcessShape(Geometry *shape, ParserContext &ctx);
};

#endif
