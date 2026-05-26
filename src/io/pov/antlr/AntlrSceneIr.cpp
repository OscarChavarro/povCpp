#include "io/pov/antlr/AntlrSceneIr.h"

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
    childShapeCount = 0;
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
    childShapeCount = 0;
    childReferenceCount = 0;
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
            delete declareNode->objectValue;
            declareNode->objectValue = nullptr;
        }
        if (declareNode->compositeValue != nullptr) {
            delete declareNode->compositeValue;
            declareNode->compositeValue = nullptr;
        }
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
