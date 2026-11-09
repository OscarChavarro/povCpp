#ifndef __CUBEMAP_BACKGROUND__
#define __CUBEMAP_BACKGROUND__

#include "vsdk/toolkit/environment/background/Background.h"

class Box;
class Camera;
class RGBAImageUncompressed;

/**
Background defined by the six faces of a cube map. Images and camera are
referenced, not owned.
*/
class CubemapBackground : public Background {
private:
    RGBAImageUncompressed* images[6];
    Camera* camera;
    Box* boundingCube;

    int classifyPlane(const Vector3Dd& normal) const;

    CubemapBackground(const CubemapBackground& other);
    CubemapBackground& operator=(const CubemapBackground& other);

public:
    CubemapBackground(Camera* camera,
                      RGBAImageUncompressed* front,
                      RGBAImageUncompressed* right,
                      RGBAImageUncompressed* back,
                      RGBAImageUncompressed* left,
                      RGBAImageUncompressed* down,
                      RGBAImageUncompressed* up);
    virtual ~CubemapBackground();

    /**
    @param d viewing direction
    @return color as viewed in given direction
    */
    virtual ColorRgb colorInDireccion(const Vector3Dd& d) override;

    /**
    @return array of 6 images: front, right, back, left, down and up
    */
    RGBAImageUncompressed* const* getImages() const;
    Camera* getCamera() const;
    void setCamera(Camera* camera);
};

#endif
