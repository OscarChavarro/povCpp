#ifndef __HEIGHT_FIELD__
#define __HEIGHT_FIELD__

#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"
#include "vsdk/toolkit/environment/geometry/volume/HeightFieldSolid.h"
#include "vsdk/toolkit/environment/raycasting/geometry/RayCastingGeometry.h"

// PovRay height_field: a VITRAL HeightFieldSolid placed by an affine map. The
// PovRay image rules (border and water level points at -10000, POT and TGA
// fractional heights) are applied when the heights are loaded.
class HeightField : public RayCastingGeometry {
  public:
    static constexpr int GIF = 0;
    static constexpr int POT = 1;
    static constexpr int TGA = 2;
    // Height of points on the image border or at or below the water level.
    static constexpr double HIDDEN_HEIGHT = -10000.0;

    HeightField(const Matrix4x4d &transformation,
        const Matrix4x4d &transformationInverse, const Vector3Dd &minBounds,
        const Vector3Dd &maxBounds);
    HeightField(const HeightField &other);
    ~HeightField() override;

    void setWaterLevel(double level);
    const HeightFieldSolid *getSolid() const { return solid; }

    static void findHfMinMax(HeightField *hField,
        const IndexedColorImageHDRUncompressed *image, int imageType);
    static void findHfMinMax(HeightField *hField,
        const RGBAImageHDRUncompressed *image, int imageType);

    BoundingVolumeHierarchy *createBoundingVolume() const override;

    int doIntersectionForAllRayCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr) override;
    int doIntersectionForAllRayCrossingsAnnotated(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const GeometryIntersectionEmissionContext &context) override;
    bool hasNativeAnnotatedCrossings() const override { return true; }
    int doContainmentTest(const Vector3Dd &point, double distanceTolerance) override;
    void *copy() override;
    void invertGeometry() override;

  protected:
    void computeSurfaceNormal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;

  private:
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;
    Vector3Dd minBounds;
    Vector3Dd maxBounds;
    HeightFieldSolid *solid;

    double imageHeight(double height) const;
    void allocateGrid(int maxX, int maxZ);
    int traceCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride,
        const GeometryIntersectionEmissionContext *context);
};

#endif
