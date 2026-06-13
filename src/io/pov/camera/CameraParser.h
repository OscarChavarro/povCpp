#ifndef __CAMERA_PARSER_H__
#define __CAMERA_PARSER_H__

#include "environment/camera/Camera.h"
#include "io/pov/context/ParserContext.h"

class CameraParser {
  public:
    static void parseCamera(Camera *givenVp);
    static void parseCamera(Camera *givenVp, ParserContext &ctx);
};

#endif
