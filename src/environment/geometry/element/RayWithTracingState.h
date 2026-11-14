#ifndef __RAY_WITH_TRACING_STATE__
#define __RAY_WITH_TRACING_STATE__

#include "vsdk/toolkit/common/statistics/GeometryStatistics.h"
#include "vsdk/toolkit/environment/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/material/Material.h"
#include "environment/geometry/element/DetailMask.h"
#include "environment/geometry/element/PriorityQueuePool.h"

class IntersectionCandidate;

class RayWithTracingState : public Ray {
  private:
    static constexpr int INITIAL_CONTAINING_OBJECT_CAPACITY = 10;
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
    const RendererConfiguration *config;
    PriorityQueuePool<IntersectionCandidate> *intersectionQueuePool;

    RayWithTracingState(
        const Ray &baseRay,
        bool isShadowRay,
        bool isPrimaryRay,
        int requiredDetailMask,
        GeometryStatistics *statistics,
        const RendererConfiguration *config,
        PriorityQueuePool<IntersectionCandidate> *intersectionQueuePool);

  public:
    static constexpr int DETAIL_NONE = DetailMask::NONE;
    static constexpr int DETAIL_POINT = DetailMask::POINT;
    static constexpr int DETAIL_NORMAL = DetailMask::NORMAL;
    static constexpr int DETAIL_UV = DetailMask::UV;
    static constexpr int DETAIL_TANGENT = DetailMask::TANGENT;
    static constexpr int DETAIL_ALL = DetailMask::ALL;

    RayWithTracingState();

    static RayWithTracingState localIntersectionClone(const RayWithTracingState &source);

    using Ray::setDirection;
    using Ray::setOrigin;
    using Ray::setOriginAndDirection;
    using Ray::setT;

    Vector3Dd &getPosition2();
    const Vector3Dd &getPosition2() const;
    Vector3Dd &getDirection2();
    const Vector3Dd &getDirection2() const;
    Vector3Dd &getPositionDirection();
    const Vector3Dd &getPositionDirection() const;
    Vector3Dd &getMixedPositionPosition();
    const Vector3Dd &getMixedPositionPosition() const;
    Vector3Dd &getMixedDirectionDirection();
    const Vector3Dd &getMixedDirectionDirection() const;
    Vector3Dd &getMixedPositionDirection();
    const Vector3Dd &getMixedPositionDirection() const;
    int getContainingIndex() const;
    bool areQuadricConstantsCached() const;
    void setQuadricConstantsCached(bool value);
    void getAabbSlabReciprocals(
        double *invDirX, double *invDirY, double *invDirZ,
        bool *degenerateX, bool *degenerateY, bool *degenerateZ) const;
    bool isShadowRayEnabled() const;
    void setShadowRay(bool value);
    bool isPrimaryRayEnabled() const;
    void setPrimaryRay(bool value);
    int getRequiredDetailMask() const;
    void setRequiredDetailMask(int mask);
    bool needsNormal() const;
    bool needsUv() const;
    bool needsTangent() const;
    Material *getContainingTextureAt(int index) const;
    double getContainingIORAt(int index) const;
    GeometryStatistics *getGeometryStatistics() const;
    void setGeometryStatistics(GeometryStatistics *stats);
    const RendererConfiguration *getConfig() const;
    void setConfig(const RendererConfiguration *cfg);
    PriorityQueuePool<IntersectionCandidate> *getIntersectionQueuePool() const;
    void setIntersectionQueuePool(PriorityQueuePool<IntersectionCandidate> *pool);
    void makeRay();
    void initializeContainers();
    void copyContainersFrom(const RayWithTracingState *sourceRay);
    void enterContainingMedium(Material *texture, double indexOfRefraction);
    void exitContainingMedium();
    static void mixVectorTerms(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c);
};

inline
RayWithTracingState::RayWithTracingState(
    const Ray &baseRay,
    bool isShadowRay,
    bool isPrimaryRay,
    int requiredDetailMask,
    GeometryStatistics *statistics,
    const RendererConfiguration *config,
    PriorityQueuePool<IntersectionCandidate> *intersectionQueuePool) :
    Ray(baseRay),
    containingIndex(-1),
    containingTextures(0),
    containingIORs(0),
    quadricConstantsCached(false),
    aabbReciprocalsCached(false),
    isShadowRay(isShadowRay),
    isPrimaryRay(isPrimaryRay),
    requiredDetailMask(requiredDetailMask),
    statistics(statistics),
    config(config),
    intersectionQueuePool(intersectionQueuePool)
 {
 }

