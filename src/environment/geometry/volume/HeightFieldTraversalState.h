#ifndef __HEIGHT_FIELD_TRAVERSAL_STATE__
#define __HEIGHT_FIELD_TRAVERSAL_STATE__

#include "environment/geometry/element/IntersectionCandidate.h"

class HeightField;

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

    friend class HeightField;

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

    void setIsdx(int value) { isdx = value; }
    void setIsdz(int value) { isdz = value; }
    void setXDom(bool value) { xDom = value; }
    void setGdx(double value) { gdx = value; }
    void setGdy(double value) { gdy = value; }
    void setGdz(double value) { gdz = value; }
    void setMyx(double value) { myx = value; }
    void setMxz(double value) { mxz = value; }
    void setMzx(double value) { mzx = value; }
    void setMyz(double value) { myz = value; }
    void setHfIntersection(IntersectionCandidate *value) { hfIntersection = value; }
    void setHfQueue(java::PriorityQueue<IntersectionCandidate> *value) { hfQueue = value; }
    void setRRay(RayWithTracingState *value) { rRay = value; }
};

#endif
