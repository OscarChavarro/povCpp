#ifndef __AUTO_STEREOGRAM_GENERATOR__
#define __AUTO_STEREOGRAM_GENERATOR__

#include "vsdk/toolkit/render/RenderingElement.h"
class RGBImageUncompressed;
class ZBuffer;

class AutoStereogramGenerator : public RenderingElement {
public:
    static void generate(RGBImageUncompressed* result, RGBImageUncompressed* tilePattern, ZBuffer* depthMap);
};

#endif
