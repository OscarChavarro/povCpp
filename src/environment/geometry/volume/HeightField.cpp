/**
This file implements the height field shape primitive.  The shape is
implemented as a collection of triangles which are calculated as
needed.  The basic intersection routine first computes the rays
intersection with the box marking the limits of the shape, then
follows the line from one intersection point to the other, testing
the two triangles which form the pixel for an intersection with the ray at
each step.
*/

#include <cstdlib>

#include "java/lang/Math.h"
#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "common/Config.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/volume/HeightField.h"

AxisAlignedBoundingBox
HeightField::getMinMax() const
{
    if (boundingBox == nullptr) {
        return AxisAlignedBoundingBox::unbounded();
    }
    const Vector3Dd *bb = boundingBox->getBounds();
    return AxisAlignedBoundingBox::fromTransformedCorners(bb[0], bb[1], transformation);
}

HeightField::HeightField() :
    transformation(new Matrix4x4d(Matrix4x4d::identityMatrix())),
    transformationInverse(new Matrix4x4d(Matrix4x4d::identityMatrix())),
    boundingBox(new Box),
    blockSize(0.0),
    invBlkSize(0.0),
    block(nullptr),
    Map(nullptr),
    blockOuterSize(0),
    blockInnerSize(0),
    mapOuterSize(0),
    mapInnerSize(0)
{
}

HeightField::HeightField(const Matrix4x4d &transformation,
    const Matrix4x4d &transformationInverse, const Vector3Dd &minBounds,
    const Vector3Dd &maxBounds) :
    transformation(new Matrix4x4d(transformation)),
    transformationInverse(new Matrix4x4d(transformationInverse)),
    boundingBox(new Box(minBounds, maxBounds)),
    blockSize(0.0),
    invBlkSize(0.0),
    block(nullptr),
    Map(nullptr),
    blockOuterSize(0),
    blockInnerSize(0),
    mapOuterSize(0),
    mapInnerSize(0)
{
}

HeightField::HeightField(const HeightField &other) :
    transformation(other.transformation != nullptr ?
        new Matrix4x4d(*other.transformation) : nullptr),
    transformationInverse(other.transformationInverse != nullptr ?
        new Matrix4x4d(*other.transformationInverse) : nullptr),
    boundingBox(other.boundingBox != nullptr ? new Box(*other.boundingBox) : nullptr),
    blockSize(other.blockSize),
    invBlkSize(other.invBlkSize),
    block(nullptr),
    Map(nullptr),
    blockOuterSize(other.blockOuterSize),
    blockInnerSize(other.blockInnerSize),
    mapOuterSize(other.mapOuterSize),
    mapInnerSize(other.mapInnerSize)
{
    if (other.block != nullptr) {
        block = (HeightFieldBlock **)calloc(blockOuterSize, sizeof(HeightFieldBlock *));
        for (int i = 0; i < blockOuterSize; i++) {
            block[i] = (HeightFieldBlock *)calloc(blockInnerSize, sizeof(HeightFieldBlock));
            for (int j = 0; j < blockInnerSize; j++) {
                block[i][j] = other.block[i][j];
            }
        }
    }
    if (other.Map != nullptr) {
        Map = (float **)calloc(mapOuterSize, sizeof(float *));
        for (int z = 0; z < mapOuterSize; z++) {
            if (other.Map[z] != nullptr) {
                Map[z] = (float *)calloc(mapInnerSize, sizeof(float));
                for (int x = 0; x < mapInnerSize; x++) {
                    Map[z][x] = other.Map[z][x];
                }
            }
        }
    }
}

HeightField::~HeightField()
{
    delete transformation;
    delete transformationInverse;
    delete boundingBox;
    if (block != nullptr) {
        for (int i = 0; i < blockOuterSize; i++) {
            free(block[i]);
        }
        free(block);
    }
    if (Map != nullptr) {
        for (int z = 0; z < mapOuterSize; z++) {
            free(Map[z]);
        }
        free(Map);
    }
}

inline int
HeightField::signInline(double x)
{
    return (x > 0.0) ? 1 : ((x == 0.0) ? 0 : -1);
}

inline double
HeightField::minValue(double x, double y)
{
    return (x > y) ? y : x;
}

inline double
HeightField::maxValue(double x, double y)
{
    return (x < y) ? y : x;
}

