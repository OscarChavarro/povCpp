#ifndef __SCENE_CONFIG_PARSER_H__
#define __SCENE_CONFIG_PARSER_H__

#include "io/pov/ParserConstants.h"

class Camera;

class SceneConfigParser {
  public:
    static void parseCamera(Camera *givenVp);
    static void parseFog();
    static void parseDeclare();
    static CONSTANT findConstant();
};

#endif
