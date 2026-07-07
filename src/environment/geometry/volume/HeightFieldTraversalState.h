#ifndef __HEIGHT_FIELD_TRAVERSAL_STATE__
#define __HEIGHT_FIELD_TRAVERSAL_STATE__

#include "environment/geometry/element/IntersectionCandidate.h"

class HeightFieldTraversalState {
  private:
    int isdx;
    int isdz;
    bool xDom;
    double gdx;
    double gdy;
    double gdz;
    double myx;
    double mxz;
    double mzx;
    double myz;
    IntersectionCandidate *hfIntersection;
    java::PriorityQueue<IntersectionCandidate> *hfQueue;
    RayWithTracingState *rRay;

  public:
    HeightFieldTraversalState(int isdx, int isdz, bool xDom, double gdx,
        double gdy, double gdz, double myx, double mxz, double mzx,
        double myz, IntersectionCandidate *hfIntersection,
        java::PriorityQueue<IntersectionCandidate> *hfQueue, RayWithTracingState *rRay) :
        isdx(isdx),
        isdz(isdz),
        xDom(xDom),
        gdx(gdx),
        gdy(gdy),
        gdz(gdz),
        myx(myx),
        mxz(mxz),
        mzx(mzx),
        myz(myz),
        hfIntersection(hfIntersection),
        hfQueue(hfQueue),
        rRay(rRay)
    {
    }

    int getIsdx() const;
    void setIsdx(int isdx);
    int getIsdz() const;
    void setIsdz(int isdz);
    bool getXDom() const;
    void setXDom(bool xDom);
    double getGdx() const;
    void setGdx(double gdx);
    double getGdy() const;
    void setGdy(double gdy);
    double getGdz() const;
    void setGdz(double gdz);
    double getMyx() const;
    void setMyx(double myx);
    double getMxz() const;
    void setMxz(double mxz);
    double getMzx() const;
    void setMzx(double mzx);
    double getMyz() const;
    void setMyz(double myz);
    IntersectionCandidate *getHfIntersection() const;
    void setHfIntersection(IntersectionCandidate *hfIntersection);
    java::PriorityQueue<IntersectionCandidate> *getHfQueue() const;
    void setHfQueue(java::PriorityQueue<IntersectionCandidate> *hfQueue);
    RayWithTracingState *getRRay() const;
    void setRRay(RayWithTracingState *rRay);
};

inline int
HeightFieldTraversalState::getIsdx() const
{
    return isdx;
}

inline void
HeightFieldTraversalState::setIsdx(int isdx)
{
    this->isdx = isdx;
}

inline int
HeightFieldTraversalState::getIsdz() const
{
    return isdz;
}

inline void
HeightFieldTraversalState::setIsdz(int isdz)
{
    this->isdz = isdz;
}

inline bool
HeightFieldTraversalState::getXDom() const
{
    return xDom;
}

inline void
HeightFieldTraversalState::setXDom(bool xDom)
{
    this->xDom = xDom;
}

inline double
HeightFieldTraversalState::getGdx() const
{
    return gdx;
}

inline void
HeightFieldTraversalState::setGdx(double gdx)
{
    this->gdx = gdx;
}

inline double
HeightFieldTraversalState::getGdy() const
{
    return gdy;
}

inline void
HeightFieldTraversalState::setGdy(double gdy)
{
    this->gdy = gdy;
}

inline double
HeightFieldTraversalState::getGdz() const
{
    return gdz;
}

inline void
HeightFieldTraversalState::setGdz(double gdz)
{
    this->gdz = gdz;
}

inline double
HeightFieldTraversalState::getMyx() const
{
    return myx;
}

inline void
HeightFieldTraversalState::setMyx(double myx)
{
    this->myx = myx;
}

inline double
HeightFieldTraversalState::getMxz() const
{
    return mxz;
}

inline void
HeightFieldTraversalState::setMxz(double mxz)
{
    this->mxz = mxz;
}

inline double
HeightFieldTraversalState::getMzx() const
{
    return mzx;
}

inline void
HeightFieldTraversalState::setMzx(double mzx)
{
    this->mzx = mzx;
}

inline double
HeightFieldTraversalState::getMyz() const
{
    return myz;
}

inline void
HeightFieldTraversalState::setMyz(double myz)
{
    this->myz = myz;
}

inline IntersectionCandidate *
HeightFieldTraversalState::getHfIntersection() const
{
    return hfIntersection;
}

inline void
HeightFieldTraversalState::setHfIntersection(IntersectionCandidate *hfIntersection)
{
    this->hfIntersection = hfIntersection;
}

inline java::PriorityQueue<IntersectionCandidate> *
HeightFieldTraversalState::getHfQueue() const
{
    return hfQueue;
}

inline void
HeightFieldTraversalState::setHfQueue(java::PriorityQueue<IntersectionCandidate> *hfQueue)
{
    this->hfQueue = hfQueue;
}

inline RayWithTracingState *
HeightFieldTraversalState::getRRay() const
{
    return rRay;
}

inline void
HeightFieldTraversalState::setRRay(RayWithTracingState *rRay)
{
    this->rRay = rRay;
}

#endif
