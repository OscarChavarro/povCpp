#include "io/pov/ast/AstSceneBuilder.h"

#include <cmath>

#include "common/color/Color.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/camera/Camera.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/elements/Triangle.h"
#include "environment/geometry/elements/SmoothTriangle.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/Blob.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/light/Light.h"
#include "environment/material/RenderRuntimeState.h"
#include "environment/scene/ModelBuilder.h"
#include "environment/scene/SceneFrame.h"
#include "environment/scene/SimpleBodyFactory.h"
#include "io/pov/ParseErrorReporter.h"
#include "io/pov/ParseGlobals.h"
#include "io/pov/ParserConstants.h"
#include "io/pov/ParserContext.h"
#include "io/pov/ast/AstNodes.h"
#include "io/pov/mediaParser/TextureParser.h"
#include "media/Texture.h"
#include "media/TextureUtils.h"

Vector3Dd
AstSceneBuilder::asVector(const AstVector3 &v)
{
    return Vector3Dd(v.x, v.y, v.z);
}

void
AstSceneBuilder::applyTransforms(Geometry *shape, const AstTransform *transforms, int count)
{
    for (int i = 0; i < count; ++i) {
        const AstTransform &t = transforms[i];
        Vector3Dd v = asVector(t.vectorValue);
        switch (t.kind) {
        case AST_TRANSLATE:
            GeometryOperations::translate((SimpleBody *)shape, &v);
            break;
        case AST_ROTATE:
            GeometryOperations::rotate((SimpleBody *)shape, &v);
            break;
        case AST_SCALE:
            GeometryOperations::scale((SimpleBody *)shape, &v);
            break;
        case AST_INVERSE:
            GeometryOperations::invert((SimpleBody *)shape);
            break;
        default:
            break;
        }
    }
}

static void
applyTransformsRange(Geometry *shape, const AstTransform *transforms, int begin, int end)
{
    if (shape == nullptr || transforms == nullptr) {
        return;
    }
    if (begin < 0) {
        begin = 0;
    }
    if (end < begin) {
        return;
    }
    for (int i = begin; i < end; ++i) {
        const AstTransform &t = transforms[i];
        Vector3Dd v(t.vectorValue.x, t.vectorValue.y, t.vectorValue.z);
        switch (t.kind) {
        case AST_TRANSLATE:
            GeometryOperations::translate((SimpleBody *)shape, &v);
            break;
        case AST_ROTATE:
            GeometryOperations::rotate((SimpleBody *)shape, &v);
            break;
        case AST_SCALE:
            GeometryOperations::scale((SimpleBody *)shape, &v);
            break;
        case AST_INVERSE:
            GeometryOperations::invert((SimpleBody *)shape);
            break;
        default:
            break;
        }
    }
}

static void
applyShapeTexture(Texture *srcTexture, Geometry *shape)
{
    if (srcTexture == nullptr || shape == nullptr) {
        return;
    }
    Texture *localTexture = TextureParser::copyTexture(srcTexture);
    Texture *tail = localTexture;
    while (tail->Next_Texture != nullptr) {
        tail = tail->Next_Texture;
    }
    tail->Next_Texture = shape->Shape_Texture;
    shape->Shape_Texture = localTexture;
}

static void
applyObjectTexture(Texture *srcTexture, SimpleBody *object)
{
    if (srcTexture == nullptr || object == nullptr) {
        return;
    }
    Texture *localTexture = TextureParser::copyTexture(srcTexture);
    if (object->objectTexture == TextureUtils::defaultTexture()) {
        object->objectTexture = localTexture;
    } else {
        Texture *tail = localTexture;
        while (tail->Next_Texture != nullptr) {
            tail = tail->Next_Texture;
        }
        tail->Next_Texture = object->objectTexture;
        object->objectTexture = localTexture;
    }
}

void
AstSceneBuilder::applyFog(const AstFogNode &node, RenderFrame *framePtr)
{
    if (node.hasColour) {
        framePtr->fogColour.Red = node.colour.r;
        framePtr->fogColour.Green = node.colour.g;
        framePtr->fogColour.Blue = node.colour.b;
        framePtr->fogColour.Alpha = node.colour.a;
    }
    if (node.hasDistance) {
        framePtr->fogDistance = node.distance;
    }
}

void
AstSceneBuilder::applyCamera(const AstCameraNode &node, RenderFrame *framePtr, ParserContext &ctx,
    const AstDeclTable &decls)
{
    Camera *camera = &(framePtr->viewPoint);
    camera->initializeDefaults();
    for (int i = 0; i < node.opCount; ++i) {
        const AstCameraOp &op = node.ops[i];
        Vector3Dd v = asVector(op.vectorValue);
        if (op.kind == AST_CAMERA_REF) {
            const AstNode *declNode = findDecl(decls, op.referenceConstantId);
            if (declNode != nullptr) {
                if (declNode->kind != AST_CAMERA_NODE) {
                    ParseErrorReporter::Error("Invalid camera reference in AST", ctx);
                }
                const AstCameraNode &declCamera = (const AstCameraNode &)*declNode;
                applyCamera(declCamera, framePtr, ctx, decls);
            } else {
                const Constant *legacyDecl = findLegacyDecl(ctx, op.referenceConstantId);
                if (legacyDecl == nullptr ||
                    legacyDecl->constantType != ParseGlobals::VIEW_POINT_CONSTANT) {
                    ParseErrorReporter::Error("Invalid camera reference in AST", ctx);
                }
                *camera = *((Camera *)legacyDecl->constantData);
            }
        } else if (op.kind == AST_CAMERA_LOCATION) {
            camera->Location = v;
        } else if (op.kind == AST_CAMERA_DIRECTION) {
            camera->Direction = v;
        } else if (op.kind == AST_CAMERA_UP) {
            camera->Up = v;
        } else if (op.kind == AST_CAMERA_RIGHT) {
            camera->Right = v;
        } else if (op.kind == AST_CAMERA_SKY) {
            camera->Sky = v;
        } else if (op.kind == AST_CAMERA_LOOK_AT) {
            const double directionLength = camera->Direction.length();
            const double upLength = camera->Up.length();
            const double rightLength = camera->Right.length();
            Vector3Dd tempVector = camera->Direction.crossProduct(camera->Up);
            const double handedness = tempVector.dotProduct(camera->Right);
            camera->Direction = v;
            camera->Direction.sub(camera->Location);
            camera->Direction.normalize();
            camera->Right = camera->Direction.crossProduct(camera->Sky);
            camera->Right.normalize();
            camera->Up = camera->Right.crossProduct(camera->Direction);
            camera->Direction.scale(directionLength);
            if (handedness >= 0.0) {
                camera->Right.scale(rightLength);
            } else {
                camera->Right.scale(-rightLength);
            }
            camera->Up.scale(upLength);
        } else if (op.kind == AST_CAMERA_TRANSLATE) {
            GeometryOperations::translate((SimpleBody *)camera, &v);
        } else if (op.kind == AST_CAMERA_ROTATE) {
            GeometryOperations::rotate((SimpleBody *)camera, &v);
        } else if (op.kind == AST_CAMERA_SCALE) {
            GeometryOperations::scale((SimpleBody *)camera, &v);
        }
    }
}

