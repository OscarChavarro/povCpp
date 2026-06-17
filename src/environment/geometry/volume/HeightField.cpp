/**
This file implements the height field shape primitive.  The shape is
implemented as a collection of triangles which are calculated as
needed.  The basic intersection routine first computes the rays
intersection with the box marking the limits of the shape, then
follows the line from one intersection point to the other, testing
the two triangles which form the pixel for an intersection with the ray at
each step.
height field added by Doug Muir
with lots of advice and support from David Buck
and Drew Wells.
*/

#include <cstdio>

#include "java/lang/Math.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"
#include "common/Config.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/volume/HeightField.h"

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

int HeightField::isdx;
int HeightField::isdz;
bool HeightField::xDom;
double HeightField::gdx;
double HeightField::gdy;
double HeightField::gdz;
double HeightField::myx;
double HeightField::mxz;
double HeightField::mzx;
double HeightField::myz;
Intersection *HeightField::hfIntersection;
java::PriorityQueue<Intersection> *HeightField::hfQueue;
RayWithSegments *HeightField::rRay;

double
HeightField::getHeightAt(int x, int z, const HeightField *hField)
{
    return (double)hField->Map[z][x];
}
int
HeightField::intersectPixel(int x, int z, const RayWithSegments *ray,
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
    double depth1;
    double depth2;
    double s;
    double t;
    double y1;
    double y2;
    double y3;
    double y4;
    double maxHeight;
    double minHeight;

    depth1 = HUGE_VAL;
    depth2 = HUGE_VAL;

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

            if ((depth1 > GeometryConstants::Small_Tolerance) && (depth1 < GeometryConstants::Max_Distance)) {
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

            if ((depth2 > GeometryConstants::Small_Tolerance) && (depth2 < GeometryConstants::Max_Distance)) {
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

    if ((depth1 >= GeometryConstants::Max_Distance) && (depth2 >= GeometryConstants::Max_Distance)) {
        return (false);
    }

    if (depth2 < depth1) {
        hfIntersection->depth = depth2;
        hfIntersection->Object = nullptr;
        t1V1 = rRay->getDirection().multiply(depth2);
        t1V1 = t1V1.add(rRay->getOrigin());
        hfIntersection->point = t1V1;
        hfIntersection->Shape = reinterpret_cast<SimpleBody *>(hField);
        hfQueue->offer(*hfIntersection);
    } else {
        hfIntersection->depth = depth1;
        hfIntersection->Object = nullptr;
        t1V1 = rRay->getDirection().multiply(depth1);
        t1V1 = t1V1.add(rRay->getOrigin());
        hfIntersection->point = t1V1;
        hfIntersection->Shape = reinterpret_cast<SimpleBody *>(hField);
        hfQueue->offer(*hfIntersection);
    }
    Statistics::global().rayHtFieldTestsSucceeded++;
    return (true);
}

int
HeightField::intersectSubBlock(const HeightFieldBlock *block,
    const RayWithSegments *ray, HeightField *hField, const Vector3Dd *start,
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

    if (HeightField::minValue(start->y(), end->y()) > block->maxY) {
        return (false);
    }

    if (HeightField::maxValue(start->y(), end->y()) < block->minY) {
        return (false);
    }

    sx = start->x();
    sy = start->y();
    sz = start->z();
    ex = end->x();
    ez = end->z();

    if (xDom) {
        if (isdx >= 0) {
            f = java::Math::floor(sx) - sx;
            sx = java::Math::floor(sx);
            sy += myx * f;
            sz += mzx * f;
            ex = java::Math::floor(ex);
            ix = (int)sx;
        } else {
            f = java::Math::ceil(sx) - sx;
            sx = java::Math::ceil(sx);
            sy += myx * f;
            sz += mzx * f;
            ex = java::Math::ceil(ex);
            ix = (int)sx - 1;
        }

        length = 0.5 + java::Math::abs(ex - sx);

        if (isdz >= 0) {
            f = sz - java::Math::ceil(sz);
            iz = (int)java::Math::floor(sz);
        } else {
            f = java::Math::floor(sz) - sz;
            iz = (int)java::Math::ceil(sz) - 1;
        }

        if (gdy >= 0.0) {
            y1 = sy;
            y2 = sy + gdy;
        } else {
            y1 = sy + gdy;
            y2 = sy;
        }

        for (i = 0; i <= length; i++) {
            if (HeightField::intersectPixel(ix, iz, ray, hField, y1, y2)) {
                return (true);
            }
            f += gdz;
            if (f >= 0.0) {
                iz += isdz;
                if (HeightField::intersectPixel(ix, iz, ray, hField, y1, y2)) {
                    return (true);
                }
                f -= 1.0;
            }
            ix += isdx;
            y1 += gdy;
            y2 += gdy;
        }
    } else {

        if (isdz >= 0) {
            f = java::Math::floor(sz) - sz;
            sz = java::Math::floor(sz);
            sy += myz * f;
            sx += mxz * f;
            ez = java::Math::floor(ez);
            iz = (int)sz;
        } else {
            f = java::Math::ceil(sz) - sz;
            sz = java::Math::ceil(sz);
            sy += myz * f;
            sx += mxz * f;
            ez = java::Math::ceil(ez);
            iz = (int)sz - 1;
        }

        length = 0.5 + java::Math::abs(ez - sz);

        if (isdx >= 0) {
            f = sx - java::Math::ceil(sx);
            ix = (int)java::Math::floor(sx);
        } else {
            f = java::Math::floor(sx) - sx;
            ix = (int)java::Math::ceil(sx) - 1;
        }

        if (gdy >= 0.0) {
            y1 = sy;
            y2 = sy + gdy;
        } else {
            y1 = sy + gdy;
            y2 = sy;
        }

        for (i = 0; i <= length; i++) {
            if (HeightField::intersectPixel(ix, iz, ray, hField, y1, y2)) {
                return (true);
            }
            f += gdx;
            if (f >= 0.0) {
                ix += isdx;
                if (HeightField::intersectPixel(ix, iz, ray, hField, y1, y2)) {
                    return (true);
                }
                f -= 1.0;
            }
            iz += isdz;
            y1 += gdy;
            y2 += gdy;
        }
    }
    return (false);
}

int
HeightField::intersectHfNode(const RayWithSegments *ray, HeightField *hField,
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
    double t;
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
    t = 0.0;

    if (xDom) {
        if (isdx >= 0) {
            ix = (int)java::Math::floor(sx * invBlkSize);
            tnear = blockSize * (ix + 1) - sx;

            if (isdz >= 0) {
                iz = (int)java::Math::floor(sz * invBlkSize);
                tfar = gdx * (blockSize * (iz + 1) - sz);
            } else {
                iz = (int)java::Math::ceil(sz * invBlkSize) - 1;
                tfar = gdx * (sz - blockSize * (iz));
            }
            for (i = 0; i < length; i++) {
                if (tnear < tfar) {
                    t = tnear;
                    x = sx + t;
                    y = sy + myx * t;
                    z = sz + mzx * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->block[ix][iz]),
                            ray, hField, curr, next)) {
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
                    y = sy + myx * t;
                    z = sz + mzx * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->block[ix][iz]),
                            ray, hField, curr, next)) {
                        return (true);
                    }
                    temp = curr;
                    curr = next;
                    next = temp;
                    iz += isdz;
                    if (isdz >= 0) {
                        tfar = gdx * (blockSize * (iz + 1) - sz);
                    } else {
                        tfar = gdx * (sz - blockSize * (iz));
                    }
                }
            }
        } else {
            ix = (int)java::Math::ceil(sx * invBlkSize) - 1;
            tnear = sx - blockSize * (ix);

            if (isdz >= 0) {
                iz = (int)java::Math::floor(sz * invBlkSize);
                tfar = gdx * (blockSize * (iz + 1) - sz);
            } else {
                iz = (int)java::Math::ceil(sz * invBlkSize) - 1;
                tfar = gdx * (sz - blockSize * (iz));
            }

            for (i = 0; i < length; i++) {
                if (tnear < tfar) {
                    t = tnear;
                    x = sx - t;
                    y = sy - myx * t;
                    z = sz - mzx * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->block[ix][iz]),
                            ray, hField, curr, next)) {
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
                    y = sy - myx * t;
                    z = sz - mzx * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->block[ix][iz]),
                            ray, hField, curr, next)) {
                        return (true);
                    }
                    temp = curr;
                    curr = next;
                    next = temp;
                    iz += isdz;
                    if (isdz >= 0) {
                        tfar = gdx * (blockSize * (iz + 1) - sz);
                    } else {
                        tfar = gdx * (sz - blockSize * (iz));
                    }
                }
            }
        }
    } else {
        if (isdz >= 0) {
            iz = (int)java::Math::floor(sz * invBlkSize);
            tnear = blockSize * (iz + 1) - sz;

            if (isdx >= 0) {
                ix = (int)java::Math::floor(sx * invBlkSize);
                tfar = gdz * (blockSize * (ix + 1) - sx);
            } else {
                ix = (int)java::Math::ceil(sx * invBlkSize) - 1;
                tfar = gdz * (sx - blockSize * (ix));
            }
            for (i = 0; i < length; i++) {
                if (tnear < tfar) {
                    t = tnear;
                    z = sz + t;
                    y = sy + myz * t;
                    x = sx + mxz * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->block[ix][iz]),
                            ray, hField, curr, next)) {
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
                    y = sy + myz * t;
                    x = sx + mxz * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->block[ix][iz]),
                            ray, hField, curr, next)) {
                        return (true);
                    }
                    temp = curr;
                    curr = next;
                    next = temp;
                    ix += isdx;
                    if (isdx >= 0) {
                        tfar = gdz * (blockSize * (ix + 1) - sx);
                    } else {
                        tfar = gdz * (sx - blockSize * (ix));
                    }
                }
            }
        } else {
            iz = (int)java::Math::ceil(sz * invBlkSize) - 1;
            tnear = sz - blockSize * (iz);

            if (isdx >= 0) {
                ix = (int)java::Math::floor(sx * invBlkSize);
                tfar = gdz * (blockSize * (ix + 1) - sx);
            } else {
                ix = (int)java::Math::ceil(sx * invBlkSize) - 1;
                tfar = gdz * (sx - blockSize * (ix));
            }
            for (i = 0; i < length; i++) {
                if (tnear < tfar) {
                    t = tnear;
                    z = sz - t;
                    y = sy - myz * t;
                    x = sx - mxz * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->block[ix][iz]),
                            ray, hField, curr, next)) {
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
                    y = sy - myz * t;
                    x = sx - mxz * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->block[ix][iz]),
                            ray, hField, curr, next)) {
                        return (true);
                    }
                    temp = curr;
                    curr = next;
                    next = temp;
                    ix += isdx;
                    if (isdx >= 0) {
                        tfar = gdz * (blockSize * (ix + 1) - sx);
                    } else {
                        tfar = gdz * (sx - blockSize * (ix));
                    }
                }
            }
        }
    }
    *next = Vector3Dd(ex, ey, ez);
    if (isdx >= 0) {
        ix = (int)java::Math::floor(ex * invBlkSize);
    } else {
        ix = (int)java::Math::ceil(ex * invBlkSize) - 1;
    }
    if (isdz >= 0) {
        iz = (int)java::Math::floor(ez * invBlkSize);
    } else {
        iz = (int)java::Math::ceil(ez * invBlkSize) - 1;
    }
    if (HeightField::intersectSubBlock(
            &(hField->block[ix][iz]), ray, hField, curr, next)) {
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
            hField->block[i][j].minY = 65536.0;
            hField->block[i][j].maxY = 0.0;
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
                    fprintf(stderr, "Cannot allocate memory for height field\n");
                }
            }
            for (int i = 0; i < w; i++) {
                for (int i2 = 0; (i2 <= n) && (i * n + i2 <= maxX); i2++) {
                    const int x = i * n + i2;
                    double tempY = 0;
                    if ((x > 1) && (x < maxX - 1) && (z > 1) && (z < maxZ - 1)) {
                        const int temp1 = image->getPixel(x, maxZ - z - 1);
                        if (imageType == HeightField::POT) {
                            const int temp2 = image->getPixel(x + maxX, maxZ - z - 1);
                            tempY = (double)temp1 + (double)temp2 / 256.0;
                        } else {
                            tempY = (double)temp1;
                        }
                        if (tempY <= hField->boundingBox->bounds[0].y()) {
                            hField->Map[z][x] = -10000.0;
                        } else {
                            hField->Map[z][x] = (float)tempY;
                        }
                    } else {
                        tempY = -10000.0;
                        hField->Map[z][x] = (float)tempY;
                    }

                    if (tempY < hField->boundingBox->bounds[0].y()) {
                        tempY = hField->boundingBox->bounds[0].y();
                    }
                    if (tempY < hField->block[i][j].minY) {
                        hField->block[i][j].minY = tempY;
                    }
                    if (tempY > hField->block[i][j].maxY) {
                        hField->block[i][j].maxY = tempY;
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
                    fprintf(stderr, "Cannot allocate memory for height field\n");
                }
            }
            for (int i = 0; i < w; i++) {
                for (int i2 = 0; (i2 <= n) && (i * n + i2 <= maxX); i2++) {
                    const int x = i * n + i2;
                    double tempY = 0;
                    if ((x > 1) && (x < maxX - 1) && (z > 1) && (z < maxZ - 1)) {
                        RGBAPixelHDR pixel;
                        image->getPixel(x, maxZ - z - 1, &pixel);
                        tempY = (double)pixel.r + (double)pixel.g / 256.0;
                        if (tempY <= hField->boundingBox->bounds[0].y()) {
                            hField->Map[z][x] = -10000.0;
                        } else {
                            hField->Map[z][x] = (float)tempY;
                        }
                    } else {
                        tempY = -10000.0;
                        hField->Map[z][x] = (float)tempY;
                    }

                    if (tempY < hField->boundingBox->bounds[0].y()) {
                        tempY = hField->boundingBox->bounds[0].y();
                    }
                    if (tempY < hField->block[i][j].minY) {
                        hField->block[i][j].minY = tempY;
                    }
                    if (tempY > hField->block[i][j].maxY) {
                        hField->block[i][j].maxY = tempY;
                    }
                }
            }
        }
    }
}

