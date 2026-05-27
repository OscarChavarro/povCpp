#include "io/pov/antlr/AntlrSceneLowering.h"

#include <cmath>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "common/LegacyBoolean.h"
#include "common/color/Color.h"
#include "common/logger/Logger.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/camera/Camera.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/surface/parametric/ParametricPatch.h"
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
#include "media/RGBAImage.h"
#include "io/image/GifFormat.h"
#include "io/image/TargaFormat.h"
#include "io/image/IffFormat.h"
#include "render/RenderFrame.h"

#ifdef POV_WITH_ANTLR_RUNTIME
extern std::unordered_map<std::string, AntlrIrColor> gAntlrDeclaredColours;
#endif

namespace {
const std::unordered_map<std::string, const AntlrIrQuarticNode *> *gDeclaredQuartics = nullptr;
const std::unordered_map<std::string, Texture *> *gDeclaredTextures = nullptr;

const std::unordered_map<std::string, const AntlrIrQuarticNode *> &declaredQuarticsRef()
{
    static const std::unordered_map<std::string, const AntlrIrQuarticNode *> kEmpty;
    return gDeclaredQuartics != nullptr ? *gDeclaredQuartics : kEmpty;
}

Vector3Dd toVector(const AntlrIrVector3 &v)
{
    return Vector3Dd(v.x, v.y, v.z);
}

std::string toLowerAscii(std::string s)
{
    for (char &c : s) {
        c = (char)std::tolower((unsigned char)c);
    }
    return s;
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

bool shouldLogMonkeyDiagnostics()
{
    const char *flag = std::getenv("POVCPP_DIAG_MONKEY");
    return flag != nullptr && flag[0] != '\0';
}

int countGeometryChain(const Geometry *g)
{
    int count = 0;
    for (const Geometry *cur = g; cur != nullptr; cur = cur->nextObject) {
        ++count;
    }
    return count;
}

void logCameraOnce(const char *prefix, const Camera *camera)
{
    static int logged = 0;
    if (!shouldLogMonkeyDiagnostics() || logged++ > 0 || camera == nullptr) {
        return;
    }
    Logger::info(
        "[DIAG-MONKEY] %s camera loc=<%.6f,%.6f,%.6f> dir=<%.6f,%.6f,%.6f> up=<%.6f,%.6f,%.6f> right=<%.6f,%.6f,%.6f> sky=<%.6f,%.6f,%.6f>\n",
        prefix,
        camera->Location.x, camera->Location.y, camera->Location.z,
        camera->Direction.x, camera->Direction.y, camera->Direction.z,
        camera->Up.x, camera->Up.y, camera->Up.z,
        camera->Right.x, camera->Right.y, camera->Right.z,
        camera->Sky.x, camera->Sky.y, camera->Sky.z);
}

void logCameraOp(const char *prefix, const char *opName, const Vector3Dd *value)
{
    if (!shouldLogMonkeyDiagnostics() || value == nullptr) {
        return;
    }
    Logger::info(
        "[DIAG-MONKEY] %s camera op=%s value=<%.6f,%.6f,%.6f>\n",
        prefix, opName, value->x, value->y, value->z);
}

void logObjectOnce(const char *prefix, const SimpleBody *object)
{
    static int logged = 0;
    if (!shouldLogMonkeyDiagnostics() || logged++ > 0 || object == nullptr) {
        return;
    }

    const Geometry *shape = object->Shape;
    const Geometry *bounding = object->boundingShapes;
    const Geometry *clipping = object->clippingShapes;
    Logger::info("[DIAG-MONKEY] %s object type=%d shapeCount=%d boundingCount=%d clippingCount=%d\n",
        prefix, object->Type, countGeometryChain(shape), countGeometryChain(bounding),
        countGeometryChain(clipping));
    if (shape != nullptr) {
        Logger::info("[DIAG-MONKEY] %s object firstShapeType=%d\n", prefix, shape->Type);
    }
    if (bounding != nullptr) {
        Logger::info("[DIAG-MONKEY] %s boundingFirstType=%d\n", prefix, bounding->Type);
    }
    if (clipping != nullptr) {
        Logger::info("[DIAG-MONKEY] %s clippingFirstType=%d\n", prefix, clipping->Type);
    }
#ifndef POV_ANTLR_MINIMAL_BRIDGE
    auto logCsgChildren = [prefix](const char *tag, const Geometry *maybeCsg) {
        if (maybeCsg == nullptr || maybeCsg->Type != GeometryOperations::CSG_INTERSECTION_TYPE) {
            return;
        }
        const CSG *csg = (const CSG *)maybeCsg;
        int childIndex = 0;
        for (const Geometry *child = csg->Shapes; child != nullptr; child = child->nextObject, ++childIndex) {
            Logger::info("[DIAG-MONKEY] %s %s child[%d] type=%d\n", prefix, tag, childIndex, child->Type);
            if (child->Type == GeometryOperations::PLANE_TYPE) {
                const InfinitePlane *plane = (const InfinitePlane *)child;
                Logger::info("[DIAG-MONKEY] %s %s plane normal=<%.6f,%.6f,%.6f> dist=%.6f\n",
                    prefix, tag, plane->normalVector.x, plane->normalVector.y, plane->normalVector.z,
                    plane->Distance);
            } else if (child->Type == GeometryOperations::CSG_INTERSECTION_TYPE) {
                const CSG *nested = (const CSG *)child;
                int nestedIndex = 0;
                for (const Geometry *nestedChild = nested->Shapes; nestedChild != nullptr;
                    nestedChild = nestedChild->nextObject, ++nestedIndex) {
                    if (nestedChild->Type == GeometryOperations::PLANE_TYPE) {
                        const InfinitePlane *plane = (const InfinitePlane *)nestedChild;
                        Logger::info("[DIAG-MONKEY] %s %s nested child[%d] plane normal=<%.6f,%.6f,%.6f> dist=%.6f\n",
                            prefix, tag, nestedIndex, plane->normalVector.x, plane->normalVector.y,
                            plane->normalVector.z, plane->Distance);
                    }
                }
            } else if (child->Type == GeometryOperations::POLY_TYPE) {
                const PolynomialShape *poly = (const PolynomialShape *)child;
                const int coeffCount = PolynomialShape::termCounts()[poly->Order];
                Logger::info("[DIAG-MONKEY] %s %s poly order=%d coeffCount=%d\n",
                    prefix, tag, poly->Order, coeffCount);
                for (int i = 0; i < coeffCount; ++i) {
                    if (std::fabs(poly->Coeffs[i]) > 1.0e-12) {
                        Logger::info("[DIAG-MONKEY] %s %s poly coeff[%d]=%.6f\n",
                            prefix, tag, i, poly->Coeffs[i]);
                    }
                }
                if (poly->Transform != nullptr) {
                    Logger::info(
                        "[DIAG-MONKEY] %s %s poly transform row0=<%.6f,%.6f,%.6f,%.6f> row1=<%.6f,%.6f,%.6f,%.6f> row2=<%.6f,%.6f,%.6f,%.6f> row3=<%.6f,%.6f,%.6f,%.6f>\n",
                        prefix, tag,
                        poly->Transform->matrix[0][0], poly->Transform->matrix[0][1], poly->Transform->matrix[0][2], poly->Transform->matrix[0][3],
                        poly->Transform->matrix[1][0], poly->Transform->matrix[1][1], poly->Transform->matrix[1][2], poly->Transform->matrix[1][3],
                        poly->Transform->matrix[2][0], poly->Transform->matrix[2][1], poly->Transform->matrix[2][2], poly->Transform->matrix[2][3],
                        poly->Transform->matrix[3][0], poly->Transform->matrix[3][1], poly->Transform->matrix[3][2], poly->Transform->matrix[3][3]);
                }
            }
        }
    };
    logCsgChildren("object csg", shape);
    logCsgChildren("bounding csg", bounding);
#endif
}

#ifndef POV_ANTLR_MINIMAL_BRIDGE
void logPlaneOnce(const char *prefix, const InfinitePlane *plane)
{
    if (!shouldLogMonkeyDiagnostics() || plane == nullptr) {
        return;
    }
    const Texture *texture = plane->Shape_Texture;
    const RGBAColor *colour = plane->Shape_Colour;
    Logger::info(
        "[DIAG-MONKEY] %s plane type=%d normal=<%.6f,%.6f,%.6f> dist=%.6f vpcached=%d texNum=%d colour=<%.6f,%.6f,%.6f,%.6f>\n",
        prefix, plane->Type, plane->normalVector.x, plane->normalVector.y,
        plane->normalVector.z, plane->Distance, plane->VPCached,
        texture != nullptr ? texture->textureNumber : -1,
        colour != nullptr ? colour->Red : -1.0,
        colour != nullptr ? colour->Green : -1.0,
        colour != nullptr ? colour->Blue : -1.0,
        colour != nullptr ? colour->Alpha : -1.0);
}

void logCsgOnce(const char *prefix, const CSG *csg)
{
    if (!shouldLogMonkeyDiagnostics() || csg == nullptr) {
        return;
    }
    Logger::info("[DIAG-MONKEY] %s csg type=%d shapeCount=%d\n",
        prefix, csg->Type, countGeometryChain(csg->Shapes));
    if (csg->Shapes != nullptr) {
        Logger::info("[DIAG-MONKEY] %s csg firstShapeType=%d\n", prefix, csg->Shapes->Type);
    }
}

void logSphereOnce(const char *prefix, const Sphere *sphere)
{
    if (!shouldLogMonkeyDiagnostics() || sphere == nullptr) {
        return;
    }
    const RGBAColor *colour = sphere->Shape_Colour;
    const Texture *texture = sphere->Shape_Texture;
    Logger::info(
        "[DIAG-MONKEY] %s sphere center=<%.6f,%.6f,%.6f> radius=%.6f colour=<%.6f,%.6f,%.6f,%.6f> texNum=%d amb=%.6f diff=%.6f spec=%.6f rough=%.6f phong=%.6f texColour1=<%.6f,%.6f,%.6f,%.6f>\n",
        prefix, sphere->Center.x, sphere->Center.y, sphere->Center.z, sphere->Radius,
        colour != nullptr ? colour->Red : -1.0,
        colour != nullptr ? colour->Green : -1.0,
        colour != nullptr ? colour->Blue : -1.0,
        colour != nullptr ? colour->Alpha : -1.0,
        texture != nullptr ? texture->textureNumber : -1,
        texture != nullptr ? texture->objectAmbient : -1.0,
        texture != nullptr ? texture->objectDiffuse : -1.0,
        texture != nullptr ? texture->objectSpecular : -1.0,
        texture != nullptr ? texture->objectRoughness : -1.0,
        texture != nullptr ? texture->objectPhong : -1.0,
        texture != nullptr && texture->Colour1 != nullptr ? texture->Colour1->Red : -1.0,
        texture != nullptr && texture->Colour1 != nullptr ? texture->Colour1->Green : -1.0,
        texture != nullptr && texture->Colour1 != nullptr ? texture->Colour1->Blue : -1.0,
        texture != nullptr && texture->Colour1 != nullptr ? texture->Colour1->Alpha : -1.0);
}

bool shouldLogTextureState()
{
    const char *flag = std::getenv("POVCPP_DIAG_TEXTURE_STATE");
    return flag != nullptr && flag[0] != '\0';
}

void logTextureStateAntlr(const char *prefix, const Texture *texture)
{
    if (!shouldLogTextureState() || texture == nullptr) {
        return;
    }

    Logger::info(
        "[TEXTURE-STATE] %s type=%d ambient=%.6f diffuse=%.6f brilliance=%.6f reflection=%.6f turbulence=%.6f frequency=%.6f phase=%.6f octaves=%d bumpNumber=%d bumpAmount=%.6f texXform=%s\n",
        prefix,
        texture->textureNumber,
        texture->objectAmbient,
        texture->objectDiffuse,
        texture->objectBrilliance,
        texture->objectReflection,
        texture->Turbulence,
        texture->Frequency,
        texture->Phase,
        texture->Octaves,
        texture->bumpNumber,
        texture->bumpAmount,
        texture->Texture_Transformation != nullptr ? "yes" : "no");
    if (texture->Texture_Transformation != nullptr) {
        Logger::info(
            "[TEXTURE-STATE] %s xform row0=<%.6f,%.6f,%.6f,%.6f> row1=<%.6f,%.6f,%.6f,%.6f> row2=<%.6f,%.6f,%.6f,%.6f> row3=<%.6f,%.6f,%.6f,%.6f>\n",
            prefix,
            texture->Texture_Transformation->matrix[0][0], texture->Texture_Transformation->matrix[0][1],
            texture->Texture_Transformation->matrix[0][2], texture->Texture_Transformation->matrix[0][3],
            texture->Texture_Transformation->matrix[1][0], texture->Texture_Transformation->matrix[1][1],
            texture->Texture_Transformation->matrix[1][2], texture->Texture_Transformation->matrix[1][3],
            texture->Texture_Transformation->matrix[2][0], texture->Texture_Transformation->matrix[2][1],
            texture->Texture_Transformation->matrix[2][2], texture->Texture_Transformation->matrix[2][3],
            texture->Texture_Transformation->matrix[3][0], texture->Texture_Transformation->matrix[3][1],
            texture->Texture_Transformation->matrix[3][2], texture->Texture_Transformation->matrix[3][3]);
    }
}

void logLightOnce(const char *prefix, const Light *light)
{
    if (!shouldLogMonkeyDiagnostics() || light == nullptr) {
        return;
    }
    const RGBAColor *colour = light->Shape_Colour;
    Logger::info(
        "[DIAG-MONKEY] %s light center=<%.6f,%.6f,%.6f> pointsAt=<%.6f,%.6f,%.6f> colour=<%.6f,%.6f,%.6f,%.6f> coeff=%.6f radius=%.6f falloff=%.6f type=%d\n",
        prefix,
        light->Center.x, light->Center.y, light->Center.z,
        light->pointsAt.x, light->pointsAt.y, light->pointsAt.z,
        colour != nullptr ? colour->Red : -1.0,
        colour != nullptr ? colour->Green : -1.0,
        colour != nullptr ? colour->Blue : -1.0,
        colour != nullptr ? colour->Alpha : -1.0,
        light->Coeff, light->Radius, light->Falloff, light->Type);
}
#endif

#ifndef POV_ANTLR_MINIMAL_BRIDGE
void logPolyOnce(const char *prefix, const PolynomialShape *shape)
{
    if (!shouldLogMonkeyDiagnostics() || shape == nullptr) {
        return;
    }
    const int coeffCount = PolynomialShape::termCounts()[shape->Order];
    const Texture *texture = shape->Shape_Texture;
    const RGBAColor *colour = shape->Shape_Colour;
    Logger::info("[DIAG-MONKEY] %s quartic order=%d sturm=%d coeffCount=%d\n",
        prefix, shape->Order, shape->sturmFlag, coeffCount);
    Logger::info(
        "[DIAG-MONKEY] %s quartic colour=<%.6f,%.6f,%.6f,%.6f> texNum=%d amb=%.6f diff=%.6f spec=%.6f rough=%.6f phong=%.6f texColour1=<%.6f,%.6f,%.6f,%.6f>\n",
        prefix,
        colour != nullptr ? colour->Red : -1.0,
        colour != nullptr ? colour->Green : -1.0,
        colour != nullptr ? colour->Blue : -1.0,
        colour != nullptr ? colour->Alpha : -1.0,
        texture != nullptr ? texture->textureNumber : -1,
        texture != nullptr ? texture->objectAmbient : -1.0,
        texture != nullptr ? texture->objectDiffuse : -1.0,
        texture != nullptr ? texture->objectSpecular : -1.0,
        texture != nullptr ? texture->objectRoughness : -1.0,
        texture != nullptr ? texture->objectPhong : -1.0,
        texture != nullptr && texture->Colour1 != nullptr ? texture->Colour1->Red : -1.0,
        texture != nullptr && texture->Colour1 != nullptr ? texture->Colour1->Green : -1.0,
        texture != nullptr && texture->Colour1 != nullptr ? texture->Colour1->Blue : -1.0,
        texture != nullptr && texture->Colour1 != nullptr ? texture->Colour1->Alpha : -1.0);
    Logger::info(
        "[DIAG-MONKEY] %s quartic texture full number=%d ambient=%.6f diffuse=%.6f brilliance=%.6f refraction=%.6f transmit=%.6f specular=%.6f roughness=%.6f phong=%.6f colour2=<%.6f,%.6f,%.6f,%.6f>\n",
        prefix,
        texture != nullptr ? texture->textureNumber : -1,
        texture != nullptr ? texture->objectAmbient : -1.0,
        texture != nullptr ? texture->objectDiffuse : -1.0,
        texture != nullptr ? texture->objectBrilliance : -1.0,
        texture != nullptr ? texture->objectRefraction : -1.0,
        texture != nullptr ? texture->objectTransmit : -1.0,
        texture != nullptr ? texture->objectSpecular : -1.0,
        texture != nullptr ? texture->objectRoughness : -1.0,
        texture != nullptr ? texture->objectPhong : -1.0,
        texture != nullptr && texture->Colour2 != nullptr ? texture->Colour2->Red : -1.0,
        texture != nullptr && texture->Colour2 != nullptr ? texture->Colour2->Green : -1.0,
        texture != nullptr && texture->Colour2 != nullptr ? texture->Colour2->Blue : -1.0,
        texture != nullptr && texture->Colour2 != nullptr ? texture->Colour2->Alpha : -1.0);
    for (int i = 0; i < coeffCount; ++i) {
        if (std::fabs(shape->Coeffs[i]) > 1.0e-12) {
            Logger::info("[DIAG-MONKEY] %s quartic coeff[%d]=%.6f\n", prefix, i, shape->Coeffs[i]);
        }
    }
}

void logBicubicPatchOnce(const char *prefix, const ParametricBiCubicPatch *shape)
{
    static int logged = 0;
    if (!shouldLogMonkeyDiagnostics() || logged++ > 0 || shape == nullptr) {
        return;
    }
    const Texture *texture = shape->Shape_Texture;
    const RGBAColor *colour = shape->Shape_Colour;
    Logger::info(
        "[DIAG-MONKEY] %s bicubic patchType=%d flatness=%.6f uSteps=%d vSteps=%d colour=<%.6f,%.6f,%.6f,%.6f> texNum=%d amb=%.6f diff=%.6f spec=%.6f rough=%.6f phong=%.6f cp00=<%.6f,%.6f,%.6f> cp33=<%.6f,%.6f,%.6f>\n",
        prefix, shape->patchType, shape->flatnessValue, shape->uSteps, shape->vSteps,
        colour != nullptr ? colour->Red : -1.0,
        colour != nullptr ? colour->Green : -1.0,
        colour != nullptr ? colour->Blue : -1.0,
        colour != nullptr ? colour->Alpha : -1.0,
        texture != nullptr ? texture->textureNumber : -1,
        texture != nullptr ? texture->objectAmbient : -1.0,
        texture != nullptr ? texture->objectDiffuse : -1.0,
        texture != nullptr ? texture->objectSpecular : -1.0,
        texture != nullptr ? texture->objectRoughness : -1.0,
        texture != nullptr ? texture->objectPhong : -1.0,
        shape->Control_Points[0][0].x, shape->Control_Points[0][0].y, shape->Control_Points[0][0].z,
        shape->Control_Points[3][3].x, shape->Control_Points[3][3].y, shape->Control_Points[3][3].z);
    if (texture != nullptr && texture->Colour1 != nullptr) {
        Logger::info(
            "[DIAG-MONKEY] %s bicubic texColour1=<%.6f,%.6f,%.6f,%.6f>\n",
            prefix, texture->Colour1->Red, texture->Colour1->Green, texture->Colour1->Blue,
            texture->Colour1->Alpha);
    }
    if (texture != nullptr && texture->Colour2 != nullptr) {
        Logger::info(
            "[DIAG-MONKEY] %s bicubic texColour2=<%.6f,%.6f,%.6f,%.6f>\n",
            prefix, texture->Colour2->Red, texture->Colour2->Green, texture->Colour2->Blue,
            texture->Colour2->Alpha);
    }
}
#endif

Texture *materializeTextureChain(const AntlrIrTextureChain &chain,
    const std::unordered_map<std::string, Texture *> &declaredTextures);
void applyShapeTexture(Texture *srcTexture, Geometry *shape);

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

        const char *debugLog = std::getenv("POVCPP_ANTLR_DEBUG_SPHERE_COLOR");
        if (debugLog != nullptr && debugLog[0] == '1') {
            fprintf(stderr, "ANTLR_SPHERE_COLOR: center=<%.3f,%.3f,%.3f> radius=%.3f RGBA<%.3f,%.3f,%.3f,%.3f>\n",
                    node.center.x, node.center.y, node.center.z, node.radius,
                    node.colour.r, node.colour.g, node.colour.b, node.colour.a);
        }
    }
    applyTransforms((SimpleBody *)sphere, node.transforms, node.transformCount);
    if (node.hasTextureChain && gDeclaredTextures != nullptr) {
        applyShapeTexture(materializeTextureChain(node.textureChain, *gDeclaredTextures),
            (Geometry *)sphere);
    }
    logSphereOnce("antlr", sphere);
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
    if (node.hasTextureChain && gDeclaredTextures != nullptr) {
        applyShapeTexture(materializeTextureChain(node.textureChain, *gDeclaredTextures),
            (Geometry *)base);
    }
    logSphereOnce("antlr", base);
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
    if (effectiveNode->hasTextureChain && gDeclaredTextures != nullptr) {
        applyShapeTexture(materializeTextureChain(effectiveNode->textureChain, *gDeclaredTextures),
            (Geometry *)plane);
    }
    if (node.hasReferenceBase && node.hasTextureChain && gDeclaredTextures != nullptr) {
        applyShapeTexture(materializeTextureChain(node.textureChain, *gDeclaredTextures),
            (Geometry *)plane);
    }
    logPlaneOnce("antlr", plane);
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
    const AntlrIrQuadricNode *baseNode = &node;
    AntlrIrQuadricNode fallback;
    int depth = 0;
    const char *dbgQuadric = std::getenv("POVCPP_ANTLR_DEBUG_QUADRIC");
    while (baseNode->hasReferenceBase) {
        if (++depth > 16) {
            throw std::runtime_error("ANTLR quadric lowering exceeded max recursion depth");
        }
        auto it = declaredQuadrics.find(baseNode->referenceIdentifier);
        if (it != declaredQuadrics.end()) {
            if (dbgQuadric != nullptr && dbgQuadric[0] == '1') {
                fprintf(stderr, "[QUADRIC_REF] Found '%s' in declaredQuadrics\n",
                    baseNode->referenceIdentifier.c_str());
            }
            baseNode = it->second;
            continue;
        }

        // Minimal built-in fallback for legacy include constants commonly used by scenes.
        const std::string ref = toLowerAscii(baseNode->referenceIdentifier);
        if (dbgQuadric != nullptr && dbgQuadric[0] == '1') {
            fprintf(stderr, "[QUADRIC_FALLBACK] Using fallback for '%s' (ref='%s')\n",
                baseNode->referenceIdentifier.c_str(), ref.c_str());
        }
        fallback.hasReferenceBase = false;
        fallback.hasInlineBase = true;
        fallback.objectMixedTerms = {0.0, 0.0, 0.0};
        fallback.objectTerms = {0.0, 0.0, 0.0};
        if (ref == "ellipsoid" || ref == "sphere") {
            fallback.object2Terms = {1.0, 1.0, 1.0};
            fallback.objectConstant = -1.0;
        } else if (ref == "cylinder_x") {
            fallback.object2Terms = {0.0, 1.0, 1.0};
            fallback.objectConstant = -1.0;
        } else if (ref == "cylinder_y") {
            fallback.object2Terms = {1.0, 0.0, 1.0};
            fallback.objectConstant = -1.0;
        } else if (ref == "cylinder_z") {
            fallback.object2Terms = {1.0, 1.0, 0.0};
            fallback.objectConstant = -1.0;
        } else if (ref == "qcone_x") {
            fallback.object2Terms = {-1.0, 1.0, 1.0};
            fallback.objectConstant = 0.0;
        } else if (ref == "qcone_y") {
            fallback.object2Terms = {1.0, -1.0, 1.0};
            fallback.objectConstant = 0.0;
        } else if (ref == "qcone_z") {
            fallback.object2Terms = {1.0, 1.0, -1.0};
            fallback.objectConstant = 0.0;
        } else if (ref == "hyperboloid_y") {
            fallback.object2Terms = {1.0, -1.0, 1.0};
            fallback.objectConstant = -1.0;
        } else {
            fallback.object2Terms = {1.0, 1.0, 1.0};
            fallback.objectConstant = -1.0;
        }
        baseNode = &fallback;
        break;
    }
    if (!baseNode->hasInlineBase) {
        throw std::runtime_error("ANTLR lowering quadric requires inline base at " +
            formatSourceLocation(node));
    }