void
AstSceneBuilder::applyMaxTraceLevel(const AstMaxTraceLevelNode &node)
{
    RenderRuntimeState::maxTraceLevel() = node.value;
}

void
AstSceneBuilder::applyDefaultTexture(const AstDefaultTextureNode &node)
{
    if (node.texture == nullptr) {
        return;
    }
    node.texture->constantFlag = LegacyBoolean::FALSE_VALUE;
    TextureUtils::defaultTexture() = node.texture;
    TextureUtils::defaultTexture()->constantFlag = LegacyBoolean::TRUE_VALUE;
}

const AstNode *
AstSceneBuilder::findDecl(const AstDeclTable &decls, int identifierNumber)
{
    for (int i = 0; i < decls.count; ++i) {
        if (decls.ids[i] == identifierNumber) {
            return decls.nodes[i];
        }
    }
    return nullptr;
}

const Constant *
AstSceneBuilder::findLegacyDecl(ParserContext &ctx, int identifierNumber)
{
    const int constantId = ctx.symbols().findByIdentifierNumber(identifierNumber);
    if (constantId == -1) {
        return nullptr;
    }
    return ctx.symbols().byConstantId(constantId);
}

Sphere *
AstSceneBuilder::buildSphere(const AstSphereNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    Sphere *shape = nullptr;

    if (node.hasReference) {
        const AstNode *declNode = findDecl(decls, node.referenceConstantId);
        if (declNode != nullptr) {
            if (declNode->kind != AST_SPHERE_NODE) {
                ParseErrorReporter::Error("Invalid sphere reference in AST", ctx);
            }
            Sphere *declShape = buildSphere((const AstSphereNode &)*declNode, ctx, decls);
            shape = (Sphere *)GeometryOperations::copy((SimpleBody *)declShape);
            delete declShape;
        } else {
            const Constant *legacyDecl = findLegacyDecl(ctx, node.referenceConstantId);
            if (legacyDecl == nullptr || legacyDecl->constantType != ParseGlobals::SPHERE_CONSTANT) {
                ParseErrorReporter::Error("Invalid sphere reference in AST", ctx);
            }
            shape = (Sphere *)GeometryOperations::copy((SimpleBody *)legacyDecl->constantData);
        }
    } else {
        shape = ModelBuilder::getSphereShape();
        if (node.hasInlineData) {
            shape->Center = asVector(node.center);
            shape->Radius = node.radius;
            shape->radiusSquared = shape->Radius * shape->Radius;
            shape->inverseRadius = 1.0 / shape->Radius;
        }
    }

    if (node.hasColour) {
        shape->Shape_Colour = ModelBuilder::getColour();
        shape->Shape_Colour->Red = node.colour.r;
        shape->Shape_Colour->Green = node.colour.g;
        shape->Shape_Colour->Blue = node.colour.b;
        shape->Shape_Colour->Alpha = node.colour.a;
    }
    if (node.hasTexture && node.textureTransformIndex >= 0) {
        applyTransformsRange((Geometry *)shape, node.transforms, 0, node.textureTransformIndex);
        applyShapeTexture(node.texture, (Geometry *)shape);
        applyTransformsRange(
            (Geometry *)shape, node.transforms, node.textureTransformIndex, node.transformCount);
    } else {
        if (node.hasTexture) {
            applyShapeTexture(node.texture, (Geometry *)shape);
        }
        applyTransforms((Geometry *)shape, node.transforms, node.transformCount);
    }
    return shape;
}

