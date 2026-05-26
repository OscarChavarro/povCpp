#include "io/pov/antlr/AntlrSceneLowering.h"

#include <cctype>
#include <cstdio>
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
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/material/RenderRuntimeState.h"
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

bool parseSphereText(const std::string &text, AntlrIrSphereNode &out)
{
    const std::size_t left = text.find('<');
    const std::size_t right = text.find('>', left == std::string::npos ? 0 : left + 1);
    if (left == std::string::npos || right == std::string::npos || right <= left + 1) {
        return false;
    }
    const std::string vec = text.substr(left + 1, right - left - 1);
    double x = 0.0, y = 0.0, z = 0.0;
    if (std::sscanf(vec.c_str(), "%lf,%lf,%lf", &x, &y, &z) != 3) {
        return false;
    }
    std::size_t p = right + 1;
    while (p < text.size() && std::isspace((unsigned char)text[p])) {
        ++p;
    }
    char *endPtr = nullptr;
    const double radius = std::strtod(text.c_str() + p, &endPtr);
    if (endPtr == text.c_str() + p) {
        return false;
    }
    out.hasInlineBase = true;
    out.center = {x, y, z};
    out.radius = radius;
    return true;
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
            const AntlrIrObjectNode *effectiveNode = n;
            if (n->hasReference) {
                auto oit = declaredObjects.find(n->referenceIdentifier);
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
                applyObjectTexture(
                    materializeTextureChain(effectiveNode->textureChain, declaredTextures), obj);
            }
            if (effectiveNode->childShapeCount > 0) {
                AntlrIrSphereNode childSphere;
                if (parseSphereText(effectiveNode->childShapeTexts[0], childSphere)) {
                    obj->Shape = (Geometry *)buildSphere(childSphere);
                }
            } else if (effectiveNode->childReferenceCount > 0) {
                Sphere *resolved = nullptr;
                if (buildDeclaredSphereByName(
                        effectiveNode->childReferenceIdentifiers[0], declaredSpheres, resolved)) {
                    obj->Shape = (Geometry *)resolved;
                }
            }
            applyTransforms(obj, n->transforms, n->transformCount);
            SimpleBodyFactory::link(obj, &(obj->nextObject), (SimpleBody **)&(framePtr->Objects));
        } else if (node->kind == ANTLR_IR_COMPOSITE_NODE) {
            const AntlrIrCompositeNode *n = (const AntlrIrCompositeNode *)node;
            const AntlrIrCompositeNode *effectiveNode = n;
            if (n->hasReference) {
                auto it = declaredComposites.find(n->referenceIdentifier);
                if (it == declaredComposites.end()) {
                    throw std::runtime_error("Unknown ANTLR composite reference");
                }
                effectiveNode = it->second;
            }
            Composite *comp = ModelBuilder::getCompositeObject();
            for (int k = 0; k < effectiveNode->childShapeCount; ++k) {
                AntlrIrSphereNode childSphere;
                if (!parseSphereText(effectiveNode->childShapeTexts[k], childSphere)) {
                    continue;
                }
                Sphere *child = buildSphere(childSphere);
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
            applyTransforms((SimpleBody *)comp, n->transforms, n->transformCount);
            SimpleBodyFactory::link((SimpleBody *)comp,
                (SimpleBody **)&(comp->nextObject), (SimpleBody **)&(framePtr->Objects));
#endif
        }
    }
}
