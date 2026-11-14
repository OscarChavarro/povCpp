#include <cmath>
#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "java/lang/IllegalStateException.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/numerics/FloatingInterval.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayBoundary.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossing.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/volume/HeightFieldSolid.h"

HeightFieldSolid::HeightFieldSolid(long columnCount, long rowCount, const Vector3Dd& minimumCorner,
    const Vector3Dd& maximumCorner) :
    columns(columnCount < 2 ? 2 : columnCount), rows(rowCount < 2 ? 2 : rowCount), heights(),
    minimum(minimumCorner), maximum(maximumCorner), blockSize(0), inverseBlockSize(0),
    blockColumns(0), blockRows(0), blocks(), blocksValid(false)
{
    heights.reserve(columns*rows);
    for (long i = 0; i < columns*rows; i++) heights.add(0.0f);
}

HeightFieldSolid::HeightFieldSolid(const HeightFieldSolid& other) :
    Solid(other), columns(other.columns), rows(other.rows), heights(other.heights),
    minimum(other.minimum), maximum(other.maximum), blockSize(other.blockSize),
    inverseBlockSize(other.inverseBlockSize), blockColumns(other.blockColumns),
    blockRows(other.blockRows), blocks(other.blocks), blocksValid(other.blocksValid)
{
}

HeightFieldSolid& HeightFieldSolid::operator=(const HeightFieldSolid& other)
{
    if (this != &other) {
        Solid::operator=(other);
        columns = other.columns;
        rows = other.rows;
        heights = other.heights;
        minimum = other.minimum;
        maximum = other.maximum;
        blockSize = other.blockSize;
        inverseBlockSize = other.inverseBlockSize;
        blockColumns = other.blockColumns;
        blockRows = other.blockRows;
        blocks = other.blocks;
        blocksValid = other.blocksValid;
    }
    return *this;
}

HeightFieldSolid::~HeightFieldSolid()
{
}

Geometry* HeightFieldSolid::copyGeometry() const { return new HeightFieldSolid(*this); }

double HeightFieldSolid::getHeight(long x, long z) const
{
    return (double)heights[z*columns + x];
}

void HeightFieldSolid::setHeight(long x, long z, double height)
{
    heights[z*columns + x] = (float)height;
    blocksValid = false;
}

void HeightFieldSolid::setBounds(const Vector3Dd& minimumCorner, const Vector3Dd& maximumCorner)
{
    minimum = minimumCorner;
    maximum = maximumCorner;
    blocksValid = false;
}

bool HeightFieldSolid::isValid() const
{
    const double values[6] = {minimum.x(), minimum.y(), minimum.z(), maximum.x(), maximum.y(), maximum.z()};
    for (int i = 0; i < 6; i++) if (!java::Double::isFinite(values[i])) return false;
    return minimum.x() < maximum.x() && minimum.y() < maximum.y() && minimum.z() < maximum.z() &&
        minimum.x() >= 0 && minimum.z() >= 0 && maximum.x() <= (double)(columns - 1) &&
        maximum.z() <= (double)(rows - 1);
}

const HeightFieldBlock* HeightFieldSolid::blockAt(int x, int z) const
{
    if (x < 0 || z < 0 || x >= blockColumns || z >= blockRows) return nullptr;
    return &blocks[(long)x*blockRows + z];
}

void HeightFieldSolid::updateBlocks()
{
    const int maxX = (int)(columns - 1);
    const int maxZ = (int)(rows - 1);
    const double size = (double)maxValue(maxX, maxZ);
    blockSize = java::Math::ceil(java::Math::sqrt(size + 1.0));
    inverseBlockSize = 1.0 / blockSize;
    blockColumns = (long)java::Math::ceil((maxX + 1.0) * inverseBlockSize);
    blockRows = (long)java::Math::ceil((maxZ + 1.0) * inverseBlockSize);
    blocks.clear();
    for (long i = 0; i < blockColumns*blockRows; i++) blocks.add(HeightFieldBlock());
    const int n = (int)blockSize;
    // Adjacent blocks share their border points.
    for (int j = 0; j < blockRows; j++) {
        for (int j2 = 0; (j2 <= n) && (j * n + j2 <= maxZ); j2++) {
            const int z = j * n + j2;
            for (int i = 0; i < blockColumns; i++) {
                for (int i2 = 0; (i2 <= n) && (i * n + i2 <= maxX); i2++) {
                    const int x = i * n + i2;
                    double height = heightAt(x, z);
                    if (height < minimum.y()) {
                        height = minimum.y();
                    }
                    blocks[(long)i*blockRows + j].include(height);
                }
            }
        }
    }
    blocksValid = true;
}

