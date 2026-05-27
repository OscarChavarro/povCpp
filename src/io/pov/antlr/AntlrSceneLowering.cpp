#include "io/pov/antlr/AntlrSceneLowering.h"

#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "common/LegacyBoolean.h"
#include "common/color/Color.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/camera/Camera.h"
#include "environment/geometry/GeometryOperations.h"
#ifndef POV_ANTLR_MINIMAL_BRIDGE
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/elements/Triangle.h"
#include "environment/geometry/elements/SmoothTriangle.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/geometry/volume/Blob.h"
#include "environment/geometry/volume/BlobList.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/material/RenderRuntimeState.h"
#include "environment/light/Light.h"
#include "environment/scene/ModelBuilder.h"
#include "environment/scene/SimpleBodyFactory.h"
#endif
#include "environment/material/RenderRuntimeState.h"
#include "environment/scene/SimpleBody.h"
#include "io/pov/antlr/AntlrSceneIr.h"
#include "media/Texture.h"
#include "media/TextureUtils.h"
#include "render/RenderFrame.h"

namespace {
const std::unordered_map<std::string, const AntlrIrQuarticNode *> *gDeclaredQuartics = nullptr;

const std::unordered_map<std::string, const AntlrIrQuarticNode *> &declaredQuarticsRef()
{
    static const std::unordered_map<std::string, const AntlrIrQuarticNode *> kEmpty;
    return gDeclaredQuartics != nullptr ? *gDeclaredQuartics : kEmpty;
}

Vector3Dd toVector(const AntlrIrVector3 &v)
{
    return Vector3Dd(v.x, v.y, v.z);
}

std::string formatSourceLocation(const AntlrSceneIrNode &node)
{
    std::string out;
    if (node.sourceFile != nullptr) {
        out += node.sourceFile;
    } else {
        out += "<unknown>";
    }
    out += ":";
    out += std::to_string(node.sourceLine);
    out += ":";
    out += std::to_string(node.sourceColumn);
    return out;
}

AntlrIrSphereNode makeFallbackSphereNode()
{
    AntlrIrSphereNode n;
    n.hasReferenceBase = false;
    n.hasInlineBase = true;
    n.center = {0.0, 0.0, 0.0};
    n.radius = 1.0;
    return n;
}

void applyTransforms(SimpleBody *shape, const AntlrIrTransform *transforms, int count)
{
    if (shape == nullptr || transforms == nullptr || count <= 0) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        Vector3Dd v = toVector(transforms[i].vectorValue);
        if (transforms[i].kind == ANTLR_IR_SCALE) {
            const bool nonUniform =
                std::fabs(v.x - v.y) > 1e-9 || std::fabs(v.y - v.z) > 1e-9;
#ifndef POV_ANTLR_MINIMAL_BRIDGE
            const bool sphereScaleMethod =
                shape->methods != nullptr &&
                shape->methods->scaleMethod ==
                    static_cast<SCALE_METHOD>(&Sphere::scaleSphere);
#else
            const bool sphereScaleMethod = false;
#endif
            if (nonUniform && sphereScaleMethod) {
                const double s = (std::fabs(v.x) + std::fabs(v.y) + std::fabs(v.z)) / 3.0;
                v = Vector3Dd(s, s, s);
            }
        }
        try {
            if (transforms[i].kind == ANTLR_IR_TRANSLATE) {
                GeometryOperations::translate(shape, &v);
            } else if (transforms[i].kind == ANTLR_IR_ROTATE) {
                GeometryOperations::rotate(shape, &v);
            } else if (transforms[i].kind == ANTLR_IR_SCALE) {
                GeometryOperations::scale(shape, &v);
            }
        } catch (const std::exception &) {
            if (transforms[i].kind != ANTLR_IR_SCALE) {
                throw;
            }
            // Legacy engine rejects some anisotropic sphere scales; degrade to isotropic
            // scale so ANTLR strict route can continue rendering.
            const double s = (std::fabs(v.x) + std::fabs(v.y) + std::fabs(v.z)) / 3.0;
            Vector3Dd uniformScale(s, s, s);
            GeometryOperations::scale(shape, &uniformScale);
        }
    }
}

#ifndef POV_ANTLR_MINIMAL_BRIDGE
Sphere *buildSphere(const AntlrIrSphereNode &node)
{
    if (node.hasReferenceBase) {
        throw std::runtime_error("ANTLR lowering sphere reference base not supported yet");
    }
    if (!node.hasInlineBase) {
        throw std::runtime_error("ANTLR lowering sphere requires inline base");
    }
    Sphere *sphere = ModelBuilder::getSphereShape();
    sphere->Center = toVector(node.center);
    sphere->Radius = node.radius;
    sphere->radiusSquared = sphere->Radius * sphere->Radius;
    sphere->inverseRadius = 1.0 / sphere->Radius;
    if (node.hasColour) {
        sphere->Shape_Colour = ModelBuilder::getColour();
        sphere->Shape_Colour->Red = node.colour.r;
        sphere->Shape_Colour->Green = node.colour.g;
        sphere->Shape_Colour->Blue = node.colour.b;
        sphere->Shape_Colour->Alpha = node.colour.a;
    }
    applyTransforms((SimpleBody *)sphere, node.transforms, node.transformCount);
    return sphere;
}

Sphere *buildSphereResolved(const AntlrIrSphereNode &node,
    const std::unordered_map<std::string, const AntlrIrSphereNode *> &declaredSpheres, int depth)
{
    if (depth > 16) {
        throw std::runtime_error("ANTLR sphere lowering exceeded max recursion depth");
    }
    if (!node.hasReferenceBase) {
        return buildSphere(node);
    }

    auto it = declaredSpheres.find(node.referenceIdentifier);
    if (it == declaredSpheres.end()) {
        return buildSphere(makeFallbackSphereNode());
    }
    Sphere *base = buildSphereResolved(*it->second, declaredSpheres, depth + 1);

    // Apply local node modifiers over the resolved base.
    if (node.hasColour) {
        if (base->Shape_Colour == nullptr) {
            base->Shape_Colour = ModelBuilder::getColour();
        }
        base->Shape_Colour->Red = node.colour.r;
        base->Shape_Colour->Green = node.colour.g;
        base->Shape_Colour->Blue = node.colour.b;
        base->Shape_Colour->Alpha = node.colour.a;
    }
    applyTransforms((SimpleBody *)base, node.transforms, node.transformCount);
    return base;
}

bool buildDeclaredSphereByName(const std::string &name,
    const std::unordered_map<std::string, const AntlrIrSphereNode *> &declaredSpheres, Sphere *&out)
{
    auto it = declaredSpheres.find(name);
    if (it == declaredSpheres.end()) {
        return false;
    }
    out = buildSphereResolved(*it->second, declaredSpheres, 0);
    return out != nullptr;
}

InfinitePlane *buildPlane(const AntlrIrPlaneNode &node,
    const std::unordered_map<std::string, const AntlrIrPlaneNode *> &declaredPlanes)
{
    const AntlrIrPlaneNode *effectiveNode = &node;
    if (node.hasReferenceBase) {
        auto it = declaredPlanes.find(node.referenceIdentifier);
        if (it == declaredPlanes.end()) {
            AntlrIrPlaneNode fallback;
            fallback.hasReferenceBase = false;
            fallback.hasInlineBase = true;
            fallback.normal = {0.0, 1.0, 0.0};
            fallback.distance = 0.0;
            return buildPlane(fallback, declaredPlanes);
        }
        effectiveNode = it->second;
    }
    if (!effectiveNode->hasInlineBase) {
        throw std::runtime_error("ANTLR lowering plane requires inline base");
    }

    InfinitePlane *plane = ModelBuilder::getPlaneShape();
    plane->normalVector = toVector(effectiveNode->normal);
    plane->Distance = effectiveNode->distance * -1.0;
    if (effectiveNode->hasColour) {
        plane->Shape_Colour = ModelBuilder::getColour();
        plane->Shape_Colour->Red = effectiveNode->colour.r;
        plane->Shape_Colour->Green = effectiveNode->colour.g;
        plane->Shape_Colour->Blue = effectiveNode->colour.b;
        plane->Shape_Colour->Alpha = effectiveNode->colour.a;
    }
    applyTransforms((SimpleBody *)plane, effectiveNode->transforms, effectiveNode->transformCount);
    if (effectiveNode->inverted) {
        GeometryOperations::invert((SimpleBody *)plane);
    }
    return plane;
}

Box *buildBox(const AntlrIrBoxNode &node,
    const std::unordered_map<std::string, const AntlrIrBoxNode *> &declaredBoxes)
{
    const AntlrIrBoxNode *effectiveNode = &node;
    if (node.hasReferenceBase) {
        auto it = declaredBoxes.find(node.referenceIdentifier);
        if (it == declaredBoxes.end()) {
            AntlrIrBoxNode fallback;
            fallback.hasReferenceBase = false;
            fallback.hasInlineBase = true;
            fallback.minBounds = {-1.0, -1.0, -1.0};
            fallback.maxBounds = {1.0, 1.0, 1.0};
            return buildBox(fallback, declaredBoxes);
        }
        effectiveNode = it->second;
    }
    if (!effectiveNode->hasInlineBase) {
        throw std::runtime_error("ANTLR lowering box requires inline base");
    }

    Box *box = ModelBuilder::getBoxShape();
    box->bounds[0] = toVector(effectiveNode->minBounds);
    box->bounds[1] = toVector(effectiveNode->maxBounds);
    if (effectiveNode->hasColour) {
        box->Shape_Colour = ModelBuilder::getColour();
        box->Shape_Colour->Red = effectiveNode->colour.r;
        box->Shape_Colour->Green = effectiveNode->colour.g;
        box->Shape_Colour->Blue = effectiveNode->colour.b;
        box->Shape_Colour->Alpha = effectiveNode->colour.a;
    }
    applyTransforms((SimpleBody *)box, effectiveNode->transforms, effectiveNode->transformCount);
    if (effectiveNode->inverted) {
        GeometryOperations::invert((SimpleBody *)box);
    }
    return box;
}

