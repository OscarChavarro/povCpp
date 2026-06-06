#include "io/pov/ParserContext.h"
#include "common/LegacyBoolean.h"
#include "common/linealAlgebra/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/base/image/DumpFormat.h"
#include "io/base/image/GifFormat.h"
#include "io/base/image/IffFormat.h"
#include "io/base/image/TargaFormat.h"
#include "environment/scene/ModelBuilder.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/ParseGlobals.h"
#include "io/pov/ParseHelpers.h"
#include "io/pov/ParserConstants.h"
#include "io/pov/PrimitiveParser.h"
#include "io/pov/SceneConfigParser.h"
#include "io/pov/geometryParser/BicubicPatchParser.h"
#include "io/pov/geometryParser/BlobParser.h"
#include "io/pov/geometryParser/BoxParser.h"
#include "io/pov/geometryParser/HeightFieldParser.h"
#include "io/pov/geometryParser/ObjectParser.h"
#include "io/pov/geometryParser/PlaneParser.h"
#include "io/pov/geometryParser/PolyParser.h"
#include "io/pov/geometryParser/QuadricParser.h"
#include "io/pov/geometryParser/SmoothTriangleParser.h"
#include "io/pov/geometryParser/SphereParser.h"
#include "io/pov/geometryParser/TriangleParser.h"
#include "io/pov/lightParser/LightSourceParser.h"
#include "io/pov/mediaParser/TextureParser.h"
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
    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::COLOUR_TOKEN:
                PrimitiveParser::parseColour(&ctx.parsingFrame()->fogColour, ctx);
                break;

            case Tokenizer::FLOAT_TOKEN:
                ctx.parsingFrame()->fogDistance = ctx.token().tokenFloat;
                break;

            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
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

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::IDENTIFIER_TOKEN:
                if ((constantId = SceneConfigParser::findConstant(ctx)) != -1) {
                    if (ctx.constants()[(int)constantId].constantType ==
                        ParseGlobals::VIEW_POINT_CONSTANT) {
                        *givenVp = *((Camera *)ctx.constants()[(int)constantId]
                                .constantData);
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::Undeclared(ctx);
                }
                break;

            case Tokenizer::LOCATION_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Location), ctx);
                break;

            case Tokenizer::DIRECTION_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Direction), ctx);
                break;

            case Tokenizer::UP_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Up), ctx);
                break;

            case Tokenizer::RIGHT_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Right), ctx);
                break;

            case Tokenizer::SKY_TOKEN:
                PrimitiveParser::parseVector(&(givenVp->Sky), ctx);
                break;

            case Tokenizer::LOOK_AT_TOKEN:
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

            case Tokenizer::TRANSLATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::translate(
                    (SimpleBody *)givenVp, &localVector);
                break;

            case Tokenizer::ROTATE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::rotate((SimpleBody *)givenVp, &localVector);
                break;

            case Tokenizer::SCALE_TOKEN:
                PrimitiveParser::parseVector(&localVector, ctx);
                GeometryOperations::scale((SimpleBody *)givenVp, &localVector);
                break;

            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
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

    ParseHelpers::getExpectedToken(Tokenizer::IDENTIFIER_TOKEN, ctx);
    if ((constantId = SceneConfigParser::findConstant(ctx)) == -1) {
        if (++ctx.numberOfConstants() >= ParserConstants::MAX_CONSTANTS) {
            ParseErrorReporter::Error("Too many constants \"declared\"", ctx);
        } else {
            constantId = ctx.numberOfConstants();
        }
    }

    constantPtr = &(ctx.constants()[(int)constantId]);
    ParseHelpers::getExpectedToken(Tokenizer::EQUALS_TOKEN, ctx);

    {
        int Exit_Flag;
        Exit_Flag = LegacyBoolean::FALSE_VALUE;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::OBJECT_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseObject(ctx);
                constantPtr->constantType = ParseGlobals::OBJECT_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::SPHERE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)SphereParser::parseSphere(ctx);
                constantPtr->constantType = ParseGlobals::SPHERE_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::PLANE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)PlaneParser::parsePlane(ctx);
                constantPtr->constantType = ParseGlobals::PLANE_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::TRIANGLE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)TriangleParser::parseTriangle(ctx);
                constantPtr->constantType = ParseGlobals::TRIANGLE_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::SMOOTH_TRIANGLE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)SmoothTriangleParser::parseSmoothTriangle(ctx);
                constantPtr->constantType = ParseGlobals::SMOOTH_TRIANGLE_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::QUADRIC_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)QuadricParser::parseQuadric(ctx);
                constantPtr->constantType = ParseGlobals::QUADRIC_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::CUBIC_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)PolyParser::parsePoly(3, ctx);
                constantPtr->constantType = ParseGlobals::POLY_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::QUARTIC_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)PolyParser::parsePoly(4, ctx);
                constantPtr->constantType = ParseGlobals::POLY_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::HEIGHT_FIELD_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)HeightFieldParser::parseHeightField(ctx);
                constantPtr->constantType = ParseGlobals::HEIGHT_FIELD_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::POLY_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)PolyParser::parsePoly(0, ctx);
                constantPtr->constantType = ParseGlobals::POLY_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::BOX_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)BoxParser::parseBox(ctx);
                constantPtr->constantType = ParseGlobals::BOX_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::BLOB_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)BlobParser::parseBlob(ctx);
                constantPtr->constantType = ParseGlobals::BLOB_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::BICUBIC_PATCH_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)BicubicPatchParser::parseBicubicPatch(ctx);
                constantPtr->constantType = ParseGlobals::BICUBIC_PATCH_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::INTERSECTION_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseCsg(GeometryOperations::CSG_INTERSECTION_TYPE, ctx);
                constantPtr->constantType = ParseGlobals::CSG_INTERSECTION_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::UNION_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseCsg(GeometryOperations::CSG_UNION_TYPE, ctx);
                constantPtr->constantType = ParseGlobals::CSG_UNION_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::DIFFERENCE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseCsg(GeometryOperations::CSG_DIFFERENCE_TYPE, ctx);
                constantPtr->constantType = ParseGlobals::CSG_DIFFERENCE_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::COMPOSITE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseComposite(ctx);
                constantPtr->constantType = ParseGlobals::COMPOSITE_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::TEXTURE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                localTexture = nullptr;
                constantPtr->constantData = (char *)localTexture;
                constantPtr->constantType = ParseGlobals::TEXTURE_CONSTANT;
                ctx.tokenStream().ungetToken();
                {
                    int Exit_Flag;
                    Exit_Flag = LegacyBoolean::FALSE_VALUE;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().tokenId) {
                        case Tokenizer::TEXTURE_TOKEN:
                            localTexture = TextureUtils::defaultTexture();
                            localTexture = TextureParser::parseTexture(ctx);
                            if (localTexture->constantFlag) {
                                localTexture =
                                    TextureParser::copyTexture(localTexture);
                            }

                            localTexture->constantFlag = LegacyBoolean::TRUE_VALUE;

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
                            Exit_Flag = LegacyBoolean::TRUE_VALUE;
                            break;
                        }
                    }
                }
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::VIEW_POINT_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)ModelBuilder::getCamera();
                constantPtr->constantType = ParseGlobals::VIEW_POINT_CONSTANT;
                SceneConfigParser::parseCamera(
                    (Camera *)constantPtr->constantData, ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::COLOUR_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)ModelBuilder::getColour();
                constantPtr->constantType = ParseGlobals::COLOUR_CONSTANT;
                PrimitiveParser::parseColour(
                    (RGBAColor *)constantPtr->constantData, ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::LIGHT_SOURCE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)LightSourceParser::parseLightSource(ctx);
                constantPtr->constantType = ParseGlobals::LIGHT_SOURCE_CONSTANT;
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::LEFT_ANGLE_TOKEN:
                ctx.tokenStream().ungetToken();
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)ModelBuilder::getVector();
                constantPtr->constantType = ParseGlobals::VECTOR_CONSTANT;
                PrimitiveParser::parseVector(
                    (Vector3Dd *)constantPtr->constantData, ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            case Tokenizer::DASH_TOKEN:
            case Tokenizer::PLUS_TOKEN:
            case Tokenizer::FLOAT_TOKEN:
                ctx.tokenStream().ungetToken();
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)ModelBuilder::getFloat();
                constantPtr->constantType = ParseGlobals::FLOAT_CONSTANT;
                *((double *)constantPtr->constantData) =
                    PrimitiveParser::parseFloat(ctx);
                Exit_Flag = LegacyBoolean::TRUE_VALUE;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::OBJECT_TOKEN, ctx);
                break;
            }
        }
    }
}