int HeightFieldSolid::intersectPixel(int x, int z, const Vector3Dd& origin,
    const Vector3Dd& direction, HeightFieldTraversal& state, double height1, double height2) const
{
    if (x < 0 || z < 0 || x + 1 >= columns || z + 1 >= rows) {
        return (false);
    }
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

    y1 = heightAt(x, z);
    y2 = heightAt(x + 1, z);
    y3 = heightAt(x, z + 1);
    y4 = heightAt(x + 1, z + 1);

    t1V1 = Vector3Dd((double)x, y1, (double)z);
    t1V2 = Vector3Dd(1.0, y2 - y1, 0.0);
    t1V3 = Vector3Dd(0.0, y3 - y1, 1.0);
    t2V1 = Vector3Dd((double)(x + 1), y4, (double)(z + 1));
    t2V2 = Vector3Dd(-1.0, y3 - y4, 0.0);
    t2V3 = Vector3Dd(0.0, y2 - y4, -1.0);

    // First, we check to see if it is even possible for the ray to
    // intersect the triangle.

    if ((maxValue(y1, maxValue(y2, y3)) >= height1) &&
        (minValue(y1, minValue(y2, y3)) <= height2)) {
        localNormal = t1V3.crossProduct(t1V2);
        dot = localNormal.dotProduct(direction);

        if ((dot > INTERSECTION_EPSILON) || (dot < -INTERSECTION_EPSILON)) {
            pos1 = localNormal.dotProduct(t1V1);

            pos2 = localNormal.dotProduct(origin);

            pos1 -= pos2;

            depth1 = pos1 / dot;

            if ((depth1 > SMALL_TOLERANCE) && (depth1 < MAX_DISTANCE)) {
                s = origin.x() + (depth1 * direction.x()) - (double)x;
                t = origin.z() + (depth1 * direction.z()) - (double)z;

                if ((s < -INTERSECTION_EPSILON) || (t < -INTERSECTION_EPSILON) ||
                    ((s + t) > 1.0 + INTERSECTION_EPSILON)) {
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
         if((maxValue(y4,maxValue(y2,y3)) >=
    height1) && (minValue(y4,minValue(y2,y3)) <=
    height2))
    */

    maxHeight = maxValue(y4, maxValue(y2, y3));
    minHeight = minValue(y4, minValue(y2, y3));
    if ((maxHeight >= height1) && (minHeight <= height2)) {
        localNormal = t2V3.crossProduct(t2V2);
        dot = localNormal.dotProduct(direction);

        if ((dot > INTERSECTION_EPSILON) || (dot < -INTERSECTION_EPSILON)) {
            pos1 = localNormal.dotProduct(t2V1);

            pos2 = localNormal.dotProduct(origin);
            pos1 -= pos2;

            depth2 = pos1 / dot;

            if ((depth2 > SMALL_TOLERANCE) && (depth2 < MAX_DISTANCE)) {
                s = origin.x() + (depth2 * direction.x()) - (double)x;
                t = origin.z() + (depth2 * direction.z()) - (double)z;

                if ((s > 1.0 + INTERSECTION_EPSILON) || (t > 1.0 + INTERSECTION_EPSILON) ||
                    ((s + t) < 1.0 - INTERSECTION_EPSILON)) {
                    depth2 = HUGE_VAL;
                }
            } else {
                depth2 = HUGE_VAL;
            }
        }
    }

    if ((depth1 >= MAX_DISTANCE) && (depth2 >= MAX_DISTANCE)) {
        return (false);
    }

    state.setDepth(depth2 < depth1 ? depth2 : depth1);
    return (true);
}

int HeightFieldSolid::intersectSubBlock(const HeightFieldBlock* block, const Vector3Dd& origin,
    const Vector3Dd& direction, HeightFieldTraversal& state,
    const Vector3Dd* start, const Vector3Dd* end) const
{
    if (block == nullptr) {
        return (false);
    }
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

    if (minValue(start->y(), end->y()) > block->getMaximumHeight()) {
        return (false);
    }

    if (maxValue(start->y(), end->y()) < block->getMinimumHeight()) {
        return (false);
    }

    sx = start->x();
    sy = start->y();
    sz = start->z();
    ex = end->x();
    ez = end->z();

    if (state.getXDom()) {
        if (state.getIsdx() >= 0) {
            f = java::Math::floor(sx) - sx;
            sx = java::Math::floor(sx);
            sy += state.getMyx() * f;
            sz += state.getMzx() * f;
            ex = java::Math::floor(ex);
            ix = (int)sx;
        } else {
            f = java::Math::ceil(sx) - sx;
            sx = java::Math::ceil(sx);
            sy += state.getMyx() * f;
            sz += state.getMzx() * f;
            ex = java::Math::ceil(ex);
            ix = (int)sx - 1;
        }

        length = 0.5 + java::Math::abs(ex - sx);

        if (state.getIsdz() >= 0) {
            f = sz - java::Math::ceil(sz);
            iz = (int)java::Math::floor(sz);
        } else {
            f = java::Math::floor(sz) - sz;
            iz = (int)java::Math::ceil(sz) - 1;
        }

        if (state.getGdy() >= 0.0) {
            y1 = sy;
            y2 = sy + state.getGdy();
        } else {
            y1 = sy + state.getGdy();
            y2 = sy;
        }

        for (i = 0; i <= length; i++) {
            if (intersectPixel(ix, iz, origin, direction, state, y1, y2)) {
                return (true);
            }
            f += state.getGdz();
            if (f >= 0.0) {
                iz += state.getIsdz();
                if (intersectPixel(ix, iz, origin, direction, state, y1, y2)) {
                    return (true);
                }
                f -= 1.0;
            }
            ix += state.getIsdx();
            y1 += state.getGdy();
            y2 += state.getGdy();
        }
    } else {

        if (state.getIsdz() >= 0) {
            f = java::Math::floor(sz) - sz;
            sz = java::Math::floor(sz);
            sy += state.getMyz() * f;
            sx += state.getMxz() * f;
            ez = java::Math::floor(ez);
            iz = (int)sz;
        } else {
            f = java::Math::ceil(sz) - sz;
            sz = java::Math::ceil(sz);
            sy += state.getMyz() * f;
            sx += state.getMxz() * f;
            ez = java::Math::ceil(ez);
            iz = (int)sz - 1;
        }

        length = 0.5 + java::Math::abs(ez - sz);

        if (state.getIsdx() >= 0) {
            f = sx - java::Math::ceil(sx);
            ix = (int)java::Math::floor(sx);
        } else {
            f = java::Math::floor(sx) - sx;
            ix = (int)java::Math::ceil(sx) - 1;
        }

        if (state.getGdy() >= 0.0) {
            y1 = sy;
            y2 = sy + state.getGdy();
        } else {
            y1 = sy + state.getGdy();
            y2 = sy;
        }

        for (i = 0; i <= length; i++) {
            if (intersectPixel(ix, iz, origin, direction, state, y1, y2)) {
                return (true);
            }
            f += state.getGdx();
            if (f >= 0.0) {
                ix += state.getIsdx();
                if (intersectPixel(ix, iz, origin, direction, state, y1, y2)) {
                    return (true);
                }
                f -= 1.0;
            }
            iz += state.getIsdz();
            y1 += state.getGdy();
            y2 += state.getGdy();
        }
    }
    return (false);
}


int HeightFieldSolid::intersectHfNode(const Vector3Dd& origin, const Vector3Dd& direction,
    HeightFieldTraversal& state, const Vector3Dd* start, const Vector3Dd* end) const
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

    blockSize = this->blockSize;
    invBlkSize = this->inverseBlockSize;

    xSize = abs((int)(ex * invBlkSize) - (int)(sx * invBlkSize));
    zSize = abs((int)(ez * invBlkSize) - (int)(sz * invBlkSize));
    length = xSize + zSize;

    curr = &temp1;
    next = &temp2;
    *curr = Vector3Dd(x, y, z);
    double t;

    if (state.getXDom()) {
        if (state.getIsdx() >= 0) {
            ix = (int)java::Math::floor(sx * invBlkSize);
            tnear = blockSize * (ix + 1) - sx;

            if (state.getIsdz() >= 0) {
                iz = (int)java::Math::floor(sz * invBlkSize);
                tfar = state.getGdx() * (blockSize * (iz + 1) - sz);
            } else {
                iz = (int)java::Math::ceil(sz * invBlkSize) - 1;
                tfar = state.getGdx() * (sz - blockSize * (iz));
            }
            for (i = 0; i < length; i++) {
                if (tnear < tfar) {
                    t = tnear;
                    x = sx + t;
                    y = sy + state.getMyx() * t;
                    z = sz + state.getMzx() * t;
                    *next = Vector3Dd(x, y, z);
                    if (intersectSubBlock(blockAt(ix, iz), origin, direction, state, curr, next)) {
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
                    y = sy + state.getMyx() * t;
                    z = sz + state.getMzx() * t;
                    *next = Vector3Dd(x, y, z);
                    if (intersectSubBlock(blockAt(ix, iz), origin, direction, state, curr, next)) {
                        return (true);
                    }
                    temp = curr;
                    curr = next;
                    next = temp;
                    iz += state.getIsdz();
                    if (state.getIsdz() >= 0) {
                        tfar = state.getGdx() * (blockSize * (iz + 1) - sz);
                    } else {
                        tfar = state.getGdx() * (sz - blockSize * (iz));
                    }
                }
            }
        } else {
            ix = (int)java::Math::ceil(sx * invBlkSize) - 1;
            tnear = sx - blockSize * (ix);

            if (state.getIsdz() >= 0) {
                iz = (int)java::Math::floor(sz * invBlkSize);
                tfar = state.getGdx() * (blockSize * (iz + 1) - sz);
            } else {
                iz = (int)java::Math::ceil(sz * invBlkSize) - 1;
                tfar = state.getGdx() * (sz - blockSize * (iz));
            }

            for (i = 0; i < length; i++) {
                if (tnear < tfar) {
                    t = tnear;
                    x = sx - t;
                    y = sy - state.getMyx() * t;
                    z = sz - state.getMzx() * t;
                    *next = Vector3Dd(x, y, z);
                    if (intersectSubBlock(blockAt(ix, iz), origin, direction, state, curr, next)) {
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
                    y = sy - state.getMyx() * t;
                    z = sz - state.getMzx() * t;
                    *next = Vector3Dd(x, y, z);
                    if (intersectSubBlock(blockAt(ix, iz), origin, direction, state, curr, next)) {
                        return (true);
                    }
                    temp = curr;
                    curr = next;
                    next = temp;
                    iz += state.getIsdz();
                    if (state.getIsdz() >= 0) {
                        tfar = state.getGdx() * (blockSize * (iz + 1) - sz);
                    } else {
                        tfar = state.getGdx() * (sz - blockSize * (iz));
                    }
                }
            }
        }
    } else {
        if (state.getIsdz() >= 0) {
            iz = (int)java::Math::floor(sz * invBlkSize);
            tnear = blockSize * (iz + 1) - sz;

            if (state.getIsdx() >= 0) {
                ix = (int)java::Math::floor(sx * invBlkSize);
                tfar = state.getGdz() * (blockSize * (ix + 1) - sx);
            } else {
                ix = (int)java::Math::ceil(sx * invBlkSize) - 1;
                tfar = state.getGdz() * (sx - blockSize * (ix));
            }
            for (i = 0; i < length; i++) {
                if (tnear < tfar) {
                    t = tnear;
                    z = sz + t;
                    y = sy + state.getMyz() * t;
                    x = sx + state.getMxz() * t;
                    *next = Vector3Dd(x, y, z);
                    if (intersectSubBlock(blockAt(ix, iz), origin, direction, state, curr, next)) {
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
                    y = sy + state.getMyz() * t;
                    x = sx + state.getMxz() * t;
                    *next = Vector3Dd(x, y, z);
                    if (intersectSubBlock(blockAt(ix, iz), origin, direction, state, curr, next)) {
                        return (true);
                    }
                    temp = curr;
                    curr = next;
                    next = temp;
                    ix += state.getIsdx();
                    if (state.getIsdx() >= 0) {
                        tfar = state.getGdz() * (blockSize * (ix + 1) - sx);
                    } else {
                        tfar = state.getGdz() * (sx - blockSize * (ix));
                    }
                }
            }
        } else {
            iz = (int)java::Math::ceil(sz * invBlkSize) - 1;
            tnear = sz - blockSize * (iz);

            if (state.getIsdx() >= 0) {
                ix = (int)java::Math::floor(sx * invBlkSize);
                tfar = state.getGdz() * (blockSize * (ix + 1) - sx);
            } else {
                ix = (int)java::Math::ceil(sx * invBlkSize) - 1;
                tfar = state.getGdz() * (sx - blockSize * (ix));
            }
            for (i = 0; i < length; i++) {
                if (tnear < tfar) {
                    t = tnear;
                    z = sz - t;
                    y = sy - state.getMyz() * t;
                    x = sx - state.getMxz() * t;
                    *next = Vector3Dd(x, y, z);
                    if (intersectSubBlock(blockAt(ix, iz), origin, direction, state, curr, next)) {
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
                    y = sy - state.getMyz() * t;
                    x = sx - state.getMxz() * t;
                    *next = Vector3Dd(x, y, z);
                    if (intersectSubBlock(blockAt(ix, iz), origin, direction, state, curr, next)) {
                        return (true);
                    }
                    temp = curr;
                    curr = next;
                    next = temp;
                    ix += state.getIsdx();
                    if (state.getIsdx() >= 0) {
                        tfar = state.getGdz() * (blockSize * (ix + 1) - sx);
                    } else {
                        tfar = state.getGdz() * (sx - blockSize * (ix));
                    }
                }
            }
        }
    }
    *next = Vector3Dd(ex, ey, ez);
    if (state.getIsdx() >= 0) {
        ix = (int)java::Math::floor(ex * invBlkSize);
    } else {
        ix = (int)java::Math::ceil(ex * invBlkSize) - 1;
    }
    if (state.getIsdz() >= 0) {
        iz = (int)java::Math::floor(ez * invBlkSize);
    } else {
        iz = (int)java::Math::ceil(ez * invBlkSize) - 1;
    }
    if (intersectSubBlock(blockAt(ix, iz), origin, direction, state, curr, next)) {
        return (true);
    }
    return (false);
}


HeightFieldTraceResult HeightFieldSolid::traceFirstHit(const Vector3Dd& origin,
    const Vector3Dd& direction, double& depth) const
{
    depth = 0.0;
    if (!blocksValid) {
        return HeightFieldTraceResult::MissedBounds;
    }
    double depth1 = 0.0;
    double depth2 = HUGE_VAL;
    bool faceOverlap = false;
    if (Box::intersectBounds(origin, direction, minimum, maximum, INTERSECTION_EPSILON,
            false, depth1, depth2, faceOverlap) != RayFirstHitStatus::Hit) {
        return HeightFieldTraceResult::MissedBounds;
    }
    if ((depth1 < SMALL_TOLERANCE) || (depth1 > MAX_DISTANCE)) {
        if ((depth2 < SMALL_TOLERANCE) || (depth2 > MAX_DISTANCE)) {
            return HeightFieldTraceResult::MissedBounds;
        }
        depth1 = depth2;
    } else if ((depth2 < SMALL_TOLERANCE) || (depth2 > MAX_DISTANCE)) {
        depth2 = depth1;
    }
    if (depth1 == depth2) {
        depth1 = 0.0;
    }
    const Vector3Dd temp1 = direction.multiply(depth1).add(origin);
    const Vector3Dd temp2 = direction.multiply(depth2).add(origin);
    double mzx;
    double myx;
    if (java::Math::abs(direction.x()) > INTERSECTION_EPSILON) {
        mzx = direction.z() / direction.x();
        myx = direction.y() / direction.x();
    } else {
        mzx = direction.z() / INTERSECTION_EPSILON;
        myx = direction.y() / INTERSECTION_EPSILON;
    }
    double mxz;
    double myz;
    if (java::Math::abs(direction.z()) > INTERSECTION_EPSILON) {
        mxz = direction.x() / direction.z();
        myz = direction.y() / direction.z();
    } else {
        mxz = direction.x() / INTERSECTION_EPSILON;
        myz = direction.y() / INTERSECTION_EPSILON;
    }
    const int isdx = signOf(direction.x());
    const int isdz = signOf(direction.z());
    const bool xDom = java::Math::abs(direction.x()) >= java::Math::abs(direction.z());
    const double gdx = java::Math::abs(mxz);
    const double gdz = java::Math::abs(mzx);
    const double gdy = xDom ? myx * (double)isdx : myz * (double)isdz;
    HeightFieldTraversal state(isdx, isdz, xDom, gdx, gdy, gdz, myx, mxz, mzx, myz);
    if (intersectHfNode(origin, direction, state, &temp1, &temp2)) {
        depth = state.getDepth();
        return HeightFieldTraceResult::Hit;
    }
    return HeightFieldTraceResult::MissedSurface;
}

void HeightFieldSolid::getSurfaceTangents(const Vector3Dd& point, Vector3Dd& first,
    Vector3Dd& second) const
{
    int px = (int)point.x();
    int pz = (int)point.z();
    if (px < 0) px = 0;
    if (pz < 0) pz = 0;
    if (px > columns - 2) px = (int)(columns - 2);
    if (pz > rows - 2) pz = (int)(rows - 2);
    const double x = point.x() - (double)px;
    const double z = point.z() - (double)pz;
    if ((x + z) <= 1) {
        const double y1 = heightAt(px, pz);
        const double y2 = heightAt(px + 1, pz);
        const double y3 = heightAt(px, pz + 1);
        first = Vector3Dd(1.0, y2 - y1, 0.0);
        second = Vector3Dd(0.0, y3 - y1, 1.0);
    } else {
        const double y1 = heightAt(px + 1, pz + 1);
        const double y2 = heightAt(px, pz + 1);
        const double y3 = heightAt(px + 1, pz);
        first = Vector3Dd(-1.0, y2 - y1, 0.0);
        second = Vector3Dd(0.0, y3 - y1, -1.0);
    }
}

bool HeightFieldSolid::isInsideGrid(double x, double z) const
{
    return x >= 0 && z >= 0 && x <= (double)(columns - 1) && z <= (double)(rows - 1);
}

double HeightFieldSolid::evaluateHeight(double x, double z) const
{
    if (!isInsideGrid(x, z)) return java::Double::NaN;
    long cx = (long)java::Math::floor(x);
    long cz = (long)java::Math::floor(z);
    if (cx > columns - 2) cx = columns - 2;
    if (cz > rows - 2) cz = rows - 2;
    const double fx = x - (double)cx;
    const double fz = z - (double)cz;
    if (fx + fz <= 1.0) {
        const double y1 = getHeight(cx, cz);
        return y1 + fx*(getHeight(cx + 1, cz) - y1) + fz*(getHeight(cx, cz + 1) - y1);
    }
    const double y4 = getHeight(cx + 1, cz + 1);
    return y4 + (1.0 - fx)*(getHeight(cx, cz + 1) - y4) + (1.0 - fz)*(getHeight(cx + 1, cz) - y4);
}

// y - h along the ray at t, with h the plane of one triangle of a cell.
double HeightFieldSolid::heightDifference(const Vector3Dd& origin, const Vector3Dd& direction,
    double t, long cellX, long cellZ, bool firstTriangle) const
{
    const double x = origin.x() + direction.x()*t;
    const double y = origin.y() + direction.y()*t;
    const double z = origin.z() + direction.z()*t;
    const double fx = x - (double)cellX;
    const double fz = z - (double)cellZ;
    if (firstTriangle) {
        const double y1 = getHeight(cellX, cellZ);
        return y - (y1 + fx*(getHeight(cellX + 1, cellZ) - y1) + fz*(getHeight(cellX, cellZ + 1) - y1));
    }
    const double y4 = getHeight(cellX + 1, cellZ + 1);
    return y - (y4 + (1.0 - fx)*(getHeight(cellX, cellZ + 1) - y4) +
        (1.0 - fz)*(getHeight(cellX + 1, cellZ) - y4));
}

void HeightFieldSolid::heightRange(double lowX, double highX, double lowZ, double highZ,
    double& lowest, double& highest) const
{
    long x0 = (long)java::Math::floor(lowX), x1 = (long)java::Math::ceil(highX);
    long z0 = (long)java::Math::floor(lowZ), z1 = (long)java::Math::ceil(highZ);
    if (x0 < 0) x0 = 0;
    if (z0 < 0) z0 = 0;
    if (x1 > columns - 1) x1 = columns - 1;
    if (z1 > rows - 1) z1 = rows - 1;
    lowest = java::Double::POSITIVE_INFINITY;
    highest = java::Double::NEGATIVE_INFINITY;
    for (long z = z0; z <= z1; z++) {
        for (long x = x0; x <= x1; x++) {
            const double h = getHeight(x, z);
            if (h < lowest) lowest = h;
            if (h > highest) highest = h;
        }
    }
}

long HeightFieldSolid::nearestSurface(const Vector3Dd& point) const
{
    long best = 0;
    double distance = java::Double::POSITIVE_INFINITY;
    const double h = evaluateHeight(point.x(), point.z());
    if (java::Double::isFinite(h)) distance = java::Math::abs(point.y() - h);
    const double faces[6] = {point.x() - minimum.x(), maximum.x() - point.x(),
        point.y() - minimum.y(), maximum.y() - point.y(), point.z() - minimum.z(), maximum.z() - point.z()};
    for (int i = 0; i < 6; i++) {
        if (java::Math::abs(faces[i]) < distance) { distance = java::Math::abs(faces[i]); best = i + 1; }
    }
    return best;
}

Vector3Dd HeightFieldSolid::surfaceNormal(const Vector3Dd& point, long surfaceIndex) const
{
    switch (surfaceIndex) {
    case 1: return Vector3Dd(-1, 0, 0);
    case 2: return Vector3Dd(1, 0, 0);
    case 3: return Vector3Dd(0, -1, 0);
    case 4: return Vector3Dd(0, 1, 0);
    case 5: return Vector3Dd(0, 0, -1);
    case 6: return Vector3Dd(0, 0, 1);
    default: break;
    }
    Vector3Dd first, second;
    getSurfaceTangents(point, first, second);
    return second.crossProduct(first);
}

RaySolidQueryStatus HeightFieldSolid::classifyRay(const Ray& ray, const RayCrossingQuery& query,
    RaySolidClassification& result) const
{
    result.clear();
    const Vector3Dd& o = ray.getOrigin();
    const Vector3Dd& d = ray.getDirection();
    if (!java::Double::isFinite(o.x()) || !java::Double::isFinite(o.y()) || !java::Double::isFinite(o.z()) ||
        !java::Double::isFinite(d.x()) || !java::Double::isFinite(d.y()) || !java::Double::isFinite(d.z()) ||
        (d.x() == 0 && d.y() == 0 && d.z() == 0) || java::Double::isNaN(query.minT) ||
        java::Double::isNaN(query.maxT) || query.minT >= query.maxT) return RaySolidQueryStatus::InvalidInput;
    if (query.minimumChordSquared != 0 || query.minimumExteriorApproach != java::Double::NEGATIVE_INFINITY)
        return RaySolidQueryStatus::Unsupported;
    if (!isValid()) return RaySolidQueryStatus::Indeterminate;
    double entry = query.minT, exit = query.maxT;
    bool overlap = false;
    const RayFirstHitStatus clip = Box::intersectBounds(o, d, minimum, maximum, 0.0, true,
        entry, exit, overlap);
    if (clip == RayFirstHitStatus::InvalidInput) return RaySolidQueryStatus::InvalidInput;
    if (clip == RayFirstHitStatus::Miss || (clip == RayFirstHitStatus::Hit && !overlap && !(exit > entry))) {
        result.insideSegments.add(false);
        return RaySolidQueryStatus::Supported;
    }
    if (clip != RayFirstHitStatus::Hit || overlap || !java::Double::isFinite(entry) ||
        !java::Double::isFinite(exit)) return RaySolidQueryStatus::Indeterminate;

    // Cuts at cell edges and diagonals, where the surface plane changes.
    java::ArrayList<double> cuts;
    cuts.add(entry);
    cuts.add(exit);
    auto addLines = [&](double start, double slope) {
        if (slope == 0) return;
        const double a = start + slope*entry, b = start + slope*exit;
        const double low = java::Math::min(a, b), high = java::Math::max(a, b);
        for (double k = java::Math::ceil(low); k <= high; k += 1.0) {
            const double t = (k - start)/slope;
            if (t > entry && t < exit) cuts.add(t);
        }
    };
    addLines(o.x(), d.x());
    addLines(o.z(), d.z());
    addLines(o.x() + o.z(), d.x() + d.z());
    cuts.sort(nullptr);

    java::ArrayList<double> starts;
    java::ArrayList<bool> inside;
    if (entry > query.minT) { starts.add(query.minT); inside.add(false); }
    for (long i = 0; i + 1 < cuts.size(); i++) {
        const double ta = cuts[i], tb = cuts[i + 1];
        if (!(tb > ta)) continue;
        const double mid = 0.5*(ta + tb);
        long cx = (long)java::Math::floor(o.x() + d.x()*mid);
        long cz = (long)java::Math::floor(o.z() + d.z()*mid);
        if (cx < 0) cx = 0;
        if (cz < 0) cz = 0;
        if (cx > columns - 2) cx = columns - 2;
        if (cz > rows - 2) cz = rows - 2;
        const bool first = (o.x() + d.x()*mid - (double)cx) + (o.z() + d.z()*mid - (double)cz) <= 1.0;
        const double ga = heightDifference(o, d, ta, cx, cz, first);
        const double gb = heightDifference(o, d, tb, cx, cz, first);
        if (!java::Double::isFinite(ga) || !java::Double::isFinite(gb)) {
            result.clear(); return RaySolidQueryStatus::Indeterminate;
        }
        if ((ga < 0 && gb > 0) || (ga > 0 && gb < 0)) {
            // One crossing of the plane inside this piece.
            const double root = ta + (tb - ta)*(ga/(ga - gb));
            starts.add(ta); inside.add(ga < 0);
            if (root > ta && root < tb) { starts.add(root); inside.add(gb < 0); }
            continue;
        }
        const double gm = heightDifference(o, d, mid, cx, cz, first);
        if (gm == 0 || !java::Double::isFinite(gm)) {
            result.clear(); return RaySolidQueryStatus::Indeterminate;
        }
        starts.add(ta); inside.add(gm < 0);
    }
    if (exit < query.maxT) { starts.add(exit); inside.add(false); }
    if (starts.isEmpty()) { result.insideSegments.add(false); return RaySolidQueryStatus::Supported; }
    bool state = inside[0];
    result.insideSegments.add(state);
    for (long i = 1; i < starts.size(); i++) {
        if (inside[i] == state) continue;
        state = inside[i];
        const double t = starts[i];
        long surface = 0;
        if (t == entry || t == exit) surface = nearestSurface(o.add(d.multiply(t)));
        if (!result.boundaries.add(RaySolidBoundary(t, this, false, surface)) ||
            !result.insideSegments.add(state)) {
            result.clear(); return RaySolidQueryStatus::Indeterminate;
        }
    }
    return RaySolidQueryStatus::Supported;
}

RayCrossingSupport HeightFieldSolid::emitRayCrossings(const Ray& ray, const RayCrossingQuery& query,
    RayCrossingSink& sink) const
{
    RaySolidClassification classification;
    const RaySolidQueryStatus status = classifyRay(ray, query, classification);
    if (status == RaySolidQueryStatus::InvalidInput) return RayCrossingSupport::InvalidInput;
    if (status == RaySolidQueryStatus::Unsupported) return RayCrossingSupport::Unsupported;
    if (status != RaySolidQueryStatus::Supported) return RayCrossingSupport::Indeterminate;
    for (long i = 0; i < classification.boundaries.size(); i++) {
        sink.crossing(RayCrossing(classification.boundaries[i].t, this,
            classification.insideSegments[i + 1] ? RayCrossing::Kind::Entering : RayCrossing::Kind::Leaving));
    }
    return RayCrossingSupport::Supported;
}

RayFirstHitStatus HeightFieldSolid::queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
    RayHit* hit, RayBoundary* boundary) const
{
    if (hit) hit->clear();
    if (boundary) *boundary = RayBoundary();
    if (!java::Double::isNaN(query.minT) && query.minT == query.maxT) return RayFirstHitStatus::Unsupported;
    RaySolidClassification classification;
    const RaySolidQueryStatus status = classifyRay(ray, query, classification);
    if (status == RaySolidQueryStatus::InvalidInput) return RayFirstHitStatus::InvalidInput;
    if (status == RaySolidQueryStatus::Unsupported) return RayFirstHitStatus::Unsupported;
    if (status != RaySolidQueryStatus::Supported) return RayFirstHitStatus::Indeterminate;
    if (classification.boundaries.isEmpty()) return RayFirstHitStatus::Miss;
    const RayBoundary& first = classification.boundaries[0];
    if (boundary) *boundary = first;
    if (hit) {
        hit->setHitDistance(first.t);
        if (hit->shouldStoreRay()) hit->setRay(ray.withT(first.t));
        if (hit->needsAnySurfaceData()) {
            hit->beginDetailEvaluation();
            const Vector3Dd point = ray.getOrigin().add(ray.getDirection().multiply(first.t));
            if (hit->needsPoint()) { hit->point = point; hit->markDetailAvailable(RayHit::DETAIL_POINT); }
            if (hit->needsNormal()) {
                const Vector3Dd normal = surfaceNormal(point, first.surfaceIndex);
                const double length = normal.length();
                if (java::Double::isFinite(length) && length > 0) {
                    hit->normal = normal.multiply(1.0/length);
                    hit->markDetailAvailable(RayHit::DETAIL_NORMAL);
                }
            }
        }
    }
    return RayFirstHitStatus::Hit;
}

RayFirstHitStatus HeightFieldSolid::querySurfaceDetail(const Ray& ray, const RayBoundary& boundary,
    RayHit* hit) const
{
    if (hit) hit->clear();
    if (!hit || boundary.geometry != this || !java::Double::isFinite(boundary.t) ||
        boundary.surfaceIndex < 0 || boundary.surfaceIndex > 6)
        return RayFirstHitStatus::InvalidInput;
    if (boundary.getInstanceCount() != 0) return RayFirstHitStatus::Unsupported;
    hit->setHitDistance(boundary.t);
    if (hit->shouldStoreRay()) hit->setRay(ray.withT(boundary.t));
    hit->beginDetailEvaluation();
    const Vector3Dd point = ray.getOrigin().add(ray.getDirection().multiply(boundary.t));
    if (hit->needsPoint()) { hit->point = point; hit->markDetailAvailable(RayHit::DETAIL_POINT); }
    if (hit->needsNormal()) {
        const Vector3Dd normal = surfaceNormal(point, boundary.surfaceIndex);
        const double length = normal.length();
        if (java::Double::isFinite(length) && length > 0) {
            hit->normal = normal.multiply((boundary.normalReversed ? -1.0 : 1.0)/length);
            hit->markDetailAvailable(RayHit::DETAIL_NORMAL);
        }
    }
    return RayFirstHitStatus::Hit;
}

GeometryContainment HeightFieldSolid::classifyRegion(const Vector3Dd& center,
    const Vector3Dd& halfWidths, double tolerance) const
{
    if (!isValid()) return GeometryContainment::Indeterminate;
    if (!FloatingInterval::isSupportedEnvironment()) return GeometryContainment::Unsupported;
    const FloatingInterval x = FloatingInterval::around(center.x(), halfWidths.x());
    const FloatingInterval y = FloatingInterval::around(center.y(), halfWidths.y());
    const FloatingInterval z = FloatingInterval::around(center.z(), halfWidths.z());
    if (!x.isValid() || !y.isValid() || !z.isValid()) return GeometryContainment::InvalidInput;
    if (x.getUpper() < minimum.x() || x.getLower() > maximum.x() ||
        y.getUpper() < minimum.y() || y.getLower() > maximum.y() ||
        z.getUpper() < minimum.z() || z.getLower() > maximum.z()) return GeometryContainment::Outside;
    double lowest, highest;
    heightRange(x.getLower(), x.getUpper(), z.getLower(), z.getUpper(), lowest, highest);
    // The interpolated surface lies between the heights of its cell corners.
    if (y.getLower() > highest) return GeometryContainment::Outside;
    if (x.getLower() > minimum.x() && x.getUpper() < maximum.x() &&
        y.getLower() > minimum.y() && y.getUpper() < maximum.y() &&
        z.getLower() > minimum.z() && z.getUpper() < maximum.z() && y.getUpper() < lowest)
        return GeometryContainment::Inside;
    if (!(tolerance > 0) || !(center.x() > minimum.x()) || !(center.x() < maximum.x()) ||
        !(center.z() > minimum.z()) || !(center.z() < maximum.z())) return GeometryContainment::Indeterminate;
    // A surface point straight above or below the centre within the ball.
    long cx = (long)java::Math::floor(center.x());
    long cz = (long)java::Math::floor(center.z());
    if (cx > columns - 2) cx = columns - 2;
    if (cz > rows - 2) cz = rows - 2;
    const FloatingInterval fx = FloatingInterval::exact(center.x()).subtract(FloatingInterval::exact((double)cx));
    const FloatingInterval fz = FloatingInterval::exact(center.z()).subtract(FloatingInterval::exact((double)cz));
    FloatingInterval h;
    if (fx.add(fz).getUpper() <= 1.0) {
        const FloatingInterval y1 = FloatingInterval::exact(getHeight(cx, cz));
        h = y1.add(fx.multiply(FloatingInterval::exact(getHeight(cx + 1, cz)).subtract(y1))).add(
            fz.multiply(FloatingInterval::exact(getHeight(cx, cz + 1)).subtract(y1)));
    } else if (fx.add(fz).getLower() > 1.0) {
        const FloatingInterval y4 = FloatingInterval::exact(getHeight(cx + 1, cz + 1));
        const FloatingInterval one = FloatingInterval::exact(1.0);
        h = y4.add(one.subtract(fx).multiply(FloatingInterval::exact(getHeight(cx, cz + 1)).subtract(y4))).add(
            one.subtract(fz).multiply(FloatingInterval::exact(getHeight(cx + 1, cz)).subtract(y4)));
    } else {
        return GeometryContainment::Indeterminate;
    }
    if (!h.isValid() || !(h.getLower() > minimum.y()) || !(h.getUpper() < maximum.y()))
        return GeometryContainment::Indeterminate;
    const FloatingInterval offset = h.subtract(FloatingInterval::exact(center.y()));
    if (offset.isValid() && java::Math::max(java::Math::abs(offset.getLower()),
            java::Math::abs(offset.getUpper())) < tolerance) return GeometryContainment::Boundary;
    return GeometryContainment::Indeterminate;
}

GeometryContainment HeightFieldSolid::classifyPointCertified(const Vector3Dd& point, double tolerance) const
{
    if (!java::Double::isFinite(tolerance) || tolerance < 0 || !java::Double::isFinite(point.x()) ||
        !java::Double::isFinite(point.y()) || !java::Double::isFinite(point.z()))
        return GeometryContainment::InvalidInput;
    return classifyRegion(point, Vector3Dd(tolerance, tolerance, tolerance), tolerance);
}

GeometryContainment HeightFieldSolid::classifyBoxCertified(const Vector3Dd& center,
    const Vector3Dd& halfWidths) const
{
    const GeometryContainment result = classifyRegion(center, halfWidths, -1.0);
    return result == GeometryContainment::Boundary ? GeometryContainment::Indeterminate : result;
}

GeometryContainment HeightFieldSolid::classifyPoint(const Vector3Dd& point, double tolerance) const
{
    if (!java::Double::isFinite(tolerance) || tolerance < 0) return GeometryContainment::InvalidInput;
    if (tolerance != 0) return classifyPointCertified(point, tolerance);
    if (!isValid()) return GeometryContainment::Indeterminate;
    if (point.x() < minimum.x() || point.x() > maximum.x() || point.y() < minimum.y() ||
        point.y() > maximum.y() || point.z() < minimum.z() || point.z() > maximum.z())
        return GeometryContainment::Outside;
    const double h = evaluateHeight(point.x(), point.z());
    if (!java::Double::isFinite(h)) return GeometryContainment::Indeterminate;
    if (point.y() > h) return GeometryContainment::Outside;
    if (point.y() < h && point.x() > minimum.x() && point.x() < maximum.x() && point.y() > minimum.y() &&
        point.y() < maximum.y() && point.z() > minimum.z() && point.z() < maximum.z())
        return GeometryContainment::Inside;
    return GeometryContainment::Boundary;
}

void HeightFieldSolid::getBounds(GeometryBounds& result) const
{
    if (!isValid()) { result.setIndeterminate(); return; }
    double lowest, highest;
    heightRange(minimum.x(), maximum.x(), minimum.z(), maximum.z(), lowest, highest);
    const double top = java::Math::min(maximum.y(), highest);
    if (!java::Double::isFinite(top)) { result.setIndeterminate(); return; }
    if (!(top > minimum.y())) { result.setEmpty(); return; }
    if (!result.setFinite(minimum, Vector3Dd(maximum.x(), top, maximum.z()))) result.setIndeterminate();
}

bool HeightFieldSolid::doIntersectionFirstHit(const Ray& ray, RayHit* hit)
{
    return queryFirstHit(ray, RayCrossingQuery(1e-9, java::Double::POSITIVE_INFINITY), hit) ==
        RayFirstHitStatus::Hit;
}

void HeightFieldSolid::doExtraInformation(const Ray& ray, double t, RayHit* hit)
{
    if (!hit) return;
    hit->beginDetailEvaluation();
    const Vector3Dd point = ray.getOrigin().add(ray.getDirection().multiply(t));
    if (hit->needsPoint()) { hit->point = point; hit->markDetailAvailable(RayHit::DETAIL_POINT); }
    if (hit->needsNormal()) {
        const Vector3Dd normal = surfaceNormal(point, nearestSurface(point));
        const double length = normal.length();
        if (java::Double::isFinite(length) && length > 0) {
            hit->normal = normal.multiply(1.0/length);
            hit->markDetailAvailable(RayHit::DETAIL_NORMAL);
        }
    }
}

int HeightFieldSolid::doContainmentTest(const Vector3Dd& point, double tolerance)
{
    // Legacy integer adapter over the typed contract.
    const GeometryContainment result = classifyPoint(point, tolerance);
    if (result == GeometryContainment::Inside) return INSIDE;
    if (result == GeometryContainment::Boundary) return LIMIT;
    if (result == GeometryContainment::Outside) return OUTSIDE;
    throw java::IllegalStateException("Height field containment cannot be represented by the legacy integer result");
}

double* HeightFieldSolid::getMinMax()
{
    GeometryBounds bounds;
    getBounds(bounds);
    if (bounds.getKind() == GeometryBounds::Kind::Finite)
        return new double[6]{bounds.getMinimum().x(), bounds.getMinimum().y(), bounds.getMinimum().z(),
            bounds.getMaximum().x(), bounds.getMaximum().y(), bounds.getMaximum().z()};
    return new double[6]{-1e308,-1e308,-1e308,1e308,1e308,1e308};
}
