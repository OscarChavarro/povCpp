#include "io/pov/DeclarationParser.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/pov/Parse.h"
#include "io/pov/cameraParser/CameraParser.h"

extern TokenStruct globalToken;
extern Constant constants[MAX_CONSTANTS];
extern int numberOfConstants;
extern Texture *Default_Texture;

static CONSTANT
findConstant()
{
    int i;

    for (i = 1; i <= numberOfConstants; i++) {
        if (constants[i].identifierNumber == globalToken.identifierNumber) {
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
            switch (globalToken.tokenId) {
            case OBJECT_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseObject();
                constantPtr->constantType = OBJECT_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case SPHERE_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData =
                    (char *)SphereParser::parseSphere();
                constantPtr->constantType = SPHERE_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case PLANE_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData = (char *)PlaneParser::parsePlane();
                constantPtr->constantType = PLANE_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case TRIANGLE_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData =
                    (char *)TriangleParser::parseTriangle();
                constantPtr->constantType = TRIANGLE_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case SMOOTH_TRIANGLE_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData =
                    (char *)SmoothTriangleParser::parseSmoothTriangle();
                constantPtr->constantType = SMOOTH_TRIANGLE_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case QUADRIC_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData =
                    (char *)QuadricParser::parseQuadric();
                constantPtr->constantType = QUADRIC_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case CUBIC_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData = (char *)PolyParser::parsePoly(3);
                constantPtr->constantType = POLY_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case QUARTIC_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData = (char *)PolyParser::parsePoly(4);
                constantPtr->constantType = POLY_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case HEIGHT_FIELD_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData =
                    (char *)HeightFieldParser::parseHeightField();
                constantPtr->constantType = HEIGHT_FIELD_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case POLY_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData = (char *)PolyParser::parsePoly(0);
                constantPtr->constantType = POLY_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case BOX_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData = (char *)BoxParser::parseBox();
                constantPtr->constantType = BOX_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case BLOB_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData = (char *)BlobParser::parseBlob();
                constantPtr->constantType = BLOB_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case BICUBIC_PATCH_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData =
                    (char *)BicubicPatchParser::parseBicubicPatch();
                constantPtr->constantType = BICUBIC_PATCH_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case INTERSECTION_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseCsg(CSG_INTERSECTION_TYPE);
                constantPtr->constantType = CSG_INTERSECTION_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case UNION_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseCsg(CSG_UNION_TYPE);
                constantPtr->constantType = CSG_UNION_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case DIFFERENCE_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseCsg(CSG_DIFFERENCE_TYPE);
                constantPtr->constantType = CSG_DIFFERENCE_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case COMPOSITE_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseComposite();
                constantPtr->constantType = COMPOSITE_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case TEXTURE_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                localTexture = nullptr;
                constantPtr->constantData = (char *)localTexture;
                constantPtr->constantType = TEXTURE_CONSTANT;
                Tokenizer::ungetToken();
                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (globalToken.tokenId) {
                        case TEXTURE_TOKEN:
                            localTexture = Default_Texture;
                            localTexture = TextureParser::parseTexture();
                            if (localTexture->constantFlag) {
                                localTexture =
                                    TextureParser::copyTexture(localTexture);
                            }

                            localTexture->constantFlag = TRUE;

                            {
                                for (tempTexture = localTexture;
                                    tempTexture->Next_Texture != nullptr;
                                    tempTexture = tempTexture->Next_Texture) {
                                }

                                tempTexture->Next_Texture =
                                    (Texture *)constantPtr->constantData;
                                constantPtr->constantData =
                                    (char *)localTexture;
                            }
                            break;

                        default:
                            Tokenizer::ungetToken();
                            Exit_Flag = TRUE;
                            break;
                        }
                    }
                }
                Exit_Flag = TRUE;
                break;

            case VIEW_POINT_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData =
                    (char *)SceneFactory::getCamera();
                constantPtr->constantType = VIEW_POINT_CONSTANT;
                CameraParser::parseCamera(
                    (Camera *)constantPtr->constantData);
                Exit_Flag = TRUE;
                break;

            case COLOUR_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData = (char *)SceneFactory::getColour();
                constantPtr->constantType = COLOUR_CONSTANT;
                PrimitiveParser::parseColour(
                    (RGBAColor *)constantPtr->constantData);
                Exit_Flag = TRUE;
                break;

            case LIGHT_SOURCE_TOKEN:
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData =
                    (char *)LightSourceParser::parseLightSource();
                constantPtr->constantType = LIGHT_SOURCE_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case LEFT_ANGLE_TOKEN:
                Tokenizer::ungetToken();
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData = (char *)SceneFactory::getVector();
                constantPtr->constantType = VECTOR_CONSTANT;
                PrimitiveParser::parseVector(
                    (Vector3Dd *)constantPtr->constantData);
                Exit_Flag = TRUE;
                break;

            case DASH_TOKEN:
            case PLUS_TOKEN:
            case FLOAT_TOKEN:
                Tokenizer::ungetToken();
                constantPtr->identifierNumber = globalToken.identifierNumber;
                constantPtr->constantData = (char *)SceneFactory::getFloat();
                constantPtr->constantType = FLOAT_CONSTANT;
                *((double *)constantPtr->constantData) =
                    PrimitiveParser::parseFloat();
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(OBJECT_TOKEN);
                break;
            }
        }
    }
}
