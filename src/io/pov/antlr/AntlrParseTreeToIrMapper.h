#ifndef __POV_ANTLR_PARSE_TREE_TO_IR_MAPPER_H__
#define __POV_ANTLR_PARSE_TREE_TO_IR_MAPPER_H__

class ParserContext;
class AntlrParsedSceneProgram;

#ifdef POV_WITH_ANTLR_RUNTIME
#include "POVParser.h"

class AntlrParseTreeToIrMapper {
  public:
    static AntlrParsedSceneProgram *mapScene(POVParser::SceneContext *sceneCtx);
};
#endif

#endif
