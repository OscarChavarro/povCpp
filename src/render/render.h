#ifndef __RENDER_H__
#define __RENDER_H__

#include "common/frame.h"
#include "common/povproto.h"
#include "common/vector.h"
#include "geom/light.h"
#include "geom/viewpnt.h"

class Frame {
  public:
    Viewpoint View_Point;
    int Screen_Height, Screen_Width;
    Light *Light_Sources;
    SimpleBody *Objects;
    DBL Atmosphere_IOR, Antialias_Threshold;
    DBL Fog_Distance;
    RGBAColor Fog_Colour;
};

extern void Read_Rendered_Part(void);
extern void Supersample(RGBAColor *result, int x, int y, int Width, int Height);
extern void Start_Tracing(void);
extern void Trace(Ray *ray, RGBAColor *Colour);
extern void Initialize_Renderer(void);

#endif
