#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

#include "environment/camera/Camera.h"
#include "environment/geometry/SimpleBody.h"
#include "environment/geometry/element/Triangle.h"
#include "environment/geometry/surface/parametric/ParametricPatch.h"
#include "environment/geometry/volume/Blob.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/HeightField.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/light/Light.h"
#include "environment/material/PovRayMaterialUtils.h"
#include "environment/material/pigment/SolidTexturePigment.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/scene/BoundedGeometryFactory.h"
#include "environment/scene/SceneBuilder.h"
#include "io/pov/light/LightGeometryAdapter.h"

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
#include "io/pov/material/PovRayMaterialConstancy.h"
#include "io/pov/material/TextureParser.h"
#include "io/pov/parser/ParseErrorReporter.h"
#include "io/pov/parser/ParseHelpers.h"
#include "io/pov/parser/PrimitiveParser.h"

namespace {

// An untextured object/composite starts out wearing the scene's shared default
// texture directly (see objectTexture initialization below). Transforming it in
// place would mutate that shared instance for every other object relying on it,
// so callers must copy it out first whenever it is still the canonical instance
// AND the transform would actually touch it (matching PovRayMaterial's own
// needsTransform gate) -- copying unconditionally would make objectTexture no
// longer pointer-equal to ctx.getDefaultTexture(), which the TEXTURE_TOKEN
// handling below relies on to decide between replacing and layering.
Material *
ensurePrivateTexture(Material *objectTexture)
{
    PovRayMaterial *texture = static_cast<PovRayMaterial *>(objectTexture);
    bool needsTransform =
        (texture->getPigment() != nullptr && texture->getPigment()->needsTransform()) ||
        (texture->getNormal() != nullptr);
    if (needsTransform && PovRayMaterialConstancy::isConstant(texture)) {
        return texture->copy();
    }
    return objectTexture;
}

BoundedGeometry *
buildObject(
    TransformableElement *geometry,
    Material *objectTexture,
    ColorRgba *objectColor,
    bool noShadowFlag,
    const java::ArrayList<TransformableElement*> &boundingShapes,
    const java::ArrayList<TransformableElement*> &clippingShapes)
{
    return new BoundedGeometry(
        geometry, objectTexture, objectColor, noShadowFlag, boundingShapes,
        clippingShapes);
}

Composite *
buildComposite(
    TransformableElement *geometry,
    Material *objectTexture,
    ColorRgba *objectColor,
    bool noShadowFlag,
    const java::ArrayList<TransformableElement*> &boundingShapes,
    const java::ArrayList<TransformableElement*> &clippingShapes,
    const java::ArrayList<BoundedGeometry*> &simpleBodies)
{
    return new Composite(
        geometry, objectTexture, objectColor, noShadowFlag, boundingShapes,
        clippingShapes, simpleBodies);
}

void
extractObjectState(
    BoundedGeometry *object,
    TransformableElement *&geometry,
    Material *&objectTexture,
    ColorRgba *&objectColor,
    bool &noShadowFlag,
    java::ArrayList<TransformableElement*> &boundingShapes,
    java::ArrayList<TransformableElement*> &clippingShapes)
{
    geometry = object->getGeometry();
    objectTexture = object->getObjectTexture();
    objectColor = object->getObjectColor();
    noShadowFlag = object->getNoShadowFlag();
    boundingShapes = object->getBoundingShapes();
    clippingShapes = object->getClippingShapes();
}

void
extractCompositeState(
    Composite *object,
    TransformableElement *&geometry,
    Material *&objectTexture,
    ColorRgba *&objectColor,
    bool &noShadowFlag,
    java::ArrayList<TransformableElement*> &boundingShapes,
    java::ArrayList<TransformableElement*> &clippingShapes,
    java::ArrayList<BoundedGeometry*> &simpleBodies)
{
    extractObjectState(
        object, geometry, objectTexture, objectColor, noShadowFlag,
        boundingShapes, clippingShapes);
    simpleBodies = object->getSimpleBodies();
}

}

SimpleBody *
ObjectParser::parseShape()
{
    ParserContext ctx;
    return ObjectParser::parseShape(ctx);
}