double
HeightField::getHeightAt(int x, int z, const HeightField *hField)
{
    return (double)hField->Map[z][x];
}
int
HeightField::intersectPixel(int x, int z, const RayWithSegments *ray,
    HeightFieldTraversalState &state,
    HeightField *hField, double height1, double height2)
{
    Vector3Dd t1V1;
    Vector3Dd t1V2;
    Vector3Dd t1V3;
    Vector3Dd t2V1;
    Vector3Dd t2V2;
    Vector3Dd t2V3;
    Vector3Dd localNormal;
    double pos1;
    double pos2;
    double dot;
    double depth1 = HUGE_VAL;
    double depth2 = HUGE_VAL;
    double s;
    double t;
    double y1;
    double y2;
    double y3;
    double y4;
    double maxHeight;
    double minHeight;
    Statistics &stats = *ray->getStatistics();

    y1 = HeightField::getHeightAt(x, z, hField);
    y2 = HeightField::getHeightAt(x + 1, z, hField);
    y3 = HeightField::getHeightAt(x, z + 1, hField);
    y4 = HeightField::getHeightAt(x + 1, z + 1, hField);

    t1V1 = Vector3Dd((double)x, y1, (double)z);
    *&t1V2 = Vector3Dd(1.0, y2 - y1, 0.0);
    *&t1V3 = Vector3Dd(0.0, y3 - y1, 1.0);
    t2V1 = Vector3Dd((double)(x + 1), y4, (double)(z + 1));
    *&t2V2 = Vector3Dd(-1.0, y3 - y4, 0.0);
    *&t2V3 = Vector3Dd(0.0, y2 - y4, -1.0);

    // First, we check to see if it is even possible for the ray to
    // intersect the triangle.

    if ((HeightField::maxValue(y1, HeightField::maxValue(y2, y3)) >= height1) &&
        (HeightField::minValue(y1, HeightField::minValue(y2, y3)) <= height2)) {
        localNormal = t1V3.crossProduct(t1V2);
        dot = localNormal.dotProduct(ray->getDirection());

        if ((dot > Config::INTERSECTION_EPSILON) || (dot < -Config::INTERSECTION_EPSILON)) {
            pos1 = localNormal.dotProduct(t1V1);

            pos2 = localNormal.dotProduct(ray->getOrigin());

            pos1 -= pos2;

            depth1 = pos1 / dot;

            if ((depth1 > Config::SMALL_TOLERANCE) && (depth1 < Config::MAX_DISTANCE)) {
                s = ray->getOrigin().x() + (depth1 * ray->getDirection().x()) - (double)x;
                t = ray->getOrigin().z() + (depth1 * ray->getDirection().z()) - (double)z;

                if ((s < -Config::INTERSECTION_EPSILON) || (t < -Config::INTERSECTION_EPSILON) ||
                    ((s + t) > 1.0 + Config::INTERSECTION_EPSILON)) {
                    depth1 = HUGE_VAL;
                }
            } else {
                depth1 = HUGE_VAL;
            }
        }
    }

    /**
    First, we check to see if it is even possible for the ray to
    intersect the triangle.
    Rewritten to get around Code Builder FP stack problem.
    Original code:
         if((HeightField::maxValue(y4,HeightField::maxValue(y2,y3)) >=
    height1) && (HeightField::minValue(y4,HeightField::minValue(y2,y3)) <=
    height2))
    */

    maxHeight = HeightField::maxValue(y4, HeightField::maxValue(y2, y3));
    minHeight = HeightField::minValue(y4, HeightField::minValue(y2, y3));
    if ((maxHeight >= height1) && (minHeight <= height2)) {
        localNormal = t2V3.crossProduct(t2V2);
        dot = localNormal.dotProduct(ray->getDirection());

        if ((dot > Config::INTERSECTION_EPSILON) || (dot < -Config::INTERSECTION_EPSILON)) {
            pos1 = localNormal.dotProduct(t2V1);

            pos2 = localNormal.dotProduct(ray->getOrigin());
            pos1 -= pos2;

            depth2 = pos1 / dot;

            if ((depth2 > Config::SMALL_TOLERANCE) && (depth2 < Config::MAX_DISTANCE)) {
                s = ray->getOrigin().x() + (depth2 * ray->getDirection().x()) - (double)x;
                t = ray->getOrigin().z() + (depth2 * ray->getDirection().z()) - (double)z;

                if ((s > 1.0 + Config::INTERSECTION_EPSILON) || (t > 1.0 + Config::INTERSECTION_EPSILON) ||
                    ((s + t) < 1.0 - Config::INTERSECTION_EPSILON)) {
                    depth2 = HUGE_VAL;
                }
            } else {
                depth2 = HUGE_VAL;
            }
        }
    }

    if ((depth1 >= Config::MAX_DISTANCE) && (depth2 >= Config::MAX_DISTANCE)) {
        return (false);
    }

    if (depth2 < depth1) {
        state.hfIntersection->getIntersection().t = depth2;
        t1V1 = state.rRay->getDirection().multiply(depth2);
        t1V1 = t1V1.add(state.rRay->getOrigin());
        state.hfIntersection->getIntersection().point = t1V1;
        state.hfIntersection->getAttributes().setHitGeometry(hField);
        state.hfQueue->offer(*state.hfIntersection);
    } else {
        state.hfIntersection->getIntersection().t = depth1;
        t1V1 = state.rRay->getDirection().multiply(depth1);
        t1V1 = t1V1.add(state.rRay->getOrigin());
        state.hfIntersection->getIntersection().point = t1V1;
        state.hfIntersection->getAttributes().setHitGeometry(hField);
        state.hfQueue->offer(*state.hfIntersection);
    }
    stats.incrementRayHtFieldTestsSucceeded();
    return (true);
}

