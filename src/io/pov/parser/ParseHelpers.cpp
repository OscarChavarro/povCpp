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
    SimpleBody *temp;

    if (object->geometryType == GeometryTypes::COMPOSITE_TYPE) {
        java::ArrayList<SimpleBody*> &simpleBodies = ((Composite *)object)->simpleBodies;
        for (long int i = simpleBodies.size() - 1; i >= 0; i--) {
            temp = simpleBodies[i];
            ParseHelpers::postProcessObject(temp);
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

    if ((shape->geometryType == GeometryTypes::CSG_UNION_TYPE) ||
        (shape->geometryType == GeometryTypes::CSG_INTERSECTION_TYPE) ||
        (shape->geometryType == GeometryTypes::CSG_DIFFERENCE_TYPE)) {
        java::ArrayList<Geometry*> &shapes = ((CSG *)shape)->Shapes;
        for (long int i = shapes.size() - 1; i >= 0; i--) {
            tempShape = shapes[i];
            ParseHelpers::postProcessShape(tempShape);
        }
    } else if ((shape->geometryType == GeometryTypes::POINT_LIGHT_TYPE) ||
               (shape->geometryType == GeometryTypes::SPOT_LIGHT_TYPE)) {
        ParseHelpers::linkShapes((Light *)shape,
            &(((Light *)shape)->Next_Light_Source),
            &(ctx.parsingFrame()->Light_Sources));
    }
}
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
