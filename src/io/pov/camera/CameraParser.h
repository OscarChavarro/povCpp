#ifndef __CAMERA_PARSER__
#define __CAMERA_PARSER__

#include "io/pov/context/ParserContext.h"
#include "io/pov/camera/PovCameraSpec.h"
#include "vsdk/toolkit/environment/camera/CameraSnapshot.h"

class CameraParser {
  public:
    static CameraSnapshot parseCamera();
    static CameraSnapshot parseCamera(ParserContext &ctx);
    static void parseCamera(PovCameraSpec *givenVp);
    static void parseCamera(PovCameraSpec *givenVp, ParserContext &ctx);
};

#endif
