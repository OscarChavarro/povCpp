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

    if (object->type == GeometryTypes::COMPOSITE_TYPE) {
        for (temp = ((Composite *)object)->simpleBodies; temp != nullptr;
            temp = temp->nextObject) {
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
        for (tempShape = ((CSG *)shape)->Shapes; tempShape != nullptr;
            tempShape = tempShape->nextObject) {
            ParseHelpers::postProcessShape(tempShape);
        }
    } else if ((shape->geometryType == GeometryTypes::POINT_LIGHT_TYPE) ||
               (shape->geometryType == GeometryTypes::SPOT_LIGHT_TYPE)) {
        ParseHelpers::linkShapes((Light *)shape,
            &(((Light *)shape)->Next_Light_Source),
            &(ctx.parsingFrame()->Light_Sources));
    }
}
