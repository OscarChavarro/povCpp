#include <cmath>

#include "vsdk/toolkit/environment/background/CubemapBackground.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/media/RGBAImageUncompressed.h"

CubemapBackground::CubemapBackground(Camera* camera,
                                     RGBAImageUncompressed* front,
                                     RGBAImageUncompressed* right,
                                     RGBAImageUncompressed* back,
                                     RGBAImageUncompressed* left,
                                     RGBAImageUncompressed* down,
                                     RGBAImageUncompressed* up)
    : camera(camera)
{
    images[0] = front;
    images[1] = right;
    images[2] = back;
    images[3] = left;
    images[4] = down;
    images[5] = up;
    boundingCube = new Box(1, 1, 1);
}

CubemapBackground::~CubemapBackground()
{
    delete boundingCube;
}

ColorRgb CubemapBackground::colorInDireccion(const Vector3Dd& direction)
{
    double u;
    double v;
    RGBAImageUncompressed* img;

    Vector3Dd d = direction.normalized();
    Ray r(Vector3Dd(0, 0, 0), d);
    RayHit hit;
    if ( !boundingCube->doIntersectionFirstHit(r, &hit) ) {
        return ColorRgb();
    }
    int plane = classifyPlane(hit.normal);

    u = 1 - hit.u;
    v = 1 - hit.v;
    switch ( plane ) {
      case 1: // Top
        img = images[5];
        u = 1 - hit.v;
        v = hit.u;
        break;
      case 2: // Down
        img = images[4];
        u = hit.v;
        v = 1 - hit.u;
        break;
      case 3: // Front
        img = images[0];
        break;
      case 4: // Back
        img = images[2];
        break;
      case 5: // Right
        img = images[1];
        break;
      default: // Left
        img = images[3];
        break;
    }

    if ( img == nullptr ) {
        return ColorRgb();
    }
    ColorRgb* sample = img->getColorRgbBiLinear(u, v);
    ColorRgb color(*sample);
    delete sample;
    return color;
}

int CubemapBackground::classifyPlane(const Vector3Dd& normal) const
{
    double ax = std::fabs(normal.x());
    double ay = std::fabs(normal.y());
    double az = std::fabs(normal.z());

    if ( az >= ax && az >= ay ) {
        return normal.z() >= 0 ? 1 : 2;
    }
    if ( ay >= ax ) {
        return normal.y() >= 0 ? 3 : 4;
    }
    return normal.x() >= 0 ? 5 : 6;
}

RGBAImageUncompressed* const* CubemapBackground::getImages() const
{
    return images;
}

Camera* CubemapBackground::getCamera() const
{
    return camera;
}

void CubemapBackground::setCamera(Camera* camera)
{
    this->camera = camera;
}
