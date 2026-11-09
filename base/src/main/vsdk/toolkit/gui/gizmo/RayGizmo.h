#ifndef __RAY_GIZMO__
#define __RAY_GIZMO__

#include "java/util/function/Function.h"
#include <ctime>
#include <pthread.h>

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/Intersection.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/gui/gizmo/InputGizmo.h"
class Arrow;
class Sphere;
class SimpleBody;
class SimpleScene;
class SimpleMaterial;

class RayGizmo {
public:
    static const double DEFAULT_DISABLE_TIME;
    static const ColorRgb DEFAULT_RAY_COLOR;
    static const ColorRgb DEFAULT_NORMAL_COLOR;

    class RaySnapshot {
    public:
        double rotationAngleInRadians;
        java::ArrayList<Ray> rays;
        java::ArrayList<Intersection*> intersections;

        RaySnapshot();
        RaySnapshot(
            double rotationAngleInRadians,
            const java::ArrayList<Ray>& rays,
            const java::ArrayList<Intersection*>& intersections);
        RaySnapshot(const RaySnapshot& other);
        RaySnapshot& operator=(const RaySnapshot& other);
        ~RaySnapshot();
        void clear();
    };

    RayGizmo(java::Function<const Ray&, Intersection*> intersectionCallback, int maxNumOfReflections);
    ~RayGizmo();

    Vector3Dd getPosition() const;
    Vector3Dd getDirection() const;
    void setRay(const Ray& ray, double rotationAngleInRadians);
    int getMaxNumOfReflections() const;
    void setMaxNumOfReflections(int maxNumOfReflections);
    InputGizmo* getInputGizmo();
    void update();
    bool isVisible() const;
    void setVisible(bool visible);
    /// The roll marker is opt-in: most ray sources do not provide roll.
    bool isRollIndicatorEnabled() const;
    void setRollIndicatorEnabled(bool enabled);
    double getDisableAfterElapsedSeconds() const;
    void setDisableAfterElapsedSeconds(double disableAfterElapsedSeconds);
    std::time_t getLastDataTime() const;
    std::time_t getPreviousDataTime() const;
    RaySnapshot* acquireSnapshot();
    RaySnapshot* getCurrentSnapshot() const;
    double getRotationAngleInRadians() const;
    SimpleBody* getBody() const;
    Arrow* getArrow() const;
    SimpleScene* buildScene() const;

    ColorRgb getSourceRayColor() const;
    void setSourceRayColor(const ColorRgb& sourceRayColor);
    java::ArrayList<ColorRgb>& getNormalRayColors();
    void setNormalRayColors(const java::ArrayList<ColorRgb>& normalRayColors);
    java::ArrayList<ColorRgb>& getReflectedRayColors();
    void setReflectedRayColors(const java::ArrayList<ColorRgb>& reflectedRayColors);
    java::ArrayList<ColorRgb>& getRefractedRayColors();
    void setRefractedRayColors(const java::ArrayList<ColorRgb>& refractedRayColors);

private:
    static const double ARROW_BASE_LENGTH;
    static const double ARROW_HEAD_LENGTH;
    static const double ARROW_BASE_RADIUS;
    static const double ARROW_HEAD_RADIUS;
    // Colors of the boxes of the input gizmo: x, y, z, yaw and pitch
    static const int NUMBER_OF_INPUT_FIELDS = 5;
    static const ColorRgb INPUT_FIELD_COLORS[NUMBER_OF_INPUT_FIELDS];

    Arrow* arrow;
    Sphere* dotSphere;
    SimpleBody* body;
    java::Function<const Ray&, Intersection*> intersectionCallback;
    InputGizmo inputGizmo;
    int maxNumOfReflections;
    pthread_mutex_t pendingMutex;
    RaySnapshot* pendingSnapshot;

    Vector3Dd currentPosition;
    Vector3Dd currentDirection;
    double currentRotationAngleInRadians;
    RaySnapshot* currentSnapshot;

    std::time_t lastDataTime;
    std::time_t previousDataTime;
    bool visible;
    bool rollIndicatorEnabled;
    double disableAfterElapsedSeconds;

    ColorRgb sourceRayColor;
    java::ArrayList<ColorRgb> normalRayColors;
    java::ArrayList<ColorRgb> reflectedRayColors;
    java::ArrayList<ColorRgb> refractedRayColors;

    SimpleBody* buildRayBody(
        const Ray& ray, const Intersection* intersection, const ColorRgb& color,
        double arrowTotalLength) const;
    void addNormalBody(SimpleScene* scene, const Intersection* intersection, const ColorRgb& color) const;
    void addDotBodies(SimpleScene* scene, const Ray& ray, double arrowTotalLength) const;
    static SimpleBody* buildBody(Arrow* geometry, const Vector3Dd& position, const Matrix4x4d& rotation,
        const Vector3Dd& scale, const ColorRgb& color);
    static SimpleBody* buildDotBody(Sphere* geometry, const Vector3Dd& position, const ColorRgb& color);
    static SimpleBody* cloneBody(const SimpleBody* source);
    static SimpleMaterial* materialFromColor(const ColorRgb& color);
    void recordDataArrival();
    bool inactivityThresholdExceeded() const;
    void applyTransform(const Vector3Dd& position, const Vector3Dd& direction);
    void syncInputGizmo(const Vector3Dd& position, const Vector3Dd& direction);
    static Ray* computeReflectedRay(const Ray& incomingRay, const Intersection* intersection);
    static Matrix4x4d rotationFromZToDirection(const Vector3Dd& direction);
};

#endif
