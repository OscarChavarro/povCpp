#include "environment/material/povray/PovRayMaterial.h"
#include "io/pov/material/PovRayMaterialUtils.h"
#include "io/pov/camera/CameraParser.h"
#include "environment/material/povray/PovRayMaterialConstancy.h"
#include "io/pov/geometry/BicubicPatchParser.h"
#include "io/pov/geometry/BlobParser.h"
#include "io/pov/geometry/BoxParser.h"
#include "io/pov/geometry/CsgParser.h"
#include "io/pov/geometry/HeightFieldParser.h"
#include "io/pov/scene/ObjectParser.h"
#include "io/pov/geometry/PlaneParser.h"
#include "io/pov/geometry/PolyParser.h"
#include "io/pov/geometry/QuadricParser.h"
#include "io/pov/geometry/SphereParser.h"
#include "io/pov/geometry/TriangleParser.h"
#include "io/pov/light/LightSourceParser.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "io/pov/scene/DeclarationParser.h"
#include "io/pov/scene/SceneParser.h"

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
    // Re-declaring an already-#declare'd identifier (#declare X = ... appearing
    // twice for the same X) reaches this same Constant slot via
    // upsertByIdentifierNumber(). Capture what it held *without* clearing it
    // yet - POV-Ray's own include files rely on reading the old value by name
    // while computing the new one (e.g. light.inc's flicker-light idiom:
    // `#declare light1 = color ...; #declare light1 = object { light_source
    // { ... color light1 } };` - the right-hand side's `color light1` must
    // still resolve to the *old* colour while this very #declare is being
    // parsed). The switch below overwrites constantPtr's data/type once the
    // new value is fully computed; only then is it safe to free the old one.
    const int oldConstantType = constantPtr->getConstantType();
    void * const oldConstantData = constantPtr->getConstantData();
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
                                static_cast<PovRayMaterial *>(ctx.getDefaultTexture()), ctx);
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
                constantPtr->setConstantData((char *)new PovCameraSpec());
                constantPtr->setConstantType(ParseGlobals::VIEW_POINT_CONSTANT);
                CameraParser::parseCamera(
                    (PovCameraSpec *)constantPtr->getConstantData(), ctx);
                localExitFlag = true;
                break;

            case Tokenizer::COLOUR_TOKEN:
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

    // Only now, with the new value fully computed and already stored in
    // constantPtr, is it safe to free what the slot held before (see the
    // comment above oldConstantType/oldConstantData). No-op when this was a
    // fresh identifier (oldConstantData is nullptr).
    SceneParser::freeConstant(oldConstantType, oldConstantData);
}
