#ifndef __RAY_WITH_SEGMENTS_H__
#define __RAY_WITH_SEGMENTS_H__

#include "environment/material/Material.h"
#include "environment/geometry/elements/Ray.h"

class RayWithSegments : public Ray {
  public:
    static constexpr int MAX_CONTAINING_OBJECTS = 10;

    RayWithSegments();
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

    void makeRay();
    void initializeContainers();
    void copyContainersFrom(RayWithSegments *sourceRay);
    void enterContainingMedium(Material *texture);
    void exitContainingMedium();
    static inline void mixVectorTerms(
        Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c);
};

#endif
