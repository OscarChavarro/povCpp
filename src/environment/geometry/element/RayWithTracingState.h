#ifndef __RAY_WITH_TRACING_STATE__
#define __RAY_WITH_TRACING_STATE__

#include "vsdk/toolkit/common/statistics/GeometryStatistics.h"
#include "environment/geometry/element/DetailMask.h"
#include "environment/geometry/element/PriorityQueuePool.h"
#include "environment/material/Material.h"
#include "environment/material/PovRayRendererConfiguration.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"

class IntersectionCandidate;

class RayWithTracingState : public Ray {
  public:
    static constexpr int DETAIL_NONE = DetailMask::NONE;
    static constexpr int DETAIL_POINT = DetailMask::POINT;
    static constexpr int DETAIL_NORMAL = DetailMask::NORMAL;
    static constexpr int DETAIL_UV = DetailMask::UV;
    static constexpr int DETAIL_TANGENT = DetailMask::TANGENT;
    static constexpr int DETAIL_ALL = DetailMask::ALL;

  private:
    static constexpr int MAX_CONTAINING_OBJECTS = 10;
    Vector3Dd position2;               // Xo^2  Yo^2  Zo^2
    Vector3Dd direction2;              // Xv^2  Yv^2  Zv^2
    Vector3Dd positionDirection;       // XoXv  YoYv  ZoZv
    Vector3Dd mixedPositionPosition;   // XoYo  XoZo  YoZo
    Vector3Dd mixedDirectionDirection; // XvYv  XvZv  YvZv
    Vector3Dd mixedPositionDirection;  // XoYv+XvYo  XoZv+XvZo  YoZv+YvZo
    int containingIndex;
    java::ArrayList<Material *> containingTextures;
    java::ArrayList<double> containingIORs;
    bool quadricConstantsCached;
    mutable bool aabbReciprocalsCached;
    mutable double invDirectionX, invDirectionY, invDirectionZ;
    mutable bool degenerateAxisX, degenerateAxisY, degenerateAxisZ;
    bool isShadowRay;
    bool isPrimaryRay;
    int requiredDetailMask;
    GeometryStatistics *statistics;
    const PovRayRendererConfiguration *config;
    PriorityQueuePool<IntersectionCandidate> *intersectionQueuePool;
  public:
    struct LocalIntersectionClone {};

    RayWithTracingState();
    inline RayWithTracingState(LocalIntersectionClone, const RayWithTracingState &source) :
        Ray(source),
        containingIndex(-1),
        containingTextures(0),
        containingIORs(0),
        quadricConstantsCached(false),
        aabbReciprocalsCached(false),
        isShadowRay(source.isShadowRay),
        isPrimaryRay(source.isPrimaryRay),
        requiredDetailMask(source.requiredDetailMask),
        statistics(source.statistics),
        config(source.config),
        intersectionQueuePool(source.intersectionQueuePool)
    {
    }

    using Ray::setDirection;
    using Ray::setOrigin;
    using Ray::setOriginAndDirection;
    using Ray::setT;