int
HeightField::intersectSubBlock(const HeightFieldBlock *block,
    const RayWithSegments *ray, HeightFieldTraversalState &state,
    HeightField *hField, const Vector3Dd *start,
    const Vector3Dd *end)
{
    double y1;
    double y2;
    double sx;
    double sy;
    double sz;
    double ex;
    double ez;
    double f;
    int ix;
    int iz;
    int length;
    int i;

    if (HeightField::minValue(start->y(), end->y()) > block->getMaxY()) {
        return (false);
    }

    if (HeightField::maxValue(start->y(), end->y()) < block->getMinY()) {
        return (false);
    }

    sx = start->x();
    sy = start->y();
    sz = start->z();
    ex = end->x();
    ez = end->z();

    if (state.xDom) {
        if (state.isdx >= 0) {
            f = java::Math::floor(sx) - sx;
            sx = java::Math::floor(sx);
            sy += state.myx * f;
            sz += state.mzx * f;
            ex = java::Math::floor(ex);
            ix = (int)sx;
        } else {
            f = java::Math::ceil(sx) - sx;
            sx = java::Math::ceil(sx);
            sy += state.myx * f;
            sz += state.mzx * f;
            ex = java::Math::ceil(ex);
            ix = (int)sx - 1;
        }

        length = 0.5 + java::Math::abs(ex - sx);

        if (state.isdz >= 0) {
            f = sz - java::Math::ceil(sz);
            iz = (int)java::Math::floor(sz);
        } else {
            f = java::Math::floor(sz) - sz;
            iz = (int)java::Math::ceil(sz) - 1;
        }

        if (state.gdy >= 0.0) {
            y1 = sy;
            y2 = sy + state.gdy;
        } else {
            y1 = sy + state.gdy;
            y2 = sy;
        }

        for (i = 0; i <= length; i++) {
            if (HeightField::intersectPixel(ix, iz, ray, state, hField, y1, y2)) {
                return (true);
            }
            f += state.gdz;
            if (f >= 0.0) {
                iz += state.isdz;
                if (HeightField::intersectPixel(ix, iz, ray, state, hField, y1, y2)) {
                    return (true);
                }
                f -= 1.0;
            }
            ix += state.isdx;
            y1 += state.gdy;
            y2 += state.gdy;
        }
    } else {

        if (state.isdz >= 0) {
            f = java::Math::floor(sz) - sz;
            sz = java::Math::floor(sz);
            sy += state.myz * f;
            sx += state.mxz * f;
            ez = java::Math::floor(ez);
            iz = (int)sz;
        } else {
            f = java::Math::ceil(sz) - sz;
            sz = java::Math::ceil(sz);
            sy += state.myz * f;
            sx += state.mxz * f;
            ez = java::Math::ceil(ez);
            iz = (int)sz - 1;
        }

        length = 0.5 + java::Math::abs(ez - sz);

        if (state.isdx >= 0) {
            f = sx - java::Math::ceil(sx);
            ix = (int)java::Math::floor(sx);
        } else {
            f = java::Math::floor(sx) - sx;
            ix = (int)java::Math::ceil(sx) - 1;
        }

        if (state.gdy >= 0.0) {
            y1 = sy;
            y2 = sy + state.gdy;
        } else {
            y1 = sy + state.gdy;
            y2 = sy;
        }

        for (i = 0; i <= length; i++) {
            if (HeightField::intersectPixel(ix, iz, ray, state, hField, y1, y2)) {
                return (true);
            }
            f += state.gdx;
            if (f >= 0.0) {
                ix += state.isdx;
                if (HeightField::intersectPixel(ix, iz, ray, state, hField, y1, y2)) {
                    return (true);
                }
                f -= 1.0;
            }
            iz += state.isdz;
            y1 += state.gdy;
            y2 += state.gdy;
        }
    }
    return (false);
}

