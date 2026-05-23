/****************************************************************************
 *                         hfield.c
 *
 *         This file implements the height field shape primitive.  The shape is
 *         implemented as a collection of triangles which are calculated as
 *         needed.  The basic intersection routine first computes the rays
 *         intersection with the box marking the limits of the shape, then
 *         follows the line from one intersection point to the other, testing
 *the two triangles which form the pixel for an intersection with the ray at
 *         each step.
 *                    height field added by Doug Muir
 *                    with lots of advice and support from David Buck
 *                              and Drew Wells.
 *
 *****************************************************************************/

#include "environment/geometry/volume/HeightField.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "io/pov/SceneFactory.h"
#include "io/pov/TextureParser.h"
#include "media/Texture.h"

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

Methods Height_Field_Methods = {Composite::objectIntersect,
    HeightField::allHeightfldIntersections, HeightField::insideHeightfld,
    HeightField::heightFldNormal, HeightField::copyHeightfld,
    HeightField::translateHeightfld, HeightField::rotateHeightfld,
    HeightField::scaleHeightfld, HeightField::invertHeightfld};

extern long rayHtFieldTests, rayHtFieldTestsSucceeded;
extern long rayHtFieldBoxTests, rayHFieldBoxTestsSucceeded;

int isdx, isdz, xDom;
double gdx, gdy, gdz;
double myx, mxz, mzx, myz;
Intersection *hfIntersection;
PriorityQueueNode *hfQueue;
RayWithSegments *rRay;

