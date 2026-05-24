#include "io/pov/ParserContext.h"
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



CONSTANT
SceneConfigParser::findConstant()
{
    ParserContext ctx;
    return SceneConfigParser::findConstant(ctx);
}

CONSTANT
SceneConfigParser::findConstant(ParserContext &ctx)
{
    return ctx.symbols().findByIdentifierNumber(ctx.token().identifierNumber);
}

void
SceneConfigParser::parseFog()
{
    ParserContext ctx;
    SceneConfigParser::parseFog(ctx);
}

void
SceneConfigParser::parseFog(ParserContext &ctx)
{
    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case COLOUR_TOKEN:
                PrimitiveParser::parseColour(&ctx.parsingFrame()->fogColour, ctx);
                break;

            case FLOAT_TOKEN:
                ctx.parsingFrame()->fogDistance = ctx.token().tokenFloat;
                break;

            case RIGHT_CURLY_TOKEN:
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }
}

void
SceneConfigParser::parseCamera(Camera *givenVp)
{
    ParserContext ctx;
    SceneConfigParser::parseCamera(givenVp, ctx);
}

void
SceneConfigParser::parseCamera(Camera *givenVp, ParserContext &ctx)
{
    CONSTANT constantId;
    Vector3Dd localVector;
    Vector3Dd tempVector;
    double directionLength;
    double upLength;
    double rightLength;
    double handedness;

    givenVp->initializeDefaults();

    ParseHelpers::getExpectedToken(LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = FALSE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        VIEW_POINT_CONSTANT) {
                        *givenVp = *((Camera *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
                break;

            case LOCATION_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Location), ctx);
                break;

            case DIRECTION_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Direction), ctx);
                break;

            case UP_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Up), ctx);
                break;

            case RIGHT_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Right), ctx);
                break;

            case SKY_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Sky), ctx);
                break;

            case LOOK_AT_TOKEN:
                directionLength = givenVp->Direction.length();
                upLength = givenVp->Up.length();
                rightLength = givenVp->Right.length();
                tempVector = givenVp->Direction.crossProduct(givenVp->Up);
                handedness = tempVector.dotProduct(givenVp->Right);
                PrimitiveParser::parseVector(&givenVp->Direction, ctx);

                givenVp->Direction.sub(givenVp->Location);
                givenVp->Direction.normalize();
                givenVp->Right = givenVp->Direction.crossProduct(givenVp->Sky);
                givenVp->Right.normalize();
                givenVp->Up = givenVp->Right.crossProduct(givenVp->Direction);
                givenVp->Direction.scale(directionLength);
                if (handedness >= 0.0) {
                    givenVp->Right.scale(rightLength);
                } else {
                    givenVp->Right.scale(-rightLength);
                }

                givenVp->Up.scale(upLength);
                break;

            case TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(
                    (SimpleBody *)givenVp, &localVector);
                break;

            case ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate((SimpleBody *)givenVp, &localVector);
                break;

            case SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale((SimpleBody *)givenVp, &localVector);
                break;

            case RIGHT_CURLY_TOKEN:
                Exit_Flag = TRUE;
                break;

            default:
                ParseErrorReporter::parseError(RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }
}

void
SceneConfigParser::parseDeclare()
{
    ParserContext ctx;
    SceneConfigParser::parseDeclare(ctx);
}

void
SceneConfigParser::parseDeclare(ParserContext &ctx)
{
    CONSTANT constantId;
    Texture *localTexture;
    Texture *tempTexture;
    Constant *constantPtr;

    ParseHelpers::getExpectedToken(IDENTIFIER_TOKEN, ctx);
    if ((constantId = SceneConfigParser::findConstant(ctx)) == -1) {
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
            ctx.tokenStream().getToken();
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
                ctx.tokenStream().ungetToken();
                {
                    int Exit_Flag;
                    Exit_Flag = FALSE;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
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
                            ctx.tokenStream().ungetToken();
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
                    (char *)ModelBuilder::getCamera();
                constantPtr->constantType = VIEW_POINT_CONSTANT;
                SceneConfigParser::parseCamera(
                    (Camera *)constantPtr->constantData, ctx);
                Exit_Flag = TRUE;
                break;

            case COLOUR_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)ModelBuilder::getColour();
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
                ctx.tokenStream().ungetToken();
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)ModelBuilder::getVector();
                constantPtr->constantType = VECTOR_CONSTANT;
                PrimitiveParser::parseVector(
                    (Vector3Dd *)constantPtr->constantData, ctx);
                Exit_Flag = TRUE;
                break;

            case DASH_TOKEN:
            case PLUS_TOKEN:
            case FLOAT_TOKEN:
                ctx.tokenStream().ungetToken();
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)ModelBuilder::getFloat();
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