int
HeightField::intersectHfNode(const RayWithSegments *ray,
    HeightFieldTraversalState &state, HeightField *hField,
    const Vector3Dd *start, const Vector3Dd *end)
{
    Vector3Dd *curr;
    Vector3Dd *next;
    Vector3Dd *temp;
    Vector3Dd temp1;
    Vector3Dd temp2;
    double sx;
    double sy;
    double sz;
    double ex;
    double ey;
    double ez;
    double x;
    double y;
    double z;
    double tnear;
    double tfar;
    double blockSize;
    double invBlkSize;
    int ix;
    int iz;
    int xSize;
    int zSize;
    int length;
    int i;

    x = sx = start->x();
    y = sy = start->y();
    z = sz = start->z();
    ex = end->x();
    ey = end->y();
    ez = end->z();

    blockSize = hField->blockSize;
    invBlkSize = hField->invBlkSize;

    xSize = abs((int)(ex * invBlkSize) - (int)(sx * invBlkSize));
    zSize = abs((int)(ez * invBlkSize) - (int)(sz * invBlkSize));
    length = xSize + zSize;

    curr = &temp1;
    next = &temp2;
    *curr = Vector3Dd(x, y, z);
    double t;

    if (state.xDom) {
        if (state.isdx >= 0) {
            ix = (int)java::Math::floor(sx * invBlkSize);
            tnear = blockSize * (ix + 1) - sx;

            if (state.isdz >= 0) {
                iz = (int)java::Math::floor(sz * invBlkSize);
                tfar = state.gdx * (blockSize * (iz + 1) - sz);
            } else {
                iz = (int)java::Math::ceil(sz * invBlkSize) - 1;
                tfar = state.gdx * (sz - blockSize * (iz));
            }
            for (i = 0; i < length; i++) {
                if (tnear < tfar) {
                    t = tnear;
                    x = sx + t;
                    y = sy + state.myx * t;
                    z = sz + state.mzx * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->block[ix][iz]),
                            ray, state, hField, curr, next)) {
                        return (true);
                    }
                    temp = curr;
                    curr = next;
                    next = temp;
                    ix++;
                    tnear = blockSize * (ix + 1) - sx;
                } else {
                    t = tfar;
                    x = sx + t;
                    y = sy + state.myx * t;
                    z = sz + state.mzx * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->block[ix][iz]),
                            ray, state, hField, curr, next)) {
                        return (true);
                    }
                    temp = curr;
                    curr = next;
                    next = temp;
                    iz += state.isdz;
                    if (state.isdz >= 0) {
                        tfar = state.gdx * (blockSize * (iz + 1) - sz);
                    } else {
                        tfar = state.gdx * (sz - blockSize * (iz));
                    }
                }
            }
        } else {
            ix = (int)java::Math::ceil(sx * invBlkSize) - 1;
            tnear = sx - blockSize * (ix);

            if (state.isdz >= 0) {
                iz = (int)java::Math::floor(sz * invBlkSize);
                tfar = state.gdx * (blockSize * (iz + 1) - sz);
            } else {
                iz = (int)java::Math::ceil(sz * invBlkSize) - 1;
                tfar = state.gdx * (sz - blockSize * (iz));
            }

            for (i = 0; i < length; i++) {
                if (tnear < tfar) {
                    t = tnear;
                    x = sx - t;
                    y = sy - state.myx * t;
                    z = sz - state.mzx * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->block[ix][iz]),
                            ray, state, hField, curr, next)) {
                        return (true);
                    }
                    temp = curr;
                    curr = next;
                    next = temp;
                    ix--;
                    tnear = sx - blockSize * (ix);
                } else {
                    t = tfar;
                    x = sx - t;
                    y = sy - state.myx * t;
                    z = sz - state.mzx * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->block[ix][iz]),
                            ray, state, hField, curr, next)) {
                        return (true);
                    }
                    temp = curr;
                    curr = next;
                    next = temp;
                    iz += state.isdz;
                    if (state.isdz >= 0) {
                        tfar = state.gdx * (blockSize * (iz + 1) - sz);
                    } else {
                        tfar = state.gdx * (sz - blockSize * (iz));
                    }
                }
            }
        }
    } else {
        if (state.isdz >= 0) {
            iz = (int)java::Math::floor(sz * invBlkSize);
            tnear = blockSize * (iz + 1) - sz;

            if (state.isdx >= 0) {
                ix = (int)java::Math::floor(sx * invBlkSize);
                tfar = state.gdz * (blockSize * (ix + 1) - sx);
            } else {
                ix = (int)java::Math::ceil(sx * invBlkSize) - 1;
                tfar = state.gdz * (sx - blockSize * (ix));
            }
            for (i = 0; i < length; i++) {
                if (tnear < tfar) {
                    t = tnear;
                    z = sz + t;
                    y = sy + state.myz * t;
                    x = sx + state.mxz * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->block[ix][iz]),
                            ray, state, hField, curr, next)) {
                        return (true);
                    }
                    temp = curr;
                    curr = next;
                    next = temp;
                    iz++;
                    tnear = blockSize * (iz + 1) - sz;
                } else {
                    t = tfar;
                    z = sz + t;
                    y = sy + state.myz * t;
                    x = sx + state.mxz * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->block[ix][iz]),
                            ray, state, hField, curr, next)) {
                        return (true);
                    }
                    temp = curr;
                    curr = next;
                    next = temp;
                    ix += state.isdx;
                    if (state.isdx >= 0) {
                        tfar = state.gdz * (blockSize * (ix + 1) - sx);
                    } else {
                        tfar = state.gdz * (sx - blockSize * (ix));
                    }
                }
            }
        } else {
            iz = (int)java::Math::ceil(sz * invBlkSize) - 1;
            tnear = sz - blockSize * (iz);

            if (state.isdx >= 0) {
                ix = (int)java::Math::floor(sx * invBlkSize);
                tfar = state.gdz * (blockSize * (ix + 1) - sx);
            } else {
                ix = (int)java::Math::ceil(sx * invBlkSize) - 1;
                tfar = state.gdz * (sx - blockSize * (ix));
            }
            for (i = 0; i < length; i++) {
                if (tnear < tfar) {
                    t = tnear;
                    z = sz - t;
                    y = sy - state.myz * t;
                    x = sx - state.mxz * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->block[ix][iz]),
                            ray, state, hField, curr, next)) {
                        return (true);
                    }
                    temp = curr;
                    curr = next;
                    next = temp;
                    iz--;
                    tnear = sz - blockSize * iz;
                } else {
                    t = tfar;
                    z = sz - t;
                    y = sy - state.myz * t;
                    x = sx - state.mxz * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->block[ix][iz]),
                            ray, state, hField, curr, next)) {
                        return (true);
                    }
                    temp = curr;
                    curr = next;
                    next = temp;
                    ix += state.isdx;
                    if (state.isdx >= 0) {
                        tfar = state.gdz * (blockSize * (ix + 1) - sx);
                    } else {
                        tfar = state.gdz * (sx - blockSize * (ix));
                    }
                }
            }
        }
    }
    *next = Vector3Dd(ex, ey, ez);
    if (state.isdx >= 0) {
        ix = (int)java::Math::floor(ex * invBlkSize);
    } else {
        ix = (int)java::Math::ceil(ex * invBlkSize) - 1;
    }
    if (state.isdz >= 0) {
        iz = (int)java::Math::floor(ez * invBlkSize);
    } else {
        iz = (int)java::Math::ceil(ez * invBlkSize) - 1;
    }
    if (HeightField::intersectSubBlock(
            &(hField->block[ix][iz]), ray, state, hField, curr, next)) {
        return (true);
    }
    return (false);
}

