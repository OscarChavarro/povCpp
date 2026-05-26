#include "io/pov/antlr/AntlrSceneParserFrontend.h"

#include <stdexcept>

#include "io/pov/antlr/AntlrParsedSceneProgram.h"
#include "io/pov/antlr/AntlrSceneIrBuilder.h"

AntlrParsedSceneProgram *
AntlrSceneParserFrontend::parseProgram()
{
    AntlrParsedSceneProgram *parsed = new AntlrParsedSceneProgram();
    parsed->program = AntlrSceneIrBuilder::buildEmptyProgram();
    return parsed;
}

bool
AntlrSceneParserFrontend::appendMaxTraceLevel(AntlrSceneIrProgram &program, double value,
    int sourceLine, int sourceColumn, const char *sourceFile)
{
    AntlrIrMaxTraceLevelNode *node = new AntlrIrMaxTraceLevelNode();
    node->value = value;
    node->sourceLine = sourceLine;
    node->sourceColumn = sourceColumn;
    node->sourceFile = sourceFile;
    if (!AntlrSceneIrNodes::appendNode(program, node)) {
        delete node;
        throw std::runtime_error("Too many ANTLR IR scene nodes");
    }
    return true;
}

AntlrIrFogNode *
AntlrSceneParserFrontend::beginFogNode(int sourceLine, int sourceColumn, const char *sourceFile)
{
    AntlrIrFogNode *node = new AntlrIrFogNode();
    node->sourceLine = sourceLine;
    node->sourceColumn = sourceColumn;
    node->sourceFile = sourceFile;
    return node;
}

bool
AntlrSceneParserFrontend::appendFogNode(AntlrSceneIrProgram &program, AntlrIrFogNode *fog)
{
    if (fog == nullptr) {
        throw std::runtime_error("Invalid ANTLR IR fog node");
    }
    if (!AntlrSceneIrNodes::appendNode(program, fog)) {
        delete fog;
        throw std::runtime_error("Too many ANTLR IR scene nodes");
    }
    return true;
}

AntlrIrCameraNode *
AntlrSceneParserFrontend::beginCameraNode(int sourceLine, int sourceColumn, const char *sourceFile)
{
    AntlrIrCameraNode *node = new AntlrIrCameraNode();
    node->sourceLine = sourceLine;
    node->sourceColumn = sourceColumn;
    node->sourceFile = sourceFile;
    return node;
}

bool
AntlrSceneParserFrontend::appendCameraOp(AntlrIrCameraNode &camera, AntlrIrCameraOpKind kind,
    int referenceIdentifierNumber, const AntlrIrVector3 &vectorValue)
{
    AntlrIrCameraOp op;
    op.kind = kind;
    op.referenceIdentifierNumber = referenceIdentifierNumber;
    op.vectorValue = vectorValue;
    if (!AntlrSceneIrNodes::appendCameraOp(camera, op)) {
        throw std::runtime_error("Too many ANTLR IR camera operations");
    }
    return true;
}

bool
AntlrSceneParserFrontend::appendCameraNode(AntlrSceneIrProgram &program, AntlrIrCameraNode *camera)
{
    if (camera == nullptr) {
        throw std::runtime_error("Invalid ANTLR IR camera node");
    }
    if (!AntlrSceneIrNodes::appendNode(program, camera)) {
        delete camera;
        throw std::runtime_error("Too many ANTLR IR scene nodes");
    }
    return true;
}

bool
AntlrSceneParserFrontend::appendDeclareNode(
    AntlrSceneIrProgram &program, AntlrIrDeclareNode *declareNode)
{
    if (declareNode == nullptr) {
        throw std::runtime_error("Invalid ANTLR IR declare node");
    }
    if (!AntlrSceneIrNodes::appendNode(program, declareNode)) {
        delete declareNode;
        throw std::runtime_error("Too many ANTLR IR scene nodes");
    }
    return true;
}

bool
AntlrSceneParserFrontend::appendDefaultTextureNode(
    AntlrSceneIrProgram &program, AntlrIrDefaultTextureNode *defaultTextureNode)
{
    if (defaultTextureNode == nullptr) {
        throw std::runtime_error("Invalid ANTLR IR default texture node");
    }
    if (!AntlrSceneIrNodes::appendNode(program, defaultTextureNode)) {
        delete defaultTextureNode;
        throw std::runtime_error("Too many ANTLR IR scene nodes");
    }
    return true;
}

bool
AntlrSceneParserFrontend::appendSphereNode(AntlrSceneIrProgram &program, AntlrIrSphereNode *sphereNode)
{
    if (sphereNode == nullptr) {
        throw std::runtime_error("Invalid ANTLR IR sphere node");
    }
    if (!AntlrSceneIrNodes::appendNode(program, sphereNode)) {
        delete sphereNode;
        throw std::runtime_error("Too many ANTLR IR scene nodes");
    }
    return true;
}

bool
AntlrSceneParserFrontend::appendObjectNode(AntlrSceneIrProgram &program, AntlrIrObjectNode *objectNode)
{
    if (objectNode == nullptr) {
        throw std::runtime_error("Invalid ANTLR IR object node");
    }
    if (!AntlrSceneIrNodes::appendNode(program, objectNode)) {
        delete objectNode;
        throw std::runtime_error("Too many ANTLR IR scene nodes");
    }
    return true;
}

bool
AntlrSceneParserFrontend::appendCompositeNode(
    AntlrSceneIrProgram &program, AntlrIrCompositeNode *compositeNode)
{
    if (compositeNode == nullptr) {
        throw std::runtime_error("Invalid ANTLR IR composite node");
    }
    if (!AntlrSceneIrNodes::appendNode(program, compositeNode)) {
        delete compositeNode;
        throw std::runtime_error("Too many ANTLR IR scene nodes");
    }
    return true;
}