int
HeightField::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    Vector3Dd temp1;
    Vector3Dd temp2;
    RayWithSegments tempRay;
    double depth1;
    double depth2;
    bool retVal = false;
    HeightField * const hField = this;
    Intersection localElement;

    Statistics::global().rayHtFieldTests++;

    tempRay.setOriginAndDirection(
        hField->transformationInverse->transformPoint(ray->getOrigin()),
        hField->transformationInverse->transformDirection(ray->getDirection()));

    if (!Box::intersectBoxx(&tempRay, hField->boundingBox, &depth1, &depth2)) {
        return (false);
    }

    // if(      java::Math::abs(depth1 - depth2) < GeometryConstants::Small_Tolerance) { Try Config::INTERSECTION_EPSILON if
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

    if (java::Math::abs(tempRay.getDirection().x()) > Config::INTERSECTION_EPSILON) {
        mzx = tempRay.getDirection().z() / tempRay.getDirection().x();
        myx = tempRay.getDirection().y() / tempRay.getDirection().x();
    } else {
        mzx = tempRay.getDirection().z() / Config::INTERSECTION_EPSILON;
        myx = tempRay.getDirection().y() / Config::INTERSECTION_EPSILON;
    }
    if (java::Math::abs(tempRay.getDirection().z()) > Config::INTERSECTION_EPSILON) {
        mxz = tempRay.getDirection().x() / tempRay.getDirection().z();
        myz = tempRay.getDirection().y() / tempRay.getDirection().z();
    } else {
        mxz = tempRay.getDirection().x() / Config::INTERSECTION_EPSILON;
        myz = tempRay.getDirection().y() / Config::INTERSECTION_EPSILON;
    }

    hfQueue = depthQueue;
    hfIntersection = &localElement;
    rRay = ray;

    isdx = HeightField::signInline(tempRay.getDirection().x());
    isdz = HeightField::signInline(tempRay.getDirection().z());

    xDom = false;
    if (java::Math::abs(tempRay.getDirection().x()) >= java::Math::abs(tempRay.getDirection().z())) {
        xDom = true;
    }

    gdx = java::Math::abs(mxz);
    gdz = java::Math::abs(mzx);
    if (xDom) {
        gdy = myx * (double)isdx;
    } else {
        gdy = myz * (double)isdz;
    }

    if (HeightField::intersectHfNode(&tempRay, hField, &temp1, &temp2)) {
        retVal = true;
    }
    return (retVal);
}