void
HeightField::allocateHfBlocks(HeightField *hField, int maxX, int maxZ,
    double width, double height)
{
    const double size = (double)HeightField::maxValue(maxX, maxZ);
    hField->blockSize = java::Math::ceil(java::Math::sqrt(size + 1.0));
    hField->invBlkSize = 1.0 / hField->blockSize;

    const int w = (int)java::Math::ceil((width + 1.0) * hField->invBlkSize);
    const int h = (int)java::Math::ceil((height + 1.0) * hField->invBlkSize);

    hField->blockOuterSize = w;
    hField->blockInnerSize = h;
    hField->mapOuterSize = maxZ + 1;
    hField->mapInnerSize = maxX + 1;

    hField->Map = (float **)calloc(maxZ + 1, sizeof(float *));
    if (hField->Map == nullptr) {
        Logger::reportMessage("HeightField", Logger::ERROR, "", "Cannot allocate memory for height field\n");
    }

    hField->block = (HeightFieldBlock **)calloc(w, sizeof(HeightFieldBlock *));
    if (hField->block == nullptr) {
        Logger::reportMessage("HeightField", Logger::ERROR, "", "Cannot allocate memory for height field buffer\n");
    }
    for (int i = 0; i < w; i++) {
        hField->block[i] =
            (HeightFieldBlock *)calloc(h, sizeof(HeightFieldBlock));
        if (hField->block[i] == nullptr) {
            Logger::reportMessage("HeightField", Logger::ERROR, "", "Cannot allocate memory for height field buffer line\n");
        }
        for (int j = 0; j < h; j++) {
            hField->block[i][j].setMinY(65536.0);
            hField->block[i][j].setMaxY(0.0);
        }
    }

    hField->Map[0] = (float *)calloc(maxX + 1, sizeof(float));
    if (hField->Map[0] == nullptr) {
        Logger::reportMessage("HeightField", Logger::ERROR, "", "Cannot allocate memory for height field\n");
    }
}