Light *
AstSceneBuilder::buildLight(
    const AstLightSourceNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    Light *shape = nullptr;

    if (node.hasReference) {
        const AstNode *declNode = findDecl(decls, node.referenceConstantId);
        if (declNode != nullptr) {
            if (declNode->kind != AST_LIGHT_SOURCE_NODE) {
                ParseErrorReporter::Error("Invalid light reference in AST", ctx);
            }
            Light *declShape =
                buildLight((const AstLightSourceNode &)*declNode, ctx, decls);
            shape = (Light *)GeometryOperations::copy((SimpleBody *)declShape);
            delete declShape;
        } else {
            const Constant *legacyDecl = findLegacyDecl(ctx, node.referenceConstantId);
            if (legacyDecl == nullptr ||
                legacyDecl->constantType != ParseGlobals::LIGHT_SOURCE_CONSTANT) {
                ParseErrorReporter::Error("Invalid light reference in AST", ctx);
            }
            shape = (Light *)GeometryOperations::copy((SimpleBody *)legacyDecl->constantData);
        }
    } else {
        shape = ModelBuilder::getLightSourceShape();
        if (node.hasInlineData) {
            shape->Center = asVector(node.center);
            if (shape->Shape_Colour == nullptr) {
                shape->Shape_Colour = ModelBuilder::getColour();
            }
            Color::makeColor(
                shape->Shape_Colour, node.colour.r, node.colour.g, node.colour.b);
            shape->Shape_Colour->Alpha = node.colour.a;
        }
    }

    shape->pointsAt = asVector(node.pointsAt);
    shape->Coeff = node.tightness;
    if (node.radiusDegrees > 0.0) {
        shape->Radius = cos(node.radiusDegrees * M_PI / 180.0);
    }
    if (node.falloffDegrees > 0.0) {
        shape->Falloff = cos(node.falloffDegrees * M_PI / 180.0);
    }
    if (node.hasSpotlight) {
        shape->Type = GeometryOperations::SPOT_LIGHT_TYPE;
    }

    applyTransforms((Geometry *)shape, node.transforms, node.transformCount);
    return shape;
}

InfinitePlane *
AstSceneBuilder::buildPlane(
    const AstPlaneNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    InfinitePlane *shape = nullptr;

    if (node.hasReference) {
        const AstNode *declNode = findDecl(decls, node.referenceConstantId);
        if (declNode != nullptr) {
            if (declNode->kind != AST_PLANE_NODE) {
                ParseErrorReporter::Error("Invalid plane reference in AST", ctx);
            }
            InfinitePlane *declShape =
                buildPlane((const AstPlaneNode &)*declNode, ctx, decls);
            shape = (InfinitePlane *)GeometryOperations::copy((SimpleBody *)declShape);
            delete declShape;
        } else {
            const Constant *legacyDecl = findLegacyDecl(ctx, node.referenceConstantId);
            if (legacyDecl == nullptr || legacyDecl->constantType != ParseGlobals::PLANE_CONSTANT) {
                ParseErrorReporter::Error("Invalid plane reference in AST", ctx);
            }
            shape = (InfinitePlane *)GeometryOperations::copy((SimpleBody *)legacyDecl->constantData);
        }
    } else {
        shape = ModelBuilder::getPlaneShape();
        if (node.hasInlineData) {
            shape->normalVector = asVector(node.normal);
            shape->Distance = -node.distance;
        }
    }

    if (node.hasColour) {
        shape->Shape_Colour = ModelBuilder::getColour();
        shape->Shape_Colour->Red = node.colour.r;
        shape->Shape_Colour->Green = node.colour.g;
        shape->Shape_Colour->Blue = node.colour.b;
        shape->Shape_Colour->Alpha = node.colour.a;
    }
    if (node.hasTexture && node.textureTransformIndex >= 0) {
        applyTransformsRange((Geometry *)shape, node.transforms, 0, node.textureTransformIndex);
        applyShapeTexture(node.texture, (Geometry *)shape);
        applyTransformsRange(
            (Geometry *)shape, node.transforms, node.textureTransformIndex, node.transformCount);
    } else {
        if (node.hasTexture) {
            applyShapeTexture(node.texture, (Geometry *)shape);
        }
        applyTransforms((Geometry *)shape, node.transforms, node.transformCount);
    }
    return shape;
}

Box *
AstSceneBuilder::buildBox(const AstBoxNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    Box *shape = nullptr;

    if (node.hasReference) {
        const AstNode *declNode = findDecl(decls, node.referenceConstantId);
        if (declNode != nullptr) {
            if (declNode->kind != AST_BOX_NODE) {
                ParseErrorReporter::Error("Invalid box reference in AST", ctx);
            }
            Box *declShape = buildBox((const AstBoxNode &)*declNode, ctx, decls);
            shape = (Box *)GeometryOperations::copy((SimpleBody *)declShape);
            delete declShape;
        } else {
            const Constant *legacyDecl = findLegacyDecl(ctx, node.referenceConstantId);
            if (legacyDecl == nullptr || legacyDecl->constantType != ParseGlobals::BOX_CONSTANT) {
                ParseErrorReporter::Error("Invalid box reference in AST", ctx);
            }
            shape = (Box *)GeometryOperations::copy((SimpleBody *)legacyDecl->constantData);
        }
    } else {
        shape = ModelBuilder::getBoxShape();
        if (node.hasInlineData) {
            shape->bounds[0] = asVector(node.minBound);
            shape->bounds[1] = asVector(node.maxBound);
        }
    }

    if (node.hasColour) {
        shape->Shape_Colour = ModelBuilder::getColour();
        shape->Shape_Colour->Red = node.colour.r;
        shape->Shape_Colour->Green = node.colour.g;
        shape->Shape_Colour->Blue = node.colour.b;
        shape->Shape_Colour->Alpha = node.colour.a;
    }
    if (node.hasTexture && node.textureTransformIndex >= 0) {
        applyTransformsRange((Geometry *)shape, node.transforms, 0, node.textureTransformIndex);
        applyShapeTexture(node.texture, (Geometry *)shape);
        applyTransformsRange(
            (Geometry *)shape, node.transforms, node.textureTransformIndex, node.transformCount);
    } else {
        if (node.hasTexture) {
            applyShapeTexture(node.texture, (Geometry *)shape);
        }
        applyTransforms((Geometry *)shape, node.transforms, node.transformCount);
    }
    return shape;
}

