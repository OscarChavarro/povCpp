#include "io/pov/antlr/AntlrSceneIr.h"
#include "common/linealAlgebra/Vector3Dd.h"

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
    for (int i = 0; i < obj->childPlaneCount; ++i) {
        delete obj->childPlanes[i];
        obj->childPlanes[i] = nullptr;
    }
    for (int i = 0; i < obj->childBoxCount; ++i) {
        delete obj->childBoxes[i];
        obj->childBoxes[i] = nullptr;
    }
    for (int i = 0; i < obj->childTriangleCount; ++i) {
        delete obj->childTriangles[i];
        obj->childTriangles[i] = nullptr;
    }
    for (int i = 0; i < obj->childSmoothTriangleCount; ++i) {
        delete obj->childSmoothTriangles[i];
        obj->childSmoothTriangles[i] = nullptr;
    }
    for (int i = 0; i < obj->childQuadricCount; ++i) {
        delete obj->childQuadrics[i];
        obj->childQuadrics[i] = nullptr;
    }
    for (int i = 0; i < obj->childQuarticCount; ++i) {
        delete obj->childQuartics[i];
        obj->childQuartics[i] = nullptr;
    }
    for (int i = 0; i < obj->childBlobCount; ++i) {
        delete obj->childBlobs[i];
        obj->childBlobs[i] = nullptr;
    }
    for (int i = 0; i < obj->childBicubicPatchCount; ++i) {
        delete obj->childBicubicPatches[i];
        obj->childBicubicPatches[i] = nullptr;
    }
    for (int i = 0; i < obj->childLightCount; ++i) {
        delete obj->childLights[i];
        obj->childLights[i] = nullptr;
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
    for (int i = 0; i < obj->childCsgCount; ++i) {
        destroyCsgChildren(obj->childCsgs[i]);
        delete obj->childCsgs[i];
        obj->childCsgs[i] = nullptr;
    }
    for (int i = 0; i < obj->boundedSphereCount; ++i) {
        delete obj->boundedSpheres[i];
        obj->boundedSpheres[i] = nullptr;
    }
    for (int i = 0; i < obj->boundedCsgCount; ++i) {
        destroyCsgChildren(obj->boundedCsgs[i]);
        delete obj->boundedCsgs[i];
        obj->boundedCsgs[i] = nullptr;
    }
    for (int i = 0; i < obj->clippedSphereCount; ++i) {
        delete obj->clippedSpheres[i];
        obj->clippedSpheres[i] = nullptr;
    }
    for (int i = 0; i < obj->clippedCsgCount; ++i) {
        destroyCsgChildren(obj->clippedCsgs[i]);
        delete obj->clippedCsgs[i];
        obj->clippedCsgs[i] = nullptr;
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
    for (int i = 0; i < comp->boundedSphereCount; ++i) {
        delete comp->boundedSpheres[i];
        comp->boundedSpheres[i] = nullptr;
    }
    for (int i = 0; i < comp->boundedCsgCount; ++i) {
        destroyCsgChildren(comp->boundedCsgs[i]);
        delete comp->boundedCsgs[i];
        comp->boundedCsgs[i] = nullptr;
    }
    for (int i = 0; i < comp->clippedSphereCount; ++i) {
        delete comp->clippedSpheres[i];
        comp->clippedSpheres[i] = nullptr;
    }
    for (int i = 0; i < comp->clippedCsgCount; ++i) {
        destroyCsgChildren(comp->clippedCsgs[i]);
        delete comp->clippedCsgs[i];
        comp->clippedCsgs[i] = nullptr;
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
    for (int i = 0; i < csg->childPlaneCount; ++i) {
        delete csg->childPlanes[i];
        csg->childPlanes[i] = nullptr;
    }
    for (int i = 0; i < csg->childBoxCount; ++i) {
        delete csg->childBoxes[i];
        csg->childBoxes[i] = nullptr;
    }
    for (int i = 0; i < csg->childTriangleCount; ++i) {
        delete csg->childTriangles[i];
        csg->childTriangles[i] = nullptr;
    }
    for (int i = 0; i < csg->childSmoothTriangleCount; ++i) {
        delete csg->childSmoothTriangles[i];
        csg->childSmoothTriangles[i] = nullptr;
    }
    for (int i = 0; i < csg->childQuadricCount; ++i) {
        delete csg->childQuadrics[i];
        csg->childQuadrics[i] = nullptr;
    }
    for (int i = 0; i < csg->childQuarticCount; ++i) {
        delete csg->childQuartics[i];
        csg->childQuartics[i] = nullptr;
    }
    for (int i = 0; i < csg->childBlobCount; ++i) {
        delete csg->childBlobs[i];
        csg->childBlobs[i] = nullptr;
    }
    for (int i = 0; i < csg->childLightCount; ++i) {
        delete csg->childLights[i];
        csg->childLights[i] = nullptr;
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
    hasReference = false;
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
    hasPlaneValue = false;
    planeValue = nullptr;
    hasBoxValue = false;
    boxValue = nullptr;
    hasTriangleValue = false;
    triangleValue = nullptr;
    hasSmoothTriangleValue = false;
    smoothTriangleValue = nullptr;
    hasQuadricValue = false;
    quadricValue = nullptr;
    hasQuarticValue = false;
    quarticValue = nullptr;
    hasBlobValue = false;
    blobValue = nullptr;
    hasObjectValue = false;
    objectValue = nullptr;
    hasCompositeValue = false;
    compositeValue = nullptr;
    hasLightValue = false;
    lightValue = nullptr;
    hasCsgValue = false;
    csgValue = nullptr;
    hasCameraValue = false;
    cameraValue = nullptr;
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

AntlrIrPlaneNode::AntlrIrPlaneNode()
{
    kind = ANTLR_IR_PLANE_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    hasReferenceBase = false;
    hasInlineBase = false;
    distance = 0.0;
    hasColour = false;
    hasTextureChain = false;
    inverted = false;
    transformCount = 0;
}

AntlrIrBoxNode::AntlrIrBoxNode()
{
    kind = ANTLR_IR_BOX_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    hasReferenceBase = false;
    hasInlineBase = false;
    hasColour = false;
    hasTextureChain = false;
    inverted = false;
    transformCount = 0;
}

AntlrIrTriangleNode::AntlrIrTriangleNode()
{
    kind = ANTLR_IR_TRIANGLE_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    hasReferenceBase = false;
    hasInlineBase = false;
    hasColour = false;
    hasTextureChain = false;
    inverted = false;
    transformCount = 0;
}

AntlrIrSmoothTriangleNode::AntlrIrSmoothTriangleNode()
{
    kind = ANTLR_IR_SMOOTH_TRIANGLE_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    hasReferenceBase = false;
    hasInlineBase = false;
    hasColour = false;
    hasTextureChain = false;
    inverted = false;
    transformCount = 0;
}

AntlrIrQuadricNode::AntlrIrQuadricNode()
{
    kind = ANTLR_IR_QUADRIC_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    hasReferenceBase = false;
    hasInlineBase = false;
    objectConstant = 0.0;
    hasColour = false;
    hasTextureChain = false;
    inverted = false;
    transformCount = 0;
}

AntlrIrBlobNode::AntlrIrBlobNode()
{
    kind = ANTLR_IR_BLOB_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    hasReferenceBase = false;
    hasInlineBase = false;
    hasThreshold = false;
    threshold = 1.0;
    componentCount = 0;
    sturm = false;
    hasColour = false;
    hasTextureChain = false;
    inverted = false;
    transformCount = 0;
}

AntlrIrBicubicPatchNode::AntlrIrBicubicPatchNode()
{
    kind = ANTLR_IR_UNKNOWN_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    hasInlineBase = false;
    patchType = 0;
    flatnessValue = 0.1;
    uSteps = 0;
    vSteps = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            controlPoints[i][j].x = 0.0;
            controlPoints[i][j].y = 0.0;
            controlPoints[i][j].z = 0.0;
        }
    }
    hasColour = false;
    hasTextureChain = false;
    inverted = false;
    transformCount = 0;
}

AntlrIrQuarticNode::AntlrIrQuarticNode()
{
    kind = ANTLR_IR_QUARTIC_NODE;
    sourceLine = -1;
    sourceColumn = -1;
    sourceFile = nullptr;
    hasReferenceBase = false;
    hasInlineBase = false;
    coefficientCount = 0;
    for (int i = 0; i < MAX_COEFFICIENTS; ++i) {
        coefficients[i] = 0.0;
    }
    sturm = false;
    hasColour = false;
    hasTextureChain = false;
    inverted = false;
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
    childPlaneCount = 0;
    for (int i = 0; i < MAX_CHILD_PLANES; ++i) {
        childPlanes[i] = nullptr;
    }
    childBoxCount = 0;
    for (int i = 0; i < MAX_CHILD_BOXES; ++i) {
        childBoxes[i] = nullptr;
    }
    childTriangleCount = 0;
    for (int i = 0; i < MAX_CHILD_TRIANGLES; ++i) {
        childTriangles[i] = nullptr;
    }
    childSmoothTriangleCount = 0;
    for (int i = 0; i < MAX_CHILD_SMOOTH_TRIANGLES; ++i) {
        childSmoothTriangles[i] = nullptr;
    }
    childQuadricCount = 0;
    for (int i = 0; i < MAX_CHILD_QUADRICS; ++i) {
        childQuadrics[i] = nullptr;
    }
    childQuarticCount = 0;
    for (int i = 0; i < MAX_CHILD_QUARTICS; ++i) {
        childQuartics[i] = nullptr;
    }
    childBlobCount = 0;
    for (int i = 0; i < MAX_CHILD_BLOBS; ++i) {
        childBlobs[i] = nullptr;
    }
    childBicubicPatchCount = 0;
    for (int i = 0; i < MAX_CHILD_BICUBIC_PATCHES; ++i) {
        childBicubicPatches[i] = nullptr;
    }
    childLightCount = 0;
    for (int i = 0; i < MAX_CHILD_LIGHTS; ++i) {
        childLights[i] = nullptr;
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
    boundedSphereCount = 0;
    for (int i = 0; i < MAX_BOUNDED_SPHERES; ++i) {
        boundedSpheres[i] = nullptr;
    }
    boundedCsgCount = 0;
    for (int i = 0; i < MAX_BOUNDED_CSGS; ++i) {
        boundedCsgs[i] = nullptr;
    }
    boundedReferenceCount = 0;
    clippedSphereCount = 0;
    for (int i = 0; i < MAX_CLIPPED_SPHERES; ++i) {
        clippedSpheres[i] = nullptr;
    }
    clippedCsgCount = 0;
    for (int i = 0; i < MAX_CLIPPED_CSGS; ++i) {
        clippedCsgs[i] = nullptr;
    }
    clippedReferenceCount = 0;
    hasColour = false;
    hasTextureChain = false;
    noShadow = false;
    inverted = false;
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
    boundedSphereCount = 0;
    for (int i = 0; i < MAX_BOUNDED_SPHERES; ++i) {
        boundedSpheres[i] = nullptr;
    }
    boundedCsgCount = 0;
    for (int i = 0; i < MAX_BOUNDED_CSGS; ++i) {
        boundedCsgs[i] = nullptr;
    }
    boundedReferenceCount = 0;
    clippedSphereCount = 0;
    for (int i = 0; i < MAX_CLIPPED_SPHERES; ++i) {
        clippedSpheres[i] = nullptr;
    }
    clippedCsgCount = 0;
    for (int i = 0; i < MAX_CLIPPED_CSGS; ++i) {
        clippedCsgs[i] = nullptr;
    }
    clippedReferenceCount = 0;
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
    childPlaneCount = 0;
    for (int i = 0; i < MAX_CHILD_PLANES; ++i) {
        childPlanes[i] = nullptr;
    }
    childBoxCount = 0;
    for (int i = 0; i < MAX_CHILD_BOXES; ++i) {
        childBoxes[i] = nullptr;
    }
    childTriangleCount = 0;
    for (int i = 0; i < MAX_CHILD_TRIANGLES; ++i) {
        childTriangles[i] = nullptr;
    }
    childSmoothTriangleCount = 0;
    for (int i = 0; i < MAX_CHILD_SMOOTH_TRIANGLES; ++i) {
        childSmoothTriangles[i] = nullptr;
    }
    childQuadricCount = 0;
    for (int i = 0; i < MAX_CHILD_QUADRICS; ++i) {
        childQuadrics[i] = nullptr;
    }
    childQuarticCount = 0;
    for (int i = 0; i < MAX_CHILD_QUARTICS; ++i) {
        childQuartics[i] = nullptr;
    }
    childBlobCount = 0;
    for (int i = 0; i < MAX_CHILD_BLOBS; ++i) {
        childBlobs[i] = nullptr;
    }
    childLightCount = 0;
    for (int i = 0; i < MAX_CHILD_LIGHTS; ++i) {
        childLights[i] = nullptr;
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
        if (declareNode->planeValue != nullptr) {
            delete declareNode->planeValue;
            declareNode->planeValue = nullptr;
        }
        if (declareNode->boxValue != nullptr) {
            delete declareNode->boxValue;
            declareNode->boxValue = nullptr;
        }
        if (declareNode->triangleValue != nullptr) {
            delete declareNode->triangleValue;
            declareNode->triangleValue = nullptr;
        }
        if (declareNode->smoothTriangleValue != nullptr) {
            delete declareNode->smoothTriangleValue;
            declareNode->smoothTriangleValue = nullptr;
        }
        if (declareNode->quadricValue != nullptr) {
            delete declareNode->quadricValue;
            declareNode->quadricValue = nullptr;
        }
        if (declareNode->quarticValue != nullptr) {
            delete declareNode->quarticValue;
            declareNode->quarticValue = nullptr;
        }
        if (declareNode->blobValue != nullptr) {
            delete declareNode->blobValue;
            declareNode->blobValue = nullptr;
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
        if (declareNode->cameraValue != nullptr) {
            delete declareNode->cameraValue;
            declareNode->cameraValue = nullptr;
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
