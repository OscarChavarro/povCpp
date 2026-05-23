#ifndef __CAMERA_PARSER_H__
#define __CAMERA_PARSER_H__

class Camera;
class ParserContext;

class CameraParser {
  public:
    static void parseCamera(Camera *givenVp);
    static void parseCamera(Camera *givenVp, ParserContext &ctx);
};

#endif
