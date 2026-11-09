//= References:                                                             =
//= [KAUF1987] Kaufman, Arie. "Efficient Algorithms for 3D Scan-Conversion  =
//=     of Parametric Curves, Surfaces, and Volumes", ACM SIGGRAPH Computer =
//=     Graphics, volume 21, number 4, July 1987.                           =
//= [AMAN1987] Amanatides, John. Woo, Andrew. "A Fast Voxel Traversal      =
//=     Algorithm for Ray Tracing", Eurographics '87, 1987.                 =
//= [SNYD1987] Snyder, John. Barr, Alan. "Ray Tracing Complex Models       =
//=     Containing Surface Tessellations", SIGGRAPH '87, p. 119-128, 1987.  =

#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include <cmath>

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/media/IndexedColorImageUncompressed.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/volume/VoxelVolume.h"
const double VoxelVolume::PARALLEL_EPSILON = 1.0e-12;

VoxelVolume::VoxelVolume()
    : data(nullptr), xSize(0), ySize(0), zSize(0), threshold(DEFAULT_THRESHOLD) {}

VoxelVolume::~VoxelVolume() {
    if (data != nullptr) {
        for (int z = 0; z < zSize; z++) {
            delete data[z];
        }
        delete[] data;
    }
}

int VoxelVolume::getXSize() const { return xSize; }
int VoxelVolume::getYSize() const { return ySize; }
int VoxelVolume::getZSize() const { return zSize; }

int VoxelVolume::getThreshold() const
{
    return threshold;
}

void VoxelVolume::setThreshold(int threshold)
{
    this->threshold = threshold;
}

bool VoxelVolume::isFilled(int x, int y, int z) const
{
    if ( x < 0 || y < 0 || z < 0 || x >= getXSize() ||
         y >= getYSize() || z >= getZSize() ) {
        return false;
    }
    return getVoxel(x, y, z) >= threshold;
}

bool VoxelVolume::init(int xs, int ys, int zs) {
    if (xs <= 0 || ys <= 0 || zs <= 0) return false;

    if (data != nullptr) {
        for (int z = 0; z < zSize; z++) delete data[z];
        delete[] data;
        data = nullptr;
    }

    data = new IndexedColorImageUncompressed*[zs];
    xSize = xs; ySize = ys; zSize = zs;

    for (int z = 0; z < zSize; z++) {
        data[z] = new IndexedColorImageUncompressed();
        if (!data[z]->init(xSize, ySize)) {
            for (int i = 0; i <= z; i++) delete data[i];
            delete[] data;
            data = nullptr;
            xSize = ySize = zSize = 0;
            return false;
        }
    }
    return true;
}

void VoxelVolume::putVoxel(int x, int y, int z, char val) {
    if (data == nullptr) return;
    if (x < 0 || x >= xSize || y < 0 || y >= ySize || z < 0 || z >= zSize) return;
    data[z]->putPixel(x, y, val);
}

int VoxelVolume::getVoxel(int x, int y, int z) const {
    if (data == nullptr) return 0;
    if (x < 0 || x >= xSize || y < 0 || y >= ySize || z < 0 || z >= zSize) return 0;
    return (unsigned char)data[z]->getPixel(x, y);
}

Vector3Dd VoxelVolume::getVoxelPosition(int x, int y, int z) const {
    return Vector3Dd(
        (((double)x + 0.5) / (double)xSize) * 2 - 1,
        (((double)y + 0.5) / (double)ySize) * 2 - 1,
        (((double)z + 0.5) / (double)zSize) * 2 - 1);
}

int VoxelVolume::getNearestIFromX(double x) const { return (int)(((x + 1)/2) * ((double)xSize) - 0.5); }
int VoxelVolume::getNearestJFromY(double y) const { return (int)(((y + 1)/2) * ((double)ySize) - 0.5); }
int VoxelVolume::getNearestKFromZ(double z) const { return (int)(((z + 1)/2) * ((double)zSize) - 0.5); }

int VoxelVolume::getVoxelAtPosition(double x, double y, double z) const {
    if (x < -1 || x > 1 || y < -1 || y > 1 || z < -1 || z > 1) return 0;
    return getVoxel(getNearestIFromX(x), getNearestJFromY(y), getNearestKFromZ(z));
}

int VoxelVolume::getVoxelAtPosition(const Vector3Dd& p) const {
    return getVoxelAtPosition(p.x(), p.y(), p.z());
}

void VoxelVolume::putVoxelAtPosition(double x, double y, double z, char val) {
    if (x < -1 || x > 1 || y < -1 || y > 1 || z < -1 || z > 1) return;
    putVoxel(getNearestIFromX(x), getNearestJFromY(y), getNearestKFromZ(z), val);
}

void VoxelVolume::putVoxelAtPosition(const Vector3Dd& p, char val) {
    putVoxelAtPosition(p.x(), p.y(), p.z(), val);
}

double* VoxelVolume::getMinMax() {
    double* m = new double[6];
    m[0]=-1; m[1]=-1; m[2]=-1; m[3]=1; m[4]=1; m[5]=1;
    return m;
}

