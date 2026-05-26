#ifndef __POV_ANTLR_SCENE_RUNTIME_PIPELINE_H__
#define __POV_ANTLR_SCENE_RUNTIME_PIPELINE_H__

#include <string>

class ParserContext;
class RenderFrame;

class AntlrSceneRuntimePipeline {
  public:
    static bool parseAndApply(RenderFrame *framePtr, ParserContext &ctx, std::string &error);
};

#endif