int
HeightField::inside(Vector3Dd *testPoint)
{
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

    test = hField->transformationInverse->transformPoint(*testPoint);

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
    if ((dot1 < dot2) && (test.y() > (hField->boundingBox->bounds[0].y()) + 1.0)) {
        return (true);
    }
    return (false);
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
    HeightField *newShape;

    newShape = new HeightField;
    *newShape = *this;

    return (newShape);
}

void
HeightField::translateGeometry(Vector3Dd *vector)
{
    HeightField * const hField = this;
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;

    if (!hField->transformation) {
        hField->transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        hField->transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    deltaTransformation = Matrix4x4d().translation(
        vector->x(), vector->y(), vector->z()).transpose();
    deltaTransformationInverse = Matrix4x4d().translation(
        0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
    *hField->transformation = hField->transformation->multiply(deltaTransformation);
    *hField->transformationInverse =
        deltaTransformationInverse.multiply(*hField->transformationInverse);
}

void
HeightField::rotateGeometry(Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;
    HeightField * const hField = this;

    if (!hField->transformation) {
        hField->transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        hField->transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    deltaTransformation.axisRotationRodrigues(&deltaTransformationInverse, vector);
    *hField->transformation = hField->transformation->multiply(deltaTransformation);
    *hField->transformationInverse =
        deltaTransformationInverse.multiply(*hField->transformationInverse);
}

void
HeightField::scaleGeometry(Vector3Dd *vector)
{
    HeightField * const hField = this;
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;

    if (!hField->transformation) {
        hField->transformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        hField->transformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    deltaTransformation = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    deltaTransformationInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z());
    *hField->transformation = hField->transformation->multiply(deltaTransformation);
    *hField->transformationInverse =
        deltaTransformationInverse.multiply(*hField->transformationInverse);
}

void
HeightField::invertGeometry()
{
}

#include "java/util/PriorityQueue.txx"
