#ifndef __POV_ANTLR_SCENE_LOWERING_H__
#define __POV_ANTLR_SCENE_LOWERING_H__

class AntlrSceneIrProgram;
class RenderFrame;

class AntlrSceneLowering {
  public:
    static void applyProgram(const AntlrSceneIrProgram &program, RenderFrame *framePtr);
};

#endif
