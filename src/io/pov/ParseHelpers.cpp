#include "common/LegacyBoolean.h"
#include "common/linealAlgebra/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/image/DumpFormat.h"
#include "io/image/GifFormat.h"
#include "io/image/IffFormat.h"
#include "io/image/TargaFormat.h"
#include "io/pov/Parse.h"
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

extern ReservedWord globalReservedWords[];
extern int termCounts[MAX_ORDER + 1];
extern TokenStruct globalToken;
extern double maxTraceLevel;

extern RenderFrame *parsingFramePtr;
extern TokenStruct globalToken;

void
ParseHelpers::getExpectedToken(int tokenId)
{
    Tokenizer::getToken();
    if (globalToken.tokenId != tokenId) {
        ParseErrorReporter::parseError(tokenId);
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

    if (object->Type == COMPOSITE_TYPE) {
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
    Geometry *tempShape;

    if ((shape->Type == CSG_UNION_TYPE) ||
        (shape->Type == CSG_INTERSECTION_TYPE) ||
        (shape->Type == CSG_DIFFERENCE_TYPE)) {
        for (tempShape = ((CSG *)shape)->Shapes; tempShape != nullptr;
            tempShape = tempShape->nextObject) {
            ParseHelpers::postProcessShape(tempShape);
        }
    } else if ((shape->Type == POINT_LIGHT_TYPE) ||
               (shape->Type == SPOT_LIGHT_TYPE)) {
        ParseHelpers::linkShapes((Light *)shape,
            &(((Light *)shape)->Next_Light_Source),
            &(parsingFramePtr->Light_Sources));
    }
}