Triangle *buildTriangle(const AntlrIrTriangleNode &node,
    const std::unordered_map<std::string, const AntlrIrTriangleNode *> &declaredTriangles)
{
    const AntlrIrTriangleNode *effectiveNode = &node;
    if (node.hasReferenceBase) {
        auto it = declaredTriangles.find(node.referenceIdentifier);
        if (it == declaredTriangles.end()) {
            AntlrIrTriangleNode fallback;
            fallback.hasReferenceBase = false;
            fallback.hasInlineBase = true;
            fallback.p1 = {0.0, 0.0, 0.0};
            fallback.p2 = {1.0, 0.0, 0.0};
            fallback.p3 = {0.0, 1.0, 0.0};
            return buildTriangle(fallback, declaredTriangles);
        }
        effectiveNode = it->second;
    }
    if (!effectiveNode->hasInlineBase) {
        throw std::runtime_error("ANTLR lowering triangle requires inline base");
    }

    Triangle *triangle = ModelBuilder::getTriangleShape();
    triangle->P1 = toVector(effectiveNode->p1);
    triangle->P2 = toVector(effectiveNode->p2);
    triangle->P3 = toVector(effectiveNode->p3);
    if (!Triangle::computeTriangle(triangle)) {
        throw std::runtime_error("ANTLR lowering triangle is degenerate");
    }
    if (effectiveNode->hasColour) {
        triangle->Shape_Colour = ModelBuilder::getColour();
        triangle->Shape_Colour->Red = effectiveNode->colour.r;
        triangle->Shape_Colour->Green = effectiveNode->colour.g;
        triangle->Shape_Colour->Blue = effectiveNode->colour.b;
        triangle->Shape_Colour->Alpha = effectiveNode->colour.a;
    }
    applyTransforms((SimpleBody *)triangle, effectiveNode->transforms, effectiveNode->transformCount);
    if (effectiveNode->inverted) {
        GeometryOperations::invert((SimpleBody *)triangle);
    }
    return triangle;
}

SmoothTriangle *buildSmoothTriangle(const AntlrIrSmoothTriangleNode &node,
    const std::unordered_map<std::string, const AntlrIrSmoothTriangleNode *> &declaredSmoothTriangles)
{
    const AntlrIrSmoothTriangleNode *effectiveNode = &node;
    if (node.hasReferenceBase) {
        auto it = declaredSmoothTriangles.find(node.referenceIdentifier);
        if (it == declaredSmoothTriangles.end()) {
            AntlrIrSmoothTriangleNode fallback;
            fallback.hasReferenceBase = false;
            fallback.hasInlineBase = true;
            fallback.p1 = {0.0, 0.0, 0.0};
            fallback.n1 = {0.0, 0.0, 1.0};
            fallback.p2 = {1.0, 0.0, 0.0};
            fallback.n2 = {0.0, 0.0, 1.0};
            fallback.p3 = {0.0, 1.0, 0.0};
            fallback.n3 = {0.0, 0.0, 1.0};
            return buildSmoothTriangle(fallback, declaredSmoothTriangles);
        }
        effectiveNode = it->second;
    }
    if (!effectiveNode->hasInlineBase) {
        throw std::runtime_error("ANTLR lowering smooth_triangle requires inline base");
    }

    SmoothTriangle *triangle = (SmoothTriangle *)ModelBuilder::getSmoothTriangleShape();
    triangle->P1 = toVector(effectiveNode->p1);
    triangle->N1 = toVector(effectiveNode->n1);
    triangle->N1.normalize();
    triangle->P2 = toVector(effectiveNode->p2);
    triangle->N2 = toVector(effectiveNode->n2);
    triangle->N2.normalize();
    triangle->P3 = toVector(effectiveNode->p3);
    triangle->N3 = toVector(effectiveNode->n3);
    triangle->N3.normalize();
    if (!Triangle::computeTriangle((Triangle *)triangle)) {
        throw std::runtime_error("ANTLR lowering smooth_triangle is degenerate");
    }
    if (effectiveNode->hasColour) {
        triangle->Shape_Colour = ModelBuilder::getColour();
        triangle->Shape_Colour->Red = effectiveNode->colour.r;
        triangle->Shape_Colour->Green = effectiveNode->colour.g;
        triangle->Shape_Colour->Blue = effectiveNode->colour.b;
        triangle->Shape_Colour->Alpha = effectiveNode->colour.a;
    }
    applyTransforms((SimpleBody *)triangle, effectiveNode->transforms, effectiveNode->transformCount);
    if (effectiveNode->inverted) {
        GeometryOperations::invert((SimpleBody *)triangle);
    }
    return triangle;
}

Quadric *buildQuadric(const AntlrIrQuadricNode &node,
    const std::unordered_map<std::string, const AntlrIrQuadricNode *> &declaredQuadrics)
{
    const AntlrIrQuadricNode *effectiveNode = &node;
    AntlrIrQuadricNode fallback;
    bool useFallback = false;
    int depth = 0;
    while (effectiveNode->hasReferenceBase) {
        if (++depth > 16) {
            throw std::runtime_error("ANTLR quadric lowering exceeded max recursion depth");
        }
        auto it = declaredQuadrics.find(effectiveNode->referenceIdentifier);
        if (it == declaredQuadrics.end()) {
            fallback.hasReferenceBase = false;
            fallback.hasInlineBase = true;
            fallback.object2Terms = {1.0, 1.0, 1.0};
            fallback.objectMixedTerms = {0.0, 0.0, 0.0};
            fallback.objectTerms = {0.0, 0.0, 0.0};
            fallback.objectConstant = -1.0;
            useFallback = true;
            break;
        }
        effectiveNode = it->second;
    }
    if (useFallback) {
        effectiveNode = &fallback;
    }
    if (!effectiveNode->hasInlineBase) {
        throw std::runtime_error("ANTLR lowering quadric requires inline base");
    }

    Quadric *quadric = ModelBuilder::getQuadricShape();
    quadric->object2Terms = toVector(effectiveNode->object2Terms);
    quadric->objectMixedTerms = toVector(effectiveNode->objectMixedTerms);
    quadric->objectTerms = toVector(effectiveNode->objectTerms);
    quadric->objectConstant = effectiveNode->objectConstant;
    quadric->nonZeroSquareTerm =
        !((quadric->object2Terms.x == 0.0) && (quadric->object2Terms.y == 0.0) &&
            (quadric->object2Terms.z == 0.0) && (quadric->objectMixedTerms.x == 0.0) &&
            (quadric->objectMixedTerms.y == 0.0) && (quadric->objectMixedTerms.z == 0.0));
    if (effectiveNode->hasColour) {
        quadric->Shape_Colour = ModelBuilder::getColour();
        quadric->Shape_Colour->Red = effectiveNode->colour.r;
        quadric->Shape_Colour->Green = effectiveNode->colour.g;
        quadric->Shape_Colour->Blue = effectiveNode->colour.b;
        quadric->Shape_Colour->Alpha = effectiveNode->colour.a;
    }
    applyTransforms((SimpleBody *)quadric, effectiveNode->transforms, effectiveNode->transformCount);
    if (effectiveNode->inverted) {
        GeometryOperations::invert((SimpleBody *)quadric);
    }
    return quadric;
}

PolynomialShape *buildQuartic(const AntlrIrQuarticNode &node)
{
    const std::unordered_map<std::string, const AntlrIrQuarticNode *> &declaredQuartics =
        declaredQuarticsRef();

    const AntlrIrQuarticNode *effectiveNode = &node;
    if (node.hasReferenceBase) {
        auto it = declaredQuartics.find(node.referenceIdentifier);
        if (it == declaredQuartics.end()) {
            throw std::runtime_error(
                "ANTLR lowering unknown quartic reference '" + node.referenceIdentifier + "' at " +
                formatSourceLocation(node));
        }
        effectiveNode = it->second;
    }
    if (!effectiveNode->hasInlineBase) {
        throw std::runtime_error("ANTLR lowering quartic requires inline base");
    }

    const int order = 4;
    const int coeffCount = PolynomialShape::termCounts()[order];
    if (effectiveNode->coefficientCount != coeffCount) {
        throw std::runtime_error("ANTLR lowering quartic requires exactly " +
            std::to_string(coeffCount) + " coefficients at " + formatSourceLocation(node));
    }

    PolynomialShape *poly = ModelBuilder::getPolyShape(order, PolynomialShape::termCounts());
    for (int i = 0; i < coeffCount; ++i) {
        poly->Coeffs[i] = effectiveNode->coefficients[i];
    }
    if (effectiveNode->sturm) {
        poly->sturmFlag = 1;
    }
    if (effectiveNode->hasColour) {
        poly->Shape_Colour = ModelBuilder::getColour();
        poly->Shape_Colour->Red = effectiveNode->colour.r;
        poly->Shape_Colour->Green = effectiveNode->colour.g;
        poly->Shape_Colour->Blue = effectiveNode->colour.b;
        poly->Shape_Colour->Alpha = effectiveNode->colour.a;
    }
    applyTransforms((SimpleBody *)poly, effectiveNode->transforms, effectiveNode->transformCount);
    if (effectiveNode->inverted) {
        GeometryOperations::invert((SimpleBody *)poly);
    }
    return poly;
}

