#ifndef __RAY_H__
#define __RAY_H__

#include "common/linealAlgebra/Vector3Dd.h"
#include "media/Texture.h"

static constexpr int MAX_CONTAINING_OBJECTS = 10;

class Ray {
  public:
    Vector3Dd position;               /*  Xo  Yo  Zo  */
    Vector3Dd direction;              /*  Xv  Yv  Zv  */
    Vector3Dd position2;              /*  Xo^2  Yo^2  Zo^2  */
    Vector3Dd direction2;             /*  Xv^2  Yv^2  Zv^2  */
    Vector3Dd positionDirection;      /*  XoXv  YoYv  ZoZv  */
    Vector3Dd mixedPositionPosition;  /*  XoYo  XoZo  YoZo  */
    Vector3Dd mixedDirectionDirection; /*  XvYv  XvZv  YvZv  */
    Vector3Dd mixedPositionDirection; /*  XoYv+XvYo  XoZv+XvZo  YoZv+YvZo  */
    int containingIndex;
    Texture *containingTextures[MAX_CONTAINING_OBJECTS];
    double containingIORs[MAX_CONTAINING_OBJECTS];
    int quadricConstantsCached;

    void makeRay();
    void initializeContainers();
    void copyContainersFrom(Ray *sourceRay);
    void enterContainingMedium(Texture *texture);
    void exitContainingMedium();
    static inline void mixVectorTerms(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c);
};

#endif