Quadric *
AstSceneBuilder::buildQuadric(
    const AstQuadricNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    Quadric *shape = nullptr;

    if (node.hasReference) {
        const AstNode *declNode = findDecl(decls, node.referenceConstantId);
        if (declNode != nullptr) {
            if (declNode->kind != AST_QUADRIC_NODE) {
                ParseErrorReporter::Error("Invalid quadric reference in AST", ctx);
            }
            Quadric *declShape = buildQuadric((const AstQuadricNode &)*declNode, ctx, decls);
            shape = (Quadric *)GeometryOperations::copy((SimpleBody *)declShape);
            delete declShape;
        } else {
            const Constant *legacyDecl = findLegacyDecl(ctx, node.referenceConstantId);
            if (legacyDecl == nullptr || legacyDecl->constantType != ParseGlobals::QUADRIC_CONSTANT) {
                ParseErrorReporter::Error("Invalid quadric reference in AST", ctx);
            }
            shape = (Quadric *)GeometryOperations::copy((SimpleBody *)legacyDecl->constantData);
        }
    } else {
        shape = ModelBuilder::getQuadricShape();
        if (node.hasInlineData) {
            shape->object2Terms = asVector(node.object2Terms);
            shape->objectMixedTerms = asVector(node.objectMixedTerms);
            shape->objectTerms = asVector(node.objectTerms);
            shape->objectConstant = node.objectConstant;
            shape->nonZeroSquareTerm =
                !((shape->object2Terms.x == 0.0) && (shape->object2Terms.y == 0.0) &&
                    (shape->object2Terms.z == 0.0) && (shape->objectMixedTerms.x == 0.0) &&
                    (shape->objectMixedTerms.y == 0.0) && (shape->objectMixedTerms.z == 0.0));
        }
    }

    if (node.hasColour) {
        shape->Shape_Colour = ModelBuilder::getColour();
        shape->Shape_Colour->Red = node.colour.r;
        shape->Shape_Colour->Green = node.colour.g;
        shape->Shape_Colour->Blue = node.colour.b;
        shape->Shape_Colour->Alpha = node.colour.a;
    }
    if (node.hasTexture && node.textureTransformIndex >= 0) {
        applyTransformsRange((Geometry *)shape, node.transforms, 0, node.textureTransformIndex);
        applyShapeTexture(node.texture, (Geometry *)shape);
        applyTransformsRange(
            (Geometry *)shape, node.transforms, node.textureTransformIndex, node.transformCount);
    } else {
        if (node.hasTexture) {
            applyShapeTexture(node.texture, (Geometry *)shape);
        }
        applyTransforms((Geometry *)shape, node.transforms, node.transformCount);
    }
    return shape;
}

Blob *
AstSceneBuilder::buildBlob(
    const AstBlobNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    Blob *shape = nullptr;

    if (node.hasReference) {
        const AstNode *declNode = findDecl(decls, node.referenceConstantId);
        if (declNode != nullptr) {
            if (declNode->kind != AST_BLOB_NODE) {
                ParseErrorReporter::Error("Invalid blob reference in AST", ctx);
            }
            Blob *declShape = buildBlob((const AstBlobNode &)*declNode, ctx, decls);
            shape = (Blob *)GeometryOperations::copy((SimpleBody *)declShape);
            delete declShape;
        } else {
            const Constant *legacyDecl = findLegacyDecl(ctx, node.referenceConstantId);
            if (legacyDecl == nullptr || legacyDecl->constantType != ParseGlobals::BLOB_CONSTANT) {
                ParseErrorReporter::Error("Invalid blob reference in AST", ctx);
            }
            shape = (Blob *)GeometryOperations::copy((SimpleBody *)legacyDecl->constantData);
        }
    } else {
        shape = ModelBuilder::getBlobShape();
        BlobList *components = nullptr;
        for (int i = 0; i < node.componentCount; ++i) {
            BlobList *c = new BlobList;
            c->elem.coeffs[2] = node.components[i].coeff;
            c->elem.radius2 = node.components[i].radius;
            c->elem.pos = asVector(node.components[i].pos);
            c->next = components;
            components = c;
        }
        Blob::makeBlob((SimpleBody *)shape, node.threshold, components, node.componentCount,
            node.sturm ? 1 : 0);
    }

    if (node.hasColour) {
        shape->Shape_Colour = ModelBuilder::getColour();
        shape->Shape_Colour->Red = node.colour.r;
        shape->Shape_Colour->Green = node.colour.g;
        shape->Shape_Colour->Blue = node.colour.b;
        shape->Shape_Colour->Alpha = node.colour.a;
    }
    if (node.hasTexture && node.textureTransformIndex >= 0) {
        applyTransformsRange((Geometry *)shape, node.transforms, 0, node.textureTransformIndex);
        applyShapeTexture(node.texture, (Geometry *)shape);
        applyTransformsRange(
            (Geometry *)shape, node.transforms, node.textureTransformIndex, node.transformCount);
    } else {
        if (node.hasTexture) {
            applyShapeTexture(node.texture, (Geometry *)shape);
        }
        applyTransforms((Geometry *)shape, node.transforms, node.transformCount);
    }
    return shape;
}

Triangle *
AstSceneBuilder::buildTriangle(
    const AstTriangleNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    Triangle *shape = nullptr;
    if (node.hasReference) {
        const AstNode *declNode = findDecl(decls, node.referenceConstantId);
        if (declNode != nullptr) {
            if (declNode->kind != AST_TRIANGLE_NODE) {
                ParseErrorReporter::Error("Invalid triangle reference in AST", ctx);
            }
            Triangle *declShape = buildTriangle((const AstTriangleNode &)*declNode, ctx, decls);
            shape = (Triangle *)GeometryOperations::copy((SimpleBody *)declShape);
            delete declShape;
        } else {
            const Constant *legacyDecl = findLegacyDecl(ctx, node.referenceConstantId);
            if (legacyDecl == nullptr || legacyDecl->constantType != ParseGlobals::TRIANGLE_CONSTANT) {
                ParseErrorReporter::Error("Invalid triangle reference in AST", ctx);
            }
            shape = (Triangle *)GeometryOperations::copy((SimpleBody *)legacyDecl->constantData);
        }
    } else {
        shape = ModelBuilder::getTriangleShape();
        if (node.hasInlineData) {
            shape->P1 = asVector(node.p1);
            shape->P2 = asVector(node.p2);
            shape->P3 = asVector(node.p3);
            if (!Triangle::computeTriangle(shape)) {
                ctx.degenerateTriangles() = LegacyBoolean::TRUE_VALUE;
            }
        }
    }
    if (node.hasColour) {
        shape->Shape_Colour = ModelBuilder::getColour();
        shape->Shape_Colour->Red = node.colour.r;
        shape->Shape_Colour->Green = node.colour.g;
        shape->Shape_Colour->Blue = node.colour.b;
        shape->Shape_Colour->Alpha = node.colour.a;
    }
    if (node.hasTexture && node.textureTransformIndex >= 0) {
        applyTransformsRange((Geometry *)shape, node.transforms, 0, node.textureTransformIndex);
        applyShapeTexture(node.texture, (Geometry *)shape);
        applyTransformsRange(
            (Geometry *)shape, node.transforms, node.textureTransformIndex, node.transformCount);
    } else {
        if (node.hasTexture) {
            applyShapeTexture(node.texture, (Geometry *)shape);
        }
        applyTransforms((Geometry *)shape, node.transforms, node.transformCount);
    }
    return shape;
}