Blob *buildBlob(const AntlrIrBlobNode &node,
    const std::unordered_map<std::string, const AntlrIrBlobNode *> &declaredBlobs)
{
    const AntlrIrBlobNode *effectiveNode = &node;
    if (node.hasReferenceBase) {
        auto it = declaredBlobs.find(node.referenceIdentifier);
        if (it == declaredBlobs.end()) {
            AntlrIrBlobNode fallback;
            fallback.hasReferenceBase = false;
            fallback.hasInlineBase = true;
            fallback.hasThreshold = true;
            fallback.threshold = 1.0;
            fallback.componentCount = 1;
            fallback.components[0].coeff = 1.0;
            fallback.components[0].radius = 1.0;
            fallback.components[0].position = {0.0, 0.0, 0.0};
            return buildBlob(fallback, declaredBlobs);
        }
        effectiveNode = it->second;
    }
    if (!effectiveNode->hasInlineBase) {
        throw std::runtime_error("ANTLR lowering blob requires inline base");
    }
    if (effectiveNode->componentCount <= 0) {
        throw std::runtime_error("ANTLR lowering blob requires at least one component");
    }
    for (int i = 0; i < effectiveNode->componentCount; ++i) {
        const AntlrIrBlobComponent &c = effectiveNode->components[i];
        if (std::fabs(c.coeff) < 1.0e-12 || std::fabs(c.radius) < 1.0e-12) {
            throw std::runtime_error("ANTLR lowering blob has degenerate component");
        }
    }

    Blob *blob = ModelBuilder::getBlobShape();
    BlobList *head = nullptr;
    for (int i = 0; i < effectiveNode->componentCount; ++i) {
        BlobList *entry = new BlobList();
        entry->elem.coeffs[2] = effectiveNode->components[i].coeff;
        entry->elem.radius2 = effectiveNode->components[i].radius;
        entry->elem.pos = toVector(effectiveNode->components[i].position);
        entry->next = head;
        head = entry;
    }
    const double threshold = effectiveNode->hasThreshold ? effectiveNode->threshold : 1.0;
    Blob::makeBlob((SimpleBody *)blob, threshold, head, effectiveNode->componentCount,
        effectiveNode->sturm ? 1 : 0);

    if (effectiveNode->hasColour) {
        blob->Shape_Colour = ModelBuilder::getColour();
        blob->Shape_Colour->Red = effectiveNode->colour.r;
        blob->Shape_Colour->Green = effectiveNode->colour.g;
        blob->Shape_Colour->Blue = effectiveNode->colour.b;
        blob->Shape_Colour->Alpha = effectiveNode->colour.a;
    }
    applyTransforms((SimpleBody *)blob, effectiveNode->transforms, effectiveNode->transformCount);
    if (effectiveNode->inverted) {
        GeometryOperations::invert((SimpleBody *)blob);
    }
    return blob;
}

Light *buildLight(const AntlrIrLightNode &node,
    const std::unordered_map<std::string, const AntlrIrLightNode *> &declaredLights)
{
    static constexpr double PI = 3.14159265358979323846;
    const AntlrIrLightNode *effectiveNode = &node;
    if (node.hasReference) {
        auto it = declaredLights.find(node.referenceIdentifier);
        if (it == declaredLights.end()) {
            AntlrIrLightNode fallback;
            fallback.hasReference = false;
            fallback.hasCenter = true;
            fallback.center = {0.0, 10.0, 0.0};
            return buildLight(fallback, declaredLights);
        }
        effectiveNode = it->second;
    }
    if (!effectiveNode->hasReference && !effectiveNode->hasCenter) {
        throw std::runtime_error(
            "ANTLR light lowering requires inline center or reference at " + formatSourceLocation(node));
    }

    Light *light = ModelBuilder::getLightSourceShape();
    if (effectiveNode->hasCenter) {
        light->Center = toVector(effectiveNode->center);
    }
    if (effectiveNode->hasColour && light->Shape_Colour != nullptr) {
        light->Shape_Colour->Red = effectiveNode->colour.r;
        light->Shape_Colour->Green = effectiveNode->colour.g;
        light->Shape_Colour->Blue = effectiveNode->colour.b;
        light->Shape_Colour->Alpha = effectiveNode->colour.a;
    }
    if (effectiveNode->hasPointAt) {
        light->pointsAt = toVector(effectiveNode->pointAt);
    }
    if (effectiveNode->hasTightness) {
        light->Coeff = effectiveNode->tightness;
    }
    if (effectiveNode->hasRadius) {
        light->Radius = std::cos(effectiveNode->radiusDegrees * PI / 180.0);
    }
    if (effectiveNode->hasFalloff) {
        light->Falloff = std::cos(effectiveNode->falloffDegrees * PI / 180.0);
    }
    if (effectiveNode->spotlight) {
        light->Type = GeometryOperations::SPOT_LIGHT_TYPE;
    }
    applyTransforms((SimpleBody *)light, effectiveNode->transforms, effectiveNode->transformCount);
    return light;
}

Texture *materializeTextureChain(const AntlrIrTextureChain &chain,
    const std::unordered_map<std::string, Texture *> &declaredTextures);
void applyShapeTexture(Texture *srcTexture, Geometry *shape);
void applyObjectTexture(Texture *srcTexture, SimpleBody *object);
void linkShapeToGeometryList(Geometry *shape, Geometry **listHead)
{
    if (shape == nullptr || listHead == nullptr) {
        return;
    }
    SimpleBodyFactory::link((SimpleBody *)shape, (SimpleBody **)&(shape->nextObject),
        (SimpleBody **)listHead);
}
CSG *buildCsgFromIr(const AntlrIrCsgNode &node,
    const std::unordered_map<std::string, Texture *> &declaredTextures,
    const std::unordered_map<std::string, const AntlrIrSphereNode *> &declaredSpheres,
    const std::unordered_map<std::string, const AntlrIrPlaneNode *> &declaredPlanes,
    const std::unordered_map<std::string, const AntlrIrBoxNode *> &declaredBoxes,
    const std::unordered_map<std::string, const AntlrIrTriangleNode *> &declaredTriangles,
    const std::unordered_map<std::string, const AntlrIrSmoothTriangleNode *> &declaredSmoothTriangles,
    const std::unordered_map<std::string, const AntlrIrQuadricNode *> &declaredQuadrics,
    const std::unordered_map<std::string, const AntlrIrBlobNode *> &declaredBlobs,
    const std::unordered_map<std::string, const AntlrIrLightNode *> &declaredLights,
    const std::unordered_map<std::string, const AntlrIrObjectNode *> &declaredObjects,
    const std::unordered_map<std::string, const AntlrIrCompositeNode *> &declaredComposites,
    const std::unordered_map<std::string, const AntlrIrCsgNode *> &declaredCsgs, int depth);
SimpleBody *buildCompositeFromIr(const AntlrIrCompositeNode &node,
    const std::unordered_map<std::string, Texture *> &declaredTextures,
    const std::unordered_map<std::string, const AntlrIrSphereNode *> &declaredSpheres,
    const std::unordered_map<std::string, const AntlrIrPlaneNode *> &declaredPlanes,
    const std::unordered_map<std::string, const AntlrIrBoxNode *> &declaredBoxes,
    const std::unordered_map<std::string, const AntlrIrTriangleNode *> &declaredTriangles,
    const std::unordered_map<std::string, const AntlrIrSmoothTriangleNode *> &declaredSmoothTriangles,
    const std::unordered_map<std::string, const AntlrIrQuadricNode *> &declaredQuadrics,
    const std::unordered_map<std::string, const AntlrIrBlobNode *> &declaredBlobs,
    const std::unordered_map<std::string, const AntlrIrLightNode *> &declaredLights,
    const std::unordered_map<std::string, const AntlrIrObjectNode *> &declaredObjects,
    const std::unordered_map<std::string, const AntlrIrCompositeNode *> &declaredComposites,
    const std::unordered_map<std::string, const AntlrIrCsgNode *> &declaredCsgs, int depth);

