#ifndef __HEIGHT_FIELD_H__
#define __HEIGHT_FIELD_H__

#include "common/Frame.h"
#include "app/PovApp.h"
#include "common/Vector.h"
#include "geom/Boxes.h"
#include "geom/HeightFieldBlock.h"
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
        SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue);
    static int insideHeightfld(Vector3D *testPoint, SimpleBody *object);
    static void heightFldNormal(
        Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint);
    static void *copyHeightfld(SimpleBody *object);
    static void translateHeightfld(SimpleBody *object, Vector3D *vector);
    static void rotateHeightfld(SimpleBody *object, Vector3D *vector);
    static void scaleHeightfld(SimpleBody *object, Vector3D *vector);
    static void invertHeightfld(SimpleBody *object);
    static inline int signInline(double x);
    static inline double minValue(double x, double y);
    static inline double maxValue(double x, double y);

  private:
    static double getHeightAt(int x, int z, HeightField *hField);
    static int intersectPixel(
        int x, int z, Ray *ray, HeightField *hField, double height1, double height2);
    static int intersectSubBlock(HeightFieldBlock *block, Ray *ray, HeightField *hField,
        Vector3D *start, Vector3D *end);
    static int intersectHfNode(Ray *ray, HeightField *hField, Vector3D *start, Vector3D *end);
};

extern Methods Height_Field_Methods;
extern HeightField *getHeightFieldShape(void);

#endif
