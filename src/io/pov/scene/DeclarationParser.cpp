#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/camera/CameraBuilder.h"
#include "environment/material/PovRayMaterial.h"
#include "environment/material/PovRayMaterialUtils.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "io/pov/camera/CameraParser.h"
#include "io/pov/material/PovRayMaterialConstancy.h"
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
        bool localExitFlag;
        localExitFlag = false;
        while (!localExitFlag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::OBJECT_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)ObjectParser::parseObject(ctx));
                constantPtr->setConstantType(ParseGlobals::OBJECT_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::SPHERE_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)SphereParser::parseSphere(ctx));
                constantPtr->setConstantType(ParseGlobals::SPHERE_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::PLANE_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)PlaneParser::parsePlane(ctx));
                constantPtr->setConstantType(ParseGlobals::PLANE_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::TRIANGLE_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)TriangleParser::parseTriangle(ctx));
                constantPtr->setConstantType(ParseGlobals::TRIANGLE_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::SMOOTH_TRIANGLE_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)SmoothTriangleParser::parseSmoothTriangle(ctx));
                constantPtr->setConstantType(ParseGlobals::SMOOTH_TRIANGLE_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::QUADRIC_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)QuadricParser::parseQuadric(ctx));
                constantPtr->setConstantType(ParseGlobals::QUADRIC_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::CUBIC_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)PolyParser::parsePoly(3, ctx));
                constantPtr->setConstantType(ParseGlobals::POLY_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::QUARTIC_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)PolyParser::parsePoly(4, ctx));
                constantPtr->setConstantType(ParseGlobals::POLY_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::HEIGHT_FIELD_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)HeightFieldParser::parseHeightField(ctx));
                constantPtr->setConstantType(ParseGlobals::HEIGHT_FIELD_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::POLY_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)PolyParser::parsePoly(0, ctx));
                constantPtr->setConstantType(ParseGlobals::POLY_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::BOX_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)BoxParser::parseBox(ctx));
                constantPtr->setConstantType(ParseGlobals::BOX_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::BLOB_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)BlobParser::parseBlob(ctx));
                constantPtr->setConstantType(ParseGlobals::BLOB_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::BICUBIC_PATCH_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)BicubicPatchParser::parseBicubicPatch(ctx));
                constantPtr->setConstantType(ParseGlobals::BICUBIC_PATCH_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::INTERSECTION_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)CsgParser::parse(BooleanSetOperations::INTERSECTION, ctx));
                constantPtr->setConstantType(ParseGlobals::CSG_INTERSECTION_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::UNION_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)CsgParser::parse(BooleanSetOperations::UNION, ctx));
                constantPtr->setConstantType(ParseGlobals::CSG_UNION_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::DIFFERENCE_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)CsgParser::parse(BooleanSetOperations::DIFFERENCE, ctx));
                constantPtr->setConstantType(ParseGlobals::CSG_DIFFERENCE_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::COMPOSITE_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)ObjectParser::parseComposite(ctx));
                constantPtr->setConstantType(ParseGlobals::COMPOSITE_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::TEXTURE_TOKEN:
                // Re-declaring an already-#declare'd texture identifier reaches
                // this same Constant slot via upsertByIdentifierNumber() - free
                // the old backing PovRayMaterial first. Safe to delete: every
                // consumer of a TEXTURE_CONSTANT either clones it on use
                // (PovRayMaterialConstancy::isConstant() guard in
                // TextureParser.cpp) or absorbs it into a fresh layer chain
                // (PovRayMaterialUtils::prependTextureLayers, also a clone-on-
                // construction path), so nothing still aliases the old object
                // directly. unmarkConstant() first, since it was always
                // markConstant()'d below and the registry must not keep a
                // dangling pointer that a later allocation could reuse.
                if (constantPtr->getConstantType() == ParseGlobals::TEXTURE_CONSTANT) {
                    PovRayMaterial * const oldTexture =
                        static_cast<PovRayMaterial *>(constantPtr->getConstantData());
                    PovRayMaterialConstancy::unmarkConstant(oldTexture);
                    delete oldTexture;
                }
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                localTexture = nullptr;
                constantPtr->setConstantData((char *)localTexture);
                constantPtr->setConstantType(ParseGlobals::TEXTURE_CONSTANT);
                ctx.tokenStream().ungetToken();
                {
                    bool innerTextureExitFlag;
                    innerTextureExitFlag = false;
                    while (!innerTextureExitFlag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().getTokenId()) {
                        case Tokenizer::TEXTURE_TOKEN:
                            localTexture = TextureParser::parseTexture(
                                ctx.getDefaultTexture(), ctx);
                            if (PovRayMaterialConstancy::isConstant(localTexture)) {
                                localTexture =
                                    TextureParser::copyTexture(localTexture);
                            }

                            PovRayMaterialConstancy::markConstant(localTexture);

                            {
                                PovRayMaterial *existingHead =
                                    (PovRayMaterial *)constantPtr->getConstantData();
                                PovRayMaterialUtils::prependTextureLayers(localTexture, existingHead);
                                constantPtr->setConstantData((char *)existingHead);
                            }
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            innerTextureExitFlag = true;
                            break;
                        }
                    }
                }
                localExitFlag = true;
                break;

            case Tokenizer::VIEW_POINT_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)CameraBuilder::getCamera());
                constantPtr->setConstantType(ParseGlobals::VIEW_POINT_CONSTANT);
                CameraParser::parseCamera(
                    (Camera *)constantPtr->getConstantData(), ctx);
                localExitFlag = true;
                break;

            case Tokenizer::COLOUR_TOKEN:
                // Re-declaring an already-#declare'd identifier (#declare X = ...
                // appearing twice for the same X) reaches this same Constant slot
                // via upsertByIdentifierNumber() - free whatever it held before,
                // if it was the same simple value type, before overwriting it.
                if (constantPtr->getConstantType() == ParseGlobals::COLOUR_CONSTANT) {
                    delete static_cast<ColorRgba *>(constantPtr->getConstantData());
                }
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)new ColorRgba(0.0, 0.0, 0.0, 0.0));
                constantPtr->setConstantType(ParseGlobals::COLOUR_CONSTANT);
                PrimitiveParser::parseColor(
                    (ColorRgba *)constantPtr->getConstantData(), ctx);
                localExitFlag = true;
                break;

            case Tokenizer::LIGHT_SOURCE_TOKEN:
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData(
                    (char *)LightSourceParser::parseLightSource(ctx));
                constantPtr->setConstantType(ParseGlobals::LIGHT_SOURCE_CONSTANT);
                localExitFlag = true;
                break;

            case Tokenizer::LEFT_ANGLE_TOKEN:
                ctx.tokenStream().ungetToken();
                if (constantPtr->getConstantType() == ParseGlobals::VECTOR_CONSTANT) {
                    delete static_cast<Vector3Dd *>(constantPtr->getConstantData());
                }
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)new Vector3Dd);
                constantPtr->setConstantType(ParseGlobals::VECTOR_CONSTANT);
                PrimitiveParser::parseVector(
                    (Vector3Dd *)constantPtr->getConstantData(), ctx);
                localExitFlag = true;
                break;

            case Tokenizer::DASH_TOKEN:
            case Tokenizer::PLUS_TOKEN:
            case Tokenizer::FLOAT_TOKEN:
                ctx.tokenStream().ungetToken();
                if (constantPtr->getConstantType() == ParseGlobals::FLOAT_CONSTANT) {
                    delete static_cast<double *>(constantPtr->getConstantData());
                }
                constantPtr->setIdentifierNumber(ctx.token().getIdentifierNumber());
                constantPtr->setConstantData((char *)new double(0.0));
                constantPtr->setConstantType(ParseGlobals::FLOAT_CONSTANT);
                *((double *)constantPtr->getConstantData()) =
                    PrimitiveParser::parseFloat(ctx);
                localExitFlag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::OBJECT_TOKEN, ctx);
                break;
            }
        }
    }
}