    Quadric *quadric = ModelBuilder::getQuadricShape();
    quadric->object2Terms = toVector(baseNode->object2Terms);
    quadric->objectMixedTerms = toVector(baseNode->objectMixedTerms);
    quadric->objectTerms = toVector(baseNode->objectTerms);
    quadric->objectConstant = baseNode->objectConstant;
    quadric->nonZeroSquareTerm =
        !((quadric->object2Terms.x == 0.0) && (quadric->object2Terms.y == 0.0) &&
            (quadric->object2Terms.z == 0.0) && (quadric->objectMixedTerms.x == 0.0) &&
            (quadric->objectMixedTerms.y == 0.0) && (quadric->objectMixedTerms.z == 0.0));
    if (baseNode->hasColour) {
        quadric->Shape_Colour = ModelBuilder::getColour();
        quadric->Shape_Colour->Red = baseNode->colour.r;
        quadric->Shape_Colour->Green = baseNode->colour.g;
        quadric->Shape_Colour->Blue = baseNode->colour.b;
        quadric->Shape_Colour->Alpha = baseNode->colour.a;
    }
    applyTransforms((SimpleBody *)quadric, baseNode->transforms, baseNode->transformCount);
    if (baseNode->inverted) {
        GeometryOperations::invert((SimpleBody *)quadric);
    }

