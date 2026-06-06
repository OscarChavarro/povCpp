#include "io/pov/ParserContext.h"
#include "common/LegacyBoolean.h"
#include "common/linealAlgebra/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/base/image/DumpFormat.h"
#include "io/base/image/GifFormat.h"
#include "io/base/image/IffFormat.h"
#include "io/base/image/TargaFormat.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/ParseHelpers.h"
#include "environment/scene/SceneFrame.h"

#include "environment/camera/Camera.h"
#include "environment/geometry/elements/Triangle.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/surface/parametric/ParametricPatch.h"
#include "environment/geometry/volume/Blob.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/HeightField.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/light/Light.h"



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

    if (object->Type == GeometryOperations::COMPOSITE_TYPE) {
        for (temp = ((Composite *)object)->Objects; temp != nullptr;
            temp = temp->nextObject) {
            ParseHelpers::postProcessObject(temp);
        }
    } else {
        ParseHelpers::postProcessShape(object->Shape);
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

    if ((shape->Type == GeometryOperations::CSG_UNION_TYPE) ||
        (shape->Type == GeometryOperations::CSG_INTERSECTION_TYPE) ||
        (shape->Type == GeometryOperations::CSG_DIFFERENCE_TYPE)) {
        for (tempShape = ((CSG *)shape)->Shapes; tempShape != nullptr;
            tempShape = tempShape->nextObject) {
            ParseHelpers::postProcessShape(tempShape);
        }
    } else if ((shape->Type == GeometryOperations::POINT_LIGHT_TYPE) ||
               (shape->Type == GeometryOperations::SPOT_LIGHT_TYPE)) {
        ParseHelpers::linkShapes((Light *)shape,
            &(((Light *)shape)->Next_Light_Source),
            &(ctx.parsingFrame()->Light_Sources));
    }
}
