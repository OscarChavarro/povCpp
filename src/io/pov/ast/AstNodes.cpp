#include "io/pov/ast/AstNodes.h"

AstSphereNode::AstSphereNode()
{
    kind = AST_SPHERE_NODE;
    sourceLine = -1;
    sourceFile = nullptr;
    hasInlineData = false;
    hasReference = false;
    referenceConstantId = -1;
    radius = 0.0;
    hasColour = false;
    transformCount = 0;
}

AstLightSourceNode::AstLightSourceNode()
{
    kind = AST_LIGHT_SOURCE_NODE;
    sourceLine = -1;
    sourceFile = nullptr;
    hasInlineData = false;
    hasReference = false;
    referenceConstantId = -1;
    hasSpotlight = false;
    tightness = 0.0;
    radiusDegrees = 0.0;
    falloffDegrees = 0.0;
    transformCount = 0;
}

AstCsgNode::AstCsgNode()
{
    kind = AST_CSG_NODE;
    sourceLine = -1;
    sourceFile = nullptr;
    op = AST_CSG_UNION;
    hasReference = false;
    referenceConstantId = -1;
    childCount = 0;
    transformCount = 0;
}

AstDeclareNode::AstDeclareNode()
{
    kind = AST_DECLARE_NODE;
    sourceLine = -1;
    sourceFile = nullptr;
    identifierNumber = -1;
    value = nullptr;
}

AstObjectNode::AstObjectNode()
{
    kind = AST_OBJECT_NODE;
    sourceLine = -1;
    sourceFile = nullptr;
    hasReference = false;
    referenceConstantId = -1;
    shape = nullptr;
    boundedByCount = 0;
    clippedByCount = 0;
    transformCount = 0;
    noShadow = false;
}

AstCompositeNode::AstCompositeNode()
{
    kind = AST_COMPOSITE_NODE;
    sourceLine = -1;
    sourceFile = nullptr;
    hasReference = false;
    referenceConstantId = -1;
    childCount = 0;
    boundedByCount = 0;
    clippedByCount = 0;
    transformCount = 0;
}

AstScene::AstScene()
{
    nodeCount = 0;
}

bool
AstNodes::appendTransform(
    AstTransform *arr, int &count, AstTransformKind kind, const AstVector3 &vec)
{
    if (count >= AstLimits::MAX_AST_TRANSFORMS) {
        return false;
    }
    arr[count].kind = kind;
    arr[count].vectorValue = vec;
    count++;
    return true;
}

bool
AstNodes::appendNode(AstNode **arr, int &count, int cap, AstNode *node)
{
    if (count >= cap) {
        return false;
    }
    arr[count] = node;
    count++;
    return true;
}

void
AstNodes::destroyNode(AstNode *node)
{
    if (node == nullptr) {
        return;
    }

    if (node->kind == AST_DECLARE_NODE) {
        AstDeclareNode *n = (AstDeclareNode *)node;
        destroyNode(n->value);
    } else if (node->kind == AST_CSG_NODE) {
        AstCsgNode *n = (AstCsgNode *)node;
        for (int i = 0; i < n->childCount; ++i) {
            destroyNode(n->children[i]);
        }
    } else if (node->kind == AST_OBJECT_NODE) {
        AstObjectNode *n = (AstObjectNode *)node;
        destroyNode(n->shape);
        for (int i = 0; i < n->boundedByCount; ++i) {
            destroyNode(n->boundedBy[i]);
        }
        for (int i = 0; i < n->clippedByCount; ++i) {
            destroyNode(n->clippedBy[i]);
        }
    } else if (node->kind == AST_COMPOSITE_NODE) {
        AstCompositeNode *n = (AstCompositeNode *)node;
        for (int i = 0; i < n->childCount; ++i) {
            destroyNode(n->children[i]);
        }
        for (int i = 0; i < n->boundedByCount; ++i) {
            destroyNode(n->boundedBy[i]);
        }
        for (int i = 0; i < n->clippedByCount; ++i) {
            destroyNode(n->clippedBy[i]);
        }
    }

    delete node;
}

void
AstNodes::destroyScene(AstScene *scene)
{
    if (scene == nullptr) {
        return;
    }
    for (int i = 0; i < scene->nodeCount; ++i) {
        destroyNode(scene->nodes[i]);
    }
    delete scene;
}