    // If this node references another quadric, apply local modifiers after base resolution.
    if (node.hasReferenceBase && baseNode != &node) {
        if (dbgQuadric != nullptr && dbgQuadric[0] == '1') {
            fprintf(stderr, "[QUADRIC_TRANSFORMS] Applying node transforms: count=%d\n",
                node.transformCount);
        }
        if (node.hasColour) {
            if (quadric->Shape_Colour == nullptr) {
                quadric->Shape_Colour = ModelBuilder::getColour();
            }
            quadric->Shape_Colour->Red = node.colour.r;
            quadric->Shape_Colour->Green = node.colour.g;
            quadric->Shape_Colour->Blue = node.colour.b;
            quadric->Shape_Colour->Alpha = node.colour.a;
        }
        applyTransforms((SimpleBody *)quadric, node.transforms, node.transformCount);
        if (node.inverted) {
            GeometryOperations::invert((SimpleBody *)quadric);
        }
    }

    // Apply the quadric's texture chain, mirroring buildQuartic. Without this the
    // quadric renders with no texture (e.g. the coordinate-axis cylinders in
    // axisbox.inc, `quadric { Cylinder_X ... texture { color X_Axis_Color } }`,
    // would lose their colour and render black).
    if (baseNode->hasTextureChain && gDeclaredTextures != nullptr) {
        applyShapeTexture(
            materializeTextureChain(baseNode->textureChain, *gDeclaredTextures),
            (Geometry *)quadric);
    }
    if (node.hasReferenceBase && node.hasTextureChain && gDeclaredTextures != nullptr) {
        applyShapeTexture(
            materializeTextureChain(node.textureChain, *gDeclaredTextures),
            (Geometry *)quadric);
    }
    return quadric;
}

PolynomialShape *buildQuartic(const AntlrIrQuarticNode &node)
{
    const std::unordered_map<std::string, const AntlrIrQuarticNode *> &declaredQuartics =
        declaredQuarticsRef();

    const AntlrIrQuarticNode *effectiveNode = &node;
    const char *dbgQuartic = std::getenv("POVCPP_ANTLR_DEBUG_QUARTIC_BUILD");
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
    if (dbgQuartic != nullptr && dbgQuartic[0] == '1') {
        fprintf(stderr, "[QUARTIC_BUILD] coeffCount=%d, transformCount=%d, inverted=%d\n",
            effectiveNode->coefficientCount, effectiveNode->transformCount, effectiveNode->inverted);
    }

    PolynomialShape *poly = ModelBuilder::getPolyShape(order, PolynomialShape::termCounts());
    for (int i = 0; i < coeffCount; ++i) {
        poly->Coeffs[i] = effectiveNode->coefficients[i];
    }
    if (dbgQuartic != nullptr && dbgQuartic[0] == '1') {
        fprintf(stderr, "[QUARTIC_COEFFS] First 5 coeffs: ");
        for (int i = 0; i < 5 && i < coeffCount; ++i) {
            fprintf(stderr, "%.6f ", poly->Coeffs[i]);
        }
        fprintf(stderr, "... Last 5: ");
        for (int i = coeffCount - 5; i < coeffCount; ++i) {
            if (i >= 0) fprintf(stderr, "%.6f ", poly->Coeffs[i]);
        }
        fprintf(stderr, "\n");
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

    // Apply local modifiers for quartic references, matching legacy behavior.
    if (node.hasReferenceBase && effectiveNode != &node) {
        if (node.sturm) {
            poly->sturmFlag = 1;
        }
        if (node.hasColour) {
            if (poly->Shape_Colour == nullptr) {
                poly->Shape_Colour = ModelBuilder::getColour();
            }
            poly->Shape_Colour->Red = node.colour.r;
            poly->Shape_Colour->Green = node.colour.g;
            poly->Shape_Colour->Blue = node.colour.b;
            poly->Shape_Colour->Alpha = node.colour.a;
        }
        applyTransforms((SimpleBody *)poly, node.transforms, node.transformCount);
        if (node.inverted) {
            GeometryOperations::invert((SimpleBody *)poly);
        }
    }

    if (effectiveNode->hasTextureChain && gDeclaredTextures != nullptr) {
        applyShapeTexture(materializeTextureChain(effectiveNode->textureChain, *gDeclaredTextures),
            (Geometry *)poly);
    }
    if (node.hasReferenceBase && node.hasTextureChain && gDeclaredTextures != nullptr) {
        applyShapeTexture(materializeTextureChain(node.textureChain, *gDeclaredTextures),
            (Geometry *)poly);
    }
    logPolyOnce("antlr", poly);

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

ParametricBiCubicPatch *buildBicubicPatch(const AntlrIrBicubicPatchNode &node)
{
    if (!node.hasInlineBase) {
        throw std::runtime_error("ANTLR lowering bicubic_patch requires inline base");
    }

    ParametricBiCubicPatch *patch = ModelBuilder::getBicubicPatchShape();
    patch->patchType = node.patchType;
    patch->flatnessValue = node.flatnessValue;
    patch->uSteps = node.uSteps;
    patch->vSteps = node.vSteps;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            patch->Control_Points[i][j] = toVector(node.controlPoints[i][j]);
        }
    }
    ParametricBiCubicPatch::precomputePatchValues(patch);
    if (node.hasColour) {
        patch->Shape_Colour = ModelBuilder::getColour();
        patch->Shape_Colour->Red = node.colour.r;
        patch->Shape_Colour->Green = node.colour.g;
        patch->Shape_Colour->Blue = node.colour.b;
        patch->Shape_Colour->Alpha = node.colour.a;
    }
    applyTransforms((SimpleBody *)patch, node.transforms, node.transformCount);
    if (node.inverted) {
        GeometryOperations::invert((SimpleBody *)patch);
    }
    if (node.hasTextureChain && gDeclaredTextures != nullptr) {
        applyShapeTexture(materializeTextureChain(node.textureChain, *gDeclaredTextures),
            (Geometry *)patch);
    }
    logBicubicPatchOnce("antlr", patch);
    return patch;
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
    logLightOnce("antlr", light);
    return light;
}

void applyObjectTexture(Texture *srcTexture, SimpleBody *object);
Texture *cloneDefaultTexture();
double parseScalarAfterKeyword(const std::string &lower, const char *keyword, bool &ok)
{
    ok = false;
    const size_t keyLen = std::char_traits<char>::length(keyword);
    size_t pos = 0;
    while ((pos = lower.find(keyword, pos)) != std::string::npos) {
        const size_t endPos = pos + keyLen;

        if (pos > 0) {
            char prevChar = lower[pos - 1];
            if (std::isalnum((unsigned char)prevChar) || prevChar == '_') {
                pos = endPos;
                continue;
            }
        }

        if (endPos < lower.size()) {
            char nextChar = lower[endPos];
            if (std::isalnum((unsigned char)nextChar) || nextChar == '_') {
                pos = endPos;
                continue;
            }
        }

        size_t i = endPos;
        while (i < lower.size() && std::isspace((unsigned char)lower[i])) {
            ++i;
        }
        if (i >= lower.size()) {
            return 0.0;
        }
        char *endPtr = nullptr;
        const double value = std::strtod(lower.c_str() + i, &endPtr);
        if (endPtr != lower.c_str() + i) {
            ok = true;
            return value;
        }
        pos = endPos;
    }
    return 0.0;
}

AntlrIrColor parseNamedColourIdentifierForLowering(const std::string &name)
{
    const char *debugLog = std::getenv("POVCPP_ANTLR_DEBUG_COLOR_RESOLVE");

#ifdef POV_WITH_ANTLR_RUNTIME
    auto it = gAntlrDeclaredColours.find(name);
    if (it != gAntlrDeclaredColours.end()) {
        if (debugLog != nullptr && debugLog[0] == '1') {
            fprintf(stderr, "ANTLR_COLOR_RESOLVE: '%s' found in gAntlrDeclaredColours: RGBA<%.3f,%.3f,%.3f,%.3f>\n",
                    name.c_str(), it->second.r, it->second.g, it->second.b, it->second.a);
        }
        return it->second;
    }
#endif

    if (gDeclaredTextures != nullptr) {
        auto it2 = gDeclaredTextures->find(name);
        if (it2 != gDeclaredTextures->end() && it2->second != nullptr && it2->second->Colour1 != nullptr) {
            if (debugLog != nullptr && debugLog[0] == '1') {
                fprintf(stderr, "ANTLR_COLOR_RESOLVE: '%s' found in gDeclaredTextures: RGBA<%.3f,%.3f,%.3f,%.3f>\n",
                        name.c_str(), it2->second->Colour1->Red, it2->second->Colour1->Green,
                        it2->second->Colour1->Blue, it2->second->Colour1->Alpha);
            }
            return {
                it2->second->Colour1->Red,
                it2->second->Colour1->Green,
                it2->second->Colour1->Blue,
                it2->second->Colour1->Alpha
            };
        }
    }

    std::string key = name;
    for (char &c : key) {
        c = (char)std::tolower((unsigned char)c);
    }

    // Extract base color name and number for colors like Gray80, Gray50
    std::string baseName = key;
    int colorNumber = -1;
    {
        size_t firstDigit = key.find_first_of("0123456789");
        if (firstDigit != std::string::npos) {
            baseName = key.substr(0, firstDigit);
            std::string numPart = key.substr(firstDigit);
            try {
                colorNumber = std::stoi(numPart);
            } catch (...) {
                colorNumber = -1;
            }
        }
    }

    AntlrIrColor result = {1.0, 1.0, 1.0, 0.0};
    if (key == "white") result = {1.0, 1.0, 1.0, 0.0};
    else if (key == "clear") result = {1.0, 1.0, 1.0, 1.0};
    else if (key == "black") result = {0.0, 0.0, 0.0, 0.0};
    else if (key == "red") result = {1.0, 0.0, 0.0, 0.0};
    else if (key == "green") result = {0.0, 1.0, 0.0, 0.0};
    else if (key == "blue") result = {0.0, 0.0, 1.0, 0.0};
    else if (key == "yellow") result = {1.0, 1.0, 0.0, 0.0};
    else if (key == "cyan") result = {0.0, 1.0, 1.0, 0.0};
    else if (key == "magenta") result = {1.0, 0.0, 1.0, 0.0};
    else if (key == "gray" || key == "grey") result = {0.5, 0.5, 0.5, 0.0};
    else if (key == "lightgray" || key == "lightgrey") result = {0.75, 0.75, 0.75, 0.0};
    else if (key == "darkgray" || key == "darkgrey") result = {0.25, 0.25, 0.25, 0.0};
    else if (key == "dimgray" || key == "dimgrey") result = {0.41, 0.41, 0.41, 0.0};
    // Support GrayXX where XX is 0-100
    else if ((baseName == "gray" || baseName == "grey") && colorNumber >= 0 && colorNumber <= 100) {
        double intensity = colorNumber / 100.0;
        result = {intensity, intensity, intensity, 0.0};
    }

    if (debugLog != nullptr && debugLog[0] == '1') {
        fprintf(stderr, "ANTLR_COLOR_RESOLVE: '%s' resolved to builtin: RGBA<%.3f,%.3f,%.3f,%.3f>\n",
                name.c_str(), result.r, result.g, result.b, result.a);
    }
    return result;
}

std::vector<std::string> extractIdentifiers(const std::string &text)
{
    std::vector<std::string> ids;
    std::string current;
    for (char ch : text) {
        const bool identChar = std::isalnum((unsigned char)ch) || ch == '_';
        if (identChar) {
            current.push_back(ch);
            continue;
        }
        if (!current.empty() && std::isalpha((unsigned char)current[0])) {
            ids.push_back(current);
        }
        current.clear();
    }
    if (!current.empty() && std::isalpha((unsigned char)current[0])) {
        ids.push_back(current);
    }
    return ids;
}

void skipSpaces(const std::string &s, size_t &i)
{
    while (i < s.size() && std::isspace((unsigned char)s[i])) {
        ++i;
    }
}

bool readIdentifier(const std::string &s, size_t &i, std::string &out)
{
    skipSpaces(s, i);
    if (i >= s.size() || !(std::isalpha((unsigned char)s[i]) || s[i] == '_')) {
        return false;
    }
    size_t start = i++;
    while (i < s.size() && (std::isalpha((unsigned char)s[i]) || s[i] == '_')) {
        ++i;
    }
    out = s.substr(start, i - start);
    return true;
}

bool readNumber(const std::string &s, size_t &i, double &out)
{
    skipSpaces(s, i);
    if (i >= s.size()) {
        return false;
    }
    char *endPtr = nullptr;
    out = std::strtod(s.c_str() + i, &endPtr);
    if (endPtr == s.c_str() + i) {
        return false;
    }
    i = (size_t)(endPtr - s.c_str());
    return true;
}

