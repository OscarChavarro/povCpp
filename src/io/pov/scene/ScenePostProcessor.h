#ifndef __SCENE_POST_PROCESSOR__
#define __SCENE_POST_PROCESSOR__

#include "environment/scene/SimpleBody.h"
#include "environment/light/Light.h"
#include "java/util/ArrayList.h"

class ScenePostProcessor {
  public:
    static void linkLights(SimpleBody *object, java::ArrayList<Light*> &lights);

  private:
    static void applyTransformsToLight(
        Light *light, const java::ArrayList<const Matrix4x4d*> &transforms);
    static void linkLights(
        SimpleBody *object,
        java::ArrayList<Light*> &lights,
        java::ArrayList<const Matrix4x4d*> &transforms);
    static void linkLightsInOperand(
        CsgOperand *operand,
        java::ArrayList<Light*> &lights,
        java::ArrayList<const Matrix4x4d*> &transforms);
    static void linkLightsInShape(
        Geometry *shape,
        java::ArrayList<Light*> &lights,
        java::ArrayList<const Matrix4x4d*> &transforms);
};

#endif