double
HeightField::getHeightAt(int x, int z, HeightField *hField)
{
    return (double)hField->Map[z][x];
}
int
HeightField::intersectPixel(int x, int z, RayWithSegments *ray,
    HeightField *hField, double height1, double height2)
{
    Vector3Dd t1V1;
    Vector3Dd t1V2;
    Vector3Dd t1V3;
    Vector3Dd t2V1;
    Vector3Dd t2V2;
    Vector3Dd t2V3;
    Vector3Dd localNormal;
    double pos1, pos2, dot, depth1, depth2, s, t, y1, y2, y3, y4;
    double maxHeight, minHeight;

    depth1 = HUGE_VAL;
    depth2 = HUGE_VAL;

    y1 = HeightField::getHeightAt(x, z, hField);
    y2 = HeightField::getHeightAt(x + 1, z, hField);
    y3 = HeightField::getHeightAt(x, z + 1, hField);
    y4 = HeightField::getHeightAt(x + 1, z + 1, hField);

    VectorOps::makeVector(&t1V1, (double)x, y1, (double)z);
    *&t1V2 = Vector3Dd(1.0, y2 - y1, 0.0);
    *&t1V3 = Vector3Dd(0.0, y3 - y1, 1.0);
    VectorOps::makeVector(&t2V1, (double)(x + 1), y4, (double)(z + 1));
    *&t2V2 = Vector3Dd(-1.0, y3 - y4, 0.0);
    *&t2V3 = Vector3Dd(0.0, y2 - y4, -1.0);

    /*
     * first, we check to see if it is even possible for the ray to
     * intersect the triangle.
     */

    if ((HeightField::maxValue(y1, HeightField::maxValue(y2, y3)) >= height1) &&
        (HeightField::minValue(y1, HeightField::minValue(y2, y3)) <= height2)) {
        localNormal = t1V3.crossProduct(t1V2);
        dot = localNormal.dotProduct(ray->direction);

        if ((dot > kEpsilon) || (dot < -kEpsilon)) {
            pos1 = localNormal.dotProduct(t1V1);

            pos2 = localNormal.dotProduct(ray->position);

            pos1 -= pos2;

            depth1 = pos1 / dot;

            if ((depth1 > Small_Tolerance) && (depth1 < Max_Distance)) {
                s = ray->position.x + (depth1 * ray->direction.x) - (double)x;
                t = ray->position.z + (depth1 * ray->direction.z) - (double)z;

                if ((s < -kEpsilon) || (t < -kEpsilon) ||
                    ((s + t) > 1.0 + kEpsilon)) {
                    depth1 = HUGE_VAL;
                }
            } else {
                depth1 = HUGE_VAL;
            }
        }
    }

    /*
     * first, we check to see if it is even possible for the ray to
     * intersect the triangle.
        Rewritten to get around Code Builder FP stack problem.
        Original code:
             if((HeightField::maxValue(y4,HeightField::maxValue(y2,y3)) >=
     height1) && (HeightField::minValue(y4,HeightField::minValue(y2,y3)) <=
     height2))                */

    maxHeight = HeightField::maxValue(y4, HeightField::maxValue(y2, y3));
    minHeight = HeightField::minValue(y4, HeightField::minValue(y2, y3));
    if ((maxHeight >= height1) && (minHeight <= height2)) {
        localNormal = t2V3.crossProduct(t2V2);
        dot = localNormal.dotProduct(ray->direction);

        if ((dot > kEpsilon) || (dot < -kEpsilon)) {
            pos1 = localNormal.dotProduct(t2V1);

            pos2 = localNormal.dotProduct(ray->position);
            pos1 -= pos2;

            depth2 = pos1 / dot;

            if ((depth2 > Small_Tolerance) && (depth2 < Max_Distance)) {
                s = ray->position.x + (depth2 * ray->direction.x) - (double)x;
                t = ray->position.z + (depth2 * ray->direction.z) - (double)z;

                if ((s > 1.0 + kEpsilon) || (t > 1.0 + kEpsilon) ||
                    ((s + t) < 1.0 - kEpsilon)) {
                    depth2 = HUGE_VAL;
                }
            } else {
                depth2 = HUGE_VAL;
            }
        }
    }

    if ((depth1 >= Max_Distance) && (depth2 >= Max_Distance)) {
        return (FALSE);
    }

    if (depth2 < depth1) {
        hfIntersection->Depth = depth2;
        hfIntersection->Object = hField->Parent_Object;
        VectorOps::vScale(t1V1, rRay->direction, depth2);
        t1V1.add(rRay->position);
        hfIntersection->Point = t1V1;
        hfIntersection->Shape = (Geometry *)hField;
        hfQueue->add(hfIntersection);
    } else {
        hfIntersection->Depth = depth1;
        hfIntersection->Object = hField->Parent_Object;
        VectorOps::vScale(t1V1, rRay->direction, depth1);
        t1V1.add(rRay->position);
        hfIntersection->Point = t1V1;
        hfIntersection->Shape = (Geometry *)hField;
        hfQueue->add(hfIntersection);
    }
    rayHtFieldTestsSucceeded++;
    return (TRUE);
}

