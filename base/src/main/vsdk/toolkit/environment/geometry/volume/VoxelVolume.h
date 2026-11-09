#ifndef __VOXEL_VOLUME__
#define __VOXEL_VOLUME__

#include "vsdk/toolkit/environment/geometry/volume/Solid.h"
class IndexedColorImageUncompressed;
class Ray;
class RayHit;
class Matrix4x4d;

class VoxelVolume : public Solid {
public:
    /** Default lowest value of a voxel considered part of the solid */
    static const int DEFAULT_THRESHOLD = 127;

private:
    /** Smallest ray direction component not considered parallel to an axis */
    static const double PARALLEL_EPSILON;

    IndexedColorImageUncompressed** data;
    int xSize;
    int ySize;
    int zSize;
    int threshold;

    /**
    Face of a voxel entered by a ray.
    */
    struct VoxelFaceHit {
        /** Distance along the ray to the face */
        double t;
        /** Axis perpendicular to the face (0: x, 1: y, 2: z) */
        int axis;
        /** Sign of the outer normal of the face along its axis */
        int side;
    };

    bool traceFirstFilledVoxel(const Ray& ray, VoxelFaceHit& outHit) const;
    static void fillSurfaceData(const Ray& hitRay, const VoxelFaceHit& hit,
                                RayHit* outData);

public:
    VoxelVolume();
    virtual ~VoxelVolume();

    int getXSize() const;
    int getYSize() const;
    int getZSize() const;

    /**
    @return lowest value of a voxel considered part of the solid (by the ray
    intersection and by the renderers)
    */
    int getThreshold() const;

    /**
    @param threshold lowest value of a voxel considered part of the solid
    */
    void setThreshold(int threshold);

    /**
    @param x voxel index along X
    @param y voxel index along Y
    @param z voxel index along Z
    @return true if the voxel is inside the volume and its value reaches the
    threshold
    */
    bool isFilled(int x, int y, int z) const;

    bool init(int xSize, int ySize, int zSize);

    void putVoxel(int x, int y, int z, char val);
    int getVoxel(int x, int y, int z) const;

    Vector3Dd getVoxelPosition(int x, int y, int z) const;
    int getNearestIFromX(double x) const;
    int getNearestJFromY(double y) const;
    int getNearestKFromZ(double z) const;

    int getVoxelAtPosition(double x, double y, double z) const;
    int getVoxelAtPosition(const Vector3Dd& p) const;
    void putVoxelAtPosition(double x, double y, double z, char val);
    void putVoxelAtPosition(const Vector3Dd& p, char val);

    virtual double* getMinMax();
    Ray* doIntersectionFirstHit(const Ray& inOutRay);
    virtual bool doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData);

    static Matrix4x4d getTransformFromVoxelFrameToMinMax(const double minmax[6]);
    virtual Vector3Dd doCenterOfMass();
};

#endif
