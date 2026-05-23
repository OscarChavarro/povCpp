#ifndef __SCENE_CONFIG_PARSER_H__
#define __SCENE_CONFIG_PARSER_H__

#include "io/pov/ParserConstants.h"

class Camera;
class ParserContext;

class SceneConfigParser {
  public:
    static void parseCamera(Camera *givenVp);
    static void parseCamera(Camera *givenVp, ParserContext &ctx);
    static void parseFog();
    static void parseFog(ParserContext &ctx);
    static void parseDeclare();
    static void parseDeclare(ParserContext &ctx);
    static CONSTANT findConstant();
    static CONSTANT findConstant(ParserContext &ctx);
};

#endif
