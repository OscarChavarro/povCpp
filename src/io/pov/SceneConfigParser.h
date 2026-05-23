#ifndef __SCENE_CONFIG_PARSER_H__
#define __SCENE_CONFIG_PARSER_H__

class Camera;

typedef int CONSTANT;

class SceneConfigParser {
  public:
    static void parseCamera(Camera *givenVp);
    static void parseFog();
    static void parseDeclare();
    static CONSTANT findConstant();
};

#endif