int
HeightField::intersectSubBlock(HeightFieldBlock *block, RayWithSegments *ray,
    HeightField *hField, Vector3Dd *start, Vector3Dd *end)
{
    double y1, y2;
    double sx, sy, sz, ex, ez, f;
    int ix;
    int iz;
    int length;
    int i;

    if (HeightField::minValue(start->y, end->y) > block->max_y) {
        return (FALSE);
    }

    if (HeightField::maxValue(start->y, end->y) < block->min_y) {
        return (FALSE);
    }

    sx = start->x;
    sy = start->y;
    sz = start->z;
    ex = end->x;
    ez = end->z;

    if (xDom) {
        if (isdx >= 0) {
            f = floor(sx) - sx;
            sx = floor(sx);
            sy += myx * f;
            sz += mzx * f;
            ex = floor(ex);
            ix = (int)sx;
        } else {
            f = ceil(sx) - sx;
            sx = ceil(sx);
            sy += myx * f;
            sz += mzx * f;
            ex = ceil(ex);
            ix = (int)sx - 1;
        }

        length = 0.5 + fabs(ex - sx);

        if (isdz >= 0) {
            f = sz - ceil(sz);
            iz = (int)floor(sz);
        } else {
            f = floor(sz) - sz;
            iz = (int)ceil(sz) - 1;
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
                return (TRUE);
            }
            f += gdz;
            if (f >= 0.0) {
                iz += isdz;
                if (HeightField::intersectPixel(ix, iz, ray, hField, y1, y2)) {
                    return (TRUE);
                }
                f -= 1.0;
            }
            ix += isdx;
            y1 += gdy;
            y2 += gdy;
        }
    } else {

        if (isdz >= 0) {
            f = floor(sz) - sz;
            sz = floor(sz);
            sy += myz * f;
            sx += mxz * f;
            ez = floor(ez);
            iz = (int)sz;
        } else {
            f = ceil(sz) - sz;
            sz = ceil(sz);
            sy += myz * f;
            sx += mxz * f;
            ez = ceil(ez);
            iz = (int)sz - 1;
        }

        length = 0.5 + fabs(ez - sz);

        if (isdx >= 0) {
            f = sx - ceil(sx);
            ix = (int)floor(sx);
        } else {
            f = floor(sx) - sx;
            ix = (int)ceil(sx) - 1;
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
                return (TRUE);
            }
            f += gdx;
            if (f >= 0.0) {
                ix += isdx;
                if (HeightField::intersectPixel(ix, iz, ray, hField, y1, y2)) {
                    return (TRUE);
                }
                f -= 1.0;
            }
            iz += isdz;
            y1 += gdy;
            y2 += gdy;
        }
    }
    return (FALSE);
}