void
HeightField::findHfMinMax(HeightField *hField,
    const IndexedColorImageHDRUncompressed *image, int imageType)
{
    int maxX = image->getXSize();
    if (imageType == HeightField::POT) {
        maxX = maxX / 2;
    }
    const int maxZ = image->getYSize();

    HeightField::allocateHfBlocks(hField, maxX, maxZ, image->getXSize(), image->getYSize());

    const int n = (int)hField->blockSize;
    const int w = (int)java::Math::ceil((image->getXSize() + 1.0) * hField->invBlkSize);
    const int h = (int)java::Math::ceil((image->getYSize() + 1.0) * hField->invBlkSize);

    for (int j = 0; j < h; j++) {
        for (int j2 = 0; (j2 <= n) && (j * n + j2 <= maxZ); j2++) {
            const int z = j * n + j2;
            if (j2 != 0) {
                hField->Map[z] = (float *)calloc(maxX + 1, sizeof(float));
                if (hField->Map[z] == nullptr) {
                    Logger::reportMessage("HeightField", Logger::ERROR, "",
                        "Cannot allocate memory for height field\n");
                }
            }
            for (int i = 0; i < w; i++) {
                for (int i2 = 0; (i2 <= n) && (i * n + i2 <= maxX); i2++) {
                    const int x = i * n + i2;
                    double tempY;
                    if ((x > 1) && (x < maxX - 1) && (z > 1) && (z < maxZ - 1)) {
                        const int temp1 = image->getPixel(x, maxZ - z - 1);
                        if (imageType == HeightField::POT) {
                            const int temp2 = image->getPixel(x + maxX, maxZ - z - 1);
                            tempY = (double)temp1 + (double)temp2 / 256.0;
                        } else {
                            tempY = (double)temp1;
                        }
                        if (tempY <= hField->getBoundingBox()->getBounds()[0].y()) {
                            hField->Map[z][x] = -10000.0;
                        } else {
                            hField->Map[z][x] = (float)tempY;
                        }
                    } else {
                        tempY = -10000.0;
                        hField->Map[z][x] = (float)tempY;
                    }

                    if (tempY < hField->getBoundingBox()->getBounds()[0].y()) {
                        tempY = hField->getBoundingBox()->getBounds()[0].y();
                    }
                    if (tempY < hField->block[i][j].getMinY()) {
                        hField->block[i][j].setMinY(tempY);
                    }
                    if (tempY > hField->block[i][j].getMaxY()) {
                        hField->block[i][j].setMaxY(tempY);
                    }
                }
            }
        }
    }
}