SimpleBody *buildObjectFromIr(const AntlrIrObjectNode &node,
    const std::unordered_map<std::string, Texture *> &declaredTextures,
    const std::unordered_map<std::string, const AntlrIrSphereNode *> &declaredSpheres,
    const std::unordered_map<std::string, const AntlrIrPlaneNode *> &declaredPlanes,
    const std::unordered_map<std::string, const AntlrIrBoxNode *> &declaredBoxes,
    const std::unordered_map<std::string, const AntlrIrTriangleNode *> &declaredTriangles,
    const std::unordered_map<std::string, const AntlrIrSmoothTriangleNode *> &declaredSmoothTriangles,
    const std::unordered_map<std::string, const AntlrIrQuadricNode *> &declaredQuadrics,
    const std::unordered_map<std::string, const AntlrIrBlobNode *> &declaredBlobs,
    const std::unordered_map<std::string, const AntlrIrLightNode *> &declaredLights,
    const std::unordered_map<std::string, const AntlrIrObjectNode *> &declaredObjects,
    const std::unordered_map<std::string, const AntlrIrCompositeNode *> &declaredComposites,
    const std::unordered_map<std::string, const AntlrIrCsgNode *> &declaredCsgs, int depth)
{
    if (depth > 16) {
        throw std::runtime_error("ANTLR object lowering exceeded max recursion depth");
    }

    const AntlrIrObjectNode *effectiveNode = &node;
    bool unresolvedObjectReference = false;
    std::string unresolvedObjectReferenceIdentifier;
    if (node.hasReference) {
        auto oit = declaredObjects.find(node.referenceIdentifier);
        if (oit == declaredObjects.end()) {
            unresolvedObjectReference = true;
            unresolvedObjectReferenceIdentifier = node.referenceIdentifier;
        } else {
            effectiveNode = oit->second;
        }
    }

    SimpleBody *obj = SimpleBodyFactory::getObject();
    if (effectiveNode->hasColour) {
        obj->objectColour = ModelBuilder::getColour();
        obj->objectColour->Red = effectiveNode->colour.r;
        obj->objectColour->Green = effectiveNode->colour.g;
        obj->objectColour->Blue = effectiveNode->colour.b;
        obj->objectColour->Alpha = effectiveNode->colour.a;
    }
    obj->noShadowFlag = effectiveNode->noShadow ? 1 : 0;
    if (effectiveNode->hasTextureChain) {
        applyObjectTexture(materializeTextureChain(effectiveNode->textureChain, declaredTextures), obj);
    }
    if (effectiveNode->childSphereCount > 0 && effectiveNode->childSpheres[0] != nullptr) {
        obj->Shape = (Geometry *)buildSphereResolved(
            *effectiveNode->childSpheres[0], declaredSpheres, depth + 1);
    } else if (effectiveNode->childPlaneCount > 0 && effectiveNode->childPlanes[0] != nullptr) {
        obj->Shape = (Geometry *)buildPlane(*effectiveNode->childPlanes[0], declaredPlanes);
    } else if (effectiveNode->childBoxCount > 0 && effectiveNode->childBoxes[0] != nullptr) {
        obj->Shape = (Geometry *)buildBox(*effectiveNode->childBoxes[0], declaredBoxes);
    } else if (effectiveNode->childTriangleCount > 0 && effectiveNode->childTriangles[0] != nullptr) {
        obj->Shape = (Geometry *)buildTriangle(*effectiveNode->childTriangles[0], declaredTriangles);
    } else if (
        effectiveNode->childSmoothTriangleCount > 0 &&
        effectiveNode->childSmoothTriangles[0] != nullptr) {
        obj->Shape = (Geometry *)buildSmoothTriangle(
            *effectiveNode->childSmoothTriangles[0], declaredSmoothTriangles);
    } else if (effectiveNode->childQuadricCount > 0 && effectiveNode->childQuadrics[0] != nullptr) {
        obj->Shape = (Geometry *)buildQuadric(*effectiveNode->childQuadrics[0], declaredQuadrics);
    } else if (effectiveNode->childQuarticCount > 0 && effectiveNode->childQuartics[0] != nullptr) {
        obj->Shape = (Geometry *)buildQuartic(*effectiveNode->childQuartics[0]);
    } else if (effectiveNode->childBlobCount > 0 && effectiveNode->childBlobs[0] != nullptr) {
        obj->Shape = (Geometry *)buildBlob(*effectiveNode->childBlobs[0], declaredBlobs);
    } else if (effectiveNode->childLightCount > 0 && effectiveNode->childLights[0] != nullptr) {
        obj->Shape = (Geometry *)buildLight(*effectiveNode->childLights[0], declaredLights);
    } else if (effectiveNode->childObjectCount > 0 && effectiveNode->childObjects[0] != nullptr) {
        SimpleBody *childObj = buildObjectFromIr(*effectiveNode->childObjects[0],
            declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
            declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites, declaredCsgs,
            depth + 1);
        applyTransforms(childObj, effectiveNode->childObjects[0]->transforms,
            effectiveNode->childObjects[0]->transformCount);
        if (childObj->Shape == nullptr) {
            throw std::runtime_error("ANTLR object child object has null Shape at " +
                formatSourceLocation(node));
        }
        obj->Shape = childObj->Shape;
    } else if (
        effectiveNode->childCompositeCount > 0 && effectiveNode->childComposites[0] != nullptr) {
        SimpleBody *childComp = buildCompositeFromIr(*effectiveNode->childComposites[0],
            declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
            declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites, declaredCsgs,
            depth + 1);
        applyTransforms(childComp, effectiveNode->childComposites[0]->transforms,
            effectiveNode->childComposites[0]->transformCount);
        obj->Shape = (Geometry *)childComp;
    } else if (effectiveNode->childCsgCount > 0 && effectiveNode->childCsgs[0] != nullptr) {
        obj->Shape = (Geometry *)buildCsgFromIr(*effectiveNode->childCsgs[0], declaredTextures,
            declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles, declaredSmoothTriangles,
            declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites,
            declaredCsgs, depth + 1);
    } else if (unresolvedObjectReference) {
        Sphere *resolved = nullptr;
        if (buildDeclaredSphereByName(
                unresolvedObjectReferenceIdentifier, declaredSpheres, resolved)) {
            obj->Shape = (Geometry *)resolved;
        } else {
            auto pit = declaredPlanes.find(unresolvedObjectReferenceIdentifier);
            if (pit != declaredPlanes.end()) {
                obj->Shape = (Geometry *)buildPlane(*pit->second, declaredPlanes);
                goto resolved_object_reference_done;
            }
            auto bit = declaredBoxes.find(unresolvedObjectReferenceIdentifier);
            if (bit != declaredBoxes.end()) {
                obj->Shape = (Geometry *)buildBox(*bit->second, declaredBoxes);
                goto resolved_object_reference_done;
            }
            auto tit = declaredTriangles.find(unresolvedObjectReferenceIdentifier);
            if (tit != declaredTriangles.end()) {
                obj->Shape = (Geometry *)buildTriangle(*tit->second, declaredTriangles);
                goto resolved_object_reference_done;
            }
            auto stit = declaredSmoothTriangles.find(unresolvedObjectReferenceIdentifier);
            if (stit != declaredSmoothTriangles.end()) {
                obj->Shape = (Geometry *)buildSmoothTriangle(*stit->second, declaredSmoothTriangles);
                goto resolved_object_reference_done;
            }
            auto qit = declaredQuadrics.find(unresolvedObjectReferenceIdentifier);
            if (qit != declaredQuadrics.end()) {
                obj->Shape = (Geometry *)buildQuadric(*qit->second, declaredQuadrics);
                goto resolved_object_reference_done;
            }
            auto qtit = declaredQuarticsRef().find(unresolvedObjectReferenceIdentifier);
            if (qtit != declaredQuarticsRef().end()) {
                obj->Shape = (Geometry *)buildQuartic(*qtit->second);
                goto resolved_object_reference_done;
            }
            auto blit = declaredBlobs.find(unresolvedObjectReferenceIdentifier);
            if (blit != declaredBlobs.end()) {
                obj->Shape = (Geometry *)buildBlob(*blit->second, declaredBlobs);
                goto resolved_object_reference_done;
            }
            auto oit = declaredObjects.find(unresolvedObjectReferenceIdentifier);
            if (oit != declaredObjects.end()) {
                SimpleBody *resolvedObject = buildObjectFromIr(*oit->second,
                    declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                    declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites,
                    declaredCsgs, depth + 1);
                if (resolvedObject->Shape == nullptr) {
                    throw std::runtime_error("ANTLR object reference resolved to object with null Shape at " +
                        formatSourceLocation(node));
                }
                obj->Shape = resolvedObject->Shape;
            } else {
                auto cit = declaredComposites.find(unresolvedObjectReferenceIdentifier);
                if (cit != declaredComposites.end()) {
                    obj->Shape = (Geometry *)buildCompositeFromIr(*cit->second, declaredTextures,
                        declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                        declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites, declaredCsgs,
                        depth + 1);
                } else {
                    auto csgIt = declaredCsgs.find(unresolvedObjectReferenceIdentifier);
                    if (csgIt != declaredCsgs.end()) {
                        obj->Shape = (Geometry *)buildCsgFromIr(*csgIt->second, declaredTextures,
                            declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                            declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites, declaredCsgs,
                            depth + 1);
                    } else {
                        obj->Shape = (Geometry *)buildSphere(makeFallbackSphereNode());
                    }
                }
            }
            auto lit = declaredLights.find(unresolvedObjectReferenceIdentifier);
            if (lit != declaredLights.end()) {
                obj->Shape = (Geometry *)buildLight(*lit->second, declaredLights);
                goto resolved_object_reference_done;
            }
        }
resolved_object_reference_done:;
    } else if (effectiveNode->childReferenceCount > 0) {
        Sphere *resolved = nullptr;
        if (buildDeclaredSphereByName(
                effectiveNode->childReferenceIdentifiers[0], declaredSpheres, resolved)) {
            obj->Shape = (Geometry *)resolved;
        } else {
            const std::string &name = effectiveNode->childReferenceIdentifiers[0];
            auto pit = declaredPlanes.find(name);
            if (pit != declaredPlanes.end()) {
                obj->Shape = (Geometry *)buildPlane(*pit->second, declaredPlanes);
                goto resolved_child_reference_done;
            }
            auto bit = declaredBoxes.find(name);
            if (bit != declaredBoxes.end()) {
                obj->Shape = (Geometry *)buildBox(*bit->second, declaredBoxes);
                goto resolved_child_reference_done;
            }
            auto tit = declaredTriangles.find(name);
            if (tit != declaredTriangles.end()) {
                obj->Shape = (Geometry *)buildTriangle(*tit->second, declaredTriangles);
                goto resolved_child_reference_done;
            }
            auto stit = declaredSmoothTriangles.find(name);
            if (stit != declaredSmoothTriangles.end()) {
                obj->Shape = (Geometry *)buildSmoothTriangle(*stit->second, declaredSmoothTriangles);
                goto resolved_child_reference_done;
            }
            auto qit = declaredQuadrics.find(name);
            if (qit != declaredQuadrics.end()) {
                obj->Shape = (Geometry *)buildQuadric(*qit->second, declaredQuadrics);
                goto resolved_child_reference_done;
            }
            auto qtit = declaredQuarticsRef().find(name);
            if (qtit != declaredQuarticsRef().end()) {
                obj->Shape = (Geometry *)buildQuartic(*qtit->second);
                goto resolved_child_reference_done;
            }
            auto blit = declaredBlobs.find(name);
            if (blit != declaredBlobs.end()) {
                obj->Shape = (Geometry *)buildBlob(*blit->second, declaredBlobs);
                goto resolved_child_reference_done;
            }
            auto oit = declaredObjects.find(name);
            if (oit != declaredObjects.end()) {
                SimpleBody *resolvedObject = buildObjectFromIr(*oit->second,
                    declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                    declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites,
                    declaredCsgs, depth + 1);
                if (resolvedObject->Shape == nullptr) {
                    throw std::runtime_error("ANTLR object child object reference resolved to null Shape at " +
                        formatSourceLocation(node));
                }
                obj->Shape = resolvedObject->Shape;
            } else {
                auto cit = declaredComposites.find(name);
                if (cit != declaredComposites.end()) {
                    obj->Shape = (Geometry *)buildCompositeFromIr(*cit->second, declaredTextures,
                        declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                        declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites, declaredCsgs,
                        depth + 1);
                } else {
                    auto csgIt = declaredCsgs.find(name);
                    if (csgIt != declaredCsgs.end()) {
                        obj->Shape = (Geometry *)buildCsgFromIr(*csgIt->second, declaredTextures,
                            declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                            declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites, declaredCsgs,
                            depth + 1);
                    } else {
                        obj->Shape = (Geometry *)buildSphere(makeFallbackSphereNode());
                    }
                }
            }
            auto lit = declaredLights.find(name);
            if (lit != declaredLights.end()) {
                obj->Shape = (Geometry *)buildLight(*lit->second, declaredLights);
                goto resolved_child_reference_done;
            }
        }
resolved_child_reference_done:;
    }
    if (obj->Shape == nullptr) {
        obj->Shape = (Geometry *)buildSphere(makeFallbackSphereNode());
    }
    if (effectiveNode->inverted) {
        GeometryOperations::invert(obj);
    }

    for (int i = 0; i < effectiveNode->boundedSphereCount; ++i) {
        if (effectiveNode->boundedSpheres[i] != nullptr) {
            Geometry *shape = (Geometry *)buildSphereResolved(
                *effectiveNode->boundedSpheres[i], declaredSpheres, depth + 1);
            linkShapeToGeometryList(shape, &(obj->boundingShapes));
        }
    }
    for (int i = 0; i < effectiveNode->boundedCsgCount; ++i) {
        if (effectiveNode->boundedCsgs[i] != nullptr) {
            Geometry *shape = (Geometry *)buildCsgFromIr(*effectiveNode->boundedCsgs[i],
                declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites, declaredCsgs,
                depth + 1);
            linkShapeToGeometryList(shape, &(obj->boundingShapes));
        }
    }
    for (int i = 0; i < effectiveNode->clippedSphereCount; ++i) {
        if (effectiveNode->clippedSpheres[i] != nullptr) {
            Geometry *shape = (Geometry *)buildSphereResolved(
                *effectiveNode->clippedSpheres[i], declaredSpheres, depth + 1);
            linkShapeToGeometryList(shape, &(obj->clippingShapes));
        }
    }
    for (int i = 0; i < effectiveNode->clippedCsgCount; ++i) {
        if (effectiveNode->clippedCsgs[i] != nullptr) {
            Geometry *shape = (Geometry *)buildCsgFromIr(*effectiveNode->clippedCsgs[i],
                declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites, declaredCsgs,
                depth + 1);
            linkShapeToGeometryList(shape, &(obj->clippingShapes));
        }
    }
    return obj;
}

