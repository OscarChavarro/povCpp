#ifndef __POV_ANTLR_SCENE_RUNTIME_PIPELINE_H__
#define __POV_ANTLR_SCENE_RUNTIME_PIPELINE_H__

#include <string>

class RenderFrame;

class AntlrSceneRuntimePipeline {
  public:
    static bool parseAndApply(RenderFrame *framePtr, std::string &error);
};

#endif
