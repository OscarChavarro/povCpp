#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "io/pov/parser/LightGeometryAdapter.h"
#include "environment/scene/SceneFrame.h"
#include "environment/geometry/SimpleBody.h"

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
    if (ctx.token().getTokenId() != tokenId) {
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
ParseHelpers::postProcessObject(BoundedGeometry *object, Light *&lightHead)
{
    if (Composite *composite = dynamic_cast<Composite *>(object)) {
        java::ArrayList<BoundedGeometry*> &simpleBodies = composite->getSimpleBodies();
        for (long int i = simpleBodies.size() - 1; i >= 0; i--) {
            ParseHelpers::postProcessObject(simpleBodies[i], lightHead);
        }
    } else {
        ParseHelpers::postProcessShape(
            static_cast<SimpleBody*>(object->getGeometry()), lightHead);
    }
}

void
ParseHelpers::postProcessShape(SimpleBody *shape, Light *&lightHead)
{
    SimpleBody *tempShape;

    if (CSG *csg = dynamic_cast<CSG *>(shape->getGeometry())) {
        java::ArrayList<TransformableElement*> &shapes = csg->getShapes();
        for (long int i = shapes.size() - 1; i >= 0; i--) {
            tempShape = static_cast<SimpleBody*>(shapes[i]);
            ParseHelpers::postProcessShape(tempShape, lightHead);
        }
    } else if (LightGeometryAdapter *lightAdapter =
                   dynamic_cast<LightGeometryAdapter *>(shape->getGeometry())) {
        lightAdapter->getLight()->setNextLightSource(lightHead);
        lightHead = lightAdapter->getLight();
    }
}
