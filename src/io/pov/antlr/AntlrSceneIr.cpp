#include "io/pov/antlr/AntlrSceneIr.h"

namespace {
void destroyCompositeChildren(AntlrIrCompositeNode *comp);
void destroyCsgChildren(AntlrIrCsgNode *csg);

void destroyObjectChildren(AntlrIrObjectNode *obj)
{
    if (obj == nullptr) {
        return;
    }
    for (int i = 0; i < obj->childSphereCount; ++i) {
        delete obj->childSpheres[i];
        obj->childSpheres[i] = nullptr;
    }
    for (int i = 0; i < obj->childObjectCount; ++i) {
        destroyObjectChildren(obj->childObjects[i]);
        delete obj->childObjects[i];
        obj->childObjects[i] = nullptr;
    }
    for (int i = 0; i < obj->childCompositeCount; ++i) {
        destroyCompositeChildren(obj->childComposites[i]);
        delete obj->childComposites[i];
        obj->childComposites[i] = nullptr;
    }
}

void destroyCompositeChildren(AntlrIrCompositeNode *comp)
{
    if (comp == nullptr) {
        return;
    }
    for (int i = 0; i < comp->childSphereCount; ++i) {
        delete comp->childSpheres[i];
        comp->childSpheres[i] = nullptr;
    }
    for (int i = 0; i < comp->childObjectCount; ++i) {
        destroyObjectChildren(comp->childObjects[i]);
        delete comp->childObjects[i];
        comp->childObjects[i] = nullptr;
    }
    for (int i = 0; i < comp->childCompositeCount; ++i) {
        destroyCompositeChildren(comp->childComposites[i]);
        delete comp->childComposites[i];
        comp->childComposites[i] = nullptr;
    }
}

void destroyCsgChildren(AntlrIrCsgNode *csg)
{
    if (csg == nullptr) {
        return;
    }
    for (int i = 0; i < csg->childSphereCount; ++i) {
        delete csg->childSpheres[i];
        csg->childSpheres[i] = nullptr;
    }
    for (int i = 0; i < csg->childObjectCount; ++i) {
        destroyObjectChildren(csg->childObjects[i]);
        delete csg->childObjects[i];
        csg->childObjects[i] = nullptr;
    }
    for (int i = 0; i < csg->childCompositeCount; ++i) {
        destroyCompositeChildren(csg->childComposites[i]);
        delete csg->childComposites[i];
        csg->childComposites[i] = nullptr;
    }
    for (int i = 0; i < csg->childCsgCount; ++i) {
        destroyCsgChildren(csg->childCsgs[i]);
        delete csg->childCsgs[i];
        csg->childCsgs[i] = nullptr;
    }
}
}

AntlrIrMaxTraceLevelNode::AntlrIrMaxTraceLevelNode()
{
    kind = ANTLR_IR_MAX_TRACE_LEVEL_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    value = 0.0;
}

AntlrIrFogNode::AntlrIrFogNode()
{
    kind = ANTLR_IR_FOG_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    hasColour = false;
    hasDistance = false;
    distance = 0.0;
}

AntlrIrCameraNode::AntlrIrCameraNode()
{
    kind = ANTLR_IR_CAMERA_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    opCount = 0;
}

AntlrIrDeclareNode::AntlrIrDeclareNode()
{
    kind = ANTLR_IR_DECLARE_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    valueKind = DECLARE_UNKNOWN;
    hasTextureChainValue = false;
    hasSphereValue = false;
    sphereValue = nullptr;
    hasObjectValue = false;
    objectValue = nullptr;
    hasCompositeValue = false;
    compositeValue = nullptr;
    hasLightValue = false;
    lightValue = nullptr;
    hasCsgValue = false;
    csgValue = nullptr;
}

AntlrIrDefaultTextureNode::AntlrIrDefaultTextureNode()
{
    kind = ANTLR_IR_DEFAULT_TEXTURE_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    hasTextureChain = false;
}

AntlrIrSphereNode::AntlrIrSphereNode()
{
    kind = ANTLR_IR_SPHERE_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    hasReferenceBase = false;
    hasInlineBase = false;
    radius = 0.0;
    hasColour = false;
    hasTextureChain = false;
    transformCount = 0;
}

AntlrIrObjectNode::AntlrIrObjectNode()
{
    kind = ANTLR_IR_OBJECT_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    hasReference = false;
    childSphereCount = 0;
    for (int i = 0; i < MAX_CHILD_SPHERES; ++i) {
        childSpheres[i] = nullptr;
    }
    childObjectCount = 0;
    for (int i = 0; i < MAX_CHILD_OBJECTS; ++i) {
        childObjects[i] = nullptr;
    }
    childCompositeCount = 0;
    for (int i = 0; i < MAX_CHILD_COMPOSITES; ++i) {
        childComposites[i] = nullptr;
    }
    childReferenceCount = 0;
    hasColour = false;
    hasTextureChain = false;
    noShadow = false;
    transformCount = 0;
}