SimpleBody *buildCompositeFromIr(const AntlrIrCompositeNode &node,
    const std::unordered_map<std::string, Texture *> &declaredTextures,
    const std::unordered_map<std::string, const AntlrIrSphereNode *> &declaredSpheres,
    const std::unordered_map<std::string, const AntlrIrPlaneNode *> &declaredPlanes,
    const std::unordered_map<std::string, const AntlrIrBoxNode *> &declaredBoxes,
    const std::unordered_map<std::string, const AntlrIrTriangleNode *> &declaredTriangles,
    const std::unordered_map<std::string, const AntlrIrSmoothTriangleNode *> &declaredSmoothTriangles,
    const std::unordered_map<std::string, const AntlrIrQuadricNode *> &declaredQuadrics,
    const std::unordered_map<std::string, const AntlrIrBlobNode *> &declaredBlobs,
    const std::unordered_map<std::string, const AntlrIrLightNode *> &declaredLights,
    const std::unordered_map<std::string, const AntlrIrObjectNode *> &declaredObjects,
    const std::unordered_map<std::string, const AntlrIrCompositeNode *> &declaredComposites,
    const std::unordered_map<std::string, const AntlrIrCsgNode *> &declaredCsgs, int depth)
{
    if (depth > 16) {
        throw std::runtime_error("ANTLR composite lowering exceeded max recursion depth");
    }

    const AntlrIrCompositeNode *effectiveNode = &node;
    if (node.hasReference) {
        auto it = declaredComposites.find(node.referenceIdentifier);
        if (it == declaredComposites.end()) {
            return (SimpleBody *)ModelBuilder::getCompositeObject();
        }
        effectiveNode = it->second;
    }

    Composite *comp = ModelBuilder::getCompositeObject();
    for (int k = 0; k < effectiveNode->childSphereCount; ++k) {
        if (effectiveNode->childSpheres[k] == nullptr) {
            continue;
        }
        Sphere *child = buildSphereResolved(*effectiveNode->childSpheres[k], declaredSpheres, depth + 1);
        SimpleBodyFactory::link((SimpleBody *)child, (SimpleBody **)&(child->nextObject),
            (SimpleBody **)&(comp->Objects));
    }
    for (int k = 0; k < effectiveNode->childObjectCount; ++k) {
        if (effectiveNode->childObjects[k] == nullptr) {
            continue;
        }
        SimpleBody *childObj = buildObjectFromIr(
            *effectiveNode->childObjects[k], declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes,
            declaredTriangles, declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects,
            declaredComposites, declaredCsgs, depth + 1);
        applyTransforms(childObj, effectiveNode->childObjects[k]->transforms,
            effectiveNode->childObjects[k]->transformCount);
        SimpleBodyFactory::link(childObj, &(childObj->nextObject), (SimpleBody **)&(comp->Objects));
    }
    for (int k = 0; k < effectiveNode->childCompositeCount; ++k) {
        if (effectiveNode->childComposites[k] == nullptr) {
            continue;
        }
        SimpleBody *childComp = buildCompositeFromIr(*effectiveNode->childComposites[k],
            declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
            declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites,
            declaredCsgs, depth + 1);
        applyTransforms(childComp, effectiveNode->childComposites[k]->transforms,
            effectiveNode->childComposites[k]->transformCount);
        SimpleBodyFactory::link(childComp, &(childComp->nextObject), (SimpleBody **)&(comp->Objects));
    }
    for (int k = 0; k < effectiveNode->childReferenceCount; ++k) {
        const std::string &name = effectiveNode->childReferenceIdentifiers[k];
        Sphere *resolvedSphere = nullptr;
        if (buildDeclaredSphereByName(name, declaredSpheres, resolvedSphere)) {
            SimpleBodyFactory::link((SimpleBody *)resolvedSphere, (SimpleBody **)&(resolvedSphere->nextObject),
                (SimpleBody **)&(comp->Objects));
            continue;
        }

        auto oit = declaredObjects.find(name);
        if (oit != declaredObjects.end()) {
            SimpleBody *resolvedObject = buildObjectFromIr(
                *oit->second, declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes,
                declaredTriangles, declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects,
                declaredComposites, declaredCsgs, depth + 1);
            SimpleBodyFactory::link(
                resolvedObject, &(resolvedObject->nextObject), (SimpleBody **)&(comp->Objects));
            continue;
        }

        auto cit = declaredComposites.find(name);
        if (cit != declaredComposites.end()) {
            SimpleBody *resolvedComposite = buildCompositeFromIr(
                *cit->second, declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes,
                declaredTriangles, declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites,
                declaredCsgs, depth + 1);
            SimpleBodyFactory::link(resolvedComposite, &(resolvedComposite->nextObject),
                (SimpleBody **)&(comp->Objects));
            continue;
        }

        auto csgIt = declaredCsgs.find(name);
        if (csgIt != declaredCsgs.end()) {
            CSG *resolvedCsg = buildCsgFromIr(*csgIt->second, declaredTextures, declaredSpheres,
                declaredPlanes, declaredBoxes, declaredTriangles, declaredSmoothTriangles, declaredQuadrics, declaredBlobs,
                declaredLights, declaredObjects, declaredComposites, declaredCsgs, depth + 1);
            SimpleBodyFactory::link((SimpleBody *)resolvedCsg, (SimpleBody **)&(resolvedCsg->nextObject),
                (SimpleBody **)&(comp->Objects));
            continue;
        }

        continue;
    }

    for (int i = 0; i < effectiveNode->boundedSphereCount; ++i) {
        if (effectiveNode->boundedSpheres[i] != nullptr) {
            Geometry *shape = (Geometry *)buildSphereResolved(
                *effectiveNode->boundedSpheres[i], declaredSpheres, depth + 1);
            linkShapeToGeometryList(shape, &(comp->boundingShapes));
        }
    }
    for (int i = 0; i < effectiveNode->boundedCsgCount; ++i) {
        if (effectiveNode->boundedCsgs[i] != nullptr) {
            Geometry *shape = (Geometry *)buildCsgFromIr(*effectiveNode->boundedCsgs[i],
                declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites, declaredCsgs,
                depth + 1);
            linkShapeToGeometryList(shape, &(comp->boundingShapes));
        }
    }
    for (int i = 0; i < effectiveNode->clippedSphereCount; ++i) {
        if (effectiveNode->clippedSpheres[i] != nullptr) {
            Geometry *shape = (Geometry *)buildSphereResolved(
                *effectiveNode->clippedSpheres[i], declaredSpheres, depth + 1);
            linkShapeToGeometryList(shape, &(comp->clippingShapes));
        }
    }
    for (int i = 0; i < effectiveNode->clippedCsgCount; ++i) {
        if (effectiveNode->clippedCsgs[i] != nullptr) {
            Geometry *shape = (Geometry *)buildCsgFromIr(*effectiveNode->clippedCsgs[i],
                declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites, declaredCsgs,
                depth + 1);
            linkShapeToGeometryList(shape, &(comp->clippingShapes));
        }
    }
    return (SimpleBody *)comp;
}

