#ifndef __RASTERTILEAREA__
#define __RASTERTILEAREA__

class Image;

class RasterTileArea {
private:
    Image* image;
    int x0;
    int y0;
    int dx;
    int dy;

public:
    RasterTileArea();
    RasterTileArea(Image* image, int x0, int y0, int dx, int dy);

    Image* getImage() const;
    int getX0() const;
    int getY0() const;
    int getDx() const;
    int getDy() const;
    int getX1() const;
    int getY1() const;
};

#endif