AntlrIrCompositeNode::AntlrIrCompositeNode()
{
    kind = ANTLR_IR_COMPOSITE_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    hasReference = false;
    childSphereCount = 0;
    for (int i = 0; i < MAX_CHILD_SPHERES; ++i) {
        childSpheres[i] = nullptr;
    }
    childObjectCount = 0;
    for (int i = 0; i < MAX_CHILD_OBJECTS; ++i) {
        childObjects[i] = nullptr;
    }
    childCompositeCount = 0;
    for (int i = 0; i < MAX_CHILD_COMPOSITES; ++i) {
        childComposites[i] = nullptr;
    }
    childReferenceCount = 0;
    transformCount = 0;
}

AntlrIrLightNode::AntlrIrLightNode()
{
    kind = ANTLR_IR_LIGHT_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    hasReference = false;
    hasCenter = false;
    hasColour = false;
    hasPointAt = false;
    hasTightness = false;
    tightness = 0.0;
    hasRadius = false;
    radiusDegrees = 0.0;
    hasFalloff = false;
    falloffDegrees = 0.0;
    spotlight = false;
    transformCount = 0;
}

AntlrIrCsgNode::AntlrIrCsgNode()
{
    kind = ANTLR_IR_CSG_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    op = ANTLR_IR_CSG_UNION;
    hasReference = false;
    childSphereCount = 0;
    for (int i = 0; i < MAX_CHILD_SPHERES; ++i) {
        childSpheres[i] = nullptr;
    }
    childObjectCount = 0;
    for (int i = 0; i < MAX_CHILD_OBJECTS; ++i) {
        childObjects[i] = nullptr;
    }
    childCompositeCount = 0;
    for (int i = 0; i < MAX_CHILD_COMPOSITES; ++i) {
        childComposites[i] = nullptr;
    }
    childCsgCount = 0;
    for (int i = 0; i < MAX_CHILD_CSGS; ++i) {
        childCsgs[i] = nullptr;
    }
    childReferenceCount = 0;
    inverted = false;
    transformCount = 0;
}

AntlrSceneIrProgram::AntlrSceneIrProgram()
{
    nodeCount = 0;
    for (int i = 0; i < MAX_NODES; ++i) {
        nodes[i] = nullptr;
    }
}

bool
AntlrSceneIrNodes::appendNode(AntlrSceneIrProgram &program, AntlrSceneIrNode *node)
{
    if (program.nodeCount >= AntlrSceneIrProgram::MAX_NODES) {
        return false;
    }
    program.nodes[program.nodeCount++] = node;
    return true;
}

bool
AntlrSceneIrNodes::appendCameraOp(AntlrIrCameraNode &camera, const AntlrIrCameraOp &op)
{
    if (camera.opCount >= AntlrIrCameraNode::MAX_CAMERA_OPS) {
        return false;
    }
    camera.ops[camera.opCount++] = op;
    return true;
}

bool
AntlrSceneIrNodes::appendTransform(
    AntlrIrTransform *arr, int &count, AntlrIrTransformKind kind, const AntlrIrVector3 &v)
{
    if (count >= AntlrIrSphereNode::MAX_TRANSFORMS) {
        return false;
    }
    arr[count].kind = kind;
    arr[count].vectorValue = v;
    count++;
    return true;
}

void
AntlrSceneIrNodes::destroyNode(AntlrSceneIrNode *node)
{
    if (node == nullptr) {
        return;
    }
    if (node->kind == ANTLR_IR_DECLARE_NODE) {
        AntlrIrDeclareNode *declareNode = (AntlrIrDeclareNode *)node;
        if (declareNode->sphereValue != nullptr) {
            delete declareNode->sphereValue;
            declareNode->sphereValue = nullptr;
        }
        if (declareNode->objectValue != nullptr) {
            destroyObjectChildren(declareNode->objectValue);
            delete declareNode->objectValue;
            declareNode->objectValue = nullptr;
        }
        if (declareNode->compositeValue != nullptr) {
            destroyCompositeChildren(declareNode->compositeValue);
            delete declareNode->compositeValue;
            declareNode->compositeValue = nullptr;
        }
        if (declareNode->lightValue != nullptr) {
            delete declareNode->lightValue;
            declareNode->lightValue = nullptr;
        }
        if (declareNode->csgValue != nullptr) {
            destroyCsgChildren(declareNode->csgValue);
            delete declareNode->csgValue;
            declareNode->csgValue = nullptr;
        }
    } else if (node->kind == ANTLR_IR_OBJECT_NODE) {
        destroyObjectChildren((AntlrIrObjectNode *)node);
    } else if (node->kind == ANTLR_IR_COMPOSITE_NODE) {
        destroyCompositeChildren((AntlrIrCompositeNode *)node);
    } else if (node->kind == ANTLR_IR_CSG_NODE) {
        destroyCsgChildren((AntlrIrCsgNode *)node);
    }
    delete node;
}

void
AntlrSceneIrNodes::destroyProgram(AntlrSceneIrProgram *program)
{
    if (program == nullptr) {
        return;
    }
    for (int i = 0; i < program->nodeCount; ++i) {
        destroyNode(program->nodes[i]);
    }
    delete program;
}