CSG *buildCsgFromIr(const AntlrIrCsgNode &node,
    const std::unordered_map<std::string, Texture *> &declaredTextures,
    const std::unordered_map<std::string, const AntlrIrSphereNode *> &declaredSpheres,
    const std::unordered_map<std::string, const AntlrIrPlaneNode *> &declaredPlanes,
    const std::unordered_map<std::string, const AntlrIrBoxNode *> &declaredBoxes,
    const std::unordered_map<std::string, const AntlrIrTriangleNode *> &declaredTriangles,
    const std::unordered_map<std::string, const AntlrIrSmoothTriangleNode *> &declaredSmoothTriangles,
    const std::unordered_map<std::string, const AntlrIrQuadricNode *> &declaredQuadrics,
    const std::unordered_map<std::string, const AntlrIrBlobNode *> &declaredBlobs,
    const std::unordered_map<std::string, const AntlrIrLightNode *> &declaredLights,
    const std::unordered_map<std::string, const AntlrIrObjectNode *> &declaredObjects,
    const std::unordered_map<std::string, const AntlrIrCompositeNode *> &declaredComposites,
    const std::unordered_map<std::string, const AntlrIrCsgNode *> &declaredCsgs, int depth)
{
    if (depth > 16) {
        throw std::runtime_error("ANTLR csg lowering exceeded max recursion depth");
    }

    const AntlrIrCsgNode *effectiveNode = &node;
    AntlrIrCsgNode referenceAsChildNode;
    bool hasReferenceAsChildNode = false;
    if (node.hasReference) {
        auto it = declaredCsgs.find(node.referenceIdentifier);
        if (it != declaredCsgs.end()) {
            effectiveNode = it->second;
        } else {
            // Degrade gracefully for references defined in legacy include files not
            // visible in current ANTLR subset: treat as one child reference and let
            // normal child resolution attempt cross-kind binding.
            referenceAsChildNode = node;
            referenceAsChildNode.hasReference = false;
            referenceAsChildNode.referenceIdentifier.clear();
            referenceAsChildNode.childReferenceCount = 1;
            referenceAsChildNode.childReferenceIdentifiers[0] = node.referenceIdentifier;
            hasReferenceAsChildNode = true;
            effectiveNode = &referenceAsChildNode;
        }
    }
    if (!effectiveNode->hasReference && effectiveNode->childSphereCount == 0 &&
        effectiveNode->childPlaneCount == 0 && effectiveNode->childBoxCount == 0 &&
        effectiveNode->childTriangleCount == 0 && effectiveNode->childSmoothTriangleCount == 0 &&
        effectiveNode->childQuadricCount == 0 && effectiveNode->childQuarticCount == 0 &&
        effectiveNode->childBlobCount == 0 && effectiveNode->childLightCount == 0 &&
        effectiveNode->childObjectCount == 0 && effectiveNode->childCompositeCount == 0 &&
        effectiveNode->childCsgCount == 0 && effectiveNode->childReferenceCount == 0) {
        throw std::runtime_error(
            "ANTLR CSG lowering requires at least one child or reference at " +
            formatSourceLocation(node));
    }

    CSG *container = nullptr;
    if (effectiveNode->op == ANTLR_IR_CSG_UNION) {
        container = ModelBuilder::getCsgUnion();
    } else {
        container = ModelBuilder::getCsgIntersection();
    }

    int firstShapeParsed = LegacyBoolean::FALSE_VALUE;
    auto linkCsgChild = [&](Geometry *child) {
        if (child == nullptr) {
            return;
        }
        if (effectiveNode->op == ANTLR_IR_CSG_DIFFERENCE && firstShapeParsed) {
            GeometryOperations::invert((SimpleBody *)child);
        }
        firstShapeParsed = LegacyBoolean::TRUE_VALUE;
        SimpleBodyFactory::link((SimpleBody *)child, (SimpleBody **)&(child->nextObject),
            (SimpleBody **)&(container->Shapes));
    };

    for (int i = 0; i < effectiveNode->childSphereCount; ++i) {
        if (effectiveNode->childSpheres[i] != nullptr) {
            linkCsgChild((Geometry *)buildSphereResolved(
                *effectiveNode->childSpheres[i], declaredSpheres, depth + 1));
        }
    }
    for (int i = 0; i < effectiveNode->childPlaneCount; ++i) {
        if (effectiveNode->childPlanes[i] != nullptr) {
            linkCsgChild((Geometry *)buildPlane(*effectiveNode->childPlanes[i], declaredPlanes));
        }
    }
    for (int i = 0; i < effectiveNode->childBoxCount; ++i) {
        if (effectiveNode->childBoxes[i] != nullptr) {
            linkCsgChild((Geometry *)buildBox(*effectiveNode->childBoxes[i], declaredBoxes));
        }
    }
    for (int i = 0; i < effectiveNode->childTriangleCount; ++i) {
        if (effectiveNode->childTriangles[i] != nullptr) {
            linkCsgChild((Geometry *)buildTriangle(*effectiveNode->childTriangles[i], declaredTriangles));
        }
    }
    for (int i = 0; i < effectiveNode->childSmoothTriangleCount; ++i) {
        if (effectiveNode->childSmoothTriangles[i] != nullptr) {
            linkCsgChild((Geometry *)buildSmoothTriangle(
                *effectiveNode->childSmoothTriangles[i], declaredSmoothTriangles));
        }
    }
    for (int i = 0; i < effectiveNode->childQuadricCount; ++i) {
        if (effectiveNode->childQuadrics[i] != nullptr) {
            linkCsgChild((Geometry *)buildQuadric(*effectiveNode->childQuadrics[i], declaredQuadrics));
        }
    }
    for (int i = 0; i < effectiveNode->childQuarticCount; ++i) {
        if (effectiveNode->childQuartics[i] != nullptr) {
            linkCsgChild((Geometry *)buildQuartic(*effectiveNode->childQuartics[i]));
        }
    }
    for (int i = 0; i < effectiveNode->childBlobCount; ++i) {
        if (effectiveNode->childBlobs[i] != nullptr) {
            linkCsgChild((Geometry *)buildBlob(*effectiveNode->childBlobs[i], declaredBlobs));
        }
    }
    for (int i = 0; i < effectiveNode->childLightCount; ++i) {
        if (effectiveNode->childLights[i] != nullptr) {
            linkCsgChild((Geometry *)buildLight(*effectiveNode->childLights[i], declaredLights));
        }
    }
    for (int i = 0; i < effectiveNode->childObjectCount; ++i) {
        if (effectiveNode->childObjects[i] != nullptr) {
            linkCsgChild((Geometry *)buildObjectFromIr(*effectiveNode->childObjects[i],
                declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects,
                declaredComposites, declaredCsgs, depth + 1));
        }
    }
    for (int i = 0; i < effectiveNode->childCompositeCount; ++i) {
        if (effectiveNode->childComposites[i] != nullptr) {
            linkCsgChild((Geometry *)buildCompositeFromIr(*effectiveNode->childComposites[i],
                declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites,
                declaredCsgs, depth + 1));
        }
    }
    for (int i = 0; i < effectiveNode->childCsgCount; ++i) {
        if (effectiveNode->childCsgs[i] != nullptr) {
            linkCsgChild((Geometry *)buildCsgFromIr(*effectiveNode->childCsgs[i],
                declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites,
                declaredCsgs, depth + 1));
        }
    }
    for (int i = 0; i < effectiveNode->childReferenceCount; ++i) {
        const std::string &name = effectiveNode->childReferenceIdentifiers[i];
        auto sit = declaredSpheres.find(name);
        if (sit != declaredSpheres.end()) {
            linkCsgChild((Geometry *)buildSphereResolved(*sit->second, declaredSpheres, depth + 1));
            continue;
        }
        auto oit = declaredObjects.find(name);
        if (oit != declaredObjects.end()) {
            linkCsgChild((Geometry *)buildObjectFromIr(
                *oit->second, declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes,
                declaredTriangles, declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects,
                declaredComposites, declaredCsgs, depth + 1));
            continue;
        }
        auto pit = declaredPlanes.find(name);
        if (pit != declaredPlanes.end()) {
            linkCsgChild((Geometry *)buildPlane(*pit->second, declaredPlanes));
            continue;
        }
        auto bit = declaredBoxes.find(name);
        if (bit != declaredBoxes.end()) {
            linkCsgChild((Geometry *)buildBox(*bit->second, declaredBoxes));
            continue;
        }
        auto tit = declaredTriangles.find(name);
        if (tit != declaredTriangles.end()) {
            linkCsgChild((Geometry *)buildTriangle(*tit->second, declaredTriangles));
            continue;
        }
        auto stit = declaredSmoothTriangles.find(name);
        if (stit != declaredSmoothTriangles.end()) {
            linkCsgChild((Geometry *)buildSmoothTriangle(*stit->second, declaredSmoothTriangles));
            continue;
        }
        auto qit = declaredQuadrics.find(name);
        if (qit != declaredQuadrics.end()) {
            linkCsgChild((Geometry *)buildQuadric(*qit->second, declaredQuadrics));
            continue;
        }
        auto qtit = declaredQuarticsRef().find(name);
        if (qtit != declaredQuarticsRef().end()) {
            linkCsgChild((Geometry *)buildQuartic(*qtit->second));
            continue;
        }
        auto blit = declaredBlobs.find(name);
        if (blit != declaredBlobs.end()) {
            linkCsgChild((Geometry *)buildBlob(*blit->second, declaredBlobs));
            continue;
        }
        auto cit = declaredComposites.find(name);
        if (cit != declaredComposites.end()) {
            linkCsgChild((Geometry *)buildCompositeFromIr(*cit->second,
                declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites,
                declaredCsgs, depth + 1));
            continue;
        }
        auto csgIt = declaredCsgs.find(name);
        if (csgIt != declaredCsgs.end()) {
            linkCsgChild((Geometry *)buildCsgFromIr(*csgIt->second,
                declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites,
                declaredCsgs, depth + 1));
            continue;
        }

        if (!hasReferenceAsChildNode) {
            continue;
        }
    }

    if (!firstShapeParsed) {
        linkCsgChild((Geometry *)buildSphere(makeFallbackSphereNode()));
    }

    if (effectiveNode->inverted) {
        GeometryOperations::invert((SimpleBody *)container);
    }
    applyTransforms((SimpleBody *)container, effectiveNode->transforms, effectiveNode->transformCount);
    return container;
}

