#ifndef __PARAMETRIC_CONTROL_POINTS__
#define __PARAMETRIC_CONTROL_POINTS__


class ParametricControlPoints {
  public:
    Vector3Dd *getVertices();
    const Vector3Dd *getVertices() const;

  private:
    Vector3Dd vertices[4];
};

inline Vector3Dd *
ParametricControlPoints::getVertices()
{
    return vertices;
}

inline const Vector3Dd *
ParametricControlPoints::getVertices() const
{
    return vertices;
}

#endif