SmoothTriangle *
AstSceneBuilder::buildSmoothTriangle(
    const AstSmoothTriangleNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    SmoothTriangle *shape = nullptr;
    if (node.hasReference) {
        const AstNode *declNode = findDecl(decls, node.referenceConstantId);
        if (declNode != nullptr) {
            if (declNode->kind != AST_SMOOTH_TRIANGLE_NODE) {
                ParseErrorReporter::Error("Invalid smooth_triangle reference in AST", ctx);
            }
            SmoothTriangle *declShape =
                buildSmoothTriangle((const AstSmoothTriangleNode &)*declNode, ctx, decls);
            shape = (SmoothTriangle *)GeometryOperations::copy((SimpleBody *)declShape);
            delete declShape;
        } else {
            const Constant *legacyDecl = findLegacyDecl(ctx, node.referenceConstantId);
            if (legacyDecl == nullptr ||
                legacyDecl->constantType != ParseGlobals::SMOOTH_TRIANGLE_CONSTANT) {
                ParseErrorReporter::Error("Invalid smooth_triangle reference in AST", ctx);
            }
            shape = (SmoothTriangle *)GeometryOperations::copy((SimpleBody *)legacyDecl->constantData);
        }
    } else {
        shape = ModelBuilder::getSmoothTriangleShape();
        if (node.hasInlineData) {
            shape->P1 = asVector(node.p1);
            shape->N1 = asVector(node.n1);
            shape->N1.normalize();
            shape->P2 = asVector(node.p2);
            shape->N2 = asVector(node.n2);
            shape->N2.normalize();
            shape->P3 = asVector(node.p3);
            shape->N3 = asVector(node.n3);
            shape->N3.normalize();
            if (!Triangle::computeTriangle((Triangle *)shape)) {
                ctx.degenerateTriangles() = LegacyBoolean::TRUE_VALUE;
            }
        }
    }
    if (node.hasColour) {
        shape->Shape_Colour = ModelBuilder::getColour();
        shape->Shape_Colour->Red = node.colour.r;
        shape->Shape_Colour->Green = node.colour.g;
        shape->Shape_Colour->Blue = node.colour.b;
        shape->Shape_Colour->Alpha = node.colour.a;
    }
    if (node.hasTexture && node.textureTransformIndex >= 0) {
        applyTransformsRange((Geometry *)shape, node.transforms, 0, node.textureTransformIndex);
        applyShapeTexture(node.texture, (Geometry *)shape);
        applyTransformsRange(
            (Geometry *)shape, node.transforms, node.textureTransformIndex, node.transformCount);
    } else {
        if (node.hasTexture) {
            applyShapeTexture(node.texture, (Geometry *)shape);
        }
        applyTransforms((Geometry *)shape, node.transforms, node.transformCount);
    }
    return shape;
}

PolynomialShape *
AstSceneBuilder::buildPoly(
    const AstPolyNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    PolynomialShape *shape = nullptr;
    if (node.hasReference) {
        const AstNode *declNode = findDecl(decls, node.referenceConstantId);
        if (declNode != nullptr) {
            if (declNode->kind != AST_POLY_NODE) {
                ParseErrorReporter::Error("Invalid poly reference in AST", ctx);
            }
            PolynomialShape *declShape = buildPoly((const AstPolyNode &)*declNode, ctx, decls);
            shape = (PolynomialShape *)GeometryOperations::copy((SimpleBody *)declShape);
            delete declShape;
        } else {
            const Constant *legacyDecl = findLegacyDecl(ctx, node.referenceConstantId);
            if (legacyDecl == nullptr || legacyDecl->constantType != ParseGlobals::POLY_CONSTANT) {
                ParseErrorReporter::Error("Invalid poly reference in AST", ctx);
            }
            shape = (PolynomialShape *)GeometryOperations::copy((SimpleBody *)legacyDecl->constantData);
        }
    } else {
        const int order = node.knownOrder > 0 ? node.knownOrder : node.explicitOrder;
        shape = ModelBuilder::getPolyShape(order, ctx.termCounts());
        if (node.hasInlineData) {
            const int coeffCount = ctx.termCounts()[shape->Order];
            for (int i = 0; i < coeffCount; ++i) {
                shape->Coeffs[i] = node.coeffs[i];
            }
        }
    }
    if (node.sturm) {
        shape->sturmFlag = 1;
    }
    if (node.hasColour) {
        shape->Shape_Colour = ModelBuilder::getColour();
        shape->Shape_Colour->Red = node.colour.r;
        shape->Shape_Colour->Green = node.colour.g;
        shape->Shape_Colour->Blue = node.colour.b;
        shape->Shape_Colour->Alpha = node.colour.a;
    }
    if (node.hasTexture && node.textureTransformIndex >= 0) {
        applyTransformsRange((Geometry *)shape, node.transforms, 0, node.textureTransformIndex);
        applyShapeTexture(node.texture, (Geometry *)shape);
        applyTransformsRange(
            (Geometry *)shape, node.transforms, node.textureTransformIndex, node.transformCount);
    } else {
        if (node.hasTexture) {
            applyShapeTexture(node.texture, (Geometry *)shape);
        }
        applyTransforms((Geometry *)shape, node.transforms, node.transformCount);
    }
    return shape;
}

