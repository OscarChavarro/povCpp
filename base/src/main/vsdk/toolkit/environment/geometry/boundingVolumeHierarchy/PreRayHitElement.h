#ifndef __PRE_RAY_HIT_ELEMENT__
#define __PRE_RAY_HIT_ELEMENT__

class Ray;

/**
Conservative ray filter evaluated before an exact ray/geometry hit: false
positives are allowed, false negatives are forbidden.
*/
class PreRayHitElement {
  public:
    virtual ~PreRayHitElement();

    /** Filter for ray parameters in [0, +inf). */
    virtual bool mayIntersect(const Ray &ray) const = 0;

    /** Filter for ray parameters in [0, maxT]; the unbounded filter is a
    valid default because false positives are allowed. */
    virtual bool mayIntersectBefore(const Ray &ray, double maxT) const;
};

#endif
