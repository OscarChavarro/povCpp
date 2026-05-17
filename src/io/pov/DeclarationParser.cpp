#include "io/pov/DeclarationParser.h"
#include "io/pov/ViewPointParser.h"
#include "io/pov/Parse.h"
#include "common/PovProto.h"
#include "common/Vector.h"

extern TokenStruct globalToken;
extern Constant constants[MAX_CONSTANTS];
extern int numberOfConstants;
extern Texture *Default_Texture;

static CONSTANT
findConstant()
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
DeclarationParser::parseDeclare()
{
    CONSTANT constantId;
    Texture *localTexture;
    Texture *tempTexture;
    Constant *constantPtr;

    ParseHelpers::getExpectedToken(IDENTIFIER_TOKEN);
    if ((constantId = findConstant()) == -1) {
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
    ViewPointParser::parseViewpoint((Viewpoint *)constantPtr->Constant_Data);
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