void
HeightField::findHfMinMax(HeightField *hField,
    const RGBAImageHDRUncompressed *image, int imageType)
{
    (void)imageType;
    int maxX = image->getXSize();
    const int maxZ = image->getYSize();

    HeightField::allocateHfBlocks(hField, maxX, maxZ, image->getXSize(), image->getYSize());

    const int n = (int)hField->blockSize;
    const int w = (int)java::Math::ceil((image->getXSize() + 1.0) * hField->invBlkSize);
    const int h = (int)java::Math::ceil((image->getYSize() + 1.0) * hField->invBlkSize);

    for (int j = 0; j < h; j++) {
        for (int j2 = 0; (j2 <= n) && (j * n + j2 <= maxZ); j2++) {
            const int z = j * n + j2;
            if (j2 != 0) {
                hField->Map[z] = (float *)calloc(maxX + 1, sizeof(float));
                if (hField->Map[z] == nullptr) {
                    Logger::reportMessage("HeightField", Logger::ERROR, "",
                        "Cannot allocate memory for height field\n");
                }
            }
            for (int i = 0; i < w; i++) {
                for (int i2 = 0; (i2 <= n) && (i * n + i2 <= maxX); i2++) {
                    const int x = i * n + i2;
                    double tempY;
                    if ((x > 1) && (x < maxX - 1) && (z > 1) && (z < maxZ - 1)) {
                        RGBAPixelHDR pixel;
                        image->getPixel(x, maxZ - z - 1, &pixel);
                        tempY = (double)pixel.r + (double)pixel.g / 256.0;
                        if (tempY <= hField->getBoundingBox()->getBounds()[0].y()) {
                            hField->Map[z][x] = -10000.0;
                        } else {
                            hField->Map[z][x] = (float)tempY;
                        }
                    } else {
                        tempY = -10000.0;
                        hField->Map[z][x] = (float)tempY;
                    }

                    if (tempY < hField->getBoundingBox()->getBounds()[0].y()) {
                        tempY = hField->getBoundingBox()->getBounds()[0].y();
                    }
                    if (tempY < hField->block[i][j].getMinY()) {
                        hField->block[i][j].setMinY(tempY);
                    }
                    if (tempY > hField->block[i][j].getMaxY()) {
                        hField->block[i][j].setMaxY(tempY);
                    }
                }
            }
        }
    }
}

int
HeightField::traceCrossings(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride,
    const GeometryIntersectionEmissionContext *context)
{
    Vector3Dd temp1;
    Vector3Dd temp2;
    RayWithSegments tempRay;
    double depth1;
    double depth2;
    bool retVal = false;
    HeightField * const hField = this;
    IntersectionCandidate localElement;

    Statistics &stats = *ray->getStatistics();
    stats.incrementRayHtFieldTests();
    localElement.getAttributes().setMaterial(materialOverride);
    if (context != nullptr) {
        localElement.getAttributes().pushDetailOwner(context->detailOwner);
        localElement.getAttributes().setMaterialUsesObjectLocalPoint(
            context->materialUsesObjectLocalPoint);
    }

    tempRay.setOriginAndDirection(
        hField->transformationInverse->transformPoint(ray->getOrigin()),
        hField->transformationInverse->transformDirection(ray->getDirection()));
    tempRay.setStatistics(ray->getStatistics());
    tempRay.setConfig(ray->getConfig());
    tempRay.setIntersectionQueuePool(ray->getIntersectionQueuePool());

    if (!Box::intersectBoxx(&tempRay, hField->boundingBox, &depth1, &depth2)) {
        return (false);
    }

    // if(      java::Math::abs(depth1 - depth2) < Config::SMALL_TOLERANCE) { Try Config::INTERSECTION_EPSILON if
    // next line doesn't work

    if (depth1 == depth2) {
        depth1 = 0.0;
        temp1 = tempRay.getDirection().multiply(depth1);
        temp1 = temp1.add(tempRay.getOrigin());
        temp2 = tempRay.getDirection().multiply(depth2);
        temp2 = temp2.add(tempRay.getOrigin());
    } else {
        temp1 = tempRay.getDirection().multiply(depth1);
        temp1 = temp1.add(tempRay.getOrigin());
        temp2 = tempRay.getDirection().multiply(depth2);
        temp2 = temp2.add(tempRay.getOrigin());
    }

    double mzx;
    double myx;
    if (java::Math::abs(tempRay.getDirection().x()) > Config::INTERSECTION_EPSILON) {
        mzx = tempRay.getDirection().z() / tempRay.getDirection().x();
        myx = tempRay.getDirection().y() / tempRay.getDirection().x();
    } else {
        mzx = tempRay.getDirection().z() / Config::INTERSECTION_EPSILON;
        myx = tempRay.getDirection().y() / Config::INTERSECTION_EPSILON;
    }
    double mxz;
    double myz;
    if (java::Math::abs(tempRay.getDirection().z()) > Config::INTERSECTION_EPSILON) {
        mxz = tempRay.getDirection().x() / tempRay.getDirection().z();
        myz = tempRay.getDirection().y() / tempRay.getDirection().z();
    } else {
        mxz = tempRay.getDirection().x() / Config::INTERSECTION_EPSILON;
        myz = tempRay.getDirection().y() / Config::INTERSECTION_EPSILON;
    }

    const int isdx = HeightField::signInline(tempRay.getDirection().x());
    const int isdz = HeightField::signInline(tempRay.getDirection().z());
    const bool xDom = java::Math::abs(tempRay.getDirection().x()) >=
        java::Math::abs(tempRay.getDirection().z());
    const double gdx = java::Math::abs(mxz);
    const double gdz = java::Math::abs(mzx);
    const double gdy = xDom ? myx * (double)isdx : myz * (double)isdz;
    HeightFieldTraversalState state(isdx, isdz, xDom, gdx, gdy, gdz, myx,
        mxz, mzx, myz, &localElement, depthQueue, ray);

    if (HeightField::intersectHfNode(&tempRay, state, hField, &temp1, &temp2)) {
        retVal = true;
    }
    return (retVal);
}

