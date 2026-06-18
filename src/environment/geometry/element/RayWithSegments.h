#ifndef __RAY_WITH_SEGMENTS__
#define __RAY_WITH_SEGMENTS__

#include "environment/material/Material.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"

class RayWithSegments : public Ray {
  public:
    static constexpr int MAX_CONTAINING_OBJECTS = 10;

    RayWithSegments();

    using Ray::setDirection;
    using Ray::setOrigin;
    using Ray::setOriginAndDirection;
    using Ray::setT;

    Vector3Dd &getPosition2() { return position2; }
    const Vector3Dd &getPosition2() const { return position2; }
    Vector3Dd &getDirection2() { return direction2; }
    const Vector3Dd &getDirection2() const { return direction2; }
    Vector3Dd &getPositionDirection() { return positionDirection; }
    const Vector3Dd &getPositionDirection() const { return positionDirection; }
    Vector3Dd &getMixedPositionPosition() { return mixedPositionPosition; }
    const Vector3Dd &getMixedPositionPosition() const { return mixedPositionPosition; }
    Vector3Dd &getMixedDirectionDirection() { return mixedDirectionDirection; }
    const Vector3Dd &getMixedDirectionDirection() const { return mixedDirectionDirection; }
    Vector3Dd &getMixedPositionDirection() { return mixedPositionDirection; }
    const Vector3Dd &getMixedPositionDirection() const { return mixedPositionDirection; }
    int getContainingIndex() const { return containingIndex; }
    bool areQuadricConstantsCached() const { return quadricConstantsCached; }
    void setQuadricConstantsCached(bool value) { quadricConstantsCached = value; }
    bool isShadowRayEnabled() const { return isShadowRay; }
    void setShadowRay(bool value) { isShadowRay = value; }
    bool isPrimaryRayEnabled() const { return isPrimaryRay; }
    void setPrimaryRay(bool value) { isPrimaryRay = value; }
    Material *getContainingTextureAt(int index) const { return containingTextures[index]; }
    double getContainingIORAt(int index) const { return containingIORs[index]; }

    void makeRay();
    void initializeContainers();
    void copyContainersFrom(const RayWithSegments *sourceRay);
    void enterContainingMedium(Material *texture);
    void exitContainingMedium();
    static inline void mixVectorTerms(
        Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c);

  private:
    Vector3Dd position2;               // Xo^2  Yo^2  Zo^2
    Vector3Dd direction2;              // Xv^2  Yv^2  Zv^2
    Vector3Dd positionDirection;       // XoXv  YoYv  ZoZv
    Vector3Dd mixedPositionPosition;   // XoYo  XoZo  YoZo
    Vector3Dd mixedDirectionDirection; // XvYv  XvZv  YvZv
    Vector3Dd mixedPositionDirection;  // XoYv+XvYo  XoZv+XvZo  YoZv+YvZo
    int containingIndex;
    Material *containingTextures[MAX_CONTAINING_OBJECTS];
    double containingIORs[MAX_CONTAINING_OBJECTS];
    bool quadricConstantsCached;
    bool isShadowRay;
    bool isPrimaryRay;
};

#endif