int
HeightField::intersectHfNode(
    RayWithSegments *ray, HeightField *hField, Vector3Dd *start, Vector3Dd *end)
{
    Vector3Dd *curr;
    Vector3Dd *next;
    Vector3Dd *temp;
    Vector3Dd temp1;
    Vector3Dd temp2;
    double sx, sy, sz, ex, ey, ez, x, y, z;
    double tnear, tfar, t, blockSize, invBlkSize;
    int ix;
    int iz;
    int xSize;
    int zSize;
    int length;
    int i;

    x = sx = start->x;
    y = sy = start->y;
    z = sz = start->z;
    ex = end->x;
    ey = end->y;
    ez = end->z;

    blockSize = hField->Block_Size;
    invBlkSize = hField->Inv_Blk_Size;

    xSize = abs((int)(ex * invBlkSize) - (int)(sx * invBlkSize));
    zSize = abs((int)(ez * invBlkSize) - (int)(sz * invBlkSize));
    length = xSize + zSize;

    curr = &temp1;
    next = &temp2;
    *curr = Vector3Dd(x, y, z);
    t = 0.0;

    if (xDom) {
        if (isdx >= 0) {
            ix = (int)floor(sx * invBlkSize);
            tnear = blockSize * (ix + 1) - sx;

            if (isdz >= 0) {
                iz = (int)floor(sz * invBlkSize);
                tfar = gdx * (blockSize * (iz + 1) - sz);
            } else {
                iz = (int)ceil(sz * invBlkSize) - 1;
                tfar = gdx * (sz - blockSize * (iz));
            }
            for (i = 0; i < length; i++) {
                if (tnear < tfar) {
                    t = tnear;
                    x = sx + t;
                    y = sy + myx * t;
                    z = sz + mzx * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->Block[ix][iz]),
                            ray, hField, curr, next)) {
                        return (TRUE);
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
                    if (HeightField::intersectSubBlock(&(hField->Block[ix][iz]),
                            ray, hField, curr, next)) {
                        return (TRUE);
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
            ix = (int)ceil(sx * invBlkSize) - 1;
            tnear = sx - blockSize * (ix);

            if (isdz >= 0) {
                iz = (int)floor(sz * invBlkSize);
                tfar = gdx * (blockSize * (iz + 1) - sz);
            } else {
                iz = (int)ceil(sz * invBlkSize) - 1;
                tfar = gdx * (sz - blockSize * (iz));
            }

            for (i = 0; i < length; i++) {
                if (tnear < tfar) {
                    t = tnear;
                    x = sx - t;
                    y = sy - myx * t;
                    z = sz - mzx * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->Block[ix][iz]),
                            ray, hField, curr, next)) {
                        return (TRUE);
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
                    if (HeightField::intersectSubBlock(&(hField->Block[ix][iz]),
                            ray, hField, curr, next)) {
                        return (TRUE);
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
            iz = (int)floor(sz * invBlkSize);
            tnear = blockSize * (iz + 1) - sz;

            if (isdx >= 0) {
                ix = (int)floor(sx * invBlkSize);
                tfar = gdz * (blockSize * (ix + 1) - sx);
            } else {
                ix = (int)ceil(sx * invBlkSize) - 1;
                tfar = gdz * (sx - blockSize * (ix));
            }
            for (i = 0; i < length; i++) {
                if (tnear < tfar) {
                    t = tnear;
                    z = sz + t;
                    y = sy + myz * t;
                    x = sx + mxz * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->Block[ix][iz]),
                            ray, hField, curr, next)) {
                        return (TRUE);
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
                    if (HeightField::intersectSubBlock(&(hField->Block[ix][iz]),
                            ray, hField, curr, next)) {
                        return (TRUE);
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
            iz = (int)ceil(sz * invBlkSize) - 1;
            tnear = sz - blockSize * (iz);

            if (isdx >= 0) {
                ix = (int)floor(sx * invBlkSize);
                tfar = gdz * (blockSize * (ix + 1) - sx);
            } else {
                ix = (int)ceil(sx * invBlkSize) - 1;
                tfar = gdz * (sx - blockSize * (ix));
            }
            for (i = 0; i < length; i++) {
                if (tnear < tfar) {
                    t = tnear;
                    z = sz - t;
                    y = sy - myz * t;
                    x = sx - mxz * t;
                    *next = Vector3Dd(x, y, z);
                    if (HeightField::intersectSubBlock(&(hField->Block[ix][iz]),
                            ray, hField, curr, next)) {
                        return (TRUE);
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
                    if (HeightField::intersectSubBlock(&(hField->Block[ix][iz]),
                            ray, hField, curr, next)) {
                        return (TRUE);
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
        ix = (int)floor(ex * invBlkSize);
    } else {
        ix = (int)ceil(ex * invBlkSize) - 1;
    }
    if (isdz >= 0) {
        iz = (int)floor(ez * invBlkSize);
    } else {
        iz = (int)ceil(ez * invBlkSize) - 1;
    }
    if (HeightField::intersectSubBlock(
            &(hField->Block[ix][iz]), ray, hField, curr, next)) {
        return (TRUE);
    }
    return (FALSE);
}

void
HeightField::findHfMinMax(HeightField *hField, RGBAImage *image, int imageType)
{
    int n;
    int i;
    int i2;
    int j;
    int j2;
    int x;
    int z;
    int w;
    int h;
    int maxX;
    int maxZ;
    int temp1;
    int temp2;
    double size;
    double tempY = 0, blockSize, invBlkSize;

    maxX = image->iwidth;
    if (imageType == POT) {
        maxX = maxX / 2;
    }
    maxZ = image->iheight;

    size = (double)HeightField::maxValue(maxX, maxZ);
    hField->Block_Size = blockSize = ceil(sqrt(size + 1.0));
    hField->Inv_Blk_Size = invBlkSize = 1.0 / blockSize;
    n = (int)blockSize;

    w = (int)ceil((image->width + 1.0) * invBlkSize);
    h = (int)ceil((image->height + 1.0) * invBlkSize);

    hField->Map = (float **)calloc(maxZ + 1, sizeof(float *));
    if (hField->Map == nullptr) {
        Logger::error( "Cannot allocate memory for height field\n");
    }

    hField->Block = (HeightFieldBlock **)calloc(w, sizeof(HeightFieldBlock *));
    if (hField->Block == nullptr) {
        Logger::error( "Cannot allocate memory for height field buffer\n");
    }
    for (i = 0; i < w; i++) {
        hField->Block[i] =
            (HeightFieldBlock *)calloc(h, sizeof(HeightFieldBlock));
        if (hField->Block[i] == nullptr) {
            Logger::error(
                "Cannot allocate memory for height field buffer line\n");
        }
        for (j = 0; j < h; j++) {
            hField->Block[i][j].min_y = 65536.0;
            hField->Block[i][j].max_y = 0.0;
        }
    }

    hField->Map[0] = (float *)calloc(maxX + 1, sizeof(float));
    if (hField->Map[0] == nullptr) {
        Logger::error( "Cannot allocate memory for height field\n");
    }

    for (j = 0; j < h; j++) {
        for (j2 = 0; (j2 <= n) && (j * n + j2 <= maxZ); j2++) {
            z = j * n + j2;
            if (j2 != 0) {
                hField->Map[z] = (float *)calloc(maxX + 1, sizeof(float));
                if (hField->Map[z] == nullptr) {
                    fprintf(
                        stderr, "Cannot allocate memory for height field\n");
                }
            }
            for (i = 0; i < w; i++) {
                for (i2 = 0; (i2 <= n) && (i * n + i2 <= maxX); i2++) {
                    x = i * n + i2;
                    if ((x > 1) && (x < maxX - 1) && (z > 1) &&
                        (z < maxZ - 1)) {
                        switch (imageType) {
                        case GIF:
                            temp1 = image->data.map_lines[maxZ - z - 1][x];
                            tempY = (double)(temp1);
                            break;
                        case POT:
                            temp1 = image->data.map_lines[maxZ - z - 1][x];
                            temp2 =
                                image->data.map_lines[maxZ - z - 1][x + maxX];
                            tempY =
                                (double)((double)temp1 + (double)temp2 / 256.0);
                            break;
                        case TGA:
                            temp1 = image->data.rgb_lines[maxZ - z - 1].red[x];
                            temp2 =
                                image->data.rgb_lines[maxZ - z - 1].green[x];
                            tempY =
                                (double)((double)temp1 + (double)temp2 / 256.0);
                            break;
                        }
                        if (tempY <= hField->bounding_box->bounds[0].y) {
                            hField->Map[z][x] = -10000.0;
                        } else {
                            hField->Map[z][x] = (float)tempY;
                        }
                    } else {
                        tempY = -10000.0;
                        hField->Map[z][x] = (float)tempY;
                    }

                    if (tempY < hField->bounding_box->bounds[0].y) {
                        tempY = hField->bounding_box->bounds[0].y;
                    }
                    if (tempY < hField->Block[i][j].min_y) {
                        hField->Block[i][j].min_y = tempY;
                    }
                    if (tempY > hField->Block[i][j].max_y) {
                        hField->Block[i][j].max_y = tempY;
                    }
                }
            }
            if ((z >= 0) && (z < maxZ) && (j2 != n)) {
                switch (imageType) {
                case GIF:
                    delete image->data.map_lines[maxZ - z - 1];
                    break;
                case POT:
                    delete image->data.map_lines[maxZ - z - 1];
                    break;
                case TGA:
                    delete image->data.rgb_lines[maxZ - z - 1].blue;
                    delete image->data.rgb_lines[maxZ - z - 1].green;
                    delete image->data.rgb_lines[maxZ - z - 1].red;
                    break;
                }
            }
        }
    }
}

int
HeightField::allHeightfldIntersections(
    SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue)
{
    Vector3Dd temp1;
    Vector3Dd temp2;
    RayWithSegments tempRay;
    double depth1, depth2;
    int retVal = FALSE;
    HeightField *hField = (HeightField *)object;
    Intersection localElement;

    rayHtFieldTests++;

    Transformation::MInverseTransformVector(
        &(tempRay.position), &(ray->position), hField->transformation);
    Transformation::MInvTransVector(
        &(tempRay.direction), &(ray->direction), hField->transformation);

    if (!Box::intersectBoxx(&tempRay, hField->bounding_box, &depth1, &depth2)) {
        return (FALSE);
    }

    /*    if(      fabs(depth1 - depth2) < Small_Tolerance) { Try kEpsilon if
     * next line doesn't work */

    if (depth1 == depth2) {
        depth1 = 0.0;
        VectorOps::vScale(temp1, tempRay.direction, depth1);
        temp1.add(tempRay.position);
        VectorOps::vScale(temp2, tempRay.direction, depth2);
        temp2.add(tempRay.position);
    } else {
        VectorOps::vScale(temp1, tempRay.direction, depth1);
        temp1.add(tempRay.position);
        VectorOps::vScale(temp2, tempRay.direction, depth2);
        temp2.add(tempRay.position);
    }

    if (fabs(tempRay.direction.x) > kEpsilon) {
        mzx = tempRay.direction.z / tempRay.direction.x;
        myx = tempRay.direction.y / tempRay.direction.x;
    } else {
        mzx = tempRay.direction.z / kEpsilon;
        myx = tempRay.direction.y / kEpsilon;
    }
    if (fabs(tempRay.direction.z) > kEpsilon) {
        mxz = tempRay.direction.x / tempRay.direction.z;
        myz = tempRay.direction.y / tempRay.direction.z;
    } else {
        mxz = tempRay.direction.x / kEpsilon;
        myz = tempRay.direction.y / kEpsilon;
    }

    hfQueue = depthQueue;
    hfIntersection = &localElement;
    rRay = ray;

    isdx = HeightField::signInline(tempRay.direction.x);
    isdz = HeightField::signInline(tempRay.direction.z);

    xDom = FALSE;
    if (fabs(tempRay.direction.x) >= fabs(tempRay.direction.z)) {
        xDom = TRUE;
    }

    gdx = fabs(mxz);
    gdz = fabs(mzx);
    if (xDom) {
        gdy = myx * (double)isdx;
    } else {
        gdy = myz * (double)isdz;
    }

    if (HeightField::intersectHfNode(&tempRay, hField, &temp1, &temp2)) {
        retVal = TRUE;
    }
    return (retVal);
}

int
HeightField::insideHeightfld(Vector3Dd *testPoint, SimpleBody *object)
{
    HeightField *hField = (HeightField *)object;
    int px;
    int pz;
    int dot1;
    int dot2;
    double dot1Value;
    double dot2Value;
    double x, z, y1, y2, y3;
    Vector3Dd localOrigin;
    Vector3Dd temp1;
    Vector3Dd temp2;
    Vector3Dd localNormal;
    Vector3Dd test;

    Transformation::MInverseTransformVector(
        &test, testPoint, hField->transformation);

    px = (int)test.x;
    pz = (int)test.z;
    x = test.x - (double)px;
    z = test.z - (double)pz;

    if ((x + z) < 1.0) {
        y1 = HeightField::getHeightAt(px, pz, hField);
        y2 = HeightField::getHeightAt(px + 1, pz, hField);
        y3 = HeightField::getHeightAt(px, pz + 1, hField);
        VectorOps::makeVector(&localOrigin, (double)px, y1, (double)pz);
        temp1.x = 1.0;
        temp1.z = 0.0;
        temp1.y = y2 - y1;
        temp2.x = 0.0;
        temp2.z = 1.0;
        temp2.y = y3 - y1;
    } else {
        px = ceil(test.x);
        pz = ceil(test.z);
        y1 = HeightField::getHeightAt(px, pz, hField);
        y2 = HeightField::getHeightAt(px - 1, pz, hField);
        y3 = HeightField::getHeightAt(px, pz - 1, hField);
        VectorOps::makeVector(&localOrigin, (double)px, y1, (double)pz);
        temp1.x = -1.0;
        temp1.z = 0.0;
        temp1.y = y2 - y1;
        temp2.x = 0.0;
        temp2.z = -1.0;
        temp2.y = y3 - y1;
    }
    localNormal = temp2.crossProduct(temp1);
    if (localNormal.y < 0.0) {
        localNormal.scale(-1.0);
    }
    dot1Value = test.dotProduct(localNormal);
    dot2Value = localOrigin.dotProduct(localNormal);
    dot1 = (int)dot1Value;
    dot2 = (int)dot2Value;
    if ((dot1 < dot2) && (test.y > (hField->bounding_box->bounds[0].y) + 1.0)) {
        return (TRUE);
    }
    return (FALSE);
}

void
HeightField::heightFldNormal(
    Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint)
{
    HeightField *hField = (HeightField *)object;
    int px;
    int pz;
    double x, z, y1, y2, y3;
    Vector3Dd localOrigin;
    Vector3Dd temp1;
    Vector3Dd temp2;

    Transformation::MInverseTransformVector(
        &localOrigin, intersectionPoint, hField->transformation);

    px = (int)localOrigin.x;
    pz = (int)localOrigin.z;
    x = localOrigin.x - (double)px;
    z = localOrigin.z - (double)pz;

    if ((x + z) <= 1) {
        y1 = HeightField::getHeightAt(px, pz, hField);
        y2 = HeightField::getHeightAt(px + 1, pz, hField);
        y3 = HeightField::getHeightAt(px, pz + 1, hField);
        temp1.x = 1.0;
        temp1.z = 0.0;
        temp1.y = y2 - y1;
        temp2.x = 0.0;
        temp2.z = 1.0;
        temp2.y = y3 - y1;
    } else {
        y1 = HeightField::getHeightAt(px + 1, pz + 1, hField);
        y2 = HeightField::getHeightAt(px, pz + 1, hField);
        y3 = HeightField::getHeightAt(px + 1, pz, hField);
        temp1.x = -1.0;
        temp1.z = 0.0;
        temp1.y = y2 - y1;
        temp2.x = 0.0;
        temp2.z = -1.0;
        temp2.y = y3 - y1;
    }

    Transformation::MTransVector(&temp1, &temp1, hField->transformation);
    Transformation::MTransVector(&temp2, &temp2, hField->transformation);
    *result = temp2.crossProduct(temp1);
    (*result).normalize();
}

void *
HeightField::copyHeightfld(SimpleBody *object)
{
    HeightField *newShape;

    newShape = SceneFactory::getHeightFieldShape();
    *newShape = *((HeightField *)object);
    newShape->Next_Object = nullptr;

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture =
            TextureParser::copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
HeightField::translateHeightfld(SimpleBody *object, Vector3Dd *vector)
{
    HeightField *hField = (HeightField *)object;
    Transformation transformation;

    if (!hField->transformation) {
        hField->transformation = Transformation::getTransformation();
    }
    Transformation::getTranslationTransformation(&transformation, vector);
    Transformation::composeTransformations(
        hField->transformation, &transformation);

    TextureUtils::translateTexture(
        &((HeightField *)object)->Shape_Texture, vector);
}

void
HeightField::rotateHeightfld(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transformation;
    HeightField *hField = (HeightField *)object;

    if (!hField->transformation) {
        hField->transformation = Transformation::getTransformation();
    }
    Transformation::getRotationTransformation(&transformation, vector);
    Transformation::composeTransformations(
        hField->transformation, &transformation);

    TextureUtils::rotateTexture(
        &((HeightField *)object)->Shape_Texture, vector);
}

void
HeightField::scaleHeightfld(SimpleBody *object, Vector3Dd *vector)
{
    HeightField *hField = (HeightField *)object;
    Transformation transformation;

    if (!hField->transformation) {
        hField->transformation = Transformation::getTransformation();
    }
    Transformation::getScalingTransformation(&transformation, vector);
    Transformation::composeTransformations(
        hField->transformation, &transformation);

    TextureUtils::scaleTexture(&((HeightField *)object)->Shape_Texture, vector);
}

void
HeightField::invertHeightfld(SimpleBody *object)
{
}
