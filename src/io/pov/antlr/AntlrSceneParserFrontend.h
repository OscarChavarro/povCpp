#ifndef __POV_ANTLR_SCENE_PARSER_FRONTEND_H__
#define __POV_ANTLR_SCENE_PARSER_FRONTEND_H__

#include "io/pov/antlr/AntlrSceneIr.h"

class AntlrParsedSceneProgram;

class AntlrSceneParserFrontend {
  public:
    static AntlrParsedSceneProgram *parseProgram();
    static bool appendMaxTraceLevel(AntlrSceneIrProgram &program, double value,
        int sourceLine = -1, int sourceColumn = -1, const char *sourceFile = nullptr);
    static AntlrIrFogNode *beginFogNode(
        int sourceLine = -1, int sourceColumn = -1, const char *sourceFile = nullptr);
    static bool appendFogNode(AntlrSceneIrProgram &program, AntlrIrFogNode *fog);
    static AntlrIrCameraNode *beginCameraNode(
        int sourceLine = -1, int sourceColumn = -1, const char *sourceFile = nullptr);
    static bool appendCameraOp(AntlrIrCameraNode &camera, AntlrIrCameraOpKind kind,
        int referenceIdentifierNumber, const AntlrIrVector3 &vectorValue);
    static bool appendCameraNode(AntlrSceneIrProgram &program, AntlrIrCameraNode *camera);
    static bool appendDeclareNode(AntlrSceneIrProgram &program, AntlrIrDeclareNode *declareNode);
    static bool appendDefaultTextureNode(
        AntlrSceneIrProgram &program, AntlrIrDefaultTextureNode *defaultTextureNode);
    static bool appendSphereNode(AntlrSceneIrProgram &program, AntlrIrSphereNode *sphereNode);
    static bool appendObjectNode(AntlrSceneIrProgram &program, AntlrIrObjectNode *objectNode);
    static bool appendCompositeNode(
        AntlrSceneIrProgram &program, AntlrIrCompositeNode *compositeNode);
    static bool appendLightNode(AntlrSceneIrProgram &program, AntlrIrLightNode *lightNode);
    static bool appendCsgNode(AntlrSceneIrProgram &program, AntlrIrCsgNode *csgNode);
};

#endif
