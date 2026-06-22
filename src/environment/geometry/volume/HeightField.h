#ifndef __HEIGHT_FIELD__
#define __HEIGHT_FIELD__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/HeightFieldBlock.h"
#include "environment/geometry/volume/HeightFieldTraversalState.h"

class HeightField : public Geometry {
  public:
    static constexpr int GIF = 0;
    static constexpr int POT = 1;
    static constexpr int TGA = 2;

    HeightField();
    HeightField(const Matrix4x4d &transformation,
        const Matrix4x4d &transformationInverse, const Vector3Dd &minBounds,
        const Vector3Dd &maxBounds);
    HeightField(const HeightField &other);
    ~HeightField() override;

    Box *getBoundingBox() const { return boundingBox; }

    static void findHfMinMax(HeightField *hField,
        const IndexedColorImageHDRUncompressed *image, int imageType);
    static void findHfMinMax(HeightField *hField,
        const RGBAImageHDRUncompressed *image, int imageType);
    static inline int signInline(double x);
    static inline double minValue(double x, double y);
    static inline double maxValue(double x, double y);

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;

  private:
    Matrix4x4d *transformation;
    Matrix4x4d *transformationInverse;
    Box *boundingBox;
    double blockSize;
    double invBlkSize;
    HeightFieldBlock **block;
    float **Map;
    // Block/Map are raw 2D calloc'd arrays with no self-describing size; these
    // record the extents allocateHfBlocks() computed so the copy constructor and
    // destructor can walk/clone/free them correctly.
    int blockOuterSize; // number of HeightFieldBlock* rows in `block` (and the
                        // number of HeightFieldBlock entries in each row)
    int blockInnerSize;
    int mapOuterSize;   // number of float* rows in `Map`
    int mapInnerSize;   // number of float entries in each allocated `Map` row

    static void allocateHfBlocks(HeightField *hField, int maxX, int maxZ,
        double width, double height);
    static double getHeightAt(int x, int z, const HeightField *hField);
    static int intersectPixel(int x, int z, const RayWithSegments *ray,
        HeightFieldTraversalState &state,
        HeightField *hField, double height1, double height2);
    static int intersectSubBlock(const HeightFieldBlock *block,
        const RayWithSegments *ray, HeightFieldTraversalState &state,
        HeightField *hField,
        const Vector3Dd *start, const Vector3Dd *end);
    static int intersectHfNode(const RayWithSegments *ray,
        HeightFieldTraversalState &state, HeightField *hField,
        const Vector3Dd *start, const Vector3Dd *end);
};

#endif
