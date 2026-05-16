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

#include "geom/HField.h"
#include "geom/Objects.h"
inline int signInline(DBL x)
{
    return (x > 0.0) ? 1 : ((x == 0.0) ? 0 : -1);
}

inline DBL minValue(DBL x, DBL y)
{
    return (x > y) ? y : x;
}

inline DBL maxValue(DBL x, DBL y)
{
    return (x < y) ? y : x;
}

Methods Height_Field_Methods = {objectIntersect, allHeightfldIntersections,
    insideHeightfld, heightFldNormal, copyHeightfld, translateHeightfld,
    rotateHeightfld, scaleHeightfld, invertHeightfld};

extern HeightField *getHeightFieldShape();

extern long rayHtFieldTests, rayHtFieldTestsSucceeded;
extern long rayHtFieldBoxTests, rayHFieldBoxTestsSucceeded;

int isdx, isdz, xDom;
DBL gdx, gdy, gdz;
DBL myx, mxz, mzx, myz;
Intersection *hfIntersection;
PriorityQueueNode *hfQueue;
Ray *rRay;

inline DBL getHeightAt(int x, int z, HeightField *hField)
{
    return (DBL)hField->Map[z][x];
}
static int
intersectPixel(
    int x, int z, Ray *ray, HeightField *hField, DBL height1, DBL height2)
{
    Vector3D t1V1;
    Vector3D t1V2;
    Vector3D t1V3;
    Vector3D t2V1;
    Vector3D t2V2;
    Vector3D t2V3;
    Vector3D localNormal;
    DBL pos1, pos2, dot, depth1, depth2, s, t, y1, y2, y3, y4;
    DBL maxHeight, minHeight;

    depth1 = HUGE_VAL;
    depth2 = HUGE_VAL;

    y1 = getHeightAt(x, z, hField);
    y2 = getHeightAt(x + 1, z, hField);
    y3 = getHeightAt(x, z + 1, hField);
    y4 = getHeightAt(x + 1, z + 1, hField);

    makeVector(&t1V1, (DBL)x, y1, (DBL)z);
    makeVector(&t1V2, 1.0, y2 - y1, 0.0);
    makeVector(&t1V3, 0.0, y3 - y1, 1.0);
    makeVector(&t2V1, (DBL)(x + 1), y4, (DBL)(z + 1));
    makeVector(&t2V2, -1.0, y3 - y4, 0.0);
    makeVector(&t2V3, 0.0, y2 - y4, -1.0);

    /*
     * first, we check to see if it is even possible for the ray to
     * intersect the triangle.
     */

    if ((maxValue(y1, maxValue(y2, y3)) >= height1) &&
        (minValue(y1, minValue(y2, y3)) <= height2)) {
        VCross(localNormal, t1V3, t1V2);
        VDot(dot, localNormal, ray->Direction);

        if ((dot > kEpsilon) || (dot < -kEpsilon)) {
            VDot(pos1, localNormal, t1V1);

            VDot(pos2, localNormal, ray->Initial);

            pos1 -= pos2;

            depth1 = pos1 / dot;

            if ((depth1 > Small_Tolerance) && (depth1 < Max_Distance)) {
                s = ray->Initial.x + (depth1 * ray->Direction.x) - (DBL)x;
                t = ray->Initial.z + (depth1 * ray->Direction.z) - (DBL)z;

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
             if((maxValue(y4,maxValue(y2,y3)) >= height1) &&
                 (minValue(y4,minValue(y2,y3)) <= height2))                */

    maxHeight = maxValue(y4, maxValue(y2, y3));
    minHeight = minValue(y4, minValue(y2, y3));
    if ((maxHeight >= height1) && (minHeight <= height2)) {
        VCross(localNormal, t2V3, t2V2);
        VDot(dot, localNormal, ray->Direction);

        if ((dot > kEpsilon) || (dot < -kEpsilon)) {
            VDot(pos1, localNormal, t2V1);

            VDot(pos2, localNormal, ray->Initial);
            pos1 -= pos2;

            depth2 = pos1 / dot;

            if ((depth2 > Small_Tolerance) && (depth2 < Max_Distance)) {
                s = ray->Initial.x + (depth2 * ray->Direction.x) - (DBL)x;
                t = ray->Initial.z + (depth2 * ray->Direction.z) - (DBL)z;

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
        VScale(t1V1, rRay->Direction, depth2);
        VAdd(t1V1, t1V1, rRay->Initial);
        hfIntersection->Point = t1V1;
        hfIntersection->Shape = (Geometry *)hField;
        hfQueue->add(hfIntersection);
    } else {
        hfIntersection->Depth = depth1;
        hfIntersection->Object = hField->Parent_Object;
        VScale(t1V1, rRay->Direction, depth1);
        VAdd(t1V1, t1V1, rRay->Initial);
        hfIntersection->Point = t1V1;
        hfIntersection->Shape = (Geometry *)hField;
        hfQueue->add(hfIntersection);
    }
    rayHtFieldTestsSucceeded++;
    return (TRUE);
}

static int
intersectSubBlock(HeightFieldBlock *block, Ray *ray, HeightField *hField,
    Vector3D *start, Vector3D *end)
{
    DBL y1, y2;
    DBL sx, sy, sz, ex, ez, f;
    int ix;
    int iz;
    int length;
    int i;

    if (minValue(start->y, end->y) > block->max_y) {
        return (FALSE);
    }

    if (maxValue(start->y, end->y) < block->min_y) {
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
            if (intersectPixel(ix, iz, ray, hField, y1, y2)) {
                return (TRUE);
            }
            f += gdz;
            if (f >= 0.0) {
                iz += isdz;
                if (intersectPixel(ix, iz, ray, hField, y1, y2)) {
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
            if (intersectPixel(ix, iz, ray, hField, y1, y2)) {
                return (TRUE);
            }
            f += gdx;
            if (f >= 0.0) {
                ix += isdx;
                if (intersectPixel(ix, iz, ray, hField, y1, y2)) {
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

static int
intersectHfNode(Ray *ray, HeightField *hField, Vector3D *start, Vector3D *end)
{
    Vector3D *curr;
    Vector3D *next;
    Vector3D *temp;
    Vector3D temp1;
    Vector3D temp2;
    DBL sx, sy, sz, ex, ey, ez, x, y, z;
    DBL tnear, tfar, t, blockSize, invBlkSize;
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
    makeVector(curr, x, y, z);
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
                    makeVector(next, x, y, z);
                    if (intersectSubBlock(&(hField->Block[ix][iz]), ray, hField,
                            curr, next)) {
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
                    makeVector(next, x, y, z);
                    if (intersectSubBlock(&(hField->Block[ix][iz]), ray, hField,
                            curr, next)) {
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
                    makeVector(next, x, y, z);
                    if (intersectSubBlock(&(hField->Block[ix][iz]), ray, hField,
                            curr, next)) {
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
                    makeVector(next, x, y, z);
                    if (intersectSubBlock(&(hField->Block[ix][iz]), ray, hField,
                            curr, next)) {
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
                    makeVector(next, x, y, z);
                    if (intersectSubBlock(&(hField->Block[ix][iz]), ray, hField,
                            curr, next)) {
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
                    makeVector(next, x, y, z);
                    if (intersectSubBlock(&(hField->Block[ix][iz]), ray, hField,
                            curr, next)) {
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
                    makeVector(next, x, y, z);
                    if (intersectSubBlock(&(hField->Block[ix][iz]), ray, hField,
                            curr, next)) {
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
                    makeVector(next, x, y, z);
                    if (intersectSubBlock(&(hField->Block[ix][iz]), ray, hField,
                            curr, next)) {
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
    makeVector(next, ex, ey, ez);
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
    if (intersectSubBlock(&(hField->Block[ix][iz]), ray, hField, curr, next)) {
        return (TRUE);
    }
    return (FALSE);
}

void
findHfMinMax(HeightField *hField, RGBAImage *image, int imageType)
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
    DBL size;
    DBL tempY = 0, blockSize, invBlkSize;

    maxX = image->iwidth;
    if (imageType == POT) {
        maxX = maxX / 2;
    }
    maxZ = image->iheight;

    size = (DBL)maxValue(maxX, maxZ);
    hField->Block_Size = blockSize = ceil(sqrt(size + 1.0));
    hField->Inv_Blk_Size = invBlkSize = 1.0 / blockSize;
    n = (int)blockSize;

    w = (int)ceil((image->width + 1.0) * invBlkSize);
    h = (int)ceil((image->height + 1.0) * invBlkSize);

    hField->Map = (float **)calloc(maxZ + 1, sizeof(float *));
    if (hField->Map == nullptr) {
        fprintf(stderr, "Cannot allocate memory for height field\n");
    }

    hField->Block = (HeightFieldBlock **)calloc(w, sizeof(HeightFieldBlock *));
    if (hField->Block == nullptr) {
        fprintf(stderr, "Cannot allocate memory for height field buffer\n");
    }
    for (i = 0; i < w; i++) {
        hField->Block[i] =
            (HeightFieldBlock *)calloc(h, sizeof(HeightFieldBlock));
        if (hField->Block[i] == nullptr) {
            fprintf(stderr,
                "Cannot allocate memory for height field buffer line\n");
        }
        for (j = 0; j < h; j++) {
            hField->Block[i][j].min_y = 65536.0;
            hField->Block[i][j].max_y = 0.0;
        }
    }

    hField->Map[0] = (float *)calloc(maxX + 1, sizeof(float));
    if (hField->Map[0] == nullptr) {
        fprintf(stderr, "Cannot allocate memory for height field\n");
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
                            tempY = (DBL)(temp1);
                            break;
                        case POT:
                            temp1 = image->data.map_lines[maxZ - z - 1][x];
                            temp2 =
                                image->data.map_lines[maxZ - z - 1][x + maxX];
                            tempY = (DBL)((DBL)temp1 + (DBL)temp2 / 256.0);
                            break;
                        case TGA:
                            temp1 = image->data.rgb_lines[maxZ - z - 1].red[x];
                            temp2 =
                                image->data.rgb_lines[maxZ - z - 1].green[x];
                            tempY = (DBL)((DBL)temp1 + (DBL)temp2 / 256.0);
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
allHeightfldIntersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    Vector3D temp1;
    Vector3D temp2;
    Ray tempRay;
    DBL depth1, depth2;
    int retVal = FALSE;
    HeightField *hField = (HeightField *)object;
    Intersection localElement;

    rayHtFieldTests++;

    MInverseTransformVector(
        &(tempRay.Initial), &(ray->Initial), hField->transformation);
    MInvTransVector(
        &(tempRay.Direction), &(ray->Direction), hField->transformation);

    if (!intersectBoxx(&tempRay, hField->bounding_box, &depth1, &depth2)) {
        return (FALSE);
    }

    /*    if(      fabs(depth1 - depth2) < Small_Tolerance) { Try kEpsilon if
     * next line doesn't work */

    if (depth1 == depth2) {
        depth1 = 0.0;
        VScale(temp1, tempRay.Direction, depth1);
        VAdd(temp1, temp1, tempRay.Initial);
        VScale(temp2, tempRay.Direction, depth2);
        VAdd(temp2, temp2, tempRay.Initial);
    } else {
        VScale(temp1, tempRay.Direction, depth1);
        VAdd(temp1, temp1, tempRay.Initial);
        VScale(temp2, tempRay.Direction, depth2);
        VAdd(temp2, temp2, tempRay.Initial);
    }

    if (fabs(tempRay.Direction.x) > kEpsilon) {
        mzx = tempRay.Direction.z / tempRay.Direction.x;
        myx = tempRay.Direction.y / tempRay.Direction.x;
    } else {
        mzx = tempRay.Direction.z / kEpsilon;
        myx = tempRay.Direction.y / kEpsilon;
    }
    if (fabs(tempRay.Direction.z) > kEpsilon) {
        mxz = tempRay.Direction.x / tempRay.Direction.z;
        myz = tempRay.Direction.y / tempRay.Direction.z;
    } else {
        mxz = tempRay.Direction.x / kEpsilon;
        myz = tempRay.Direction.y / kEpsilon;
    }

    hfQueue = depthQueue;
    hfIntersection = &localElement;
    rRay = ray;

    isdx = signInline(tempRay.Direction.x);
    isdz = signInline(tempRay.Direction.z);

    xDom = FALSE;
    if (fabs(tempRay.Direction.x) >= fabs(tempRay.Direction.z)) {
        xDom = TRUE;
    }

    gdx = fabs(mxz);
    gdz = fabs(mzx);
    if (xDom) {
        gdy = myx * (DBL)isdx;
    } else {
        gdy = myz * (DBL)isdz;
    }

    if (intersectHfNode(&tempRay, hField, &temp1, &temp2)) {
        retVal = TRUE;
    }
    return (retVal);
}

int
insideHeightfld(Vector3D *testPoint, SimpleBody *object)
{
    HeightField *hField = (HeightField *)object;
    int px;
    int pz;
    int dot1;
    int dot2;
    DBL dot1Value;
    DBL dot2Value;
    DBL x, z, y1, y2, y3;
    Vector3D localOrigin;
    Vector3D temp1;
    Vector3D temp2;
    Vector3D localNormal;
    Vector3D test;

    MInverseTransformVector(&test, testPoint, hField->transformation);

    px = (int)test.x;
    pz = (int)test.z;
    x = test.x - (DBL)px;
    z = test.z - (DBL)pz;

    if ((x + z) < 1.0) {
        y1 = getHeightAt(px, pz, hField);
        y2 = getHeightAt(px + 1, pz, hField);
        y3 = getHeightAt(px, pz + 1, hField);
        makeVector(&localOrigin, (DBL)px, y1, (DBL)pz);
        temp1.x = 1.0;
        temp1.z = 0.0;
        temp1.y = y2 - y1;
        temp2.x = 0.0;
        temp2.z = 1.0;
        temp2.y = y3 - y1;
    } else {
        px = ceil(test.x);
        pz = ceil(test.z);
        y1 = getHeightAt(px, pz, hField);
        y2 = getHeightAt(px - 1, pz, hField);
        y3 = getHeightAt(px, pz - 1, hField);
        makeVector(&localOrigin, (DBL)px, y1, (DBL)pz);
        temp1.x = -1.0;
        temp1.z = 0.0;
        temp1.y = y2 - y1;
        temp2.x = 0.0;
        temp2.z = -1.0;
        temp2.y = y3 - y1;
    }
    VCross(localNormal, temp2, temp1);
    if (localNormal.y < 0.0) {
        VScale(localNormal, localNormal, -1.0);
    }
    VDot(dot1Value, test, localNormal);
    VDot(dot2Value, localOrigin, localNormal);
    dot1 = (int)dot1Value;
    dot2 = (int)dot2Value;
    if ((dot1 < dot2) && (test.y > (hField->bounding_box->bounds[0].y) + 1.0)) {
        return (TRUE);
    }
    return (FALSE);
}

void
heightFldNormal(
    Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint)
{
    HeightField *hField = (HeightField *)object;
    int px;
    int pz;
    DBL x, z, y1, y2, y3;
    Vector3D localOrigin;
    Vector3D temp1;
    Vector3D temp2;

    MInverseTransformVector(
        &localOrigin, intersectionPoint, hField->transformation);

    px = (int)localOrigin.x;
    pz = (int)localOrigin.z;
    x = localOrigin.x - (DBL)px;
    z = localOrigin.z - (DBL)pz;

    if ((x + z) <= 1) {
        y1 = getHeightAt(px, pz, hField);
        y2 = getHeightAt(px + 1, pz, hField);
        y3 = getHeightAt(px, pz + 1, hField);
        temp1.x = 1.0;
        temp1.z = 0.0;
        temp1.y = y2 - y1;
        temp2.x = 0.0;
        temp2.z = 1.0;
        temp2.y = y3 - y1;
    } else {
        y1 = getHeightAt(px + 1, pz + 1, hField);
        y2 = getHeightAt(px, pz + 1, hField);
        y3 = getHeightAt(px + 1, pz, hField);
        temp1.x = -1.0;
        temp1.z = 0.0;
        temp1.y = y2 - y1;
        temp2.x = 0.0;
        temp2.z = -1.0;
        temp2.y = y3 - y1;
    }

    MTransVector(&temp1, &temp1, hField->transformation);
    MTransVector(&temp2, &temp2, hField->transformation);
    VCross(*result, temp2, temp1);
    VNormalize(*result, *result);
}

void *
copyHeightfld(SimpleBody *object)
{
    HeightField *newShape;

    newShape = getHeightFieldShape();
    *newShape = *((HeightField *)object);
    newShape->Next_Object = nullptr;

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture = copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
translateHeightfld(SimpleBody *object, Vector3D *vector)
{
    HeightField *hField = (HeightField *)object;
    Transformation transformation;

    if (!hField->transformation) {
        hField->transformation = getTransformation();
    }
    getTranslationTransformation(&transformation, vector);
    composeTransformations(hField->transformation, &transformation);

    translateTexture(&((HeightField *)object)->Shape_Texture, vector);
}

void
rotateHeightfld(SimpleBody *object, Vector3D *vector)
{
    Transformation transformation;
    HeightField *hField = (HeightField *)object;

    if (!hField->transformation) {
        hField->transformation = getTransformation();
    }
    getRotationTransformation(&transformation, vector);
    composeTransformations(hField->transformation, &transformation);

    rotateTexture(&((HeightField *)object)->Shape_Texture, vector);
}

void
scaleHeightfld(SimpleBody *object, Vector3D *vector)
{
    HeightField *hField = (HeightField *)object;
    Transformation transformation;

    if (!hField->transformation) {
        hField->transformation = getTransformation();
    }
    getScalingTransformation(&transformation, vector);
    composeTransformations(hField->transformation, &transformation);

    scaleTexture(&((HeightField *)object)->Shape_Texture, vector);
}

void
invertHeightfld(SimpleBody *object)
{
}
