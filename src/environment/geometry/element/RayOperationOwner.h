#ifndef __RAY_OPERATION_OWNER__
#define __RAY_OPERATION_OWNER__

class PovRayHit;
class RayWithSegments;

class RayOperationOwner {
  public:
    virtual ~RayOperationOwner() = default;
    virtual void doExtraInformation(
        const RayWithSegments &ray, double t, PovRayHit *hit) = 0;
};

#endif
