#ifndef __RAY_H__
#define __RAY_H__

#include "common/Vector.h"
#include "media/Texture.h"

static constexpr int MAX_CONTAINING_OBJECTS = 10;

class Ray {
  public:
    Vector3D Initial;               /*  Xo  Yo  Zo  */
    Vector3D Direction;             /*  Xv  Yv  Zv  */
    Vector3D Initial_2;             /*  Xo^2  Yo^2  Zo^2  */
    Vector3D Direction_2;           /*  Xv^2  Yv^2  Zv^2  */
    Vector3D Initial_Direction;     /*  XoXv  YoYv  ZoZv  */
    Vector3D Mixed_Initial_Initial; /*  XoYo  XoZo  YoZo  */
    Vector3D Mixed_Dir_Dir;         /*  XvYv  XvZv  YvZv  */
    Vector3D Mixed_Init_Dir;        /*  XoYv+XvYo  XoZv+XvZo  YoZv+YvZo  */
    int Containing_Index;
    Texture *Containing_Textures[MAX_CONTAINING_OBJECTS];
    DBL Containing_IORs[MAX_CONTAINING_OBJECTS];
    int Quadric_Constants_Cached;

    void makeRay();
    void initializeContainers();
    void copyContainersFrom(Ray *sourceRay);
    void enterContainingMedium(Texture *texture);
    void exitContainingMedium();
};

#endif