/**
Check the general interface contract in superclass method
Geometry.doIntersectionFirstHit.
@param inOutRay
@return the ray with the distance to the first filled voxel hit, or null
if no filled voxel is hit
*/
Ray* VoxelVolume::doIntersectionFirstHit(const Ray& inOutRay) {
    RayHit hit;
    if (doIntersectionFirstHit(inOutRay, &hit) && hit.getRay() != nullptr) {
        return new Ray(*hit.getRay());
    }
    return nullptr;
}

/**
Check the general interface contract in superclass method
Geometry.doIntersectionFirstHit. The volume is seen as the union of the
cubes of its filled voxels (see `isFilled`), and the ray hits the first
face through which it enters one of them. A ray starting inside a filled
voxel does not hit the voxels it leaves: as with the other solids, only
the surface it enters counts.

The voxels are visited in the order the ray crosses them, as the uniform
grid traversal of [SNYD1987] (with the incremental formulation of
[AMAN1987]), so the cost grows with the number of voxels crossed, not
with the number of voxels of the volume.
@param inRay ray in the space of the volume
@param outHit receives the hit, or null if only the test is needed
@return true if the ray hits a filled voxel
*/
bool VoxelVolume::doIntersectionFirstHit(const Ray& inRay, RayHit* outHit)
{
    VoxelFaceHit hit;

    if ( !traceFirstFilledVoxel(inRay, hit) ) {
        return false;
    }
    if ( outHit != nullptr ) {
        if ( outHit->shouldStoreRay() || outHit->needsAnySurfaceData() ) {
            Ray hitRay = inRay.withT(hit.t);
            outHit->setRay(hitRay);
            if ( outHit->needsAnySurfaceData() ) {
                fillSurfaceData(hitRay, hit, outHit);
                outHit->setRay(hitRay);
            }
        }
        else {
            outHit->setHitDistance(hit.t);
        }
    }
    return true;
}

/**
Check the general interface contract in superclass method
Geometry.doExtraInformation.
@param inRay ray (in the space of the volume) whose first hit is described
@param inT distance to the hit
@param outData receives the point, normal, texture coordinates and
tangent of the hit
*/
void VoxelVolume::doExtraInformation(const Ray& inRay, double /*inT*/,
                                     RayHit* outData)
{
    VoxelFaceHit hit;

    if ( outData != nullptr && traceFirstFilledVoxel(inRay, hit) ) {
        fillSurfaceData(inRay.withT(hit.t), hit, outData);
    }
}

/**
Finds the face of the first filled voxel entered by the ray: clips the
ray against the cube <-1, -1, -1>-<1, 1, 1> and visits the voxels it
crosses, as `VoxelGrid.gridIntersect` of [SNYD1987]. For each axis,
`tNext` is the distance to the next voxel boundary along that axis and
`tDelta` the distance between two boundaries; each step moves to the
voxel whose boundary is nearest.
@param ray ray in the space of the volume
@param outHit receives the face entered
@return false if the ray enters no filled voxel (Java returns null)
*/
bool VoxelVolume::traceFirstFilledVoxel(const Ray& ray,
                                        VoxelFaceHit& outHit) const
{
    int size[3] = { getXSize(), getYSize(), getZSize() };

    if ( size[0] <= 0 || size[1] <= 0 || size[2] <= 0 ) {
        return false;
    }

    Vector3Dd o = ray.getOrigin();
    Vector3Dd d = ray.getDirection();
    double origin[3] = { o.x(), o.y(), o.z() };
    double direction[3] = { d.x(), d.y(), d.z() };
    double cellSize[3];
    double tEnter = -java::Double::POSITIVE_INFINITY;
    double tExit = java::Double::POSITIVE_INFINITY;
    int enterAxis = 0;
    int i;

    //- Clip the ray against the volume cube (slabs method) ------------
    for ( i = 0; i < 3; i++ ) {
        cellSize[i] = 2.0 / size[i];
        if ( std::abs(direction[i]) < PARALLEL_EPSILON ) {
            if ( origin[i] < -1.0 || origin[i] > 1.0 ) {
                return false;
            }
            continue;
        }
        double t1 = (-1.0 - origin[i]) / direction[i];
        double t2 = (1.0 - origin[i]) / direction[i];
        double tNear = java::Math::min(t1, t2);
        double tFar = java::Math::max(t1, t2);
        if ( tNear > tEnter ) {
            tEnter = tNear;
            enterAxis = i;
        }
        tExit = java::Math::min(tExit, tFar);
    }
    if ( tExit < java::Math::max(tEnter, 0.0) ) {
        return false;
    }
    bool startsInside = tEnter < 0.0;
    double t = startsInside ? 0.0 : tEnter;

    //- Setup of the traversal, from the voxel where the ray starts ----
    int g[3];
    int step[3];
    double tDelta[3];
    double tNext[3];

    for ( i = 0; i < 3; i++ ) {
        double p = origin[i] + t * direction[i];
        g[i] = (int)std::floor((p + 1.0) / cellSize[i]);
        if ( g[i] < 0 ) {
            g[i] = 0;
        }
        if ( g[i] >= size[i] ) {
            g[i] = size[i] - 1;
        }
        if ( direction[i] > PARALLEL_EPSILON ) {
            step[i] = 1;
            tDelta[i] = cellSize[i] / direction[i];
            tNext[i] = (-1.0 + (g[i] + 1) * cellSize[i] - origin[i]) / direction[i];
        }
        else if ( direction[i] < -PARALLEL_EPSILON ) {
            step[i] = -1;
            tDelta[i] = cellSize[i] / -direction[i];
            tNext[i] = (-1.0 + g[i] * cellSize[i] - origin[i]) / direction[i];
        }
        else {
            step[i] = 0;
            tDelta[i] = java::Double::POSITIVE_INFINITY;
            tNext[i] = java::Double::POSITIVE_INFINITY;
        }
    }

    bool previousFilled = isFilled(g[0], g[1], g[2]);
    if ( previousFilled && !startsInside ) {
        outHit.t = t;
        outHit.axis = enterAxis;
        outHit.side = direction[enterAxis] > 0 ? -1 : 1;
        return true;
    }

    //- Visit the voxels crossed by the ray -----------------------------
    while ( true ) {
        int axis;
        if ( tNext[0] <= tNext[1] && tNext[0] <= tNext[2] ) {
            axis = 0;
        }
        else if ( tNext[1] <= tNext[2] ) {
            axis = 1;
        }
        else {
            axis = 2;
        }
        if ( tNext[axis] > tExit ) {
            return false;
        }
        double tCell = tNext[axis];
        g[axis] += step[axis];
        tNext[axis] += tDelta[axis];
        if ( g[axis] < 0 || g[axis] >= size[axis] ) {
            return false;
        }
        bool filled = isFilled(g[0], g[1], g[2]);
        if ( filled && !previousFilled ) {
            outHit.t = tCell;
            outHit.axis = axis;
            outHit.side = -step[axis];
            return true;
        }
        previousFilled = filled;
    }
}

