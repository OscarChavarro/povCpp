#ifndef __SCENE_POST_PROCESSOR__
#define __SCENE_POST_PROCESSOR__


class ScenePostProcessor {
  public:
    static void linkLights(PovrayBody *object, java::ArrayList<Light*> &lights);

  private:
    static void applyTransformsToLight(
        Light *light, const java::ArrayList<const Matrix4x4d*> &transforms);
    static void linkLights(
        PovrayBody *object,
        java::ArrayList<Light*> &lights,
        java::ArrayList<const Matrix4x4d*> &transforms);
    static void linkLightsInOperand(
        CsgOperand *operand,
        java::ArrayList<Light*> &lights,
        java::ArrayList<const Matrix4x4d*> &transforms);
    // Links the lights of a body's CSG operands or light geometry.
    static void linkLightsInShape(
        PovrayBody *body,
        java::ArrayList<Light*> &lights,
        java::ArrayList<const Matrix4x4d*> &transforms);
};

#endif
