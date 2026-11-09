#ifndef __LIGHT_GIZMO_OMNI_BILLBOARD__
#define __LIGHT_GIZMO_OMNI_BILLBOARD__

class Calligraphic2DBuffer;
class Camera;
class Vector3Dd;

class LightGizmoOmniBillboard {
public:
    /**
    Calculates half the size of the gizmo in world units, which is also the
    world radius of its outermost rays. The gizmo keeps a constant apparent
    size in the viewport, so this depends on the distance to the camera.
    This value is shared by the gizmo drawing and by the picking of lights.
    @param camera camera that views the gizmo, or null
    @param position position of the light, in world coordinates
    @param viewportWidth width of the viewport in pixels
    @param viewportHeight height of the viewport in pixels
    @return half the size of the gizmo, in world units
    */
    static double calculateWorldHalfSize(const Camera* camera,
                                         const Vector3Dd& position,
                                         int viewportWidth,
                                         int viewportHeight);

    static Calligraphic2DBuffer createLinePattern();
};

#endif
