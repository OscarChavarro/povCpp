#ifndef __TRANSFORMED_GEOMETRY__
#define __TRANSFORMED_GEOMETRY__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/element/AxisAlignedBox.h"

class TransformedGeometry : public Geometry {
  protected:
    Matrix4x4d *transformation = nullptr;
    Matrix4x4d *transformationInverse = nullptr;

  public:
    ~TransformedGeometry() override;

    Matrix4x4d *getTransformation() const { return transformation; }
    Matrix4x4d *getTransformationInverse() const { return transformationInverse; }

    virtual AxisAlignedBox getMinMax() const { return AxisAlignedBox::unbounded(); }

    virtual void translateGeometry(Vector3Dd *vector);
    virtual void rotateGeometry(Vector3Dd *vector);
    virtual void scaleGeometry(Vector3Dd *vector);

  private:
    void ensureMatrices();
};

#endif