Texture *cloneTexture(Texture *texture)
{
    if (texture == nullptr) {
        return nullptr;
    }
    return TextureUtils::copyTexture(texture);
}

Texture *cloneDefaultTexture()
{
    return cloneTexture(TextureUtils::defaultTexture());
}

Texture *materializeTextureChain(const AntlrIrTextureChain &chain,
    const std::unordered_map<std::string, Texture *> &declaredTextures)
{
    Texture *head = nullptr;
    for (const std::string &name : chain.simpleReferenceIdentifiers) {
        auto it = declaredTextures.find(name);
        Texture *piece = nullptr;
        if (it != declaredTextures.end()) {
            piece = cloneTexture(it->second);
        } else {
            piece = cloneDefaultTexture();
        }
        if (piece == nullptr) {
            continue;
        }
        Texture *tail = piece;
        while (tail->Next_Texture != nullptr) {
            tail = tail->Next_Texture;
        }
        tail->Next_Texture = head;
        head = piece;
    }
    return head;
}

void applyShapeTexture(Texture *srcTexture, Geometry *shape)
{
    if (srcTexture == nullptr || shape == nullptr) {
        return;
    }
    Texture *tail = srcTexture;
    while (tail->Next_Texture != nullptr) {
        tail = tail->Next_Texture;
    }
    tail->Next_Texture = shape->Shape_Texture;
    shape->Shape_Texture = srcTexture;
}

void applyObjectTexture(Texture *srcTexture, SimpleBody *object)
{
    if (srcTexture == nullptr || object == nullptr) {
        return;
    }
    if (object->objectTexture == TextureUtils::defaultTexture()) {
        object->objectTexture = srcTexture;
    } else {
        Texture *tail = srcTexture;
        while (tail->Next_Texture != nullptr) {
            tail = tail->Next_Texture;
        }
        tail->Next_Texture = object->objectTexture;
        object->objectTexture = srcTexture;
    }
}
#endif

void applyCameraNode(const AntlrIrCameraNode &node, RenderFrame *framePtr,
    const std::unordered_map<std::string, const AntlrIrCameraNode *> &declaredCameras);

void applyCameraNode(const AntlrIrCameraNode &node, RenderFrame *framePtr,
    const std::unordered_map<std::string, const AntlrIrCameraNode *> &declaredCameras)
{
    const AntlrIrCameraNode *effectiveNode = &node;
    if (node.hasReference) {
        auto it = declaredCameras.find(node.referenceIdentifier);
        if (it == declaredCameras.end()) {
            return;
        }
        effectiveNode = it->second;
    }

    Camera *camera = &(framePtr->viewPoint);
    camera->initializeDefaults();

    for (int i = 0; i < effectiveNode->opCount; ++i) {
        const AntlrIrCameraOp &op = effectiveNode->ops[i];
        Vector3Dd v = toVector(op.vectorValue);
        if (op.kind == ANTLR_IR_CAMERA_REF) {
            throw std::runtime_error("ANTLR lowering does not support camera reference yet");
        } else if (op.kind == ANTLR_IR_CAMERA_LOCATION) {
            camera->Location = v;
        } else if (op.kind == ANTLR_IR_CAMERA_DIRECTION) {
            camera->Direction = v;
        } else if (op.kind == ANTLR_IR_CAMERA_UP) {
            camera->Up = v;
        } else if (op.kind == ANTLR_IR_CAMERA_RIGHT) {
            camera->Right = v;
        } else if (op.kind == ANTLR_IR_CAMERA_SKY) {
            camera->Sky = v;
        } else if (op.kind == ANTLR_IR_CAMERA_LOOK_AT) {
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
        } else if (op.kind == ANTLR_IR_CAMERA_TRANSLATE) {
            GeometryOperations::translate((SimpleBody *)camera, &v);
        } else if (op.kind == ANTLR_IR_CAMERA_ROTATE) {
            GeometryOperations::rotate((SimpleBody *)camera, &v);
        } else if (op.kind == ANTLR_IR_CAMERA_SCALE) {
            GeometryOperations::scale((SimpleBody *)camera, &v);
        }
    }
}
}

