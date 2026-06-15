#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/light/Light.h"
#include "environment/scene/SceneFrame.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"



void
ParseHelpers::getExpectedToken(int tokenId)
{
    ParserContext ctx;
    ParseHelpers::getExpectedToken(tokenId, ctx);
}

void
ParseHelpers::getExpectedToken(int tokenId, ParserContext &ctx)
{
    ctx.tokenStream().getToken();
    if (ctx.token().tokenId != tokenId) {
        ParseErrorReporter::parseError(tokenId, ctx);
    }
}

void
ParseHelpers::linkShapes(Light *newObject, Light **field, Light **oldObjectList)
{
    *field = *oldObjectList;
    *oldObjectList = newObject;
}

void
ParseHelpers::postProcessObject(SimpleBody *object)
{
    if (Composite *composite = dynamic_cast<Composite *>(object)) {
        java::ArrayList<SimpleBody*> &simpleBodies = composite->simpleBodies;
        for (long int i = simpleBodies.size() - 1; i >= 0; i--) {
            ParseHelpers::postProcessObject(simpleBodies[i]);
        }
    } else {
        ParseHelpers::postProcessShape(object->geometry);
    }
}

void
ParseHelpers::postProcessShape(Geometry *shape)
{
    ParserContext ctx;
    ParseHelpers::postProcessShape(shape, ctx);
}

void
ParseHelpers::postProcessShape(Geometry *shape, ParserContext &ctx)
{
    Geometry *tempShape;

    if (CSG *csg = dynamic_cast<CSG *>(shape)) {
        java::ArrayList<Geometry*> &shapes = csg->Shapes;
        for (long int i = shapes.size() - 1; i >= 0; i--) {
            tempShape = shapes[i];
            ParseHelpers::postProcessShape(tempShape);
        }
    } else if (Light *light = dynamic_cast<Light *>(shape)) {
        ParseHelpers::linkShapes(light,
            &(light->Next_Light_Source),
            &(ctx.parsingFrame()->Light_Sources));
    }
}
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