CSG *
AstSceneBuilder::buildCsg(const AstCsgNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    CSG *container = nullptr;
    if (node.op == AST_CSG_UNION) {
        container = ModelBuilder::getCsgUnion();
    } else {
        container = ModelBuilder::getCsgIntersection();
    }

    if (node.hasReference) {
        const AstNode *declNode = findDecl(decls, node.referenceConstantId);
        if (declNode != nullptr) {
            if (declNode->kind != AST_CSG_NODE) {
                ParseErrorReporter::Error("Invalid CSG reference in AST", ctx);
            }
            CSG *declShape = buildCsg((const AstCsgNode &)*declNode, ctx, decls);
            delete container;
            container = (CSG *)GeometryOperations::copy((SimpleBody *)declShape);
            delete declShape;
        } else {
            const Constant *legacyDecl = findLegacyDecl(ctx, node.referenceConstantId);
            if (legacyDecl == nullptr ||
                (legacyDecl->constantType != ParseGlobals::CSG_UNION_CONSTANT &&
                    legacyDecl->constantType != ParseGlobals::CSG_INTERSECTION_CONSTANT &&
                    legacyDecl->constantType != ParseGlobals::CSG_DIFFERENCE_CONSTANT)) {
                ParseErrorReporter::Error("Invalid CSG reference in AST", ctx);
            }
            delete container;
            container = (CSG *)GeometryOperations::copy((SimpleBody *)legacyDecl->constantData);
        }
    } else {
        int firstShapeParsed = LegacyBoolean::FALSE_VALUE;
        for (int i = 0; i < node.childCount; ++i) {
            Geometry *child = buildGeometryNode(*node.children[i], ctx, decls);
            if (node.op == AST_CSG_DIFFERENCE && firstShapeParsed) {
                GeometryOperations::invert((SimpleBody *)child);
            }
            firstShapeParsed = LegacyBoolean::TRUE_VALUE;
            SimpleBodyFactory::link((SimpleBody *)child,
                (SimpleBody **)&(child->nextObject),
                (SimpleBody **)&(container->Shapes));
        }
    }

    applyTransforms((Geometry *)container, node.transforms, node.transformCount);
    return container;
}

Geometry *
AstSceneBuilder::buildGeometryNode(const AstNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    if (node.kind == AST_SPHERE_NODE) {
        return (Geometry *)buildSphere((const AstSphereNode &)node, ctx, decls);
    }
    if (node.kind == AST_PLANE_NODE) {
        return (Geometry *)buildPlane((const AstPlaneNode &)node, ctx, decls);
    }
    if (node.kind == AST_BOX_NODE) {
        return (Geometry *)buildBox((const AstBoxNode &)node, ctx, decls);
    }
    if (node.kind == AST_QUADRIC_NODE) {
        return (Geometry *)buildQuadric((const AstQuadricNode &)node, ctx, decls);
    }
    if (node.kind == AST_BLOB_NODE) {
        return (Geometry *)buildBlob((const AstBlobNode &)node, ctx, decls);
    }
    if (node.kind == AST_TRIANGLE_NODE) {
        return (Geometry *)buildTriangle((const AstTriangleNode &)node, ctx, decls);
    }
    if (node.kind == AST_SMOOTH_TRIANGLE_NODE) {
        return (Geometry *)buildSmoothTriangle((const AstSmoothTriangleNode &)node, ctx, decls);
    }
    if (node.kind == AST_POLY_NODE) {
        return (Geometry *)buildPoly((const AstPolyNode &)node, ctx, decls);
    }
    if (node.kind == AST_LEGACY_GEOMETRY_NODE) {
        const AstLegacyGeometryNode &legacyNode = (const AstLegacyGeometryNode &)node;
        if (legacyNode.shape == nullptr) {
            ParseErrorReporter::Error("Invalid legacy geometry AST node", ctx);
        }
        return (Geometry *)GeometryOperations::copy((SimpleBody *)legacyNode.shape);
    }
    if (node.kind == AST_HEIGHT_FIELD_NODE) {
        const AstHeightFieldNode &heightFieldNode = (const AstHeightFieldNode &)node;
        if (heightFieldNode.shape == nullptr) {
            ParseErrorReporter::Error("Invalid height_field AST node", ctx);
        }
        return (Geometry *)GeometryOperations::copy((SimpleBody *)heightFieldNode.shape);
    }
    if (node.kind == AST_BICUBIC_PATCH_NODE) {
        const AstBicubicPatchNode &bicubicPatchNode = (const AstBicubicPatchNode &)node;
        if (bicubicPatchNode.shape == nullptr) {
            ParseErrorReporter::Error("Invalid bicubic_patch AST node", ctx);
        }
        return (Geometry *)GeometryOperations::copy((SimpleBody *)bicubicPatchNode.shape);
    }
    if (node.kind == AST_LIGHT_SOURCE_NODE) {
        return (Geometry *)buildLight((const AstLightSourceNode &)node, ctx, decls);
    }
    if (node.kind == AST_CSG_NODE) {
        return (Geometry *)buildCsg((const AstCsgNode &)node, ctx, decls);
    }

    ParseErrorReporter::Error("Unsupported AST geometry node", ctx);
    return nullptr;
}

