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

CONSTANT
SceneConfigParser::findConstant()
{
    register int i;

    for (i = 1; i <= numberOfConstants; i++) {
        if (constants[i].Identifier_Number == globalToken.Identifier_Number) {
            return (i);
        }
    }

    return (-1);
}

void
SceneConfigParser::parseFog()
{
    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
    case COLOUR_TOKEN:
    PrimitiveParser::parseColour(&parsingFramePtr->Fog_Colour);
    break;

    case FLOAT_TOKEN:
    parsingFramePtr->Fog_Distance = globalToken.Token_Float;
    break;

    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        default: ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }
}

void
SceneConfigParser::parseViewpoint(Viewpoint *givenVp)
{
    CONSTANT constantId;
    Vector3D localVector;
    Vector3D tempVector;
    double directionLength, upLength, rightLength, handedness;

    givenVp->initializeDefaults();

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
    case IDENTIFIER_TOKEN:
    if ((constantId = SceneConfigParser::findConstant()) != -1) {
        if (constants[(int)constantId].Constant_Type == VIEW_POINT_CONSTANT) {
            *givenVp = *((Viewpoint *)constants[(int)constantId].Constant_Data);
        } else {
            ParseErrorReporter::typeError();
        }
    } else {
        ParseErrorReporter::Undeclared();
    }
    break;

    case LOCATION_TOKEN:
    PrimitiveParser::parseVector(&(givenVp->Location));
    break;

    case DIRECTION_TOKEN:
    PrimitiveParser::parseVector(&(givenVp->Direction));
    break;

    case UP_TOKEN:
    PrimitiveParser::parseVector(&(givenVp->Up));
    break;

    case RIGHT_TOKEN:
    PrimitiveParser::parseVector(&(givenVp->Right));
    break;

    case SKY_TOKEN:
    PrimitiveParser::parseVector(&(givenVp->Sky));
    break;

    case LOOK_AT_TOKEN:
    VectorOps::vLength(directionLength, givenVp->Direction);
    VectorOps::vLength(upLength, givenVp->Up);
    VectorOps::vLength(rightLength, givenVp->Right);
    VectorOps::vCross(tempVector, givenVp->Direction, givenVp->Up);
    VectorOps::vDot(handedness, tempVector, givenVp->Right);
    PrimitiveParser::parseVector(&givenVp->Direction);

    VectorOps::vSub(givenVp->Direction, givenVp->Direction, givenVp->Location);
    VectorOps::vNormalize(givenVp->Direction, givenVp->Direction);
    VectorOps::vCross(givenVp->Right, givenVp->Direction, givenVp->Sky);
    VectorOps::vNormalize(givenVp->Right, givenVp->Right);
    VectorOps::vCross(givenVp->Up, givenVp->Right, givenVp->Direction);
    VectorOps::vScale(givenVp->Direction, givenVp->Direction, directionLength);
    if (handedness >= 0.0) {
        VectorOps::vScale(givenVp->Right, givenVp->Right, rightLength);
    } else {
        VectorOps::vScale(givenVp->Right, givenVp->Right, -rightLength);
    }

    VectorOps::vScale(givenVp->Up, givenVp->Up, upLength);
    break;

    case TRANSLATE_TOKEN:
    PrimitiveParser::parseVector(&localVector);
    GeometryOps::translate((SimpleBody *)givenVp, &localVector);
    break;

    case ROTATE_TOKEN:
    PrimitiveParser::parseVector(&localVector);
    GeometryOps::rotate((SimpleBody *)givenVp, &localVector);
    break;

    case SCALE_TOKEN:
    PrimitiveParser::parseVector(&localVector);
    GeometryOps::scale((SimpleBody *)givenVp, &localVector);
    break;

    case RIGHT_CURLY_TOKEN:
    Exit_Flag = TRUE; break;

        default: ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN);
    break;
    }
        }
    }
}

