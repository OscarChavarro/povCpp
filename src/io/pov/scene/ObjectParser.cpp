

#include "environment/scene/constructiveSolidGeometry/ConstructiveSolidGeometry.h"
#include "environment/material/povray/DefaultTextureAliasTracker.h"
#include "io/pov/material/PovRayMaterialUtils.h"
#include "io/pov/geometry/SceneBuilder.h"

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
#include "environment/material/povray/PovRayMaterialConstancy.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

// objectTexture is always a PovRayMaterial in this parser; releaseFromOwner()
// already tells private clones (delete this) apart from shared-default
// aliases (untrack, delete only once retired and unreferenced). PovrayBody's
// destructor calls this through a plain function pointer so the scene layer
// never needs to know about PovRayMaterial.
static void
releasePovRayObjectTexture(Material *objectTexture)
{
    static_cast<PovRayMaterial *>(objectTexture)->releaseFromOwner();
}

Material *
ObjectParser::ensurePrivateTexture(Material *objectTexture)
{
    PovRayMaterial *texture = static_cast<PovRayMaterial *>(objectTexture);
    bool needsTransform =
        (texture->getPigment() != nullptr && texture->getPigment()->needsTransform()) ||
        (texture->getNormal() != nullptr);
    if (needsTransform && PovRayMaterialConstancy::isConstant(texture)) {
        // Safe no-op if `texture` isn't the tracked default-texture alias
        // (e.g. it's some other #declare'd TEXTURE_CONSTANT).
        DefaultTextureAliasTracker::releaseAlias(texture);
        return new PovRayMaterial(*texture);
    }
    return objectTexture;
}

RayCastingComposite *
ObjectParser::newComposite(Material *objectTexture)
{
    java::ArrayList<PovrayBody*> none(0);
    return new RayCastingComposite(
        nullptr, nullptr, objectTexture, nullptr, false, none, none, none,
        nullptr, nullptr, nullptr, nullptr, releasePovRayObjectTexture);
}

bool
ObjectParser::transformsBodyLayer(const PovrayBody *object)
{
    return object->getCsg() != nullptr;
}

SimpleBodyBuilder *
ObjectParser::parseShape()
{
    ParserContext ctx;
    return ObjectParser::parseShape(ctx);
}

PovrayBody *
ObjectParser::parseObject()
{
    ParserContext ctx;
    return ObjectParser::parseObject(ctx);
}

PovrayBody *
ObjectParser::parseComposite()
{
    ParserContext ctx;
    return ObjectParser::parseComposite(ctx);
}