bool parseColourSpec(const std::string &s, size_t &i, AntlrIrColor &out)
{
    std::string kwToken;
    if (!readIdentifier(s, i, kwToken)) {
        return false;
    }
    const char *debugColorSpec = std::getenv("POVCPP_ANTLR_DEBUG_COLORSPEC");
    if (debugColorSpec != nullptr && debugColorSpec[0] == '1') {
        fprintf(stderr, "parseColourSpec: kwToken='%s'\n", kwToken.c_str());
    }
    std::string lowKwToken = toLowerAscii(kwToken);
    std::string head;
    if (lowKwToken == "color" || lowKwToken == "colour") {
        if (!readIdentifier(s, i, head)) {
            return false;
        }
        // Also read trailing digits if present (for color names like Gray80, Gray50)
        while (i < s.size() && std::isdigit((unsigned char)s[i])) {
            head += s[i];
            ++i;
        }
    } else if (lowKwToken.rfind("color", 0) == 0 && kwToken.size() > 5) {
        head = kwToken.substr(5);
    } else if (lowKwToken.rfind("colour", 0) == 0 && kwToken.size() > 6) {
        head = kwToken.substr(6);
    } else {
        return false;
    }
    std::string lowHead = toLowerAscii(head);
    if (lowHead != "red" && lowHead != "green" && lowHead != "blue" && lowHead != "alpha") {
        if (debugColorSpec != nullptr && debugColorSpec[0] == '1') {
            fprintf(stderr, "parseColourSpec: resolved named color '%s'\n", head.c_str());
        }
        out = parseNamedColourIdentifierForLowering(head);
        size_t probe = i;
        // Optional "alpha <n>" after named colour.
        size_t opt = probe;
        std::string nextId;
        if (readIdentifier(s, opt, nextId) && toLowerAscii(nextId) == "alpha") {
            double a = 0.0;
            if (readNumber(s, opt, a)) {
                out.a = a;
                probe = opt;
            }
        }
        i = probe;
        return true;
    }

    out = {0.0, 0.0, 0.0, 0.0};
    double v = 0.0;
    if (!readNumber(s, i, v)) {
        return false;
    }
    if (lowHead == "red") out.r = v;
    else if (lowHead == "green") out.g = v;
    else if (lowHead == "blue") out.b = v;
    else out.a = v;

    while (true) {
        std::string comp;
        size_t mark = i;
        if (!readIdentifier(s, mark, comp)) {
            break;
        }
        std::string lowComp = toLowerAscii(comp);
        if (lowComp != "red" && lowComp != "green" && lowComp != "blue" && lowComp != "alpha") {
            break;
        }
        double v = 0.0;
        if (!readNumber(s, mark, v)) {
            break;
        }
        if (lowComp == "red") out.r = v;
        else if (lowComp == "green") out.g = v;
        else if (lowComp == "blue") out.b = v;
        else out.a = v;
        i = mark;
    }
    return true;
}

bool parseColourMapRaw(const std::string &raw, RGBAColorPalette *&outPalette)
{
    outPalette = nullptr;
    const char *debugColourMapRaw = std::getenv("POVCPP_ANTLR_DEBUG_COLOURMAP_RAW");
    if (debugColourMapRaw != nullptr && debugColourMapRaw[0] == '1') {
        fprintf(stderr, "DEBUG_COLOURMAP_RAW_INPUT: %s\n", raw.c_str());
    }
    std::vector<RGBAColorPaletteSpan> spans;
    size_t i = 0;
    while (i < raw.size()) {
        if (raw[i] != '[') {
            ++i;
            continue;
        }
        size_t startBracket = i++;
        size_t endBracket = raw.find(']', i);
        if (endBracket == std::string::npos) {
            break;
        }
        std::string entry = raw.substr(startBracket + 1, endBracket - startBracket - 1);

        // Pre-normalize: insert space before known keywords when preceded by letter/digit
        // This fixes "Graycolor" → "Gray color", "Orangeambient" → "Orange ambient", "red1alpha" → "red 1 alpha"
        for (const char *kw : {"colour", "color", "alpha", "ambient", "diffuse", "phong", "reflection",
                               "turbulence", "frequency", "phase", "octaves", "scale", "translate", "rotate",
                               "bumps", "ripples", "wrinkles", "dents", "bumpy1", "bumpy2", "bumpy3"}) {
            std::string needle = kw;
            size_t pos = 0;
            while ((pos = entry.find(needle, pos)) != std::string::npos) {
                // Check if preceded by letter or digit (not start of string)
                if (pos > 0 && (std::isalpha((unsigned char)entry[pos - 1]) ||
                                std::isdigit((unsigned char)entry[pos - 1]))) {
                    entry.insert(pos, " ");
                    pos += needle.size() + 1;
                } else {
                    pos += needle.size();
                }
            }
        }

        // Normalize: insert spaces between numbers and before letters
        std::string normalized;
        for (size_t k = 0; k < entry.size(); ++k) {
            const char c = entry[k];
            const char prev = (k > 0) ? entry[k-1] : ' ';
            const char next = (k + 1 < entry.size()) ? entry[k+1] : ' ';

            // Insert space before letters (color keywords) if preceded by digit or '.'
            if (std::isalpha((unsigned char)c) && (std::isdigit((unsigned char)prev) || prev == '.')) {
                normalized += ' ';
            }
            // Insert space before '.' or digit if preceded by letter (after color name)
            else if ((c == '.' || std::isdigit((unsigned char)c)) && std::isalpha((unsigned char)prev)) {
                normalized += ' ';
            }
            normalized += c;

            // Insert space after '.' if next is '.' (transition between two numbers like 0.0 and 0.95)
            if (c == '.' && next == '.') {
                normalized += ' ';
            }
        }

        size_t p = 0;
        double start = 0.0;
        double end = 0.0;
        if (!readNumber(normalized, p, start) || !readNumber(normalized, p, end)) {
            i = endBracket + 1;
            continue;
        }
        AntlrIrColor c1 = {1.0, 1.0, 1.0, 0.0};
        AntlrIrColor c2 = {1.0, 1.0, 1.0, 0.0};
        if (!parseColourSpec(entry, p, c1) || !parseColourSpec(entry, p, c2)) {
            i = endBracket + 1;
            continue;
        }

        RGBAColorPaletteSpan span;
        span.start = start;
        span.end = end;
        span.startColour.Red = c1.r;
        span.startColour.Green = c1.g;
        span.startColour.Blue = c1.b;
        span.startColour.Alpha = c1.a;
        span.endColour.Red = c2.r;
        span.endColour.Green = c2.g;
        span.endColour.Blue = c2.b;
        span.endColour.Alpha = c2.a;
        spans.push_back(span);
        i = endBracket + 1;
    }

    if (spans.empty()) {
        return false;
    }

    std::sort(spans.begin(), spans.end(),
        [](const RGBAColorPaletteSpan &a, const RGBAColorPaletteSpan &b) {
            return a.start < b.start;
        });
    for (RGBAColorPaletteSpan &span : spans) {
        if (span.start > span.end) {
            const double tmp = span.start;
            span.start = span.end;
            span.end = tmp;
        }
        if (span.start < 0.0) span.start = 0.0;
        if (span.end > 1.0) span.end = 1.0;
    }
    if (!spans.empty()) {
        spans.front().start = 0.0;
        spans.back().end = 1.0;
        for (size_t i = 1; i < spans.size(); ++i) {
            if (spans[i].start > spans[i - 1].end) {
                spans[i].start = spans[i - 1].end;
            }
            if (spans[i].end < spans[i].start) {
                spans[i].end = spans[i].start;
            }
        }
    }

    RGBAColorPalette *palette = new RGBAColorPalette();
    palette->numberOfEntries = (int)spans.size();
    palette->transparencyFlag = LegacyBoolean::FALSE_VALUE;
    palette->Colour_Map_Entries = new RGBAColorPaletteSpan[spans.size()];

    const char *debugColourMap = std::getenv("POVCPP_ANTLR_DEBUG_COLOURMAP");
    if (debugColourMap != nullptr && debugColourMap[0] == '1') {
        fprintf(stderr, "DEBUG_COLOURMAP: parsing %zu spans\n", spans.size());
    }

    for (size_t k = 0; k < spans.size(); ++k) {
        palette->Colour_Map_Entries[k] = spans[k];
        if (debugColourMap != nullptr && debugColourMap[0] == '1') {
            fprintf(stderr, "  SPAN[%zu]: [%.2f %.2f] start=(R:%.3f G:%.3f B:%.3f A:%.3f) end=(R:%.3f G:%.3f B:%.3f A:%.3f)\n",
                k, spans[k].start, spans[k].end,
                spans[k].startColour.Red, spans[k].startColour.Green, spans[k].startColour.Blue, spans[k].startColour.Alpha,
                spans[k].endColour.Red, spans[k].endColour.Green, spans[k].endColour.Blue, spans[k].endColour.Alpha);
        }
        if (spans[k].startColour.Alpha > 0.0 || spans[k].endColour.Alpha > 0.0) {
            palette->transparencyFlag = LegacyBoolean::TRUE_VALUE;
        }
    }

    outPalette = palette;
    return true;
}

std::vector<std::string> extractInlineTextureBlocks(const std::string &raw)
{
    std::vector<std::string> blocks;
    std::string lower = raw;
    for (char &ch : lower) {
        ch = (char)std::tolower((unsigned char)ch);
    }

    // First, find the content inside tiles{...}
    size_t tilesPos = lower.find("tiles");
    if (tilesPos == std::string::npos) {
        return blocks;
    }

    size_t tilesOpenPos = lower.find('{', tilesPos + 5);
    if (tilesOpenPos == std::string::npos) {
        return blocks;
    }

    // Find the matching closing brace for tiles{...}
    int depth = 0;
    size_t tilesClosePos = tilesOpenPos;
    bool tilesFound = false;
    for (; tilesClosePos < raw.size(); ++tilesClosePos) {
        const char c = raw[tilesClosePos];
        if (c == '{') {
            ++depth;
        } else if (c == '}') {
            --depth;
            if (depth == 0) {
                tilesFound = true;
                break;
            }
        }
    }

    if (!tilesFound) {
        return blocks;
    }

    // Extract content between tiles{ and }
    std::string tilesContent = raw.substr(tilesOpenPos + 1, tilesClosePos - tilesOpenPos - 1);
    std::string tilesContentLower = tilesContent;
    for (char &ch : tilesContentLower) {
        ch = (char)std::tolower((unsigned char)ch);
    }

    // Now find texture blocks within this content
    size_t pos = 0;
    while (true) {
        pos = tilesContentLower.find("texture", pos);
        if (pos == std::string::npos) {
            break;
        }
        size_t bracePos = tilesContentLower.find('{', pos + 7);
        if (bracePos == std::string::npos) {
            break;
        }

        int bDepth = 0;
        size_t endPos = bracePos;
        bool closed = false;
        for (; endPos < tilesContent.size(); ++endPos) {
            const char c = tilesContent[endPos];
            if (c == '{') {
                ++bDepth;
            } else if (c == '}') {
                --bDepth;
                if (bDepth == 0) {
                    closed = true;
                    ++endPos;
                    break;
                }
            }
        }
        if (!closed || endPos <= pos) {
            break;
        }

        blocks.push_back(tilesContent.substr(pos, endPos - pos));
        pos = endPos;
    }

    return blocks;
}

static bool parseVectorAfterKeyword(const std::string &s, const char *keyword,
                                    double &x, double &y, double &z)
{
    size_t pos = s.find(keyword);
    if (pos == std::string::npos) return false;

    size_t keylen = std::strlen(keyword);
    if (pos > 0) {
        char prevChar = s[pos - 1];
        if (std::isalnum((unsigned char)prevChar) || prevChar == '_') {
            return false;
        }
    }

    if (pos + keylen < s.size()) {
        char nextChar = s[pos + keylen];
        if (std::isalnum((unsigned char)nextChar) || nextChar == '_') {
            return false;
        }
    }

    pos += keylen;
    while (pos < s.size() && s[pos] != '<') ++pos;
    if (pos >= s.size()) return false;
    ++pos;
    char *end = nullptr;
    x = std::strtod(s.c_str() + pos, &end);
    if (end == s.c_str() + pos) return false;
    pos = end - s.c_str();

    while (pos < s.size() && (s[pos] == ',' || s[pos] == ' ' || s[pos] == '\t')) ++pos;
    if (pos >= s.size() || s[pos] == '>') return false;
    y = std::strtod(s.c_str() + pos, &end);
    if (end == s.c_str() + pos) return false;
    pos = end - s.c_str();

    while (pos < s.size() && (s[pos] == ',' || s[pos] == ' ' || s[pos] == '\t')) ++pos;
    if (pos >= s.size() || s[pos] == '>') return false;
    z = std::strtod(s.c_str() + pos, &end);
    if (end == s.c_str() + pos) return false;
    return true;
}