void
SceneConfigParser::parseDeclare()
{
    CONSTANT constantId;
    Texture *localTexture;
    Texture *tempTexture;
    Constant *constantPtr;

    ParseHelpers::getExpectedToken(IDENTIFIER_TOKEN);
    if ((constantId = SceneConfigParser::findConstant()) == -1) {
        if (++numberOfConstants >= MAX_CONSTANTS) {
            ParseErrorReporter::Error("Too many constants \"declared\"");
        } else {
            constantId = numberOfConstants;
        }
    }

    constantPtr = &(constants[(int)constantId]);
    ParseHelpers::getExpectedToken(EQUALS_TOKEN);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
    case OBJECT_TOKEN:
    constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)ObjectParser::parseObject();
    constantPtr->Constant_Type = OBJECT_CONSTANT;
    Exit_Flag = TRUE; break;

        case SPHERE_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)ShapeParser::parseSphere();
    constantPtr->Constant_Type = SPHERE_CONSTANT;
    Exit_Flag = TRUE; break;

        case PLANE_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)ShapeParser::parsePlane();
    constantPtr->Constant_Type = PLANE_CONSTANT;
    Exit_Flag = TRUE; break;

        case TRIANGLE_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)ShapeParser::parseTriangle();
    constantPtr->Constant_Type = TRIANGLE_CONSTANT;
    Exit_Flag = TRUE; break;

        case SMOOTH_TRIANGLE_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)ShapeParser::parseSmoothTriangle();
    constantPtr->Constant_Type = SMOOTH_TRIANGLE_CONSTANT;
    Exit_Flag = TRUE; break;

        case QUADRIC_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)ShapeParser::parseQuadric();
    constantPtr->Constant_Type = QUADRIC_CONSTANT;
    Exit_Flag = TRUE; break;

        case CUBIC_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)ShapeParser::parsePoly(3);
    constantPtr->Constant_Type = POLY_CONSTANT;
    Exit_Flag = TRUE; break;

        case QUARTIC_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)ShapeParser::parsePoly(4);
    constantPtr->Constant_Type = POLY_CONSTANT;
    Exit_Flag = TRUE; break;

        case HEIGHT_FIELD_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)ShapeParser::parseHeightField();
    constantPtr->Constant_Type = HEIGHT_FIELD_CONSTANT;
    Exit_Flag = TRUE; break;

        case POLY_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)ShapeParser::parsePoly(0);
    constantPtr->Constant_Type = POLY_CONSTANT;
    Exit_Flag = TRUE; break;

        case BOX_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)ShapeParser::parseBox();
    constantPtr->Constant_Type = BOX_CONSTANT;
    Exit_Flag = TRUE; break;

        case BLOB_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)ShapeParser::parseBlob();
    constantPtr->Constant_Type = BLOB_CONSTANT;
    Exit_Flag = TRUE; break;

        case BICUBIC_PATCH_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)ShapeParser::parseBicubicPatch();
    constantPtr->Constant_Type = BICUBIC_PATCH_CONSTANT;
    Exit_Flag = TRUE; break;

        case INTERSECTION_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data =
        (char *)ObjectParser::parseCsg(CSG_INTERSECTION_TYPE, nullptr);
    constantPtr->Constant_Type = CSG_INTERSECTION_CONSTANT;
    Exit_Flag = TRUE; break;

        case UNION_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)ObjectParser::parseCsg(CSG_UNION_TYPE, nullptr);
    constantPtr->Constant_Type = CSG_UNION_CONSTANT;
    Exit_Flag = TRUE; break;

        case DIFFERENCE_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data =
        (char *)ObjectParser::parseCsg(CSG_DIFFERENCE_TYPE, nullptr);
    constantPtr->Constant_Type = CSG_DIFFERENCE_CONSTANT;
    Exit_Flag = TRUE; break;

        case COMPOSITE_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)ObjectParser::parseComposite();
    constantPtr->Constant_Type = COMPOSITE_CONSTANT;
    Exit_Flag = TRUE; break;

        case TEXTURE_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    localTexture = nullptr;
    constantPtr->Constant_Data = (char *)localTexture;
    constantPtr->Constant_Type = TEXTURE_CONSTANT;
    Tokenizer::ungetToken();
    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (globalToken.Token_Id) {
    case TEXTURE_TOKEN:
    localTexture = Default_Texture;
    localTexture = TextureParser::parseTexture();
    if (localTexture->Constant_Flag) {
        localTexture = TextureParser::copyTexture(localTexture);
    }

    localTexture->Constant_Flag = TRUE;

    {
        for (tempTexture = localTexture; tempTexture->Next_Texture != nullptr;
             tempTexture = tempTexture->Next_Texture) {
        }

        tempTexture->Next_Texture = (Texture *)constantPtr->Constant_Data;
        constantPtr->Constant_Data = (char *)localTexture;
    }
    break;

    default:
    Tokenizer::ungetToken();
    Exit_Flag = TRUE; break; }
        }
    } Exit_Flag = TRUE; break;

        case VIEW_POINT_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)SceneFactory::getViewpoint();
    constantPtr->Constant_Type = VIEW_POINT_CONSTANT;
    SceneConfigParser::parseViewpoint((Viewpoint *)constantPtr->Constant_Data);
    Exit_Flag = TRUE; break;

        case COLOUR_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)SceneFactory::getColour();
    constantPtr->Constant_Type = COLOUR_CONSTANT;
    PrimitiveParser::parseColour((RGBAColor *)constantPtr->Constant_Data);
    Exit_Flag = TRUE; break;

        case LIGHT_SOURCE_TOKEN:
            constantPtr->Identifier_Number = globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)ShapeParser::parseLightSource();
    constantPtr->Constant_Type = LIGHT_SOURCE_CONSTANT;
    Exit_Flag = TRUE; break;

        case LEFT_ANGLE_TOKEN: Tokenizer::ungetToken(); constantPtr->Identifier_Number =
        globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)SceneFactory::getVector();
    constantPtr->Constant_Type = VECTOR_CONSTANT;
    PrimitiveParser::parseVector((Vector3D *)constantPtr->Constant_Data);
    Exit_Flag = TRUE; break;

        case DASH_TOKEN:
    case PLUS_TOKEN:
    case FLOAT_TOKEN:
            Tokenizer::ungetToken(); constantPtr->Identifier_Number =
        globalToken.Identifier_Number;
    constantPtr->Constant_Data = (char *)SceneFactory::getFloat();
    constantPtr->Constant_Type = FLOAT_CONSTANT;
    *((double *)constantPtr->Constant_Data) = PrimitiveParser::parseFloat();
    Exit_Flag = TRUE; break;

        default: ParseErrorReporter::parseError(OBJECT_TOKEN);
    break;
    }
        }
    }
}