void
AntlrSceneLowering::applyProgram(const AntlrSceneIrProgram &program, RenderFrame *framePtr)
{
    if (framePtr == nullptr) {
        throw std::runtime_error("ANTLR lowering received null RenderFrame");
    }

    std::unordered_map<std::string, const AntlrIrCameraNode *> declaredCameras;
#ifndef POV_ANTLR_MINIMAL_BRIDGE
    std::unordered_map<std::string, Texture *> declaredTextures;
    std::unordered_map<std::string, const AntlrIrSphereNode *> declaredSpheres;
    std::unordered_map<std::string, const AntlrIrPlaneNode *> declaredPlanes;
    std::unordered_map<std::string, const AntlrIrBoxNode *> declaredBoxes;
    std::unordered_map<std::string, const AntlrIrTriangleNode *> declaredTriangles;
    std::unordered_map<std::string, const AntlrIrSmoothTriangleNode *> declaredSmoothTriangles;
    std::unordered_map<std::string, const AntlrIrQuadricNode *> declaredQuadrics;
    std::unordered_map<std::string, const AntlrIrQuarticNode *> declaredQuartics;
    std::unordered_map<std::string, const AntlrIrBlobNode *> declaredBlobs;
    std::unordered_map<std::string, const AntlrIrObjectNode *> declaredObjects;
    std::unordered_map<std::string, const AntlrIrCompositeNode *> declaredComposites;
    std::unordered_map<std::string, const AntlrIrLightNode *> declaredLights;
    std::unordered_map<std::string, const AntlrIrCsgNode *> declaredCsgs;
    gDeclaredQuartics = &declaredQuartics;
#endif
    for (int i = 0; i < program.nodeCount; ++i) {
        AntlrSceneIrNode *node = program.nodes[i];
        if (node == nullptr) {
            continue;
        }
        if (node->kind == ANTLR_IR_MAX_TRACE_LEVEL_NODE) {
            const AntlrIrMaxTraceLevelNode *n = (const AntlrIrMaxTraceLevelNode *)node;
            RenderRuntimeState::maxTraceLevel() = n->value;
        } else if (node->kind == ANTLR_IR_FOG_NODE) {
            const AntlrIrFogNode *n = (const AntlrIrFogNode *)node;
            if (n->hasColour) {
                framePtr->fogColour.Red = n->colour.r;
                framePtr->fogColour.Green = n->colour.g;
                framePtr->fogColour.Blue = n->colour.b;
                framePtr->fogColour.Alpha = n->colour.a;
            }
            if (n->hasDistance) {
                framePtr->fogDistance = n->distance;
            }
        } else if (node->kind == ANTLR_IR_CAMERA_NODE) {
            applyCameraNode(*(const AntlrIrCameraNode *)node, framePtr, declaredCameras);
#ifndef POV_ANTLR_MINIMAL_BRIDGE
        } else if (node->kind == ANTLR_IR_DECLARE_NODE) {
            const AntlrIrDeclareNode *n = (const AntlrIrDeclareNode *)node;
            if (n->hasTextureChainValue) {
                Texture *tex = materializeTextureChain(n->textureChainValue, declaredTextures);
                if (tex == nullptr) {
                    tex = cloneDefaultTexture();
                }
                if (tex != nullptr) {
                    tex->constantFlag = LegacyBoolean::TRUE_VALUE;
                }
                declaredTextures[n->identifier] = tex;
            } else if (n->valueKind == AntlrIrDeclareNode::DECLARE_SPHERE && n->hasSphereValue) {
                declaredSpheres[n->identifier] = n->sphereValue;
            } else if (n->valueKind == AntlrIrDeclareNode::DECLARE_PLANE && n->hasPlaneValue) {
                declaredPlanes[n->identifier] = n->planeValue;
            } else if (n->valueKind == AntlrIrDeclareNode::DECLARE_BOX && n->hasBoxValue) {
                declaredBoxes[n->identifier] = n->boxValue;
            } else if (n->valueKind == AntlrIrDeclareNode::DECLARE_TRIANGLE && n->hasTriangleValue) {
                declaredTriangles[n->identifier] = n->triangleValue;
            } else if (
                n->valueKind == AntlrIrDeclareNode::DECLARE_SMOOTH_TRIANGLE &&
                n->hasSmoothTriangleValue) {
                declaredSmoothTriangles[n->identifier] = n->smoothTriangleValue;
            } else if (n->valueKind == AntlrIrDeclareNode::DECLARE_QUADRIC && n->hasQuadricValue) {
                declaredQuadrics[n->identifier] = n->quadricValue;
            } else if (n->valueKind == AntlrIrDeclareNode::DECLARE_QUARTIC && n->hasQuarticValue) {
                declaredQuartics[n->identifier] = n->quarticValue;
            } else if (n->valueKind == AntlrIrDeclareNode::DECLARE_BLOB && n->hasBlobValue) {
                declaredBlobs[n->identifier] = n->blobValue;
            } else if (n->valueKind == AntlrIrDeclareNode::DECLARE_OBJECT && n->hasObjectValue) {
                declaredObjects[n->identifier] = n->objectValue;
            } else if (
                n->valueKind == AntlrIrDeclareNode::DECLARE_COMPOSITE && n->hasCompositeValue) {
                declaredComposites[n->identifier] = n->compositeValue;
            } else if (n->valueKind == AntlrIrDeclareNode::DECLARE_LIGHT && n->hasLightValue) {
                declaredLights[n->identifier] = n->lightValue;
            } else if (n->valueKind == AntlrIrDeclareNode::DECLARE_CSG && n->hasCsgValue) {
                declaredCsgs[n->identifier] = n->csgValue;
            } else if (n->valueKind == AntlrIrDeclareNode::DECLARE_CAMERA && n->hasCameraValue) {
                declaredCameras[n->identifier] = n->cameraValue;
            }
        } else if (node->kind == ANTLR_IR_DEFAULT_TEXTURE_NODE) {
            const AntlrIrDefaultTextureNode *n = (const AntlrIrDefaultTextureNode *)node;
            if (n->hasTextureChain) {
                Texture *tex = materializeTextureChain(n->textureChain, declaredTextures);
                if (tex != nullptr) {
                    tex->constantFlag = LegacyBoolean::FALSE_VALUE;
                    TextureUtils::defaultTexture() = tex;
                    TextureUtils::defaultTexture()->constantFlag = LegacyBoolean::TRUE_VALUE;
                }
            }
        } else if (node->kind == ANTLR_IR_SPHERE_NODE) {
            const AntlrIrSphereNode *n = (const AntlrIrSphereNode *)node;
            Sphere *sphere = nullptr;
            if (n->hasReferenceBase) {
                auto it = declaredSpheres.find(n->referenceIdentifier);
                if (it == declaredSpheres.end()) {
                    sphere = buildSphere(makeFallbackSphereNode());
                } else {
                    sphere = buildSphereResolved(*it->second, declaredSpheres, 0);
                }
            } else {
                sphere = buildSphereResolved(*n, declaredSpheres, 0);
            }
            if (n->hasTextureChain) {
                applyShapeTexture(materializeTextureChain(n->textureChain, declaredTextures), sphere);
            }
            SimpleBodyFactory::link((SimpleBody *)sphere,
                (SimpleBody **)&(sphere->nextObject), (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == ANTLR_IR_PLANE_NODE) {
            const AntlrIrPlaneNode *n = (const AntlrIrPlaneNode *)node;
            InfinitePlane *plane = buildPlane(*n, declaredPlanes);
            if (n->hasTextureChain) {
                applyShapeTexture(materializeTextureChain(n->textureChain, declaredTextures), (Geometry *)plane);
            }
            SimpleBodyFactory::link((SimpleBody *)plane,
                (SimpleBody **)&(plane->nextObject), (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == ANTLR_IR_BOX_NODE) {
            const AntlrIrBoxNode *n = (const AntlrIrBoxNode *)node;
            Box *box = buildBox(*n, declaredBoxes);
            if (n->hasTextureChain) {
                applyShapeTexture(materializeTextureChain(n->textureChain, declaredTextures), (Geometry *)box);
            }
            SimpleBodyFactory::link((SimpleBody *)box,
                (SimpleBody **)&(box->nextObject), (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == ANTLR_IR_TRIANGLE_NODE) {
            const AntlrIrTriangleNode *n = (const AntlrIrTriangleNode *)node;
            Triangle *triangle = buildTriangle(*n, declaredTriangles);
            if (n->hasTextureChain) {
                applyShapeTexture(materializeTextureChain(n->textureChain, declaredTextures), (Geometry *)triangle);
            }
            SimpleBodyFactory::link((SimpleBody *)triangle,
                (SimpleBody **)&(triangle->nextObject), (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == ANTLR_IR_SMOOTH_TRIANGLE_NODE) {
            const AntlrIrSmoothTriangleNode *n = (const AntlrIrSmoothTriangleNode *)node;
            SmoothTriangle *triangle = buildSmoothTriangle(*n, declaredSmoothTriangles);
            if (n->hasTextureChain) {
                applyShapeTexture(materializeTextureChain(n->textureChain, declaredTextures), (Geometry *)triangle);
            }
            SimpleBodyFactory::link((SimpleBody *)triangle,
                (SimpleBody **)&(triangle->nextObject), (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == ANTLR_IR_QUADRIC_NODE) {
            const AntlrIrQuadricNode *n = (const AntlrIrQuadricNode *)node;
            Quadric *quadric = buildQuadric(*n, declaredQuadrics);
            if (n->hasTextureChain) {
                applyShapeTexture(materializeTextureChain(n->textureChain, declaredTextures), (Geometry *)quadric);
            }
            SimpleBodyFactory::link((SimpleBody *)quadric,
                (SimpleBody **)&(quadric->nextObject), (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == ANTLR_IR_QUARTIC_NODE) {
            const AntlrIrQuarticNode *n = (const AntlrIrQuarticNode *)node;
            PolynomialShape *poly = buildQuartic(*n);
            if (n->hasTextureChain) {
                applyShapeTexture(materializeTextureChain(n->textureChain, declaredTextures), (Geometry *)poly);
            }
            SimpleBodyFactory::link((SimpleBody *)poly,
                (SimpleBody **)&(poly->nextObject), (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == ANTLR_IR_BLOB_NODE) {
            const AntlrIrBlobNode *n = (const AntlrIrBlobNode *)node;
            Blob *blob = buildBlob(*n, declaredBlobs);
            if (n->hasTextureChain) {
                applyShapeTexture(materializeTextureChain(n->textureChain, declaredTextures), (Geometry *)blob);
            }
            SimpleBodyFactory::link((SimpleBody *)blob,
                (SimpleBody **)&(blob->nextObject), (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == ANTLR_IR_OBJECT_NODE) {
            const AntlrIrObjectNode *n = (const AntlrIrObjectNode *)node;
            SimpleBody *obj = buildObjectFromIr(
                *n, declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects,
                declaredComposites, declaredCsgs, 0);
            applyTransforms(obj, n->transforms, n->transformCount);
            SimpleBodyFactory::link(obj, &(obj->nextObject), (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == ANTLR_IR_COMPOSITE_NODE) {
            const AntlrIrCompositeNode *n = (const AntlrIrCompositeNode *)node;
            SimpleBody *comp = buildCompositeFromIr(
                *n, declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites,
                declaredCsgs, 0);
            applyTransforms(comp, n->transforms, n->transformCount);
            SimpleBodyFactory::link(comp, &(comp->nextObject), (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == ANTLR_IR_LIGHT_NODE) {
            const AntlrIrLightNode *n = (const AntlrIrLightNode *)node;
            Light *light = buildLight(*n, declaredLights);
            light->Next_Light_Source = framePtr->Light_Sources;
            framePtr->Light_Sources = light;
        } else if (node->kind == ANTLR_IR_CSG_NODE) {
            const AntlrIrCsgNode *n = (const AntlrIrCsgNode *)node;
            CSG *csg = buildCsgFromIr(*n, declaredTextures, declaredSpheres,
                declaredPlanes, declaredBoxes, declaredTriangles, declaredSmoothTriangles, declaredQuadrics, declaredBlobs,
                declaredLights, declaredObjects, declaredComposites, declaredCsgs, 0);
            SimpleBodyFactory::link((SimpleBody *)csg, (SimpleBody **)&(csg->nextObject),
                (SimpleBody **)&(framePtr->Objects));
#endif
        }
    }
#ifndef POV_ANTLR_MINIMAL_BRIDGE
    gDeclaredQuartics = nullptr;
#endif
}
