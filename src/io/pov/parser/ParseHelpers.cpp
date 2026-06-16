#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/light/Light.h"
#include "environment/scene/SceneFrame.h"
#include "environment/scene/TranslatedBody.h"
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
        ParseHelpers::postProcessShape(static_cast<TranslatedBody*>(object->geometry));
    }
}

void
ParseHelpers::postProcessShape(TranslatedBody *shape)
{
    ParserContext ctx;
    ParseHelpers::postProcessShape(shape, ctx);
}

void
ParseHelpers::postProcessShape(TranslatedBody *shape, ParserContext &ctx)
{
    TranslatedBody *tempShape;

    if (CSG *csg = dynamic_cast<CSG *>(shape->geometry)) {
        java::ArrayList<TransformableElement*> &shapes = csg->shapes;
        for (long int i = shapes.size() - 1; i >= 0; i--) {
            tempShape = static_cast<TranslatedBody*>(shapes[i]);
            ParseHelpers::postProcessShape(tempShape);
        }
    } else if (Light *light = dynamic_cast<Light *>(shape->geometry)) {
        ParseHelpers::linkShapes(light,
            &(light->nextLightSource),
            &(ctx.parsingFrame()->lightSources));
    }
}
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"
