#ifndef __RAY_H__
#define __RAY_H__

#include "common/linealAlgebra/Vector3Dd.h"
#include "media/Texture.h"

static constexpr int MAX_CONTAINING_OBJECTS = 10;

class Ray {
  public:
    Vector3Dd Initial;               /*  Xo  Yo  Zo  */
    Vector3Dd Direction;             /*  Xv  Yv  Zv  */
    Vector3Dd Initial_2;             /*  Xo^2  Yo^2  Zo^2  */
    Vector3Dd Direction_2;           /*  Xv^2  Yv^2  Zv^2  */
    Vector3Dd Initial_Direction;     /*  XoXv  YoYv  ZoZv  */
    Vector3Dd Mixed_Initial_Initial; /*  XoYo  XoZo  YoZo  */
    Vector3Dd Mixed_Dir_Dir;         /*  XvYv  XvZv  YvZv  */
    Vector3Dd Mixed_Init_Dir;        /*  XoYv+XvYo  XoZv+XvZo  YoZv+YvZo  */
    int Containing_Index;
    Texture *Containing_Textures[MAX_CONTAINING_OBJECTS];
    double Containing_IORs[MAX_CONTAINING_OBJECTS];
    int Quadric_Constants_Cached;

    void makeRay();
    void initializeContainers();
    void copyContainersFrom(Ray *sourceRay);
    void enterContainingMedium(Texture *texture);
    void exitContainingMedium();
    static inline void mixVectorTerms(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c);
};

#endif
