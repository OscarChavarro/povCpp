#ifndef __HEIGHT_FIELD_H__
#define __HEIGHT_FIELD_H__

#include "common/FrameConfig.h"
#include "app/PovApp.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/HeightFieldBlock.h"
class RGBAImage;

static constexpr int GIF = 0;
static constexpr int POT = 1;
static constexpr int TGA = 2;

class HeightField : public Geometry {
  public:
    Transformation *transformation;
    Box *bounding_box;
    double Block_Size;
    double Inv_Blk_Size;
    HeightFieldBlock **Block;
    float **Map;

    static void findHfMinMax(
        HeightField *hField, RGBAImage *image, int imageType);
    static int allHeightfldIntersections(
        SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue);
    static int insideHeightfld(Vector3Dd *testPoint, SimpleBody *object);
    static void heightFldNormal(
        Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint);
    static void *copyHeightfld(SimpleBody *object);
    static void translateHeightfld(SimpleBody *object, Vector3Dd *vector);
    static void rotateHeightfld(SimpleBody *object, Vector3Dd *vector);
    static void scaleHeightfld(SimpleBody *object, Vector3Dd *vector);
    static void invertHeightfld(SimpleBody *object);
    static inline int signInline(double x);
    static inline double minValue(double x, double y);
    static inline double maxValue(double x, double y);

  private:
    static double getHeightAt(int x, int z, HeightField *hField);
    static int intersectPixel(
        int x, int z, RayWithSegments *ray, HeightField *hField, double height1, double height2);
    static int intersectSubBlock(HeightFieldBlock *block, RayWithSegments *ray, HeightField *hField,
        Vector3Dd *start, Vector3Dd *end);
    static int intersectHfNode(RayWithSegments *ray, HeightField *hField, Vector3Dd *start, Vector3Dd *end);
};

extern Methods Height_Field_Methods;
extern HeightField *getHeightFieldShape(void);

#endif
