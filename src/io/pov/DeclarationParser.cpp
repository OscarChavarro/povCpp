#include "io/pov/ParserContext.h"
#include "io/pov/DeclarationParser.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/pov/Parse.h"
#include "io/pov/cameraParser/CameraParser.h"

static CONSTANT
findConstant(ParserContext &ctx)
{
    int i;

    for (i = 1; i <= ctx.numberOfConstants(); i++) {
        if (ctx.constants()[i].identifierNumber == ctx.token().identifierNumber) {
            return (i);
        }
    }

    return (-1);
}

void
DeclarationParser::parseDeclare()
{
    ParserContext ctx;
    DeclarationParser::parseDeclare(ctx);
}

void
DeclarationParser::parseDeclare(ParserContext &ctx)
{
    CONSTANT constantId;
    Texture *localTexture;
    Texture *tempTexture;
    Constant *constantPtr;

    ParseHelpers::getExpectedToken(IDENTIFIER_TOKEN, ctx);
    if ((constantId = findConstant(ctx)) == -1) {
        if (++ctx.numberOfConstants() >= MAX_CONSTANTS) {
            ParseErrorReporter::Error("Too many constants \"declared\"", ctx);
        } else {
            constantId = ctx.numberOfConstants();
        }
    }

    constantPtr = &(ctx.constants()[(int)constantId]);
    ParseHelpers::getExpectedToken(EQUALS_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            Tokenizer::getToken();
            switch (ctx.token().tokenId) {
            case OBJECT_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseObject(ctx);
                constantPtr->constantType = OBJECT_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case SPHERE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)SphereParser::parseSphere(ctx);
                constantPtr->constantType = SPHERE_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case PLANE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)PlaneParser::parsePlane(ctx);
                constantPtr->constantType = PLANE_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case TRIANGLE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)TriangleParser::parseTriangle(ctx);
                constantPtr->constantType = TRIANGLE_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case SMOOTH_TRIANGLE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)SmoothTriangleParser::parseSmoothTriangle(ctx);
                constantPtr->constantType = SMOOTH_TRIANGLE_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case QUADRIC_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)QuadricParser::parseQuadric(ctx);
                constantPtr->constantType = QUADRIC_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case CUBIC_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)PolyParser::parsePoly(3, ctx);
                constantPtr->constantType = POLY_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case QUARTIC_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)PolyParser::parsePoly(4, ctx);
                constantPtr->constantType = POLY_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case HEIGHT_FIELD_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)HeightFieldParser::parseHeightField(ctx);
                constantPtr->constantType = HEIGHT_FIELD_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case POLY_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)PolyParser::parsePoly(0, ctx);
                constantPtr->constantType = POLY_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case BOX_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)BoxParser::parseBox(ctx);
                constantPtr->constantType = BOX_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case BLOB_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)BlobParser::parseBlob(ctx);
                constantPtr->constantType = BLOB_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case BICUBIC_PATCH_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)BicubicPatchParser::parseBicubicPatch(ctx);
                constantPtr->constantType = BICUBIC_PATCH_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case INTERSECTION_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseCsg(CSG_INTERSECTION_TYPE, ctx);
                constantPtr->constantType = CSG_INTERSECTION_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case UNION_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseCsg(CSG_UNION_TYPE, ctx);
                constantPtr->constantType = CSG_UNION_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case DIFFERENCE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseCsg(CSG_DIFFERENCE_TYPE, ctx);
                constantPtr->constantType = CSG_DIFFERENCE_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case COMPOSITE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseComposite(ctx);
                constantPtr->constantType = COMPOSITE_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case TEXTURE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                localTexture = nullptr;
                constantPtr->constantData = (char *)localTexture;
                constantPtr->constantType = TEXTURE_CONSTANT;
                Tokenizer::ungetToken();
                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        Tokenizer::getToken();
                        switch (ctx.token().tokenId) {
                        case TEXTURE_TOKEN:
                            localTexture = TextureUtils::defaultTexture();
                            localTexture = TextureParser::parseTexture(ctx);
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
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)SceneFactory::getCamera();
                constantPtr->constantType = VIEW_POINT_CONSTANT;
                CameraParser::parseCamera(
                    (Camera *)constantPtr->constantData, ctx);
                Exit_Flag = TRUE;
                break;

            case COLOUR_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)SceneFactory::getColour();
                constantPtr->constantType = COLOUR_CONSTANT;
                PrimitiveParser::parseColour(
                    (RGBAColor *)constantPtr->constantData, ctx);
                Exit_Flag = TRUE;
                break;

            case LIGHT_SOURCE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)LightSourceParser::parseLightSource(ctx);
                constantPtr->constantType = LIGHT_SOURCE_CONSTANT;
                Exit_Flag = TRUE;
                break;

            case LEFT_ANGLE_TOKEN:
                Tokenizer::ungetToken();
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)SceneFactory::getVector();
                constantPtr->constantType = VECTOR_CONSTANT;
                PrimitiveParser::parseVector(
                    (Vector3Dd *)constantPtr->constantData, ctx);
                Exit_Flag = TRUE;
                break;

            case DASH_TOKEN:
            case PLUS_TOKEN:
            case FLOAT_TOKEN:
                Tokenizer::ungetToken();
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)SceneFactory::getFloat();
                constantPtr->constantType = FLOAT_CONSTANT;
                *((double *)constantPtr->constantData) =
                    PrimitiveParser::parseFloat(ctx);
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(OBJECT_TOKEN, ctx);
                break;
            }
        }
    }
}