/**
Fills the surface data of a hit on a voxel face: the point, the normal of
the face, texture coordinates spanning the volume along the two axes of
the face, and a tangent along the first of them.
*/
void VoxelVolume::fillSurfaceData(const Ray& hitRay, const VoxelFaceHit& hit,
                                  RayHit* outData)
{
    Vector3Dd p = hitRay.getOrigin().add(
        hitRay.getDirection().multiply(hit.t));
    double coordinates[3] = { p.x(), p.y(), p.z() };
    double normal[3] = { 0, 0, 0 };
    double tangent[3] = { 0, 0, 0 };
    int uAxis = (hit.axis + 1) % 3;
    int vAxis = (hit.axis + 2) % 3;

    normal[hit.axis] = hit.side;
    tangent[uAxis] = 1.0;
    if ( outData->needsPoint() ) {
        outData->point = p;
    }
    if ( outData->needsNormal() ) {
        outData->normal = Vector3Dd(normal[0], normal[1], normal[2]);
    }
    if ( outData->needsTextureCoordinates() ) {
        outData->u = (coordinates[uAxis] + 1.0) / 2.0;
        outData->v = (coordinates[vAxis] + 1.0) / 2.0;
    }
    if ( outData->needsTangent() ) {
        outData->tangent = Vector3Dd(tangent[0], tangent[1], tangent[2]);
    }
}

Matrix4x4d VoxelVolume::getTransformFromVoxelFrameToMinMax(const double minmax[6]) {
    double sx = minmax[3]-minmax[0], sy = minmax[4]-minmax[1], sz = minmax[5]-minmax[2];
    double greaterScale = sx;
    if (sy > greaterScale) greaterScale = sy;
    if (sz > greaterScale) greaterScale = sz;

    Matrix4x4d S, T1, T2;
    S = S.scale(greaterScale/2, greaterScale/2, greaterScale/2);
    T1 = T1.translation(1, 1, 1);
    T2 = T2.translation(minmax[0]-(greaterScale-sx)/2, minmax[1]-(greaterScale-sy)/2, minmax[2]-(greaterScale-sz)/2);
    return T2.multiply(S.multiply(T1));
}

Vector3Dd VoxelVolume::doCenterOfMass() {
    double cmx=0,cmy=0,cmz=0,mi,M=0;
    for (int x=0; x<getXSize(); x++) {
        for (int y=0; y<getYSize(); y++) {
            for (int z=0; z<getZSize(); z++) {
                mi = ((double)getVoxel(x,y,z))/255.0;
                M += mi;
                Vector3Dd p = getVoxelPosition(x,y,z);
                cmx += mi*p.x(); cmy += mi*p.y(); cmz += mi*p.z();
            }
        }
    }
    if (std::abs(M) < VSDK::EPSILON) return Vector3Dd(0,0,0);
    return Vector3Dd(cmx/M, cmy/M, cmz/M);
}
