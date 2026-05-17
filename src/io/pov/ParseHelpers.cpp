#include "io/pov/Parse.h"
#include "common/FrameConfig.h"
#include "common/Transformation.h"
#include "app/PovApp.h"
#include "common/Vector3Dd.h"
#include "io/GifFormat.h"
#include "io/IffFormat.h"
#include "io/TargaFormat.h"
#include "io/DumpFormat.h"
#include "render/RenderEngine.h"

#include "geom/Bezier.h"
#include "geom/Blob.h"
#include "geom/Box.h"
#include "geom/CSG.h"
#include "geom/HeightField.h"
#include "geom/Light.h"
#include "geom/Composite.h"
#include "geom/InfinitePlane.h"
#include "geom/PolynomialShape.h"
#include "geom/Quadric.h"
#include "geom/Sphere.h"
#include "geom/Triangle.h"
#include "geom/Viewpoint.h"

extern ReservedWord globalReservedWords[];
extern double antialiasThreshold;
extern int termCounts[MAX_ORDER + 1];
extern TokenStruct globalToken;
extern double maxTraceLevel;
extern char verboseFormat;
extern unsigned int Options;
extern char statFileName[FILE_NAME_LENGTH];

extern RenderFrame *parsingFramePtr;
extern TokenStruct globalToken;

void
ParseHelpers::getExpectedToken(int tokenId)
{
    Tokenizer::getToken();
    if (globalToken.Token_Id != tokenId) {
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
             temp = temp->Next_Object) {
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
             tempShape = tempShape->Next_Object) {
            ParseHelpers::postProcessShape(tempShape);
        }
    } else if ((shape->Type == POINT_LIGHT_TYPE) ||
               (shape->Type == SPOT_LIGHT_TYPE)) {
        ParseHelpers::linkShapes((Light *)shape, &(((Light *)shape)->Next_Light_Source),
            &(parsingFramePtr->Light_Sources));
    }
}
