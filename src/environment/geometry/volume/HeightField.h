#ifndef __HEIGHT_FIELD__
#define __HEIGHT_FIELD__

#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"
#include "vsdk/toolkit/environment/geometry/volume/HeightFieldSolid.h"
#include "environment/geometry/RayCastingGeometry.h"

// PovRay height_field: a VITRAL HeightFieldSolid in its grid coordinates. The
// body that holds it places the grid (the parser scales it to the unit square
// before the scene transforms). The PovRay image rules (border and water level
// points at -10000, POT and TGA fractional heights) are applied when the
// heights are loaded. PovRay ignores inverse on height fields, so typed
// queries report the solid as given.
class HeightField : public RayCastingGeometry {
  public:
    static constexpr int GIF = 0;
    static constexpr int POT = 1;
    static constexpr int TGA = 2;
    // Height of points on the image border or at or below the water level.
    static constexpr double HIDDEN_HEIGHT = -10000.0;

    // A grid of columns x rows heights at zero inside the given box.
    HeightField(long columns, long rows, const Vector3Dd &minBounds,
        const Vector3Dd &maxBounds);
    HeightField(const HeightField &other);
    ~HeightField() override;

    void setWaterLevel(double level);
    const HeightFieldSolid *getSolid() const { return solid; }
    const Geometry *getIntrinsicGeometry() const override { return solid; }
    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override;

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
    HeightFieldSolid *solid;

    double imageHeight(double height) const;
    int traceCrossings(
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride,
        const GeometryIntersectionEmissionContext *context);
};

#endif
