#ifndef __IMAGE_DATA_H__
#define __IMAGE_DATA_H__

#include "media/ImageLine.h"

class ImageData {
  public:
    ImageLine *lines;
    unsigned char **mapLines;
};

#endif