SimpleBodyBuilder *
ObjectParser::parseShape(ParserContext &ctx)
{
    SimpleBodyBuilder *localShape = nullptr;

    {
        bool Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::LIGHT_SOURCE_TOKEN:
                localShape = SceneBuilder::wrap(LightSourceParser::parseLightSource(ctx));
                Exit_Flag = true;
                break;

            case Tokenizer::SPHERE_TOKEN:
                localShape = SphereParser::parseSphere(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::PLANE_TOKEN:
                localShape = PlaneParser::parsePlane(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::TRIANGLE_TOKEN:
                localShape = TriangleParser::parseTriangle(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::QUADRIC_TOKEN:
                localShape = QuadricParser::parseQuadric(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::HEIGHT_FIELD_TOKEN:
                localShape = HeightFieldParser::parseHeightField(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::CUBIC_TOKEN:
                localShape = PolyParser::parsePoly(3, ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::QUARTIC_TOKEN:
                localShape = PolyParser::parsePoly(4, ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::POLY_TOKEN:
                localShape = PolyParser::parsePoly(0, ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::BOX_TOKEN:
                localShape = BoxParser::parseBox(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::BLOB_TOKEN:
                localShape = BlobParser::parseBlob(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::BICUBIC_PATCH_TOKEN:
                localShape = BicubicPatchParser::parseBicubicPatch(ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::UNION_TOKEN:
                localShape = CsgParser::parse(CsgOperation::UNION, ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::INTERSECTION_TOKEN:
                localShape = CsgParser::parse(CsgOperation::INTERSECTION, ctx);
                Exit_Flag = true;
                break;

            case Tokenizer::DIFFERENCE_TOKEN:
                localShape = CsgParser::parse(CsgOperation::DIFFERENCE, ctx);
                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::QUADRIC_TOKEN, ctx);
                break;
            }
        }
    }
    return (localShape);
}

PovrayBody *
ObjectParser::parseObject(ParserContext &ctx)
{
    PovrayBody *object = nullptr;
    SimpleBodyBuilder *localShape;
    Material *defaultTexture = ctx.getDefaultTexture();
    DefaultTextureAliasTracker::trackAlias(static_cast<PovRayMaterial *>(defaultTexture));

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    ctx.tokenStream().getToken();
    switch (ctx.token().getTokenId()) {
    case Tokenizer::IDENTIFIER_TOKEN:
    {
        int constantId;
        if ((constantId = ctx.findConstant()) != -1) {
            if (ctx.constants()[(int)constantId].getConstantType() ==
                ParseGlobals::OBJECT_CONSTANT) {
                object = new PovrayBody(
                    *(PovrayBody *)ctx.constants()[(int)constantId].getConstantData());
            } else {
                ParseErrorReporter::typeError(ctx);
            }
        } else {
            ParseErrorReporter::reportUndeclared(ctx);
        }
        break;
    }

    case Tokenizer::SPHERE_TOKEN:
    case Tokenizer::QUADRIC_TOKEN:
    case Tokenizer::QUARTIC_TOKEN:
    case Tokenizer::UNION_TOKEN:
    case Tokenizer::INTERSECTION_TOKEN:
    case Tokenizer::DIFFERENCE_TOKEN:
    case Tokenizer::TRIANGLE_TOKEN:
    case Tokenizer::SMOOTH_TRIANGLE_TOKEN:
    case Tokenizer::PLANE_TOKEN:
    case Tokenizer::CUBIC_TOKEN:
    case Tokenizer::POLY_TOKEN:
    case Tokenizer::BICUBIC_PATCH_TOKEN:
    case Tokenizer::HEIGHT_FIELD_TOKEN:
    case Tokenizer::LIGHT_SOURCE_TOKEN:
    case Tokenizer::BOX_TOKEN:
    case Tokenizer::BLOB_TOKEN:
        ctx.tokenStream().ungetToken();
        localShape = ObjectParser::parseShape(ctx);
        object = localShape->buildBody(defaultTexture, releasePovRayObjectTexture);
        delete localShape;
        break;

    default:
        ParseErrorReporter::parseError(Tokenizer::SHAPE_TOKEN, ctx);
        break;
    }
    if (object == nullptr) {
        java::ArrayList<PovrayBody*> none(0);
        object = new PovrayBody(
            nullptr, nullptr, defaultTexture, nullptr, false, none, none,
            nullptr, nullptr, nullptr, nullptr,
            java::ArrayList<TransformStep>(), java::ArrayList<TransformStep>(),
            releasePovRayObjectTexture);
    }

    parseObjectModifiers(object, true, ctx);
    return object;
}

void
ObjectParser::parseRegions(java::ArrayList<PovrayBody*> &regions, ParserContext &ctx)
{
    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);
    while (true) {
        ctx.tokenStream().getToken();
        if (ctx.token().getTokenId() == Tokenizer::RIGHT_CURLY_TOKEN) {
            return;
        }
        ctx.tokenStream().ungetToken();
        SimpleBodyBuilder *localShape = ObjectParser::parseShape(ctx);
        regions.add(localShape->buildRegion());
        delete localShape;
    }
}

void
ObjectParser::parseObjectModifiers(PovrayBody *object, bool acceptsSurface, ParserContext &ctx)
{
    Vector3Dd localVector;
    bool Exit_Flag = false;
    while (!Exit_Flag) {
        ctx.tokenStream().getToken();
        switch (ctx.token().getTokenId()) {
        case Tokenizer::BOUNDED_TOKEN:
            parseRegions(object->getBoundingShapes(), ctx);
            break;

        case Tokenizer::CLIPPED_TOKEN:
            parseRegions(object->getClippingShapes(), ctx);
            break;

        case Tokenizer::COLOUR_TOKEN:
            if (!acceptsSurface) {
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
            object->setObjectColor(new ColorRgba(0.0, 0.0, 0.0, 0.0));
            PrimitiveParser::parseColor(object->getObjectColor(), ctx);
            break;

        case Tokenizer::TEXTURE_TOKEN:
        {
            if (!acceptsSurface) {
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
            PovRayMaterial *localTexture = TextureParser::parseTexture(ctx);
            if (PovRayMaterialConstancy::isConstant(localTexture)) {
                localTexture = TextureParser::copyTexture(localTexture);
            }
            // Object transforms parsed *before* this texture block are not
            // applied to it: in POV 1.0 they hit the still-default texture,
            // whose translate/rotate/scale are a deliberate no-op (see
            // PovRayMaterial::translateTexture). Transforms that follow the
            // texture reach it through the body's translate/rotate/scale.
            Material *objectTexture = object->getObjectTexture();
            if (objectTexture == ctx.getDefaultTexture()) {
                DefaultTextureAliasTracker::releaseAlias(
                    static_cast<PovRayMaterial *>(objectTexture));
                objectTexture = localTexture;
            } else {
                PovRayMaterialUtils::prependTextureLayers(localTexture, objectTexture);
            }
            object->setObjectTexture(objectTexture);
            break;
        }

        case Tokenizer::NO_SHADOW_TOKEN:
            if (!acceptsSurface) {
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
            object->setNoShadowFlag(true);
            break;

        case Tokenizer::LIGHT_SOURCE_TOKEN:
            if (!acceptsSurface) {
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
            ParseErrorReporter::reportError(
                "Light source must be defined using new syntax", ctx);
            break;

        case Tokenizer::TRANSLATE_TOKEN:
            object->setObjectTexture(ensurePrivateTexture(object->getObjectTexture()));
            PrimitiveParser::parseVector(&localVector, ctx);
            if (!acceptsSurface || transformsBodyLayer(object)) {
                object->translate(&localVector);
            } else {
                object->translateGeometryLayer(&localVector);
            }
            break;

        case Tokenizer::ROTATE_TOKEN:
            object->setObjectTexture(ensurePrivateTexture(object->getObjectTexture()));
            PrimitiveParser::parseVector(&localVector, ctx);
            if (!acceptsSurface || transformsBodyLayer(object)) {
                object->rotate(&localVector);
            } else {
                object->rotateGeometryLayer(&localVector);
            }
            break;

        case Tokenizer::SCALE_TOKEN:
            object->setObjectTexture(ensurePrivateTexture(object->getObjectTexture()));
            PrimitiveParser::parseVector(&localVector, ctx);
            if (!acceptsSurface || transformsBodyLayer(object)) {
                object->scale(&localVector);
            } else {
                object->scaleGeometryLayer(&localVector);
            }
            break;

        case Tokenizer::INVERSE_TOKEN:
            object->invert();
            break;

        case Tokenizer::RIGHT_CURLY_TOKEN:
            Exit_Flag = true;
            break;

        default:
            ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
            break;
        }
    }
}

PovrayBody *
ObjectParser::parseComposite(ParserContext &ctx)
{
    Material *defaultTexture = ctx.getDefaultTexture();
    DefaultTextureAliasTracker::trackAlias(static_cast<PovRayMaterial *>(defaultTexture));
    RayCastingComposite *composite = newComposite(defaultTexture);

    ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

    {
        bool Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::IDENTIFIER_TOKEN:
            {
                int constantId;
                if ((constantId = ctx.findConstant()) != -1) {
                    if (ctx.constants()[(int)constantId].getConstantType() ==
                        ParseGlobals::COMPOSITE_CONSTANT) {
                        // A declared composite replaces whatever was parsed
                        // before it in this block.
                        delete composite;
                        composite = new RayCastingComposite(
                            *(RayCastingComposite *)ctx.constants()[(int)constantId]
                                .getConstantData());
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                break;
            }

            case Tokenizer::COMPOSITE_TOKEN:
                composite->getSimpleBodies().add(ObjectParser::parseComposite(ctx));
                break;

            case Tokenizer::OBJECT_TOKEN:
                composite->getSimpleBodies().add(ObjectParser::parseObject(ctx));
                break;

            default:
                ctx.tokenStream().ungetToken();
                Exit_Flag = true;
                break;
            }
        }
    }

    parseObjectModifiers(composite, false, ctx);
    return composite;
}
