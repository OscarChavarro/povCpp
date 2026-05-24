#ifndef __POV_AST_PARSED_SCENE_PROGRAM_H__
#define __POV_AST_PARSED_SCENE_PROGRAM_H__

#include "environment/scene/SceneFrame.h"

class AstScene;

class AstParsedSceneProgram {
  public:
    AstParsedSceneProgram();

    AstScene *scene;
    RenderFrame legacyFrame;
    bool hasCamera;
    bool hasFog;
};

#endif