    Vector3Dd &getPosition2() { return position2; }
    const Vector3Dd &getPosition2() const { return position2; }
    Vector3Dd &getDirection2() { return direction2; }
    const Vector3Dd &getDirection2() const { return direction2; }
    Vector3Dd &getPositionDirection() { return positionDirection; }
    const Vector3Dd &getPositionDirection() const { return positionDirection; }
    Vector3Dd &getMixedPositionPosition() { return mixedPositionPosition; }
    const Vector3Dd &getMixedPositionPosition() const { return mixedPositionPosition; }
    Vector3Dd &getMixedDirectionDirection() { return mixedDirectionDirection; }
    const Vector3Dd &getMixedDirectionDirection() const { return mixedDirectionDirection; }
    Vector3Dd &getMixedPositionDirection() { return mixedPositionDirection; }
    const Vector3Dd &getMixedPositionDirection() const { return mixedPositionDirection; }
    int getContainingIndex() const { return containingIndex; }
    bool areQuadricConstantsCached() const { return quadricConstantsCached; }
    void setQuadricConstantsCached(bool value)
    {
        quadricConstantsCached = value;
        if (!value) {
            aabbReciprocalsCached = false;
        }
    }
    void getAabbSlabReciprocals(
        double *invDirX, double *invDirY, double *invDirZ,
        bool *degenerateX, bool *degenerateY, bool *degenerateZ) const
    {
        if (!aabbReciprocalsCached) {
            const Vector3Dd &direction = getDirection();
            auto cacheAxis = [](double directionCoord, double &invDir, bool &degenerate) {
                if (directionCoord > -1e-12 && directionCoord < 1e-12) {
                    degenerate = true;
                    invDir = 0.0;
                } else {
                    degenerate = false;
                    invDir = 1.0 / directionCoord;
                }
            };
            cacheAxis(direction.x(), invDirectionX, degenerateAxisX);
            cacheAxis(direction.y(), invDirectionY, degenerateAxisY);
            cacheAxis(direction.z(), invDirectionZ, degenerateAxisZ);
            aabbReciprocalsCached = true;
        }
        *invDirX = invDirectionX;
        *invDirY = invDirectionY;
        *invDirZ = invDirectionZ;
        *degenerateX = degenerateAxisX;
        *degenerateY = degenerateAxisY;
        *degenerateZ = degenerateAxisZ;
    }
    bool isShadowRayEnabled() const { return isShadowRay; }
    void setShadowRay(bool value) { isShadowRay = value; }
    bool isPrimaryRayEnabled() const { return isPrimaryRay; }
    void setPrimaryRay(bool value) { isPrimaryRay = value; }
    int getRequiredDetailMask() const { return requiredDetailMask; }
    void setRequiredDetailMask(int mask) { requiredDetailMask = mask; }
    bool needsNormal() const { return (requiredDetailMask & DETAIL_NORMAL) != 0; }
    bool needsUv() const { return (requiredDetailMask & DETAIL_UV) != 0; }
    bool needsTangent() const { return (requiredDetailMask & DETAIL_TANGENT) != 0; }
    Material *getContainingTextureAt(int index) const { return containingTextures.get(index); }
    double getContainingIORAt(int index) const { return containingIORs.get(index); }
    GeometryStatistics *getGeometryStatistics() const { return statistics; }
    void setGeometryStatistics(GeometryStatistics *stats) { statistics = stats; }
    const PovRayRendererConfiguration *getConfig() const { return config; }
    void setConfig(const PovRayRendererConfiguration *cfg) { config = cfg; }
    PriorityQueuePool<IntersectionCandidate> *getIntersectionQueuePool() const { return intersectionQueuePool; }
    void setIntersectionQueuePool(PriorityQueuePool<IntersectionCandidate> *pool) { intersectionQueuePool = pool; }
    inline void makeRay()
    {
        Vector3Dd tempInitDir;
        const Vector3Dd& origin = getOrigin();
        const Vector3Dd& direction = getDirection();

        this->position2 = origin.multiply(origin);
        this->direction2 = direction.multiply(direction);
        this->positionDirection = origin.multiply(direction);
        RayWithTracingState::mixVectorTerms(
            this->mixedPositionPosition, origin, origin);
        RayWithTracingState::mixVectorTerms(
            this->mixedDirectionDirection, direction, direction);
        RayWithTracingState::mixVectorTerms(
            tempInitDir, origin, direction);
        RayWithTracingState::mixVectorTerms(
            this->mixedPositionDirection, direction, origin);
        this->mixedPositionDirection =
            this->mixedPositionDirection.add(tempInitDir);
        this->quadricConstantsCached = true;
    }
    void initializeContainers();
    void copyContainersFrom(const RayWithTracingState *sourceRay);
    void enterContainingMedium(Material *texture);

    void exitContainingMedium();
    static inline void mixVectorTerms(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c)
    {
        a = Vector3Dd(b.x() * c.y(), b.x() * c.z(), b.y() * c.z());
    }
};

#endif
