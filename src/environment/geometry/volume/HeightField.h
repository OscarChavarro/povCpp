#ifndef __HEIGHT_FIELD_H__
#define __HEIGHT_FIELD_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/HeightFieldBlock.h"

class HeightField : public Geometry {
  public:
    static constexpr int GIF = 0;
    static constexpr int POT = 1;
    static constexpr int TGA = 2;

    Matrix4x4d *transformation;
    Matrix4x4d *transformationInverse;
    Box *boundingBox;
    double blockSize;
    double invBlkSize;
    HeightFieldBlock **block;
    float **Map;

    Matrix4x4d *getTransformation() const { return transformation; }
    Matrix4x4d *getTransformationInverse() const { return transformationInverse; }
    Box *getBoundingBox() const { return boundingBox; }

    static void findHfMinMax(HeightField *hField,
        const IndexedColorImageHDRUncompressed *image, int imageType);
    static void findHfMinMax(HeightField *hField,
        const RGBAImageHDRUncompressed *image, int imageType);
    static inline int signInline(double x);
    static inline double minValue(double x, double y);
    static inline double maxValue(double x, double y);

    int allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue) override;
    int inside(Vector3Dd *point) override;
    void normal(Vector3Dd *result, Vector3Dd *intersectionPoint) override;
    void *copy() override;
    void translateGeometry(Vector3Dd *vector) override;
    void rotateGeometry(Vector3Dd *vector) override;
    void scaleGeometry(Vector3Dd *vector) override;
    void invertGeometry() override;

  private:
    static int isdx;
    static int isdz;
    static bool xDom;
    static double gdx;
    static double gdy;
    static double gdz;
    static double myx;
    static double mxz;
    static double mzx;
    static double myz;
    static Intersection *hfIntersection;
    static java::PriorityQueue<Intersection> *hfQueue;
    static RayWithSegments *rRay;

    static void allocateHfBlocks(HeightField *hField, int maxX, int maxZ,
        double width, double height);
    static double getHeightAt(int x, int z, const HeightField *hField);
    static int intersectPixel(int x, int z, const RayWithSegments *ray,
        HeightField *hField, double height1, double height2);
    static int intersectSubBlock(const HeightFieldBlock *block,
        const RayWithSegments *ray, HeightField *hField,
        const Vector3Dd *start, const Vector3Dd *end);
    static int intersectHfNode(const RayWithSegments *ray, HeightField *hField,
        const Vector3Dd *start, const Vector3Dd *end);
};

#endif
