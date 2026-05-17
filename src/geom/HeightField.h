#ifndef __HEIGHT_FIELD_H__
#define __HEIGHT_FIELD_H__

#include "common/Frame.h"
#include "geom/Boxes.h"
#include "geom/HeightFieldBlock.h"
class RGBAImage;

class HeightField : public Geometry {
  public:
    Transformation *transformation;
    Box *bounding_box;
    DBL Block_Size;
    DBL Inv_Blk_Size;
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

  private:
    static DBL getHeightAt(int x, int z, HeightField *hField);
    static int intersectPixel(
        int x, int z, Ray *ray, HeightField *hField, DBL height1, DBL height2);
    static int intersectSubBlock(HeightFieldBlock *block, Ray *ray, HeightField *hField,
        Vector3D *start, Vector3D *end);
    static int intersectHfNode(Ray *ray, HeightField *hField, Vector3D *start, Vector3D *end);
};

#endif
