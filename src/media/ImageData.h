#ifndef __IMAGE_DATA_H__
#define __IMAGE_DATA_H__

class ImageLine {
  public:
    unsigned char *red, *green, *blue;
};

class ImageData {
  public:
    ImageLine *rgb_lines;
    unsigned char **map_lines;
};

#endif