SimpleBody *
AstSceneBuilder::buildObject(const AstObjectNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    SimpleBody *object = nullptr;
    if (node.hasReference) {
        const AstNode *declNode = findDecl(decls, node.referenceConstantId);
        if (declNode != nullptr) {
            if (declNode->kind != AST_OBJECT_NODE && declNode->kind != AST_COMPOSITE_NODE) {
                ParseErrorReporter::Error("Invalid object reference in AST", ctx);
            }
            object =
                (SimpleBody *)GeometryOperations::copy(buildSimpleBodyNode(*declNode, ctx, decls));
        } else {
            const Constant *legacyDecl = findLegacyDecl(ctx, node.referenceConstantId);
            if (legacyDecl == nullptr ||
                (legacyDecl->constantType != ParseGlobals::OBJECT_CONSTANT &&
                    legacyDecl->constantType != ParseGlobals::COMPOSITE_CONSTANT)) {
                ParseErrorReporter::Error("Invalid object reference in AST", ctx);
            }
            object = (SimpleBody *)GeometryOperations::copy((SimpleBody *)legacyDecl->constantData);
        }
    } else {
        object = SimpleBodyFactory::getObject();
        if (node.shape != nullptr) {
            Geometry *shape = buildGeometryNode(*node.shape, ctx, decls);
            SimpleBodyFactory::link((SimpleBody *)shape,
                (SimpleBody **)&(shape->nextObject), (SimpleBody **)&(object->Shape));
        }
    }

    const int boundClipSplit =
        node.firstBoundedOrClippedTransformIndex >= 0 ?
            node.firstBoundedOrClippedTransformIndex :
            -1;
    if (boundClipSplit >= 0) {
        applyTransformsRange((Geometry *)object, node.transforms, 0, boundClipSplit);
    }

    for (int i = 0; i < node.boundedByCount; ++i) {
        Geometry *shape = buildGeometryNode(*node.boundedBy[i], ctx, decls);
        SimpleBodyFactory::link((SimpleBody *)shape,
            (SimpleBody **)&(shape->nextObject), (SimpleBody **)&(object->boundingShapes));
    }
    for (int i = 0; i < node.clippedByCount; ++i) {
        Geometry *shape = buildGeometryNode(*node.clippedBy[i], ctx, decls);
        SimpleBodyFactory::link((SimpleBody *)shape,
            (SimpleBody **)&(shape->nextObject), (SimpleBody **)&(object->clippingShapes));
    }
    if (node.hasColour) {
        object->objectColour = ModelBuilder::getColour();
        object->objectColour->Red = node.colour.r;
        object->objectColour->Green = node.colour.g;
        object->objectColour->Blue = node.colour.b;
        object->objectColour->Alpha = node.colour.a;
    }
    const int transformStart =
        boundClipSplit >= 0 ? boundClipSplit : 0;

    if (node.hasTexture && node.textureTransformIndex >= 0) {
        applyTransformsRange((Geometry *)object, node.transforms, transformStart, node.textureTransformIndex);
        applyObjectTexture(node.texture, object);
        applyTransformsRange(
            (Geometry *)object, node.transforms, node.textureTransformIndex, node.transformCount);
    } else {
        if (node.hasTexture) {
            applyObjectTexture(node.texture, object);
        }
        applyTransformsRange((Geometry *)object, node.transforms, transformStart, node.transformCount);
    }
    if (node.noShadow) {
        object->noShadowFlag = LegacyBoolean::TRUE_VALUE;
    }
    return object;
}

Composite *
AstSceneBuilder::buildComposite(
    const AstCompositeNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    Composite *composite = nullptr;
    if (node.hasReference) {
        const AstNode *declNode = findDecl(decls, node.referenceConstantId);
        if (declNode != nullptr) {
            if (declNode->kind != AST_COMPOSITE_NODE) {
                ParseErrorReporter::Error("Invalid composite reference in AST", ctx);
            }
            composite =
                (Composite *)GeometryOperations::copy(buildSimpleBodyNode(*declNode, ctx, decls));
        } else {
            const Constant *legacyDecl = findLegacyDecl(ctx, node.referenceConstantId);
            if (legacyDecl == nullptr ||
                legacyDecl->constantType != ParseGlobals::COMPOSITE_CONSTANT) {
                ParseErrorReporter::Error("Invalid composite reference in AST", ctx);
            }
            composite = (Composite *)GeometryOperations::copy((SimpleBody *)legacyDecl->constantData);
        }
    } else {
        composite = ModelBuilder::getCompositeObject();
        for (int i = 0; i < node.childCount; ++i) {
            SimpleBody *child = buildSimpleBodyNode(*node.children[i], ctx, decls);
            SimpleBodyFactory::link(
                child, &(child->nextObject), (SimpleBody **)&(composite->Objects));
        }
    }

    for (int i = 0; i < node.boundedByCount; ++i) {
        Geometry *shape = buildGeometryNode(*node.boundedBy[i], ctx, decls);
        SimpleBodyFactory::link((SimpleBody *)shape, (SimpleBody **)&(shape->nextObject),
            (SimpleBody **)&(composite->boundingShapes));
    }
    for (int i = 0; i < node.clippedByCount; ++i) {
        Geometry *shape = buildGeometryNode(*node.clippedBy[i], ctx, decls);
        SimpleBodyFactory::link((SimpleBody *)shape, (SimpleBody **)&(shape->nextObject),
            (SimpleBody **)&(composite->clippingShapes));
    }
    applyTransforms((Geometry *)composite, node.transforms, node.transformCount);
    return composite;
}