static void applyInlineTextureTransforms(Texture *texture, const std::string &rawBlockElement)
{
    if (texture == nullptr || rawBlockElement.empty()) {
        return;
    }

    const char *debugTransforms = std::getenv("POVCPP_ANTLR_DEBUG_INLINE_TRANSFORMS");
    if (debugTransforms != nullptr && debugTransforms[0] == '1') {
        fprintf(stderr, "[ANTLR_INLINE_TRANSFORMS] Processing transforms from rawElement (len=%zu)\n", rawBlockElement.size());
    }

    std::string lower = rawBlockElement;
    for (char &ch : lower) {
        ch = (char)std::tolower((unsigned char)ch);
    }

    double x, y, z;

    // Apply scale transform
    if (parseVectorAfterKeyword(lower, "scale", x, y, z)) {
        Vector3Dd v(x, y, z);
        if (debugTransforms != nullptr && debugTransforms[0] == '1') {
            fprintf(stderr, "[ANTLR_INLINE_TRANSFORMS] Applying scale <%.3f, %.3f, %.3f>\n", x, y, z);
        }
        TextureUtils::scaleTexture(&texture, &v);
    }

    // Apply rotate transform
    if (parseVectorAfterKeyword(lower, "rotate", x, y, z)) {
        Vector3Dd v(x, y, z);
        if (debugTransforms != nullptr && debugTransforms[0] == '1') {
            fprintf(stderr, "[ANTLR_INLINE_TRANSFORMS] Applying rotate <%.3f, %.3f, %.3f>\n", x, y, z);
        }
        TextureUtils::rotateTexture(&texture, &v);
    }

    // Apply translate transform
    if (parseVectorAfterKeyword(lower, "translate", x, y, z)) {
        Vector3Dd v(x, y, z);
        if (debugTransforms != nullptr && debugTransforms[0] == '1') {
            fprintf(stderr, "[ANTLR_INLINE_TRANSFORMS] Applying translate <%.3f, %.3f, %.3f>\n", x, y, z);
        }
        TextureUtils::translateTexture(&texture, &v);
    }
}

bool applyRawTextureElement(Texture *texture, const std::string &rawElement, bool applyInlineTransforms = true)
{
    if (texture == nullptr || rawElement.empty()) {
        return false;
    }

    std::string lower = rawElement;
    for (char &ch : lower) {
        ch = (char)std::tolower((unsigned char)ch);
    }

    bool touched = false;
    bool ok = false;
    bool detectedProceduralTexture = false;
    bool isTilesTexture = false;

    // Process image_map
    if (lower.find("image_map") != std::string::npos) {
        Logger::info("[ANTLR-IMAGE_MAP] Detected image_map in rawElement\n");

        // Extract filename from image_map - search for gif/tga/iff with optional space before quote
        size_t gifPos = rawElement.find("gif");
        size_t tgaPos = rawElement.find("tga");
        size_t iffPos = rawElement.find("iff");
        size_t imageTypePos = std::string::npos;
        std::string detectedFormat;

        if (gifPos != std::string::npos) {
            imageTypePos = gifPos;
            detectedFormat = "gif";
        }
        if (tgaPos != std::string::npos && (imageTypePos == std::string::npos || tgaPos < imageTypePos)) {
            imageTypePos = tgaPos;
            detectedFormat = "tga";
        }
        if (iffPos != std::string::npos && (imageTypePos == std::string::npos || iffPos < imageTypePos)) {
            imageTypePos = iffPos;
            detectedFormat = "iff";
        }

        if (imageTypePos != std::string::npos) {
            size_t startQuote = rawElement.find('"', imageTypePos + 3);
            size_t endQuote = rawElement.find('"', startQuote + 1);
            if (startQuote != std::string::npos && endQuote != std::string::npos) {
                std::string filename = rawElement.substr(startQuote + 1, endQuote - startQuote - 1);
                Logger::info("[ANTLR-IMAGE_MAP] Extracted filename: %s (format: %s)\n", filename.c_str(), detectedFormat.c_str());

                // Create and load image
                if (texture->Image == nullptr) {
                    texture->Image = new RGBAImage();
                }
                if (texture->Image != nullptr) {
                    // Default values from legacy parser
                    Vector3Dd imageGradient(1.0, -1.0, 0.0);

                    // Parse image_map gradient vector if present (e.g., <1.0 0.0 -1.0>)
                    size_t imageMapPos = rawElement.find("image_map");
                    if (imageMapPos != std::string::npos) {
                        size_t openBracket = rawElement.find('<', imageMapPos);
                        if (openBracket != std::string::npos && openBracket < imageTypePos) {
                            size_t closeBracket = rawElement.find('>', openBracket);
                            if (closeBracket != std::string::npos) {
                                std::string gradientStr = rawElement.substr(openBracket + 1, closeBracket - openBracket - 1);
                                // Parse three floats from "1.0 0.0 -1.0"
                                double gx = 0.0, gy = 0.0, gz = 0.0;
                                int parsed = sscanf(gradientStr.c_str(), "%lf %lf %lf", &gx, &gy, &gz);
                                if (parsed == 3) {
                                    imageGradient = Vector3Dd(gx, gy, gz);
                                    Logger::info("[ANTLR-IMAGE_MAP] Parsed gradient vector: <%f %f %f>\n", gx, gy, gz);
                                }
                            }
                        }
                    }

                    texture->Image->imageGradient = imageGradient;
                    texture->Image->mapType = Texture::PLANAR_MAP;
                    texture->Image->interpolationType = Texture::NO_INTERPOLATION;
                    texture->Image->onceFlag = LegacyBoolean::FALSE_VALUE;
                    texture->Image->useColourFlag = LegacyBoolean::TRUE_VALUE;

                    // Parse interpolate flag if present (e.g., "interpolate 2")
                    if (lower.find("interpolate") != std::string::npos) {
                        bool interp_ok = false;
                        const double interpolateVal = parseScalarAfterKeyword(lower, "interpolate", interp_ok);
                        if (interp_ok && interpolateVal > 0.0) {
                            int interpType = (int)interpolateVal;
                            if (interpType >= 0 && interpType <= 3) {
                                texture->Image->interpolationType = interpType;
                                Logger::info("[ANTLR-IMAGE_MAP] Set interpolation type: %d\n", interpType);
                            }
                        }
                    }

                    // Parse once flag if present
                    if (lower.find("once") != std::string::npos) {
                        texture->Image->onceFlag = LegacyBoolean::TRUE_VALUE;
                        Logger::info("[ANTLR-IMAGE_MAP] Set once flag\n");
                    }

                    // Try to load the image (with error handling)
                    try {
                        char *filenameC = new char[filename.size() + 1];
                        std::strcpy(filenameC, filename.c_str());
                        Logger::info("[ANTLR-IMAGE_MAP] Attempting to load: %s as %s\n", filenameC, detectedFormat.c_str());

                        if (detectedFormat == "gif") {
                            GifFormat::readGifImage(texture->Image, filenameC);
                        } else if (detectedFormat == "tga") {
                            TargaFormat::readTargaImage(texture->Image, filenameC);
                        } else if (detectedFormat == "iff") {
                            IffFormat::readIffImage(texture->Image, filenameC);
                        }

                        delete[] filenameC;

                        texture->textureNumber = Texture::IMAGEMAP_TEXTURE;
                        touched = true;
                        Logger::info("[ANTLR-IMAGE_MAP] Created IMAGEMAP_TEXTURE for: %s\n", filename.c_str());
                    } catch (const std::exception &e) {
                        Logger::error("[ANTLR-IMAGE_MAP] Failed to load image %s (%s): %s\n", filename.c_str(), detectedFormat.c_str(), e.what());
                    } catch (...) {
                        Logger::error("[ANTLR-IMAGE_MAP] Failed to load image %s (%s): unknown error\n", filename.c_str(), detectedFormat.c_str());
                    }
                }
            }
        }
    }

    const char *debugTextureParams = std::getenv("POVCPP_ANTLR_DEBUG_TEXTURE_PARAMS");
    if (debugTextureParams != nullptr && debugTextureParams[0] == '1') {
        fprintf(stderr, "ANTLR_TEXTURE_PARAMS_RAW: %s\n", rawElement.c_str());
    }

    const char *auditTextureRaw = std::getenv("POVCPP_AUDIT_TEXTURE_RAW");
    if (auditTextureRaw != nullptr && auditTextureRaw[0] == '1') {
        if (lower.find("tile2") != std::string::npos ||
            lower.find("tiles") != std::string::npos ||
            lower.find("checker_texture") != std::string::npos) {
            fprintf(stderr, "ANTLR_TEXTURE_RAW %s\n", rawElement.c_str());
        }
    }

    if (lower.find("tiles") != std::string::npos) {
        texture->textureNumber = Texture::CHECKER_TEXTURE_TEXTURE;
        touched = true;
        isTilesTexture = true;

        std::vector<std::string> blocks = extractInlineTextureBlocks(rawElement);
        if (blocks.size() >= 2) {
            size_t firstIdx = 0;
            size_t secondIdx = 1;
            if (blocks.size() >= 3) {
                firstIdx = 1;
                secondIdx = 2;
            }
            Texture *t1 = cloneDefaultTexture();
            Texture *t2 = cloneDefaultTexture();
            if (t1 != nullptr && t2 != nullptr) {
                applyRawTextureElement(t1, blocks[firstIdx]);
                applyRawTextureElement(t2, blocks[secondIdx]);
                texture->Colour1 = (RGBAColor *)t1;
                texture->Colour2 = (RGBAColor *)t2;
            }
        }
    }

    const double ambient = parseScalarAfterKeyword(lower, "ambient", ok);
    if (ok) {
        texture->objectAmbient = ambient;
        touched = true;
    }
    const double diffuse = parseScalarAfterKeyword(lower, "diffuse", ok);
    if (ok) {
        texture->objectDiffuse = diffuse;
        touched = true;
    }
    const double phong = parseScalarAfterKeyword(lower, "phong", ok);
    if (ok) {
        texture->objectPhong = phong;
        touched = true;
    }
    const double phongSize = parseScalarAfterKeyword(lower, "phong_size", ok);
    if (ok) {
        texture->objectPhongSize = phongSize;
        touched = true;
    }
    const double brilliance = parseScalarAfterKeyword(lower, "brilliance", ok);
    if (ok) {
        texture->objectBrilliance = brilliance;
        touched = true;
    }
    const double reflection = parseScalarAfterKeyword(lower, "reflection", ok);
    if (ok) {
        texture->objectReflection = reflection;
        touched = true;
    }
    const double refraction = parseScalarAfterKeyword(lower, "refraction", ok);
    if (ok) {
        texture->objectRefraction = refraction;
        touched = true;
    }
    const double ior = parseScalarAfterKeyword(lower, "ior", ok);
    if (ok) {
        texture->objectIndexOfRefraction = ior;
        touched = true;
    }
    const double specular = parseScalarAfterKeyword(lower, "specular", ok);
    if (ok) {
        texture->objectSpecular = specular;
        touched = true;
    }
    const double roughness = parseScalarAfterKeyword(lower, "roughness", ok);
    if (ok) {
        texture->objectRoughness = roughness;
        touched = true;
    }
    const double metallic = parseScalarAfterKeyword(lower, "metallic", ok);
    if (ok) {
        texture->metallicFlag = (metallic > 0.0) ? LegacyBoolean::TRUE_VALUE : LegacyBoolean::FALSE_VALUE;
        touched = true;
    }
    const double transmit = parseScalarAfterKeyword(lower, "transmit", ok);
    if (ok) {
        texture->objectTransmit = transmit;
        touched = true;
    }
    const double turbulence = parseScalarAfterKeyword(lower, "turbulence", ok);
    if (ok) {
        texture->Turbulence = turbulence;
        touched = true;
    }
    const double octaves = parseScalarAfterKeyword(lower, "octaves", ok);
    if (ok) {
        int o = (int)octaves;
        if (o < 1) {
            o = 6;
        }
        if (o > 10) {
            o = 10;
        }
        texture->Octaves = o;
        touched = true;
    }
    const double frequency = parseScalarAfterKeyword(lower, "frequency", ok);
    if (ok) {
        texture->Frequency = frequency;
        touched = true;
    }
    const double phase = parseScalarAfterKeyword(lower, "phase", ok);
    if (ok) {
        texture->Phase = phase;
        touched = true;
    }
    const double mortar = parseScalarAfterKeyword(lower, "mortar", ok);
    if (ok) {
        texture->Mortar = mortar < 0.0 ? 0.2 : mortar;
        touched = true;
    }

    const double bumps = parseScalarAfterKeyword(lower, "bumps", ok);
    if (ok) {
        texture->bumpNumber = Texture::BUMPS;
        texture->bumpAmount = bumps;
        touched = true;
    }
    const double ripples = parseScalarAfterKeyword(lower, "ripples", ok);
    if (ok) {
        texture->bumpNumber = Texture::RIPPLES;
        texture->bumpAmount = ripples;
        touched = true;
    }
    const double wrinkles = parseScalarAfterKeyword(lower, "wrinkles", ok);
    if (ok) {
        texture->bumpNumber = Texture::WRINKLES;
        texture->bumpAmount = wrinkles;
        touched = true;
    }
    const double dents = parseScalarAfterKeyword(lower, "dents", ok);
    if (ok) {
        texture->bumpNumber = Texture::DENTS;
        texture->bumpAmount = dents;
        touched = true;
    }
    const double bumpy1 = parseScalarAfterKeyword(lower, "bumpy1", ok);
    if (ok) {
        texture->bumpNumber = Texture::BUMPY1;
        texture->bumpAmount = bumpy1;
        touched = true;
    }
    const double bumpy2 = parseScalarAfterKeyword(lower, "bumpy2", ok);
    if (ok) {
        texture->bumpNumber = Texture::BUMPY2;
        texture->bumpAmount = bumpy2;
        touched = true;
    }
    const double bumpy3 = parseScalarAfterKeyword(lower, "bumpy3", ok);
    if (ok) {
        texture->bumpNumber = Texture::BUMPY3;
        texture->bumpAmount = bumpy3;
        touched = true;
    }

    if (!isTilesTexture && lower.find("gradient") != std::string::npos) {
        texture->textureNumber = Texture::GRADIENT_TEXTURE;
        touched = true;
        detectedProceduralTexture = true;
        double gx = 0.0, gy = 0.0, gz = 0.0;
        if (parseVectorAfterKeyword(rawElement, "gradient", gx, gy, gz)) {
            texture->textureGradient.x = gx;
            texture->textureGradient.y = gy;
            texture->textureGradient.z = gz;
        }
    } else if (!isTilesTexture && lower.find("granite") != std::string::npos) {
        texture->textureNumber = Texture::GRANITE_TEXTURE;
        touched = true;
        detectedProceduralTexture = true;
    } else if (!isTilesTexture && lower.find("marble") != std::string::npos) {
        texture->textureNumber = Texture::MARBLE_TEXTURE;
        touched = true;
        detectedProceduralTexture = true;
    } else if (!isTilesTexture && lower.find("wood") != std::string::npos) {
        texture->textureNumber = Texture::WOOD_TEXTURE;
        touched = true;
        detectedProceduralTexture = true;
    } else if (!isTilesTexture && lower.find("checker") != std::string::npos &&
        texture->textureNumber != Texture::CHECKER_TEXTURE_TEXTURE) {
        Logger::info("[ANTLR-LOWERING] Detected checker texture\n");
        texture->textureNumber = Texture::CHECKER_TEXTURE;
        touched = true;
        detectedProceduralTexture = true;
    } else if (!isTilesTexture && lower.find("bozo") != std::string::npos) {
        texture->textureNumber = Texture::BOZO_TEXTURE;
        touched = true;
        detectedProceduralTexture = true;
    } else if (!isTilesTexture && lower.find("agate") != std::string::npos) {
        texture->textureNumber = Texture::AGATE_TEXTURE;
        touched = true;
        detectedProceduralTexture = true;
    } else if (!isTilesTexture && lower.find("spotted") != std::string::npos) {
        texture->textureNumber = Texture::SPOTTED_TEXTURE;
        touched = true;
        detectedProceduralTexture = true;
    } else if (!isTilesTexture && lower.find("onion") != std::string::npos) {
        texture->textureNumber = Texture::ONION_TEXTURE;
        touched = true;
        detectedProceduralTexture = true;
    } else if (!isTilesTexture && lower.find("leopard") != std::string::npos) {
        texture->textureNumber = Texture::LEOPARD_TEXTURE;
        touched = true;
        detectedProceduralTexture = true;
    } else if (!isTilesTexture && lower.find("brick") != std::string::npos) {
        texture->textureNumber = Texture::BRICK_TEXTURE;
        touched = true;
        detectedProceduralTexture = true;
    }

    const bool containsColorMap = (lower.find("color_map") != std::string::npos) ||
        (lower.find("colour_map") != std::string::npos);
    const bool isCheckerOrBrick = texture->textureNumber == Texture::CHECKER_TEXTURE ||
        texture->textureNumber == Texture::BRICK_TEXTURE;

    if (isCheckerOrBrick && texture->Colour1 == nullptr) {
        size_t firstColourPos = lower.find("colour");
        size_t firstColorPos = lower.find("color");
        size_t firstPos = std::string::npos;
        if (firstColourPos != std::string::npos && firstColorPos != std::string::npos) {
            firstPos = (firstColourPos < firstColorPos) ? firstColourPos : firstColorPos;
        } else if (firstColourPos != std::string::npos) {
            firstPos = firstColourPos;
        } else if (firstColorPos != std::string::npos) {
            firstPos = firstColorPos;
        }

        if (firstPos != std::string::npos) {
            const std::string firstColourSpec = rawElement.substr(firstPos);
            size_t firstParsePos = 0;
            AntlrIrColor c1 = {0.0, 0.0, 0.0, 0.0};
            if (parseColourSpec(firstColourSpec, firstParsePos, c1)) {
                texture->Colour1 = ModelBuilder::getColour();
                texture->Colour1->Red = c1.r;
                texture->Colour1->Green = c1.g;
                texture->Colour1->Blue = c1.b;
                texture->Colour1->Alpha = c1.a;
                touched = true;
            }
        }
    } else {
        bool hasRed = false;
        bool hasGreen = false;
        bool hasBlue = false;
        const double red = parseScalarAfterKeyword(lower, "red", hasRed);
        const double green = parseScalarAfterKeyword(lower, "green", hasGreen);
        const double blue = parseScalarAfterKeyword(lower, "blue", hasBlue);
        if (!containsColorMap && (hasRed || hasGreen || hasBlue)) {
            if (texture->Colour1 == nullptr) {
                texture->Colour1 = ModelBuilder::getColour();
            }
            if (hasRed) {
                texture->Colour1->Red = red;
            }
            if (hasGreen) {
                texture->Colour1->Green = green;
            }
            if (hasBlue) {
                texture->Colour1->Blue = blue;
            }
            bool hasAlpha = false;
            const double alpha = parseScalarAfterKeyword(lower, "alpha", hasAlpha);
            if (hasAlpha) {
                texture->Colour1->Alpha = alpha;
            }
            if (!detectedProceduralTexture &&
                (texture->textureNumber == Texture::NO_TEXTURE || texture->textureNumber == Texture::COLOUR_TEXTURE)) {
                texture->textureNumber = Texture::COLOUR_TEXTURE;
            }
            touched = true;
        }
    }

    if (containsColorMap && texture->Colour_Map == nullptr) {
        RGBAColorPalette *palette = nullptr;
        if (parseColourMapRaw(rawElement, palette)) {
            texture->Colour_Map = palette;
            touched = true;
        }
    }

    const size_t colourPos = lower.find("colour");
    const size_t colorPos = lower.find("color");
    const size_t pos = (colourPos != std::string::npos) ? colourPos : colorPos;
    if (!isCheckerOrBrick && pos != std::string::npos && !containsColorMap) {
        // Pre-normalize: insert spaces before known keywords in the raw element
        std::string normalizedElement = rawElement;
        for (const char *kw : {"ambient", "diffuse", "phong", "reflection", "turbulence",
                              "frequency", "phase", "octaves", "scale", "translate", "rotate"}) {
            std::string needle = kw;
            size_t searchPos = 0;
            while ((searchPos = normalizedElement.find(needle, searchPos)) != std::string::npos) {
                if (searchPos > 0 && std::isalpha((unsigned char)normalizedElement[searchPos - 1])) {
                    normalizedElement.insert(searchPos, " ");
                    searchPos += needle.size() + 1;
                } else {
                    searchPos += needle.size();
                }
            }
        }

        size_t nameStart = pos + ((colourPos != std::string::npos) ? 6 : 5);
        while (nameStart < normalizedElement.size() && !std::isalpha((unsigned char)normalizedElement[nameStart])) {
            ++nameStart;
        }
        size_t nameEnd = nameStart;
        while (nameEnd < normalizedElement.size()) {
            const char c = normalizedElement[nameEnd];
            if (!std::isalnum((unsigned char)c) && c != '_') {
                break;
            }
            ++nameEnd;
        }
        if (nameEnd > nameStart) {
            const std::string rawName = normalizedElement.substr(nameStart, nameEnd - nameStart);
            const AntlrIrColor c = parseNamedColourIdentifierForLowering(rawName);
            if (texture->Colour1 == nullptr) {
                texture->Colour1 = ModelBuilder::getColour();
            }
            texture->Colour1->Red = c.r;
            texture->Colour1->Green = c.g;
            texture->Colour1->Blue = c.b;
            texture->Colour1->Alpha = c.a;
            // Allow explicit alpha to override named color's alpha
            bool hasExplicitAlpha = false;
            const double explicitAlpha = parseScalarAfterKeyword(lower, "alpha", hasExplicitAlpha);
            if (hasExplicitAlpha) {
                texture->Colour1->Alpha = explicitAlpha;
            }
            // Only set COLOUR_TEXTURE if no procedural texture was already detected
            if (texture->textureNumber == Texture::NO_TEXTURE) {
                texture->textureNumber = Texture::COLOUR_TEXTURE;
            }
            touched = true;
        }
    }

    // For CHECKER_TEXTURE and BRICK_TEXTURE, parse second color for Colour2 even when
    // the first color was specified with explicit red/green/blue components.
    if (isCheckerOrBrick &&
        texture->Colour2 == nullptr) {
        // Search for second "colour" or "color" after the first one
        size_t secondColourPos = rawElement.find("colour", pos + 1);
        size_t secondColorPos = rawElement.find("color", pos + 1);

        // Pick whichever comes first and is not npos
        size_t secondPos = std::string::npos;
        if (secondColourPos != std::string::npos && secondColorPos != std::string::npos) {
            secondPos = (secondColourPos < secondColorPos) ? secondColourPos : secondColorPos;
        } else if (secondColourPos != std::string::npos) {
            secondPos = secondColourPos;
        } else if (secondColorPos != std::string::npos) {
            secondPos = secondColorPos;
        }

        if (secondPos != std::string::npos) {
            const std::string secondColourSpec = rawElement.substr(secondPos);
            size_t secondParsePos = 0;
            AntlrIrColor c2 = {0.0, 0.0, 0.0, 0.0};
            bool parsedSecondColour = parseColourSpec(secondColourSpec, secondParsePos, c2);
            if (std::getenv("POVCPP_ANTLR_DEBUG_TEXTURE") != nullptr) {
                Logger::info("[ANTLR-CHECKER] secondPos=%zu parsed=%d slice='%s' c2=<%.6f,%.6f,%.6f,%.6f> parsePos=%zu\n",
                    secondPos, parsedSecondColour ? 1 : 0, secondColourSpec.c_str(),
                    c2.r, c2.g, c2.b, c2.a, secondParsePos);
            }
            if (parsedSecondColour) {
                texture->Colour2 = ModelBuilder::getColour();
                texture->Colour2->Red = c2.r;
                texture->Colour2->Green = c2.g;
                texture->Colour2->Blue = c2.b;
                texture->Colour2->Alpha = c2.a;
            }
        }
    }

    if (debugTextureParams != nullptr && debugTextureParams[0] == '1' && touched) {
        fprintf(stderr, "ANTLR_TEXTURE_PARSED: type=%d ambient=%.3f diffuse=%.3f phong=%.3f "
                        "brilliance=%.3f reflection=%.3f turbulence=%.3f bumpNumber=%d bumpAmount=%.3f\n",
                texture->textureNumber, texture->objectAmbient, texture->objectDiffuse,
                texture->objectPhong, texture->objectBrilliance, texture->objectReflection,
                texture->Turbulence, texture->bumpNumber, texture->bumpAmount);
    }

    if (touched && applyInlineTransforms) {
        applyInlineTextureTransforms(texture, rawElement);
    }

    return touched;
}
void linkShapeToGeometryList(Geometry *shape, Geometry **listHead)
{
    if (shape == nullptr || listHead == nullptr) {
        return;
    }
    SimpleBodyFactory::link((SimpleBody *)shape, (SimpleBody **)&(shape->nextObject),
        (SimpleBody **)listHead);
}