BoundedGeometry *
ObjectParser::parseObject()
{
    ParserContext ctx;
    return ObjectParser::parseObject(ctx);
}

BoundedGeometry *
ObjectParser::parseComposite()
{
    ParserContext ctx;
    return ObjectParser::parseComposite(ctx);
}

SimpleBody *
ObjectParser::parseShape(ParserContext &ctx)
{
    SimpleBody *localShape = nullptr;

    {
        bool Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::LIGHT_SOURCE_TOKEN:
                localShape = SceneBuilder::wrap(
                    new LightGeometryAdapter(LightSourceParser::parseLightSource(ctx)));
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

            case Tokenizer::SMOOTH_TRIANGLE_TOKEN:
                localShape = SmoothTriangleParser::parseSmoothTriangle(ctx);
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
                localShape =
                    SceneBuilder::wrap(CsgParser::parse(BooleanSetOperations::UNION, ctx));
                Exit_Flag = true;
                break;

            case Tokenizer::INTERSECTION_TOKEN:
                localShape =
                    SceneBuilder::wrap(CsgParser::parse(BooleanSetOperations::INTERSECTION, ctx));
                Exit_Flag = true;
                break;

            case Tokenizer::DIFFERENCE_TOKEN:
                localShape =
                    SceneBuilder::wrap(CsgParser::parse(BooleanSetOperations::DIFFERENCE, ctx));
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

BoundedGeometry *
ObjectParser::parseObject(ParserContext &ctx)
{
    BoundedGeometry *object = nullptr;
    SimpleBody *localShape;
    TransformableElement *geometry;
    java::ArrayList<TransformableElement*> localBoundingShapes(4);
    java::ArrayList<TransformableElement*> localClippingShapes(4);
    Vector3Dd localVector;
    ColorRgba *objectColor = nullptr;
    bool noShadowFlag = false;
    Material *objectTexture = ctx.getDefaultTexture();

    geometry = nullptr;
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
                        ParseGlobals::OBJECT_CONSTANT) {
                        object = new BoundedGeometry(
                            *(BoundedGeometry *)ctx.constants()[(int)constantId]
                                .getConstantData());
                        extractObjectState(
                            object, geometry, objectTexture, objectColor,
                            noShadowFlag, localBoundingShapes,
                            localClippingShapes);
                        object->detachOwnership();
                        delete object;
                        object = nullptr;
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                Exit_Flag = true;
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
                if (geometry == nullptr) {
                    geometry = localShape;
                }
                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::SHAPE_TOKEN, ctx);
                Exit_Flag = true;
                break;
            }
        }
    }

    {
        bool Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::BOUNDED_TOKEN:

                ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                {
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().getTokenId()) {
                        case Tokenizer::RIGHT_CURLY_TOKEN:
                            Exit_Flag = true;
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            localShape = ObjectParser::parseShape(ctx);
                            localBoundingShapes.add(localShape);
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::CLIPPED_TOKEN:

                ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                {
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().getTokenId()) {
                        case Tokenizer::RIGHT_CURLY_TOKEN:
                            Exit_Flag = true;
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            localShape = ObjectParser::parseShape(ctx);
                            localClippingShapes.add(localShape);
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::COLOUR_TOKEN:
                objectColor = new ColorRgba(0.0, 0.0, 0.0, 0.0);
                PrimitiveParser::parseColor(objectColor, ctx);
                break;

            case Tokenizer::TEXTURE_TOKEN:
            {
                PovRayMaterial *localTexture = TextureParser::parseTexture(ctx);
                if (PovRayMaterialConstancy::isConstant(localTexture)) {
                    localTexture = TextureParser::copyTexture(localTexture);
                }

                if (objectTexture == ctx.getDefaultTexture()) {
                    objectTexture = localTexture;
                } else {
                    PovRayMaterialUtils::prependTextureLayers(localTexture, objectTexture);
                }
                break;
            }

            case Tokenizer::NO_SHADOW_TOKEN:
                noShadowFlag = true;
                break;

            case Tokenizer::LIGHT_SOURCE_TOKEN:
                ParseErrorReporter::reportError(
                    "Light source must be defined using new syntax", ctx);
                break;

            case Tokenizer::TRANSLATE_TOKEN:
            {
                objectTexture = ensurePrivateTexture(objectTexture);
                object = buildObject(
                    geometry, objectTexture, objectColor, noShadowFlag,
                    localBoundingShapes, localClippingShapes);
                PrimitiveParser::parseVector(&localVector, ctx);
                object->translate(&localVector);
                extractObjectState(
                    object, geometry, objectTexture, objectColor,
                    noShadowFlag, localBoundingShapes, localClippingShapes);
                object->detachOwnership();
                delete object;
                object = nullptr;
                break;
            }

            case Tokenizer::ROTATE_TOKEN:
            {
                objectTexture = ensurePrivateTexture(objectTexture);
                object = buildObject(
                    geometry, objectTexture, objectColor, noShadowFlag,
                    localBoundingShapes, localClippingShapes);
                PrimitiveParser::parseVector(&localVector, ctx);
                object->rotate(&localVector);
                extractObjectState(
                    object, geometry, objectTexture, objectColor,
                    noShadowFlag, localBoundingShapes, localClippingShapes);
                object->detachOwnership();
                delete object;
                object = nullptr;
                break;
            }

            case Tokenizer::SCALE_TOKEN:
            {
                objectTexture = ensurePrivateTexture(objectTexture);
                object = buildObject(
                    geometry, objectTexture, objectColor, noShadowFlag,
                    localBoundingShapes, localClippingShapes);
                PrimitiveParser::parseVector(&localVector, ctx);
                object->scale(&localVector);
                extractObjectState(
                    object, geometry, objectTexture, objectColor,
                    noShadowFlag, localBoundingShapes, localClippingShapes);
                object->detachOwnership();
                delete object;
                object = nullptr;
                break;
            }

            case Tokenizer::INVERSE_TOKEN:
            {
                object = buildObject(
                    geometry, objectTexture, objectColor, noShadowFlag,
                    localBoundingShapes, localClippingShapes);
                object->invert();
                extractObjectState(
                    object, geometry, objectTexture, objectColor,
                    noShadowFlag, localBoundingShapes, localClippingShapes);
                object->detachOwnership();
                delete object;
                object = nullptr;
                break;
            }

            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return buildObject(
        geometry, objectTexture, objectColor, noShadowFlag,
        localBoundingShapes, localClippingShapes);
}

BoundedGeometry *
ObjectParser::parseComposite(ParserContext &ctx)
{
    Composite *localComposite = nullptr;
    BoundedGeometry *localObject;
    SimpleBody *localShape;
    TransformableElement *geometry;
    java::ArrayList<BoundedGeometry*> localSimpleBodies(4);
    java::ArrayList<TransformableElement*> localBoundingShapes(4);
    java::ArrayList<TransformableElement*> localClippingShapes(4);
    Vector3Dd localVector;
    Material *objectTexture = ctx.getDefaultTexture();
    ColorRgba *objectColor = nullptr;
    bool noShadowFlag = false;

    geometry = nullptr;
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
                        localComposite = new Composite(
                            *(Composite *)ctx.constants()[(int)constantId]
                                .getConstantData());
                        extractCompositeState(
                            localComposite, geometry, objectTexture,
                            objectColor, noShadowFlag, localBoundingShapes,
                            localClippingShapes, localSimpleBodies);
                        localComposite->detachOwnership();
                        delete localComposite;
                        localComposite = nullptr;
                    } else {
                        ParseErrorReporter::typeError(ctx);
                    }
                } else {
                    ParseErrorReporter::reportUndeclared(ctx);
                }
                break;
            }

            case Tokenizer::COMPOSITE_TOKEN:
                localObject = ObjectParser::parseComposite(ctx);
                localSimpleBodies.add(localObject);
                break;

            case Tokenizer::OBJECT_TOKEN:
                localObject = ObjectParser::parseObject(ctx);
                localSimpleBodies.add(localObject);
                break;

            case Tokenizer::RIGHT_CURLY_TOKEN:
                ctx.tokenStream().ungetToken();
                Exit_Flag = true;
                break;

            default:
                ctx.tokenStream().ungetToken();
                Exit_Flag = true;
                break;
            }
        }
    }

    {
        bool Exit_Flag = false;
        while (!Exit_Flag) {
            ctx.tokenStream().getToken();
            switch (ctx.token().getTokenId()) {
            case Tokenizer::RIGHT_CURLY_TOKEN:
                Exit_Flag = true;
                break;

            case Tokenizer::BOUNDED_TOKEN:

                ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                {
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().getTokenId()) {
                        case Tokenizer::RIGHT_CURLY_TOKEN:
                            Exit_Flag = true;
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            localShape = ObjectParser::parseShape(ctx);
                            localBoundingShapes.add(localShape);
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::CLIPPED_TOKEN:

                ParseHelpers::getExpectedToken(Tokenizer::LEFT_CURLY_TOKEN, ctx);

                {
                    bool Exit_Flag;
                    Exit_Flag = false;
                    while (!Exit_Flag) {
                        ctx.tokenStream().getToken();
                        switch (ctx.token().getTokenId()) {
                        case Tokenizer::RIGHT_CURLY_TOKEN:
                            Exit_Flag = true;
                            break;

                        default:
                            ctx.tokenStream().ungetToken();
                            localShape = ObjectParser::parseShape(ctx);
                            localClippingShapes.add(localShape);
                            break;
                        }
                    }
                }
                break;

            case Tokenizer::TRANSLATE_TOKEN:
            {
                objectTexture = ensurePrivateTexture(objectTexture);
                localComposite = buildComposite(
                    geometry, objectTexture, objectColor, noShadowFlag,
                    localBoundingShapes, localClippingShapes,
                    localSimpleBodies);
                PrimitiveParser::parseVector(&localVector, ctx);
                localComposite->translate(&localVector);
                extractCompositeState(
                    localComposite, geometry, objectTexture, objectColor,
                    noShadowFlag, localBoundingShapes, localClippingShapes,
                    localSimpleBodies);
                localComposite->detachOwnership();
                delete localComposite;
                localComposite = nullptr;
                break;
            }

            case Tokenizer::ROTATE_TOKEN:
            {
                objectTexture = ensurePrivateTexture(objectTexture);
                localComposite = buildComposite(
                    geometry, objectTexture, objectColor, noShadowFlag,
                    localBoundingShapes, localClippingShapes,
                    localSimpleBodies);
                PrimitiveParser::parseVector(&localVector, ctx);
                localComposite->rotate(&localVector);
                extractCompositeState(
                    localComposite, geometry, objectTexture, objectColor,
                    noShadowFlag, localBoundingShapes, localClippingShapes,
                    localSimpleBodies);
                localComposite->detachOwnership();
                delete localComposite;
                localComposite = nullptr;
                break;
            }

            case Tokenizer::SCALE_TOKEN:
            {
                objectTexture = ensurePrivateTexture(objectTexture);
                localComposite = buildComposite(
                    geometry, objectTexture, objectColor, noShadowFlag,
                    localBoundingShapes, localClippingShapes,
                    localSimpleBodies);
                PrimitiveParser::parseVector(&localVector, ctx);
                localComposite->scale(&localVector);
                extractCompositeState(
                    localComposite, geometry, objectTexture, objectColor,
                    noShadowFlag, localBoundingShapes, localClippingShapes,
                    localSimpleBodies);
                localComposite->detachOwnership();
                delete localComposite;
                localComposite = nullptr;
                break;
            }

            case Tokenizer::INVERSE_TOKEN:
            {
                localComposite = buildComposite(
                    geometry, objectTexture, objectColor, noShadowFlag,
                    localBoundingShapes, localClippingShapes,
                    localSimpleBodies);
                localComposite->invert();
                extractCompositeState(
                    localComposite, geometry, objectTexture, objectColor,
                    noShadowFlag, localBoundingShapes, localClippingShapes,
                    localSimpleBodies);
                localComposite->detachOwnership();
                delete localComposite;
                localComposite = nullptr;
                break;
            }

            default:
                ParseErrorReporter::parseError(Tokenizer::RIGHT_CURLY_TOKEN, ctx);
                break;
            }
        }
    }

    return buildComposite(
        geometry, objectTexture, objectColor, noShadowFlag,
        localBoundingShapes, localClippingShapes, localSimpleBodies);
}
