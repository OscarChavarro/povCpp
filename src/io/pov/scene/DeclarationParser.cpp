#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"

#include "environment/camera/CameraBuilder.h"
#include "environment/material/PovRayMaterial.h"
#include "environment/material/PovRayMaterialUtils.h"
#include "environment/material/ValuesBuilder.h"

#include "io/pov/camera/CameraParser.h"
#include "io/pov/material/PovRayMaterialBuilder.h"
#include "io/pov/context/ParseGlobals.h"
#include "io/pov/context/ParserContext.h"
#include "io/pov/geometry/BicubicPatchParser.h"
#include "io/pov/geometry/BlobParser.h"
#include "io/pov/geometry/BoxParser.h"
#include "io/pov/geometry/CsgParser.h"
#include "io/pov/geometry/HeightFieldParser.h"
#include "io/pov/geometry/ObjectParser.h"
#include "io/pov/geometry/PlaneParser.h"
#include "io/pov/geometry/PolyParser.h"
#include "io/pov/geometry/QuadricParser.h"
#include "io/pov/geometry/SmoothTriangleParser.h"
#include "io/pov/geometry/SphereParser.h"
#include "io/pov/geometry/TriangleParser.h"
#include "io/pov/light/LightSourceParser.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "io/pov/scene/DeclarationParser.h"

void
DeclarationParser::parseDeclare()
{
    ParserContext ctx;
    DeclarationParser::parseDeclare(ctx);
}

void
DeclarationParser::parseDeclare(ParserContext &ctx)
{
    PovRayMaterial *localTexture;

    ParseHelpers::getExpectedToken(Tokenizer::IDENTIFIER_TOKEN, ctx);
    Constant * const constantPtr =
        ctx.symbols().upsertByIdentifierNumber(ctx.token().getIdentifierNumber());
    if (constantPtr == nullptr) {
        ParseErrorReporter::reportError("Too many constants \"declared\"", ctx);
    }
    ParseHelpers::getExpectedToken(Tokenizer::EQUALS_TOKEN, ctx);

    {
        bool Exit_Flag;
        Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::OBJECT_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)ObjectParser::parseObject(ctx));
                constantPtr->setConstantType(ParseGlobals::OBJECT_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::SPHERE_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)SphereParser::parseSphere(ctx));
                constantPtr->setConstantType(ParseGlobals::SPHERE_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::PLANE_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)PlaneParser::parsePlane(ctx));
                constantPtr->setConstantType(ParseGlobals::PLANE_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::TRIANGLE_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)TriangleParser::parseTriangle(ctx));
                constantPtr->setConstantType(ParseGlobals::TRIANGLE_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::SMOOTH_TRIANGLE_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)SmoothTriangleParser::parseSmoothTriangle(ctx));
                constantPtr->setConstantType(ParseGlobals::SMOOTH_TRIANGLE_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::QUADRIC_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)QuadricParser::parseQuadric(ctx));
                constantPtr->setConstantType(ParseGlobals::QUADRIC_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::CUBIC_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)PolyParser::parsePoly(3, ctx));
                constantPtr->setConstantType(ParseGlobals::POLY_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::QUARTIC_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)PolyParser::parsePoly(4, ctx));
                constantPtr->setConstantType(ParseGlobals::POLY_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::HEIGHT_FIELD_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)HeightFieldParser::parseHeightField(ctx));
                constantPtr->setConstantType(ParseGlobals::HEIGHT_FIELD_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::POLY_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)PolyParser::parsePoly(0, ctx));
                constantPtr->setConstantType(ParseGlobals::POLY_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::BOX_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)BoxParser::parseBox(ctx));
                constantPtr->setConstantType(ParseGlobals::BOX_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::BLOB_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)BlobParser::parseBlob(ctx));
                constantPtr->setConstantType(ParseGlobals::BLOB_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::BICUBIC_PATCH_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)BicubicPatchParser::parseBicubicPatch(ctx));
                constantPtr->setConstantType(ParseGlobals::BICUBIC_PATCH_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::INTERSECTION_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)CsgParser::parse(BooleanSetOperations::INTERSECTION, ctx));
                constantPtr->setConstantType(ParseGlobals::CSG_INTERSECTION_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::UNION_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)CsgParser::parse(BooleanSetOperations::UNION, ctx));
                constantPtr->setConstantType(ParseGlobals::CSG_UNION_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::DIFFERENCE_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)CsgParser::parse(BooleanSetOperations::DIFFERENCE, ctx));
                constantPtr->setConstantType(ParseGlobals::CSG_DIFFERENCE_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::COMPOSITE_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)ObjectParser::parseComposite(ctx));
                constantPtr->setConstantType(ParseGlobals::COMPOSITE_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::TEXTURE_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                localTexture = nullptr;
                constantPtr->setConstantData((char *)localTexture);
                constantPtr->setConstantType(ParseGlobals::TEXTURE_CONSTANT);
                ctx.tokenStream().ungetToken();
                {
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().getTokenId()) {
                        case Tokenizer::TEXTURE_TOKEN:
                            localTexture = TextureParser::parseTexture(
                                ctx.getDefaultTexture(), ctx);
                            if (localTexture->isConstant()) {
                                localTexture =
                                    TextureParser::copyTexture(localTexture);
                            }

                            localTexture = PovRayMaterialBuilder(localTexture).setConstant(true).build();

                            {
                                PovRayMaterial *existingHead =
                                    (PovRayMaterial *)constantPtr->getConstantData();
                                PovRayMaterialUtils::prependTextureLayers(localTexture, existingHead);
                                constantPtr->setConstantData((char *)existingHead);
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
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)CameraBuilder::getCamera());
                constantPtr->setConstantType(ParseGlobals::VIEW_POINT_CONSTANT);
                CameraParser::parseCamera(
                    (Camera *)constantPtr->getConstantData(), ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::COLOUR_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)ValuesBuilder::getColor());
                constantPtr->setConstantType(ParseGlobals::COLOUR_CONSTANT);
                PrimitiveParser::parseColor(
                    (ColorRgba *)constantPtr->getConstantData(), ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::LIGHT_SOURCE_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)LightSourceParser::parseLightSource(ctx));
                constantPtr->setConstantType(ParseGlobals::LIGHT_SOURCE_CONSTANT);
                Exit_Flag = true;
                break;

            case Tokenizer::LEFT_ANGLE_TOKEN:
                ctx.tokenStream().ungetToken();
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)ValuesBuilder::getVector());
                constantPtr->setConstantType(ParseGlobals::VECTOR_CONSTANT);
                PrimitiveParser::parseVector(
                    (Vector3Dd *)constantPtr->getConstantData(), ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::DASH_TOKEN:
            case Tokenizer::PLUS_TOKEN:
            case Tokenizer::FLOAT_TOKEN:
                ctx.tokenStream().ungetToken();
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)ValuesBuilder::getFloat());
                constantPtr->setConstantType(ParseGlobals::FLOAT_CONSTANT);
                *((double *)constantPtr->getConstantData()) =
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