int
HeightField::doIntersectionForAllRayCrossings(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    return traceCrossings(ray, depthQueue, materialOverride, nullptr);
}

int
HeightField::doIntersectionForAllRayCrossingsAnnotated(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    return traceCrossings(ray, depthQueue, context.materialOverride, &context);
}

int
HeightField::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    (void)distanceTolerance;
    const HeightField *hField = this;
    int px;
    int pz;
    int dot1;
    int dot2;
    double dot1Value;
    double dot2Value;
    double x;
    double z;
    double y1;
    double y2;
    double y3;
    Vector3Dd localOrigin;
    Vector3Dd temp1;
    Vector3Dd temp2;
    Vector3Dd localNormal;
    Vector3Dd test;

    test = hField->transformationInverse->transformPoint(testPoint);

    px = (int)test.x();
    pz = (int)test.z();
    x = test.x() - (double)px;
    z = test.z() - (double)pz;

    if ((x + z) < 1.0) {
        y1 = HeightField::getHeightAt(px, pz, hField);
        y2 = HeightField::getHeightAt(px + 1, pz, hField);
        y3 = HeightField::getHeightAt(px, pz + 1, hField);
        localOrigin = Vector3Dd((double)px, y1, (double)pz);
        temp1 = Vector3Dd(1.0, y2 - y1, 0.0);
        temp2 = Vector3Dd(0.0, y3 - y1, 1.0);
    } else {
        px = java::Math::ceil(test.x());
        pz = java::Math::ceil(test.z());
        y1 = HeightField::getHeightAt(px, pz, hField);
        y2 = HeightField::getHeightAt(px - 1, pz, hField);
        y3 = HeightField::getHeightAt(px, pz - 1, hField);
        localOrigin = Vector3Dd((double)px, y1, (double)pz);
        temp1 = Vector3Dd(-1.0, y2 - y1, 0.0);
        temp2 = Vector3Dd(0.0, y3 - y1, -1.0);
    }
    localNormal = temp2.crossProduct(temp1);
    if (localNormal.y() < 0.0) {
        localNormal = localNormal.multiply(-1.0);
    }
    dot1Value = test.dotProduct(localNormal);
    dot2Value = localOrigin.dotProduct(localNormal);
    dot1 = (int)dot1Value;
    dot2 = (int)dot2Value;
    if ((dot1 < dot2) &&
        (test.y() > (hField->getBoundingBox()->getBounds()[0].y()) + 1.0)) {
        return INSIDE;
    }
    return OUTSIDE;
}

void
HeightField::normal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    const HeightField *hField = this;
    int px;
    int pz;
    double x;
    double z;
    double y1;
    double y2;
    double y3;
    Vector3Dd localOrigin;
    Vector3Dd temp1;
    Vector3Dd temp2;

    localOrigin = hField->transformationInverse->transformPoint(*intersectionPoint);

    px = (int)localOrigin.x();
    pz = (int)localOrigin.z();
    x = localOrigin.x() - (double)px;
    z = localOrigin.z() - (double)pz;

    if ((x + z) <= 1) {
        y1 = HeightField::getHeightAt(px, pz, hField);
        y2 = HeightField::getHeightAt(px + 1, pz, hField);
        y3 = HeightField::getHeightAt(px, pz + 1, hField);
        temp1 = Vector3Dd(1.0, y2 - y1, 0.0);
        temp2 = Vector3Dd(0.0, y3 - y1, 1.0);
    } else {
        y1 = HeightField::getHeightAt(px + 1, pz + 1, hField);
        y2 = HeightField::getHeightAt(px, pz + 1, hField);
        y3 = HeightField::getHeightAt(px + 1, pz, hField);
        temp1 = Vector3Dd(-1.0, y2 - y1, 0.0);
        temp2 = Vector3Dd(0.0, y3 - y1, -1.0);
    }

    temp1 = hField->transformation->transformDirection(temp1);
    temp2 = hField->transformation->transformDirection(temp2);
    *result = temp2.crossProduct(temp1);
    *result = (*result).normalizedFast();
}

void *
HeightField::copy()
{
    return new HeightField(*this);
}

void
HeightField::invertGeometry()
{
}
