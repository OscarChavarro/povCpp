#include "Parse.h"
#include "common/Frame.h"
#include "common/Matrices.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "io/Gif.h"
#include "io/Iff.h"
#include "io/Targa.h"
#include "io/Dump.h"
#include "render/Render.h"

#include "geom/Bezier.h"
#include "geom/Blob.h"
#include "geom/Boxes.h"
#include "geom/Csg.h"
#include "geom/HField.h"
#include "geom/Light.h"
#include "geom/Objects.h"
#include "geom/Planes.h"
#include "geom/Poly.h"
#include "geom/Quadrics.h"
#include "geom/Spheres.h"
#include "geom/Triangle.h"
#include "geom/ViewPnt.h"

extern ReservedWord globalReservedWords[];
extern double antialiasThreshold;
extern int termCounts[MAX_ORDER + 1];
extern TokenStruct globalToken;
extern double maxTraceLevel;
extern char verboseFormat;
extern unsigned int Options;
extern char statFileName[FILE_NAME_LENGTH];

extern RenderFrame *parsingFramePtr;
extern Constant constants[MAX_CONSTANTS];
extern int numberOfConstants;
extern int degenerateTriangles;


void
SceneParser::Parse(RenderFrame *framePtr)
{
    SimpleBody *object;
    parsingFramePtr = framePtr;

    degenerateTriangles = FALSE;
    SceneParser::tokenInit();
    SceneParser::frameInit();
    SceneParser::parseFrame();
    for (object = parsingFramePtr->Objects; object != nullptr;
         object = object->Next_Object) {
        ParseHelpers::postProcessObject(object);
    }
    if (degenerateTriangles) {
        fprintf(
            stderr, "Degenerate triangles were found and are being ignored.\n");
        /* exit(1); Let's ignore degen tri instead of blowing up. CdW */
    }
}

void
SceneParser::tokenInit()
{
    numberOfConstants = 0;
    /*
       Constants = new Constant[MAX_CONSTANTS];
    */
}

/* Set up the fields in the frame to default values. */
void
SceneParser::frameInit()
{
    Default_Texture = TextureUtils::getTexture();
    parsingFramePtr->View_Point.initializeDefaults();
    parsingFramePtr->Light_Sources = nullptr;
    parsingFramePtr->Objects = nullptr;
    parsingFramePtr->Atmosphere_IOR = 1.0;
    parsingFramePtr->Antialias_Threshold = antialiasThreshold;
    parsingFramePtr->Fog_Distance = 0.0;
    Color::makeColor(&(parsingFramePtr->Fog_Colour), 0.0, 0.0, 0.0);
}

void
SceneParser::parseFrame()
{
    SimpleBody *localObject;

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
    case FOG_TOKEN:
    SceneConfigParser::parseFog();
    break;

    case DEFAULT_TOKEN:
    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
    case TEXTURE_TOKEN:
    Default_Texture->Constant_Flag = FALSE;
    Default_Texture = TextureParser::parseTexture();
    Default_Texture->Constant_Flag = TRUE;
    break;
    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        default: ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }
    break;

    case MAX_TRACE_LEVEL_TOKEN:
    maxTraceLevel = PrimitiveParser::parseFloat();
    break;

    case OBJECT_TOKEN:
    localObject = ObjectParser::parseObject();
    ObjectUtils::link(localObject, &(localObject->Next_Object), &(parsingFramePtr->Objects));

    /* light sources are now linked in object parsing */
    /* if (Local_Object -> Light_Source_Flag)
    ObjectUtils::link(Local_Object, &(Local_Object -> Next_Light_Source),
            &(Parsing_Frame_Ptr -> Light_Sources)); */
    break;

    case COMPOSITE_TOKEN:
    localObject = ObjectParser::parseComposite();
    ObjectUtils::link(localObject, &(localObject->Next_Object), &(parsingFramePtr->Objects));
    /*        addCompositeLightSource ((Composite *)Local_Object);*/
    break;

    case VIEW_POINT_TOKEN:
    SceneConfigParser::parseViewpoint(&(parsingFramePtr->View_Point));
    break;

    case DECLARE_TOKEN:
    SceneConfigParser::parseDeclare();
    break;

    case END_OF_FILE_TOKEN:
    Exit_Flag = TRUE; break;

        default: ParseErrorReporter::parseError(OBJECT_TOKEN);
    break;
    }
        }
    }
}