void appendShapeToGeometryList(Geometry *shape, Geometry **listHead)
{
    if (shape == nullptr || listHead == nullptr) {
        return;
    }
    shape->nextObject = nullptr;
    if (*listHead == nullptr) {
        *listHead = shape;
        return;
    }
    Geometry *tail = *listHead;
    while (tail->nextObject != nullptr) {
        tail = tail->nextObject;
    }
    tail->nextObject = shape;
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
    } else if (
        effectiveNode->childBicubicPatchCount > 0 &&
        effectiveNode->childBicubicPatches[0] != nullptr) {
        obj->Shape = (Geometry *)buildBicubicPatch(*effectiveNode->childBicubicPatches[0]);
    } else if (effectiveNode->childLightCount > 0 && effectiveNode->childLights[0] != nullptr) {
        obj->Shape = (Geometry *)buildLight(*effectiveNode->childLights[0], declaredLights);
    } else if (effectiveNode->childObjectCount > 0 && effectiveNode->childObjects[0] != nullptr) {
        SimpleBody *childObj = buildObjectFromIr(*effectiveNode->childObjects[0],
            declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
            declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites, declaredCsgs,
            depth + 1);
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
            appendShapeToGeometryList(shape, &(obj->boundingShapes));
        }
    }
    for (int i = 0; i < effectiveNode->boundedCsgCount; ++i) {
        if (effectiveNode->boundedCsgs[i] != nullptr) {
            Geometry *shape = (Geometry *)buildCsgFromIr(*effectiveNode->boundedCsgs[i],
                declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites, declaredCsgs,
                depth + 1);
            appendShapeToGeometryList(shape, &(obj->boundingShapes));
        }
    }
    for (int i = 0; i < effectiveNode->clippedSphereCount; ++i) {
        if (effectiveNode->clippedSpheres[i] != nullptr) {
            Geometry *shape = (Geometry *)buildSphereResolved(
                *effectiveNode->clippedSpheres[i], declaredSpheres, depth + 1);
            appendShapeToGeometryList(shape, &(obj->clippingShapes));
        }
    }
    for (int i = 0; i < effectiveNode->clippedCsgCount; ++i) {
        if (effectiveNode->clippedCsgs[i] != nullptr) {
            Geometry *shape = (Geometry *)buildCsgFromIr(*effectiveNode->clippedCsgs[i],
                declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites, declaredCsgs,
                depth + 1);
            appendShapeToGeometryList(shape, &(obj->clippingShapes));
        }
    }
    applyTransforms(obj, effectiveNode->transforms, effectiveNode->transformCount);
    if (effectiveNode != &node) {
        applyTransforms(obj, node.transforms, node.transformCount);
    }
    logObjectOnce("antlr", obj);
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
            appendShapeToGeometryList(shape, &(comp->boundingShapes));
        }
    }
    for (int i = 0; i < effectiveNode->boundedCsgCount; ++i) {
        if (effectiveNode->boundedCsgs[i] != nullptr) {
            Geometry *shape = (Geometry *)buildCsgFromIr(*effectiveNode->boundedCsgs[i],
                declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites, declaredCsgs,
                depth + 1);
            appendShapeToGeometryList(shape, &(comp->boundingShapes));
        }
    }
    for (int i = 0; i < effectiveNode->clippedSphereCount; ++i) {
        if (effectiveNode->clippedSpheres[i] != nullptr) {
            Geometry *shape = (Geometry *)buildSphereResolved(
                *effectiveNode->clippedSpheres[i], declaredSpheres, depth + 1);
            appendShapeToGeometryList(shape, &(comp->clippingShapes));
        }
    }
    for (int i = 0; i < effectiveNode->clippedCsgCount; ++i) {
        if (effectiveNode->clippedCsgs[i] != nullptr) {
            Geometry *shape = (Geometry *)buildCsgFromIr(*effectiveNode->clippedCsgs[i],
                declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites, declaredCsgs,
                depth + 1);
            appendShapeToGeometryList(shape, &(comp->clippingShapes));
        }
    }
    applyTransforms((SimpleBody *)comp, effectiveNode->transforms, effectiveNode->transformCount);
    if (effectiveNode != &node) {
        applyTransforms((SimpleBody *)comp, node.transforms, node.transformCount);
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
    const char *dbgCsgRef = std::getenv("POVCPP_ANTLR_DEBUG_CSG_REF");
    if (node.hasReference) {
        auto it = declaredCsgs.find(node.referenceIdentifier);
        if (it != declaredCsgs.end()) {
            effectiveNode = it->second;
            if (dbgCsgRef != nullptr && dbgCsgRef[0] == '1') {
                fprintf(stderr, "[CSG_REF] resolved '%s': node.transformCount=%d (LOST), "
                    "effectiveNode(declared).transformCount=%d, effectiveNode.inverted=%d node.inverted=%d\n",
                    node.referenceIdentifier.c_str(), node.transformCount,
                    effectiveNode->transformCount, effectiveNode->inverted, node.inverted);
            }
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
    const char *dbgCsgOp = std::getenv("POVCPP_ANTLR_DEBUG_CSG_OP");
    if (effectiveNode->op == ANTLR_IR_CSG_UNION) {
        if (dbgCsgOp != nullptr && dbgCsgOp[0] == '1') {
            fprintf(stderr, "[CSG_OP] UNION detected, depth=%d\n", depth);
        }
        container = ModelBuilder::getCsgUnion();
    } else if (effectiveNode->op == ANTLR_IR_CSG_INTERSECTION) {
        if (dbgCsgOp != nullptr && dbgCsgOp[0] == '1') {
            fprintf(stderr, "[CSG_OP] INTERSECTION detected, depth=%d\n", depth);
        }
        container = ModelBuilder::getCsgIntersection();
    } else if (effectiveNode->op == ANTLR_IR_CSG_DIFFERENCE) {
        if (dbgCsgOp != nullptr && dbgCsgOp[0] == '1') {
            fprintf(stderr, "[CSG_OP] DIFFERENCE detected, depth=%d\n", depth);
        }
        container = ModelBuilder::getCsgIntersection();
    } else {
        if (dbgCsgOp != nullptr && dbgCsgOp[0] == '1') {
            fprintf(stderr, "[CSG_OP] UNKNOWN op=%d detected, defaulting to INTERSECTION, depth=%d\n",
                effectiveNode->op, depth);
        }
        container = ModelBuilder::getCsgIntersection();
    }

    int firstShapeParsed = LegacyBoolean::FALSE_VALUE;
    int childCount = 0;
    const bool preserveDeclaredChildOrder = (node.hasReference && effectiveNode != &node);
    auto linkCsgChild = [&](Geometry *child) {
        if (child == nullptr) {
            return;
        }
        if (dbgCsgOp != nullptr && dbgCsgOp[0] == '1') {
            fprintf(stderr, "[CSG_CHILD] depth=%d, childCount=%d, op=%d (DIFF=%d), firstShapeParsed=%d\n",
                depth, childCount, effectiveNode->op, ANTLR_IR_CSG_DIFFERENCE, firstShapeParsed);
        }
        if (effectiveNode->op == ANTLR_IR_CSG_DIFFERENCE && firstShapeParsed) {
            if (dbgCsgOp != nullptr && dbgCsgOp[0] == '1') {
                fprintf(stderr, "[CSG_INVERT] Inverting child at depth=%d\n", depth);
            }
            GeometryOperations::invert((SimpleBody *)child);
        }
        firstShapeParsed = LegacyBoolean::TRUE_VALUE;
        childCount++;
        if (preserveDeclaredChildOrder) {
            appendShapeToGeometryList(child, &(container->Shapes));
        } else {
            SimpleBodyFactory::link((SimpleBody *)child, (SimpleBody **)&(child->nextObject),
                (SimpleBody **)&(container->Shapes));
        }
    };

    struct OrderedCsgChild {
        int line;
        int column;
        std::function<Geometry *()> build;
    };
    std::vector<OrderedCsgChild> orderedChildren;
    orderedChildren.reserve(
        effectiveNode->childSphereCount + effectiveNode->childPlaneCount +
        effectiveNode->childBoxCount + effectiveNode->childTriangleCount +
        effectiveNode->childSmoothTriangleCount + effectiveNode->childQuadricCount +
        effectiveNode->childQuarticCount + effectiveNode->childBlobCount +
        effectiveNode->childLightCount + effectiveNode->childObjectCount +
        effectiveNode->childCompositeCount + effectiveNode->childCsgCount);

    auto addOrderedChild = [&](int line, int column, std::function<Geometry *()> build) {
        orderedChildren.push_back({line, column, std::move(build)});
    };

    for (int i = 0; i < effectiveNode->childSphereCount; ++i) {
        if (effectiveNode->childSpheres[i] != nullptr) {
            AntlrIrSphereNode *childNode = effectiveNode->childSpheres[i];
            addOrderedChild(childNode->sourceLine, childNode->sourceColumn, [&, childNode]() {
                return (Geometry *)buildSphereResolved(*childNode, declaredSpheres, depth + 1);
            });
        }
    }
    for (int i = 0; i < effectiveNode->childPlaneCount; ++i) {
        if (effectiveNode->childPlanes[i] != nullptr) {
            AntlrIrPlaneNode *childNode = effectiveNode->childPlanes[i];
            addOrderedChild(childNode->sourceLine, childNode->sourceColumn, [&, childNode]() {
                return (Geometry *)buildPlane(*childNode, declaredPlanes);
            });
        }
    }
    for (int i = 0; i < effectiveNode->childBoxCount; ++i) {
        if (effectiveNode->childBoxes[i] != nullptr) {
            AntlrIrBoxNode *childNode = effectiveNode->childBoxes[i];
            addOrderedChild(childNode->sourceLine, childNode->sourceColumn, [&, childNode]() {
                return (Geometry *)buildBox(*childNode, declaredBoxes);
            });
        }
    }
    for (int i = 0; i < effectiveNode->childTriangleCount; ++i) {
        if (effectiveNode->childTriangles[i] != nullptr) {
            AntlrIrTriangleNode *childNode = effectiveNode->childTriangles[i];
            addOrderedChild(childNode->sourceLine, childNode->sourceColumn, [&, childNode]() {
                return (Geometry *)buildTriangle(*childNode, declaredTriangles);
            });
        }
    }
    for (int i = 0; i < effectiveNode->childSmoothTriangleCount; ++i) {
        if (effectiveNode->childSmoothTriangles[i] != nullptr) {
            AntlrIrSmoothTriangleNode *childNode = effectiveNode->childSmoothTriangles[i];
            addOrderedChild(childNode->sourceLine, childNode->sourceColumn, [&, childNode]() {
                return (Geometry *)buildSmoothTriangle(*childNode, declaredSmoothTriangles);
            });
        }
    }
    for (int i = 0; i < effectiveNode->childQuadricCount; ++i) {
        if (effectiveNode->childQuadrics[i] != nullptr) {
            AntlrIrQuadricNode *childNode = effectiveNode->childQuadrics[i];
            addOrderedChild(childNode->sourceLine, childNode->sourceColumn, [&, childNode]() {
                return (Geometry *)buildQuadric(*childNode, declaredQuadrics);
            });
        }
    }
    for (int i = 0; i < effectiveNode->childQuarticCount; ++i) {
        if (effectiveNode->childQuartics[i] != nullptr) {
            AntlrIrQuarticNode *childNode = effectiveNode->childQuartics[i];
            addOrderedChild(childNode->sourceLine, childNode->sourceColumn, [&, childNode]() {
                return (Geometry *)buildQuartic(*childNode);
            });
        }
    }
    for (int i = 0; i < effectiveNode->childBlobCount; ++i) {
        if (effectiveNode->childBlobs[i] != nullptr) {
            AntlrIrBlobNode *childNode = effectiveNode->childBlobs[i];
            addOrderedChild(childNode->sourceLine, childNode->sourceColumn, [&, childNode]() {
                return (Geometry *)buildBlob(*childNode, declaredBlobs);
            });
        }
    }
    for (int i = 0; i < effectiveNode->childLightCount; ++i) {
        if (effectiveNode->childLights[i] != nullptr) {
            AntlrIrLightNode *childNode = effectiveNode->childLights[i];
            addOrderedChild(childNode->sourceLine, childNode->sourceColumn, [&, childNode]() {
                return (Geometry *)buildLight(*childNode, declaredLights);
            });
        }
    }
    for (int i = 0; i < effectiveNode->childObjectCount; ++i) {
        if (effectiveNode->childObjects[i] != nullptr) {
            AntlrIrObjectNode *childNode = effectiveNode->childObjects[i];
            addOrderedChild(childNode->sourceLine, childNode->sourceColumn, [&, childNode]() {
                return (Geometry *)buildObjectFromIr(*childNode, declaredTextures, declaredSpheres,
                    declaredPlanes, declaredBoxes, declaredTriangles, declaredSmoothTriangles,
                    declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites,
                    declaredCsgs, depth + 1);
            });
        }
    }
    for (int i = 0; i < effectiveNode->childCompositeCount; ++i) {
        if (effectiveNode->childComposites[i] != nullptr) {
            AntlrIrCompositeNode *childNode = effectiveNode->childComposites[i];
            addOrderedChild(childNode->sourceLine, childNode->sourceColumn, [&, childNode]() {
                return (Geometry *)buildCompositeFromIr(*childNode, declaredTextures, declaredSpheres,
                    declaredPlanes, declaredBoxes, declaredTriangles, declaredSmoothTriangles,
                    declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites,
                    declaredCsgs, depth + 1);
            });
        }
    }
    for (int i = 0; i < effectiveNode->childCsgCount; ++i) {
        if (effectiveNode->childCsgs[i] != nullptr) {
            AntlrIrCsgNode *childNode = effectiveNode->childCsgs[i];
            addOrderedChild(childNode->sourceLine, childNode->sourceColumn, [&, childNode]() {
                return (Geometry *)buildCsgFromIr(*childNode, declaredTextures, declaredSpheres,
                    declaredPlanes, declaredBoxes, declaredTriangles, declaredSmoothTriangles,
                    declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites,
                    declaredCsgs, depth + 1);
            });
        }
    }

    std::sort(orderedChildren.begin(), orderedChildren.end(),
        [](const OrderedCsgChild &a, const OrderedCsgChild &b) {
            if (a.line != b.line) {
                return a.line < b.line;
            }
            return a.column < b.column;
        });

    for (const OrderedCsgChild &child : orderedChildren) {
        linkCsgChild(child.build());
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

        // Fallbacks for common shapes from legacy include files not visible in ANTLR subset.
        const std::string refLower = toLowerAscii(name);
        if (refLower == "cube") {
            AntlrIrBoxNode fallbackBox;
            fallbackBox.hasReferenceBase = false;
            fallbackBox.hasInlineBase = true;
            fallbackBox.minBounds = {-1.0, -1.0, -1.0};
            fallbackBox.maxBounds = {1.0, 1.0, 1.0};
            fallbackBox.transformCount = 0;
            linkCsgChild((Geometry *)buildBox(fallbackBox, declaredBoxes));
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
    if (effectiveNode != &node) {
        if (node.inverted) {
            GeometryOperations::invert((SimpleBody *)container);
        }
        applyTransforms((SimpleBody *)container, node.transforms, node.transformCount);
    }
    logCsgOnce("antlr", container);
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
    // Create a new default texture with NO_TEXTURE type
    Texture *newTex = new Texture();
    if (newTex != nullptr) {
        newTex->textureNumber = Texture::NO_TEXTURE;
        newTex->Colour1 = nullptr;
        newTex->Colour2 = nullptr;
        newTex->Colour_Map = nullptr;
        newTex->Image = nullptr;
        newTex->Bump_Image = nullptr;
        newTex->Material_Image = nullptr;
        newTex->Next_Texture = nullptr;
        newTex->Next_Material = nullptr;
        newTex->numberOfMaterials = 0;
        newTex->Texture_Transformation = nullptr;
        newTex->objectAmbient = 0.1;
        newTex->objectDiffuse = 0.6;
        newTex->objectReflection = 0.0;
        newTex->objectBrilliance = 1.0;
        newTex->objectIndexOfRefraction = 1.0;
        newTex->objectRefraction = 0.0;
        newTex->objectTransmit = 0.0;
        newTex->objectSpecular = 0.0;
        newTex->objectRoughness = 0.05;
        newTex->objectPhong = 0.0;
        newTex->objectPhongSize = 40.0;
        newTex->bumpAmount = 0.0;
        newTex->bumpNumber = Texture::NO_BUMPS;
        newTex->textureRandomness = 0.0;
        newTex->Turbulence = 0.0;
        newTex->Frequency = 1.0;
        newTex->Phase = 0.0;
        newTex->Octaves = 6;
        newTex->Mortar = 0.2;
        newTex->constantFlag = LegacyBoolean::TRUE_VALUE;
        newTex->textureIndex = 0;
        newTex->metallicFlag = LegacyBoolean::FALSE_VALUE;
        newTex->onceFlag = LegacyBoolean::FALSE_VALUE;
        newTex->textureGradient = Vector3Dd(0.0, 0.0, 0.0);
    }
    return newTex;
}

void sanitizeTextureChain(Texture *texture)
{
    for (Texture *current = texture; current != nullptr; current = current->Next_Texture) {
        if (current->textureNumber == Texture::COLOUR_TEXTURE && current->Colour1 == nullptr) {
            current->Colour1 = ModelBuilder::getColour();
        }
        if (current->textureNumber == Texture::CHECKER_TEXTURE) {
            if (current->Colour1 == nullptr) {
                current->Colour1 = ModelBuilder::getColour();
                Color::makeColor(current->Colour1, 1.0, 1.0, 1.0);
            }
            if (current->Colour2 == nullptr) {
                current->Colour2 = ModelBuilder::getColour();
                Color::makeColor(current->Colour2, 0.0, 0.0, 0.0);
            }
        }
        if (current->textureNumber == Texture::BRICK_TEXTURE) {
            if (current->Colour1 == nullptr) {
                current->Colour1 = ModelBuilder::getColour();
                Color::makeColor(current->Colour1, 1.0, 1.0, 1.0);
            }
            if (current->Colour2 == nullptr) {
                current->Colour2 = ModelBuilder::getColour();
                Color::makeColor(current->Colour2, 0.0, 0.0, 0.0);
            }
        }
        if (current->textureNumber == Texture::CHECKER_TEXTURE_TEXTURE) {
            if (current->Colour1 == nullptr) {
                Texture *t1 = cloneDefaultTexture();
                if (t1 != nullptr) {
                    t1->textureNumber = Texture::COLOUR_TEXTURE;
                    if (t1->Colour1 == nullptr) {
                        t1->Colour1 = ModelBuilder::getColour();
                    }
                    Color::makeColor(t1->Colour1, 1.0, 1.0, 1.0);
                    current->Colour1 = (RGBAColor *)t1;
                }
            }
            if (current->Colour2 == nullptr) {
                Texture *t2 = cloneDefaultTexture();
                if (t2 != nullptr) {
                    t2->textureNumber = Texture::COLOUR_TEXTURE;
                    if (t2->Colour1 == nullptr) {
                        t2->Colour1 = ModelBuilder::getColour();
                    }
                    Color::makeColor(t2->Colour1, 0.0, 0.0, 0.0);
                    current->Colour2 = (RGBAColor *)t2;
                }
            }
        }
    }
}

Texture *materializeTextureChain(const AntlrIrTextureChain &chain,
    const std::unordered_map<std::string, Texture *> &declaredTextures)
{
    const bool debugTextureLowering = std::getenv("POVCPP_ANTLR_DEBUG_TEXTURE") != nullptr;
    Texture *head = nullptr;

    if (!chain.rawElements.empty()) {
        Logger::info("[ANTLR-MATERIALIZE] Processing %zu rawElements\n", chain.rawElements.size());
        for (const std::string &raw : chain.rawElements) {
            // FIX: detect declared texture reference BEFORE applying modifiers
            std::string declaredRef;
            const std::vector<std::string> ids = extractIdentifiers(raw);
            std::unordered_set<std::string> seen;
            for (const std::string &id : ids) {
                if (!seen.insert(id).second) {
                    continue;
                }
                if (declaredTextures.count(id)) {
                    declaredRef = id;
                    break;
                }
            }

            // FIX: clone declared texture as base if found, else default
            Texture *piece = nullptr;
            if (!declaredRef.empty()) {
                piece = cloneTexture(declaredTextures.at(declaredRef));
            } else {
                piece = cloneDefaultTexture();
            }
            if (piece == nullptr) {
                continue;
            }

            // Apply modifiers and parameters inline on the base texture
            bool handled = applyRawTextureElement(piece, raw, false);
            Logger::info("[ANTLR-MATERIALIZE] After applyRawTextureElement: textureNumber=%d, handled=%d\n",
                piece->textureNumber, handled);

            // Apply chain-level transforms to this piece
            for (const AntlrIrTransform &tr : chain.transforms) {
                Vector3Dd v = toVector(tr.vectorValue);
                if (tr.kind == ANTLR_IR_TRANSLATE) {
                    TextureUtils::translateTexture(&piece, &v);
                } else if (tr.kind == ANTLR_IR_ROTATE) {
                    TextureUtils::rotateTexture(&piece, &v);
                } else if (tr.kind == ANTLR_IR_SCALE) {
                    TextureUtils::scaleTexture(&piece, &v);
                }
            }

            sanitizeTextureChain(piece);
            logTextureStateAntlr("antlr", piece);

            if (debugTextureLowering) {
                fprintf(stderr,
                    "ANTLR_TEXTURE raw_seen: %s | tex=%d amb=%g diff=%g spec=%g rough=%g phong=%g\n",
                    raw.c_str(), piece->textureNumber, piece->objectAmbient, piece->objectDiffuse,
                    piece->objectSpecular, piece->objectRoughness, piece->objectPhong);
            }

            // Link piece into head chain
            Texture *tail = piece;
            while (tail->Next_Texture != nullptr) {
                tail = tail->Next_Texture;
            }
            tail->Next_Texture = head;
            head = piece;
        }
    }

    // Process simple references (texture { TextureName }) - only if not already in rawElements
    // to avoid double-materialization
    for (const std::string &name : chain.simpleReferenceIdentifiers) {
        // Skip if this identifier was already processed as a rawElement
        bool alreadyProcessed = false;
        for (const std::string &raw : chain.rawElements) {
            const std::vector<std::string> ids = extractIdentifiers(raw);
            if (std::find(ids.begin(), ids.end(), name) != ids.end()) {
                alreadyProcessed = true;
                break;
            }
        }
        if (alreadyProcessed) {
            continue;
        }

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
        sanitizeTextureChain(piece);
        Texture *tail = piece;
        while (tail->Next_Texture != nullptr) {
            tail = tail->Next_Texture;
        }
        tail->Next_Texture = head;
        head = piece;
    }

    // Apply procedural bump parameters extracted from AST
    // Only apply if not already set by raw element parsing (to avoid overwriting explicit settings)
    if (head != nullptr) {
        Texture *tex = head;
        while (tex != nullptr) {
            if (chain.bumpsAmount > 0.0 && tex->bumpNumber == Texture::NO_BUMPS) {
                tex->bumpNumber = Texture::BUMPS;
                tex->bumpAmount = chain.bumpsAmount;
            } else if (chain.ripplesAmount > 0.0 && tex->bumpNumber == Texture::NO_BUMPS) {
                tex->bumpNumber = Texture::RIPPLES;
                tex->bumpAmount = chain.ripplesAmount;
            } else if (chain.wrinklesAmount > 0.0 && tex->bumpNumber == Texture::NO_BUMPS) {
                tex->bumpNumber = Texture::WRINKLES;
                tex->bumpAmount = chain.wrinklesAmount;
            } else if (chain.dentsAmount > 0.0 && tex->bumpNumber == Texture::NO_BUMPS) {
                tex->bumpNumber = Texture::DENTS;
                tex->bumpAmount = chain.dentsAmount;
            } else if (chain.bumpy1Amount > 0.0 && tex->bumpNumber == Texture::NO_BUMPS) {
                tex->bumpNumber = Texture::BUMPY1;
                tex->bumpAmount = chain.bumpy1Amount;
            } else if (chain.bumpy2Amount > 0.0 && tex->bumpNumber == Texture::NO_BUMPS) {
                tex->bumpNumber = Texture::BUMPY2;
                tex->bumpAmount = chain.bumpy2Amount;
            } else if (chain.bumpy3Amount > 0.0 && tex->bumpNumber == Texture::NO_BUMPS) {
                tex->bumpNumber = Texture::BUMPY3;
                tex->bumpAmount = chain.bumpy3Amount;
            }
            tex = tex->Next_Texture;
        }
    }

    return head;
}

void applyShapeTexture(Texture *srcTexture, Geometry *shape)
{
    if (srcTexture == nullptr || shape == nullptr) {
        return;
    }

    const char *debugLog = std::getenv("POVCPP_ANTLR_DEBUG_SHAPE_TEXTURE");
    if (debugLog != nullptr && debugLog[0] == '1') {
        fprintf(stderr, "ANTLR_SHAPE_TEXTURE: applying texture(type=%d) to shape; texture.Colour1: ",
                srcTexture->textureNumber);
        if (srcTexture->Colour1 != nullptr) {
            fprintf(stderr, "RGBA<%.3f,%.3f,%.3f,%.3f>",
                    srcTexture->Colour1->Red, srcTexture->Colour1->Green,
                    srcTexture->Colour1->Blue, srcTexture->Colour1->Alpha);
        } else {
            fprintf(stderr, "NULL");
        }
        fprintf(stderr, "\n");
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
    if (srcTexture->textureNumber == 5) { // CHECKER_TEXTURE
        Logger::info("[ANTLR-APPLY-TEXTURE] Applying checker texture to object, textureNumber=%d\n", srcTexture->textureNumber);
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
            logCameraOp("antlr", "location", &camera->Location);
        } else if (op.kind == ANTLR_IR_CAMERA_DIRECTION) {
            camera->Direction = v;
            logCameraOp("antlr", "direction", &camera->Direction);
        } else if (op.kind == ANTLR_IR_CAMERA_UP) {
            camera->Up = v;
            logCameraOp("antlr", "up", &camera->Up);
        } else if (op.kind == ANTLR_IR_CAMERA_RIGHT) {
            camera->Right = v;
            logCameraOp("antlr", "right", &camera->Right);
        } else if (op.kind == ANTLR_IR_CAMERA_SKY) {
            camera->Sky = v;
            logCameraOp("antlr", "sky", &camera->Sky);
        } else if (op.kind == ANTLR_IR_CAMERA_LOOK_AT) {
            const double directionLength = camera->Direction.length();
            const double upLength = camera->Up.length();
            const double rightLength = camera->Right.length();
            Vector3Dd tempVector = camera->Direction.crossProduct(camera->Up);
            const double handedness = tempVector.dotProduct(camera->Right);
            camera->Direction = v;
            logCameraOp("antlr", "look_at", &camera->Direction);
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
    logCameraOnce("antlr", camera);
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
    gDeclaredTextures = &declaredTextures;
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
            SimpleBodyFactory::link(obj, &(obj->nextObject), (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == ANTLR_IR_COMPOSITE_NODE) {
            const AntlrIrCompositeNode *n = (const AntlrIrCompositeNode *)node;
            SimpleBody *comp = buildCompositeFromIr(
                *n, declaredTextures, declaredSpheres, declaredPlanes, declaredBoxes, declaredTriangles,
                declaredSmoothTriangles, declaredQuadrics, declaredBlobs, declaredLights, declaredObjects, declaredComposites,
                declaredCsgs, 0);
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
    gDeclaredTextures = nullptr;
#endif
}
