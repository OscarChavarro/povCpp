#ifndef __IMAGE_DATA_H__
#define __IMAGE_DATA_H__

class ImageLine {
  public:
    unsigned char *red;
    unsigned char *green;
    unsigned char *blue;
};

class ImageData {
  public:
    ImageLine *rgb_lines;
    unsigned char **map_lines;
};

#endif
