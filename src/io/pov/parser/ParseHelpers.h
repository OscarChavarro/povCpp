#ifndef __PARSE_HELPERS__
#define __PARSE_HELPERS__

#include "environment/light/Light.h"
#include "environment/geometry/BoundedGeometry.h"
#include "environment/geometry/Geometry.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;

class ParseHelpers {
  public:
    static void linkShapes(
        Light *newObject, Light **field, Light **oldObjectList);
    static void postProcessObject(BoundedGeometry *object, Light *&lightHead);
    static void postProcessShape(SimpleBody *shape, Light *&lightHead);
    static void getExpectedToken(int tokenId);
    static void getExpectedToken(int tokenId, ParserContext &ctx);
};

#endif
