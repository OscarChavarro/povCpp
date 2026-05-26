#include "io/pov/antlr/AntlrSceneLowering.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "common/LegacyBoolean.h"
#include "common/color/Color.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/camera/Camera.h"
#include "environment/geometry/GeometryOperations.h"
#ifndef POV_ANTLR_MINIMAL_BRIDGE
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
Vector3Dd toVector(const AntlrIrVector3 &v)
{
    return Vector3Dd(v.x, v.y, v.z);
}

void applyTransforms(SimpleBody *shape, const AntlrIrTransform *transforms, int count)
{
    for (int i = 0; i < count; ++i) {
        Vector3Dd v = toVector(transforms[i].vectorValue);
        if (transforms[i].kind == ANTLR_IR_TRANSLATE) {
            GeometryOperations::translate(shape, &v);
        } else if (transforms[i].kind == ANTLR_IR_ROTATE) {
            GeometryOperations::rotate(shape, &v);
        } else if (transforms[i].kind == ANTLR_IR_SCALE) {
            GeometryOperations::scale(shape, &v);
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

bool buildDeclaredSphereByName(const std::string &name,
    const std::unordered_map<std::string, const AntlrIrSphereNode *> &declaredSpheres, Sphere *&out)
{
    auto it = declaredSpheres.find(name);
    if (it == declaredSpheres.end()) {
        return false;
    }
    out = buildSphere(*it->second);
    return out != nullptr;
}

Light *buildLight(const AntlrIrLightNode &node,
    const std::unordered_map<std::string, const AntlrIrLightNode *> &declaredLights)
{
    static constexpr double PI = 3.14159265358979323846;
    const AntlrIrLightNode *effectiveNode = &node;
    if (node.hasReference) {
        auto it = declaredLights.find(node.referenceIdentifier);
        if (it == declaredLights.end()) {
            throw std::runtime_error("Unknown ANTLR light reference");
        }
        effectiveNode = it->second;
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

Texture *materializeTextureChain(const AntlrIrTextureChain &chain,
    const std::unordered_map<std::string, Texture *> &declaredTextures);
void applyObjectTexture(Texture *srcTexture, SimpleBody *object);

SimpleBody *buildObjectFromIr(const AntlrIrObjectNode &node,
    const std::unordered_map<std::string, Texture *> &declaredTextures,
    const std::unordered_map<std::string, const AntlrIrSphereNode *> &declaredSpheres,
    const std::unordered_map<std::string, const AntlrIrObjectNode *> &declaredObjects, int depth)
{
    if (depth > 16) {
        throw std::runtime_error("ANTLR object lowering exceeded max recursion depth");
    }

    const AntlrIrObjectNode *effectiveNode = &node;
    if (node.hasReference) {
        auto oit = declaredObjects.find(node.referenceIdentifier);
        if (oit == declaredObjects.end()) {
            throw std::runtime_error("Unknown ANTLR object reference");
        }
        effectiveNode = oit->second;
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
    if (effectiveNode->childObjectCount > 0 || effectiveNode->childCompositeCount > 0) {
        throw std::runtime_error(
            "ANTLR object lowering does not support object/composite child as Shape yet at " +
            formatSourceLocation(node));
    }
    if (effectiveNode->childSphereCount > 0 && effectiveNode->childSpheres[0] != nullptr) {
        obj->Shape = (Geometry *)buildSphere(*effectiveNode->childSpheres[0]);
    } else if (effectiveNode->childReferenceCount > 0) {
        Sphere *resolved = nullptr;
        if (buildDeclaredSphereByName(
                effectiveNode->childReferenceIdentifiers[0], declaredSpheres, resolved)) {
            obj->Shape = (Geometry *)resolved;
        }
    }
    return obj;
}

SimpleBody *buildCompositeFromIr(const AntlrIrCompositeNode &node,
    const std::unordered_map<std::string, Texture *> &declaredTextures,
    const std::unordered_map<std::string, const AntlrIrSphereNode *> &declaredSpheres,
    const std::unordered_map<std::string, const AntlrIrObjectNode *> &declaredObjects,
    const std::unordered_map<std::string, const AntlrIrCompositeNode *> &declaredComposites, int depth)
{
    if (depth > 16) {
        throw std::runtime_error("ANTLR composite lowering exceeded max recursion depth");
    }

    const AntlrIrCompositeNode *effectiveNode = &node;
    if (node.hasReference) {
        auto it = declaredComposites.find(node.referenceIdentifier);
        if (it == declaredComposites.end()) {
            throw std::runtime_error("Unknown ANTLR composite reference");
        }
        effectiveNode = it->second;
    }

    Composite *comp = ModelBuilder::getCompositeObject();
    for (int k = 0; k < effectiveNode->childSphereCount; ++k) {
        if (effectiveNode->childSpheres[k] == nullptr) {
            continue;
        }
        Sphere *child = buildSphere(*effectiveNode->childSpheres[k]);
        SimpleBodyFactory::link((SimpleBody *)child, (SimpleBody **)&(child->nextObject),
            (SimpleBody **)&(comp->Objects));
    }
    for (int k = 0; k < effectiveNode->childReferenceCount; ++k) {
        Sphere *resolved = nullptr;
        if (!buildDeclaredSphereByName(
                effectiveNode->childReferenceIdentifiers[k], declaredSpheres, resolved)) {
            continue;
        }
        SimpleBodyFactory::link((SimpleBody *)resolved, (SimpleBody **)&(resolved->nextObject),
            (SimpleBody **)&(comp->Objects));
    }
    for (int k = 0; k < effectiveNode->childObjectCount; ++k) {
        if (effectiveNode->childObjects[k] == nullptr) {
            continue;
        }
        SimpleBody *childObj = buildObjectFromIr(
            *effectiveNode->childObjects[k], declaredTextures, declaredSpheres, declaredObjects, depth + 1);
        applyTransforms(childObj, effectiveNode->childObjects[k]->transforms,
            effectiveNode->childObjects[k]->transformCount);
        SimpleBodyFactory::link(childObj, &(childObj->nextObject), (SimpleBody **)&(comp->Objects));
    }
    for (int k = 0; k < effectiveNode->childCompositeCount; ++k) {
        if (effectiveNode->childComposites[k] == nullptr) {
            continue;
        }
        SimpleBody *childComp = buildCompositeFromIr(*effectiveNode->childComposites[k],
            declaredTextures, declaredSpheres, declaredObjects, declaredComposites, depth + 1);
        applyTransforms(childComp, effectiveNode->childComposites[k]->transforms,
            effectiveNode->childComposites[k]->transformCount);
        SimpleBodyFactory::link(childComp, &(childComp->nextObject), (SimpleBody **)&(comp->Objects));
    }
    return (SimpleBody *)comp;
}

CSG *buildCsgFromIr(const AntlrIrCsgNode &node,
    const std::unordered_map<std::string, Texture *> &declaredTextures,
    const std::unordered_map<std::string, const AntlrIrSphereNode *> &declaredSpheres,
    const std::unordered_map<std::string, const AntlrIrObjectNode *> &declaredObjects,
    const std::unordered_map<std::string, const AntlrIrCompositeNode *> &declaredComposites,
    const std::unordered_map<std::string, const AntlrIrCsgNode *> &declaredCsgs, int depth)
{
    if (depth > 16) {
        throw std::runtime_error("ANTLR csg lowering exceeded max recursion depth");
    }

    const AntlrIrCsgNode *effectiveNode = &node;
    if (node.hasReference) {
        auto it = declaredCsgs.find(node.referenceIdentifier);
        if (it == declaredCsgs.end()) {
            throw std::runtime_error("Unknown ANTLR csg reference");
        }
        effectiveNode = it->second;
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
            linkCsgChild((Geometry *)buildSphere(*effectiveNode->childSpheres[i]));
        }
    }
    for (int i = 0; i < effectiveNode->childObjectCount; ++i) {
        if (effectiveNode->childObjects[i] != nullptr) {
            linkCsgChild((Geometry *)buildObjectFromIr(*effectiveNode->childObjects[i],
                declaredTextures, declaredSpheres, declaredObjects, depth + 1));
        }
    }
    for (int i = 0; i < effectiveNode->childCompositeCount; ++i) {
        if (effectiveNode->childComposites[i] != nullptr) {
            linkCsgChild((Geometry *)buildCompositeFromIr(*effectiveNode->childComposites[i],
                declaredTextures, declaredSpheres, declaredObjects, declaredComposites, depth + 1));
        }
    }
    for (int i = 0; i < effectiveNode->childCsgCount; ++i) {
        if (effectiveNode->childCsgs[i] != nullptr) {
            linkCsgChild((Geometry *)buildCsgFromIr(*effectiveNode->childCsgs[i],
                declaredTextures, declaredSpheres, declaredObjects, declaredComposites,
                declaredCsgs, depth + 1));
        }
    }
    for (int i = 0; i < effectiveNode->childReferenceCount; ++i) {
        const std::string &name = effectiveNode->childReferenceIdentifiers[i];
        auto sit = declaredSpheres.find(name);
        if (sit != declaredSpheres.end()) {
            linkCsgChild((Geometry *)buildSphere(*sit->second));
            continue;
        }
        auto oit = declaredObjects.find(name);
        if (oit != declaredObjects.end()) {
            linkCsgChild((Geometry *)buildObjectFromIr(
                *oit->second, declaredTextures, declaredSpheres, declaredObjects, depth + 1));
            continue;
        }
        auto cit = declaredComposites.find(name);
        if (cit != declaredComposites.end()) {
            linkCsgChild((Geometry *)buildCompositeFromIr(*cit->second,
                declaredTextures, declaredSpheres, declaredObjects, declaredComposites, depth + 1));
            continue;
        }
        auto csgIt = declaredCsgs.find(name);
        if (csgIt != declaredCsgs.end()) {
            linkCsgChild((Geometry *)buildCsgFromIr(*csgIt->second,
                declaredTextures, declaredSpheres, declaredObjects, declaredComposites,
                declaredCsgs, depth + 1));
        }
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

void applyShapeTexture(Texture *srcTexture, Sphere *shape)
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

void applyCameraNode(const AntlrIrCameraNode &node, RenderFrame *framePtr)
{
    Camera *camera = &(framePtr->viewPoint);
    camera->initializeDefaults();

    for (int i = 0; i < node.opCount; ++i) {
        const AntlrIrCameraOp &op = node.ops[i];
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

#ifndef POV_ANTLR_MINIMAL_BRIDGE
    std::unordered_map<std::string, Texture *> declaredTextures;
    std::unordered_map<std::string, const AntlrIrSphereNode *> declaredSpheres;
    std::unordered_map<std::string, const AntlrIrObjectNode *> declaredObjects;
    std::unordered_map<std::string, const AntlrIrCompositeNode *> declaredComposites;
    std::unordered_map<std::string, const AntlrIrLightNode *> declaredLights;
    std::unordered_map<std::string, const AntlrIrCsgNode *> declaredCsgs;
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
            applyCameraNode(*(const AntlrIrCameraNode *)node, framePtr);
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
            } else if (n->valueKind == AntlrIrDeclareNode::DECLARE_OBJECT && n->hasObjectValue) {
                declaredObjects[n->identifier] = n->objectValue;
            } else if (
                n->valueKind == AntlrIrDeclareNode::DECLARE_COMPOSITE && n->hasCompositeValue) {
                declaredComposites[n->identifier] = n->compositeValue;
            } else if (n->valueKind == AntlrIrDeclareNode::DECLARE_LIGHT && n->hasLightValue) {
                declaredLights[n->identifier] = n->lightValue;
            } else if (n->valueKind == AntlrIrDeclareNode::DECLARE_CSG && n->hasCsgValue) {
                declaredCsgs[n->identifier] = n->csgValue;
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
                    throw std::runtime_error("Unknown ANTLR sphere reference");
                }
                sphere = buildSphere(*it->second);
            } else {
                sphere = buildSphere(*n);
            }
            if (n->hasTextureChain) {
                applyShapeTexture(materializeTextureChain(n->textureChain, declaredTextures), sphere);
            }
            SimpleBodyFactory::link((SimpleBody *)sphere,
                (SimpleBody **)&(sphere->nextObject), (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == ANTLR_IR_OBJECT_NODE) {
            const AntlrIrObjectNode *n = (const AntlrIrObjectNode *)node;
            SimpleBody *obj = buildObjectFromIr(
                *n, declaredTextures, declaredSpheres, declaredObjects, 0);
            applyTransforms(obj, n->transforms, n->transformCount);
            SimpleBodyFactory::link(obj, &(obj->nextObject), (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == ANTLR_IR_COMPOSITE_NODE) {
            const AntlrIrCompositeNode *n = (const AntlrIrCompositeNode *)node;
            SimpleBody *comp = buildCompositeFromIr(
                *n, declaredTextures, declaredSpheres, declaredObjects, declaredComposites, 0);
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
                declaredObjects, declaredComposites, declaredCsgs, 0);
            SimpleBodyFactory::link((SimpleBody *)csg, (SimpleBody **)&(csg->nextObject),
                (SimpleBody **)&(framePtr->Objects));
#endif
        }
    }
}
