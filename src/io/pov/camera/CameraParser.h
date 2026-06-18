#ifndef __CAMERA_PARSER__
#define __CAMERA_PARSER__

#include "environment/camera/Camera.h"
#include "io/pov/context/ParserContext.h"

class CameraParser {
  public:
    static Camera parseCamera();
    static Camera parseCamera(ParserContext &ctx);
    static void parseCamera(Camera *givenVp);
    static void parseCamera(Camera *givenVp, ParserContext &ctx);
};

#endif
