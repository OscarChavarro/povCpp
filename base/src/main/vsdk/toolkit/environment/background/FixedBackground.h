#ifndef __FIXED_BACKGROUND__
#define __FIXED_BACKGROUND__

#include "vsdk/toolkit/environment/background/Background.h"

class Camera;
class RGBAImageUncompressed;

/**
Background defined by a fixed image. Image and camera are referenced, not
owned.
*/
class FixedBackground : public Background {
private:
    RGBAImageUncompressed* image;
    Camera* camera;

public:
    FixedBackground(Camera* camera, RGBAImageUncompressed* image);
    virtual ~FixedBackground() {}

    void setImage(RGBAImageUncompressed* image);
    RGBAImageUncompressed* getImage() const;

    /**
    BUG: not working math! (as in the Java version, a black color is
    returned instead of the null color of Java).
    @param d viewing direction
    @return color as viewed in given direction
    */
    virtual ColorRgb colorInDireccion(const Vector3Dd& d) override;

    Camera* getCamera() const;
};

#endif