SimpleBody *
AstSceneBuilder::buildSimpleBodyNode(const AstNode &node, ParserContext &ctx, const AstDeclTable &decls)
{
    if (node.kind == AST_OBJECT_NODE) {
        return buildObject((const AstObjectNode &)node, ctx, decls);
    }
    if (node.kind == AST_COMPOSITE_NODE) {
        return (SimpleBody *)buildComposite((const AstCompositeNode &)node, ctx, decls);
    }
    if (node.kind == AST_SPHERE_NODE || node.kind == AST_PLANE_NODE ||
        node.kind == AST_BOX_NODE || node.kind == AST_QUADRIC_NODE ||
        node.kind == AST_BLOB_NODE || node.kind == AST_TRIANGLE_NODE ||
        node.kind == AST_SMOOTH_TRIANGLE_NODE || node.kind == AST_POLY_NODE ||
        node.kind == AST_LEGACY_GEOMETRY_NODE ||
        node.kind == AST_LIGHT_SOURCE_NODE ||
        node.kind == AST_CSG_NODE) {
        return (SimpleBody *)buildGeometryNode(node, ctx, decls);
    }
    ParseErrorReporter::Error("Unsupported AST simple body node", ctx);
    return nullptr;
}

void
AstSceneBuilder::build(const AstScene &scene, RenderFrame *framePtr, ParserContext &ctx)
{
    AstDeclTable declarations;
    declarations.count = 0;

    for (int i = 0; i < scene.nodeCount; ++i) {
        const AstNode *node = scene.nodes[i];
        if (node->kind == AST_DECLARE_NODE) {
            const AstDeclareNode *decl = (const AstDeclareNode *)node;
            if (decl->value != nullptr && decl->value->kind == AST_TEXTURE_CHAIN_NODE) {
                Constant *constantPtr =
                    ctx.symbols().upsertByIdentifierNumber(decl->identifierNumber);
                if (constantPtr == nullptr) {
                    ParseErrorReporter::Error("Too many constants \"declared\"", ctx);
                }
                constantPtr->identifierNumber = decl->identifierNumber;
                constantPtr->constantType = ParseGlobals::TEXTURE_CONSTANT;
                constantPtr->constantData =
                    (void *)((const AstTextureChainNode *)decl->value)->texture;
            }
            if (declarations.count < ParserConstants::MAX_CONSTANTS) {
                declarations.ids[declarations.count] = decl->identifierNumber;
                declarations.nodes[declarations.count] = decl->value;
                declarations.count++;
            }
            continue;
        }
        if (node->kind == AST_SPHERE_NODE) {
            Sphere *sphere = buildSphere(*(const AstSphereNode *)node, ctx, declarations);
            SimpleBodyFactory::link((SimpleBody *)sphere,
                (SimpleBody **)&(sphere->nextObject),
                (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == AST_PLANE_NODE) {
            InfinitePlane *plane = buildPlane(*(const AstPlaneNode *)node, ctx, declarations);
            SimpleBodyFactory::link((SimpleBody *)plane,
                (SimpleBody **)&(plane->nextObject),
                (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == AST_BOX_NODE) {
            Box *box = buildBox(*(const AstBoxNode *)node, ctx, declarations);
            SimpleBodyFactory::link((SimpleBody *)box,
                (SimpleBody **)&(box->nextObject),
                (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == AST_QUADRIC_NODE) {
            Quadric *quadric = buildQuadric(*(const AstQuadricNode *)node, ctx, declarations);
            SimpleBodyFactory::link((SimpleBody *)quadric,
                (SimpleBody **)&(quadric->nextObject),
                (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == AST_BLOB_NODE) {
            Blob *blob = buildBlob(*(const AstBlobNode *)node, ctx, declarations);
            SimpleBodyFactory::link((SimpleBody *)blob,
                (SimpleBody **)&(blob->nextObject),
                (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == AST_TRIANGLE_NODE) {
            Triangle *triangle = buildTriangle(*(const AstTriangleNode *)node, ctx, declarations);
            SimpleBodyFactory::link((SimpleBody *)triangle,
                (SimpleBody **)&(triangle->nextObject),
                (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == AST_SMOOTH_TRIANGLE_NODE) {
            SmoothTriangle *triangle =
                buildSmoothTriangle(*(const AstSmoothTriangleNode *)node, ctx, declarations);
            SimpleBodyFactory::link((SimpleBody *)triangle,
                (SimpleBody **)&(triangle->nextObject),
                (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == AST_POLY_NODE) {
            PolynomialShape *poly = buildPoly(*(const AstPolyNode *)node, ctx, declarations);
            SimpleBodyFactory::link((SimpleBody *)poly,
                (SimpleBody **)&(poly->nextObject),
                (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == AST_LEGACY_GEOMETRY_NODE) {
            Geometry *shape = buildGeometryNode(*node, ctx, declarations);
            SimpleBodyFactory::link((SimpleBody *)shape,
                (SimpleBody **)&(shape->nextObject),
                (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == AST_LIGHT_SOURCE_NODE) {
            Light *light = buildLight(*(const AstLightSourceNode *)node, ctx, declarations);
            light->Next_Light_Source = framePtr->Light_Sources;
            framePtr->Light_Sources = light;
        } else if (node->kind == AST_CSG_NODE) {
            CSG *csg = buildCsg(*(const AstCsgNode *)node, ctx, declarations);
            SimpleBodyFactory::link((SimpleBody *)csg,
                (SimpleBody **)&(csg->nextObject),
                (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == AST_OBJECT_NODE || node->kind == AST_COMPOSITE_NODE) {
            SimpleBody *obj = buildSimpleBodyNode(*node, ctx, declarations);
            SimpleBodyFactory::link(
                obj, &(obj->nextObject), (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == AST_FOG_NODE) {
            applyFog((const AstFogNode &)*node, framePtr);
        } else if (node->kind == AST_CAMERA_NODE) {
            applyCamera((const AstCameraNode &)*node, framePtr, ctx, declarations);
        } else if (node->kind == AST_MAX_TRACE_LEVEL_NODE) {
            applyMaxTraceLevel((const AstMaxTraceLevelNode &)*node);
        } else if (node->kind == AST_DEFAULT_TEXTURE_NODE) {
            applyDefaultTexture((const AstDefaultTextureNode &)*node);
        }
    }
}
