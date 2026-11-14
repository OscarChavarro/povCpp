#ifndef __SURFACE_TRACE_STATE__
#define __SURFACE_TRACE_STATE__

class ProjectedTriangleTraversal;

// POV render state of one traced VITRAL surface: the inversion flag, the
// quadric's nonzero square term flag, the per-viewpoint cache of primary rays
// (the quadric constant term or the plane's normal-origin product) and the
// projected traversal of a triangle mesh. The cache is filled while tracing.
class SurfaceTraceState {
  public:
    SurfaceTraceState() :
        inverted(false), nonZeroSquareTerm(false), viewpointValue(0.0),
        viewpointCached(false), triangleTraversal(nullptr) {}

    bool isInverted() const { return inverted; }
    void setInverted(bool value) { inverted = value; }
    bool hasNonZeroSquareTerm() const { return nonZeroSquareTerm; }
    void setNonZeroSquareTerm(bool value) { nonZeroSquareTerm = value; }
    double getViewpointValue() const { return viewpointValue; }
    void setViewpointValue(double value) const { viewpointValue = value; }
    bool isViewpointCached() const { return viewpointCached; }
    void setViewpointCached(bool value) const { viewpointCached = value; }
    ProjectedTriangleTraversal *getTriangleTraversal() const { return triangleTraversal; }
    void setTriangleTraversal(ProjectedTriangleTraversal *value) { triangleTraversal = value; }

  private:
    bool inverted;
    bool nonZeroSquareTerm;
    mutable double viewpointValue;
    mutable bool viewpointCached;
    ProjectedTriangleTraversal *triangleTraversal;
};

#endif
