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
ParseHelpers::postProcessObject(BoundedGeometry *object)
{
    if (Composite *composite = dynamic_cast<Composite *>(object)) {
        java::ArrayList<BoundedGeometry*> &simpleBodies = composite->simpleBodies;
        for (long int i = simpleBodies.size() - 1; i >= 0; i--) {
            ParseHelpers::postProcessObject(simpleBodies[i]);
        }
    } else {
        ParseHelpers::postProcessShape(static_cast<SimpleBody*>(object->getGeometry()));
    }
}

void
ParseHelpers::postProcessShape(SimpleBody *shape)
{
    ParserContext ctx;
    ParseHelpers::postProcessShape(shape, ctx);
}

void
ParseHelpers::postProcessShape(SimpleBody *shape, ParserContext &ctx)
{
    SimpleBody *tempShape;
    (void)ctx;

    if (CSG *csg = dynamic_cast<CSG *>(shape->getGeometry())) {
        java::ArrayList<TransformableElement*> &shapes = csg->getShapes();
        for (long int i = shapes.size() - 1; i >= 0; i--) {
            tempShape = static_cast<SimpleBody*>(shapes[i]);
            ParseHelpers::postProcessShape(tempShape);
        }
    } else if (LightGeometryAdapter *lightAdapter =
                   dynamic_cast<LightGeometryAdapter *>(shape->getGeometry())) {
        Light * const currentHead = ctx.parsingFrame()->getLightSources();
        lightAdapter->getLight()->setNextLightSource(currentHead);
        ctx.parsingFrame()->setLightSources(lightAdapter->getLight());
    }
}