inline RayWithTracingState
RayWithTracingState::localIntersectionClone(const RayWithTracingState &source)
{
    return RayWithTracingState(
        source,
        source.isShadowRay,
        source.isPrimaryRay,
        source.requiredDetailMask,
        source.statistics,
        source.config,
        source.intersectionQueuePool);
}

inline Vector3Dd &
RayWithTracingState::getPosition2()
{
    return position2;
}

inline const Vector3Dd &
RayWithTracingState::getPosition2() const
{
    return position2;
}

inline Vector3Dd &
RayWithTracingState::getDirection2()
{
    return direction2;
}

inline const Vector3Dd &
RayWithTracingState::getDirection2() const
{
    return direction2;
}

inline Vector3Dd &
RayWithTracingState::getPositionDirection()
{
    return positionDirection;
}

inline const Vector3Dd &
RayWithTracingState::getPositionDirection() const
{
    return positionDirection;
}

inline Vector3Dd &
RayWithTracingState::getMixedPositionPosition()
{
    return mixedPositionPosition;
}

inline const Vector3Dd &
RayWithTracingState::getMixedPositionPosition() const
{
    return mixedPositionPosition;
}

inline Vector3Dd &
RayWithTracingState::getMixedDirectionDirection()
{
    return mixedDirectionDirection;
}

inline const Vector3Dd &
RayWithTracingState::getMixedDirectionDirection() const
{
    return mixedDirectionDirection;
}

inline Vector3Dd &
RayWithTracingState::getMixedPositionDirection()
{
    return mixedPositionDirection;
}

inline const Vector3Dd &
RayWithTracingState::getMixedPositionDirection() const
{
    return mixedPositionDirection;
}

inline int
RayWithTracingState::getContainingIndex() const
{
    return containingIndex;
}

inline bool
RayWithTracingState::areQuadricConstantsCached() const
{
    return quadricConstantsCached;
}

inline void
RayWithTracingState::setQuadricConstantsCached(bool value)
{
    quadricConstantsCached = value;
    if (!value) {
        aabbReciprocalsCached = false;
    }
}

inline void
RayWithTracingState::getAabbSlabReciprocals(
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

inline bool
RayWithTracingState::isShadowRayEnabled() const
{
    return isShadowRay;
}

inline void
RayWithTracingState::setShadowRay(bool value)
{
    isShadowRay = value;
}

inline bool
RayWithTracingState::isPrimaryRayEnabled() const
{
    return isPrimaryRay;
}

inline void
RayWithTracingState::setPrimaryRay(bool value)
{
    isPrimaryRay = value;
}

inline int
RayWithTracingState::getRequiredDetailMask() const
{
    return requiredDetailMask;
}

inline void
RayWithTracingState::setRequiredDetailMask(int mask)
{
    requiredDetailMask = mask;
}

inline bool
RayWithTracingState::needsNormal() const
{
    return (requiredDetailMask & DETAIL_NORMAL) != 0;
}

inline bool
RayWithTracingState::needsUv() const
{
    return (requiredDetailMask & DETAIL_UV) != 0;
}

inline bool
RayWithTracingState::needsTangent() const
{
    return (requiredDetailMask & DETAIL_TANGENT) != 0;
}

inline Material *
RayWithTracingState::getContainingTextureAt(int index) const
{
    return containingTextures.get(index);
}

inline double
RayWithTracingState::getContainingIORAt(int index) const
{
    return containingIORs.get(index);
}

inline GeometryStatistics *
RayWithTracingState::getGeometryStatistics() const
{
    return statistics;
}

inline void
RayWithTracingState::setGeometryStatistics(GeometryStatistics *stats)
{
    statistics = stats;
}

inline const RendererConfiguration *
RayWithTracingState::getConfig() const
{
    return config;
}

inline void
RayWithTracingState::setConfig(const RendererConfiguration *cfg)
{
    config = cfg;
}

inline PriorityQueuePool<IntersectionCandidate> *
RayWithTracingState::getIntersectionQueuePool() const
{
    return intersectionQueuePool;
}

inline void
RayWithTracingState::setIntersectionQueuePool(PriorityQueuePool<IntersectionCandidate> *pool)
{
    intersectionQueuePool = pool;
}

inline void
RayWithTracingState::makeRay()
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

inline void
RayWithTracingState::mixVectorTerms(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c)
{
    a = Vector3Dd(b.x() * c.y(), b.x() * c.z(), b.y() * c.z());
}

#endif
