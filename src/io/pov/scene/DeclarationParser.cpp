#include "io/pov/context/ParserContext.h"
#include "io/pov/scene/DeclarationParser.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/scene/ModelBuilder.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "io/pov/geometry/BicubicPatchParser.h"
#include "io/pov/geometry/BlobParser.h"
#include "io/pov/geometry/BoxParser.h"
#include "io/pov/geometry/HeightFieldParser.h"
#include "io/pov/geometry/ObjectParser.h"
#include "io/pov/geometry/PlaneParser.h"
#include "io/pov/geometry/PolyParser.h"
#include "io/pov/geometry/QuadricParser.h"
#include "io/pov/geometry/SmoothTriangleParser.h"
#include "io/pov/geometry/SphereParser.h"
#include "io/pov/geometry/TriangleParser.h"
#include "io/pov/light/LightSourceParser.h"
#include "io/pov/texture/TextureParser.h"
#include "io/pov/camera/CameraParser.h"
#include "media/Texture.h"
#include "media/TextureUtils.h"

void
DeclarationParser::parseDeclare()
{
    ParserContext ctx;
    DeclarationParser::parseDeclare(ctx);
}

void
DeclarationParser::parseDeclare(ParserContext &ctx)
{
    Texture *localTexture;
    Texture *tempTexture;
    Constant *constantPtr;

    ParseHelpers::getExpectedToken(Tokenizer::IDENTIFIER_TOKEN, ctx);
    constantPtr =
        ctx.symbols().upsertByIdentifierNumber(ctx.token().identifierNumber);
    if (constantPtr == nullptr) {
        ParseErrorReporter::reportError("Too many constants \"declared\"", ctx);
    }
    ParseHelpers::getExpectedToken(Tokenizer::EQUALS_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().tokenId) {
            case Tokenizer::OBJECT_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseObject(ctx);
                constantPtr->constantType = ParseGlobals::OBJECT_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::SPHERE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)SphereParser::parseSphere(ctx);
                constantPtr->constantType = ParseGlobals::SPHERE_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::PLANE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)PlaneParser::parsePlane(ctx);
                constantPtr->constantType = ParseGlobals::PLANE_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::TRIANGLE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)TriangleParser::parseTriangle(ctx);
                constantPtr->constantType = ParseGlobals::TRIANGLE_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::SMOOTH_TRIANGLE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)SmoothTriangleParser::parseSmoothTriangle(ctx);
                constantPtr->constantType = ParseGlobals::SMOOTH_TRIANGLE_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::QUADRIC_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)QuadricParser::parseQuadric(ctx);
                constantPtr->constantType = ParseGlobals::QUADRIC_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::CUBIC_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)PolyParser::parsePoly(3, ctx);
                constantPtr->constantType = ParseGlobals::POLY_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::QUARTIC_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)PolyParser::parsePoly(4, ctx);
                constantPtr->constantType = ParseGlobals::POLY_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::HEIGHT_FIELD_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)HeightFieldParser::parseHeightField(ctx);
                constantPtr->constantType = ParseGlobals::HEIGHT_FIELD_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::POLY_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)PolyParser::parsePoly(0, ctx);
                constantPtr->constantType = ParseGlobals::POLY_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::BOX_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)BoxParser::parseBox(ctx);
                constantPtr->constantType = ParseGlobals::BOX_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::BLOB_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)BlobParser::parseBlob(ctx);
                constantPtr->constantType = ParseGlobals::BLOB_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::BICUBIC_PATCH_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)BicubicPatchParser::parseBicubicPatch(ctx);
                constantPtr->constantType = ParseGlobals::BICUBIC_PATCH_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::INTERSECTION_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseCsg(GeometryOperations::CSG_INTERSECTION_TYPE, ctx);
                constantPtr->constantType = ParseGlobals::CSG_INTERSECTION_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::UNION_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseCsg(GeometryOperations::CSG_UNION_TYPE, ctx);
                constantPtr->constantType = ParseGlobals::CSG_UNION_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::DIFFERENCE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseCsg(GeometryOperations::CSG_DIFFERENCE_TYPE, ctx);
                constantPtr->constantType = ParseGlobals::CSG_DIFFERENCE_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::COMPOSITE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)ObjectParser::parseComposite(ctx);
                constantPtr->constantType = ParseGlobals::COMPOSITE_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::TEXTURE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                localTexture = nullptr;
                constantPtr->constantData = (char *)localTexture;
                constantPtr->constantType = ParseGlobals::TEXTURE_CONSTANT;
                ctx.tokenStream().ungetToken();
                {
                    bool Exit_Flag;
                    Exit_Flag = false;
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

                            localTexture->constantFlag = true;

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
                            Exit_Flag = true;
                            break;
                        }
                    }
                }
                Exit_Flag = true;
                break;

            case Tokenizer::VIEW_POINT_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)ModelBuilder::getCamera();
                constantPtr->constantType = ParseGlobals::VIEW_POINT_CONSTANT;
                CameraParser::parseCamera(
                    (Camera *)constantPtr->constantData, ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::COLOUR_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)ModelBuilder::getColour();
                constantPtr->constantType = ParseGlobals::COLOUR_CONSTANT;
                PrimitiveParser::parseColour(
                    (RGBAColor *)constantPtr->constantData, ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::LIGHT_SOURCE_TOKEN:
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData =
                    (char *)LightSourceParser::parseLightSource(ctx);
                constantPtr->constantType = ParseGlobals::LIGHT_SOURCE_CONSTANT;
                Exit_Flag = true;
                break;

            case Tokenizer::LEFT_ANGLE_TOKEN:
                ctx.tokenStream().ungetToken();
                constantPtr->identifierNumber = ctx.token().identifierNumber;
                constantPtr->constantData = (char *)ModelBuilder::getVector();
                constantPtr->constantType = ParseGlobals::VECTOR_CONSTANT;
                PrimitiveParser::parseVector(
                    (Vector3Dd *)constantPtr->constantData, ctx);
                Exit_Flag = true;
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
                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::OBJECT_TOKEN, ctx);
                break;
            }
        }
    }
}
