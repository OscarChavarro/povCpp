#ifndef __SIMPLE_BODY_BUILDER__
#define __SIMPLE_BODY_BUILDER__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/element/TransformStep.h"

class SimpleBodyBuilder {
  private:
    Geometry *geometry = nullptr;
    Material *material = nullptr;
    ColorRgba *shapeColor = nullptr;
    Matrix4x4d *transformation = nullptr;
    Matrix4x4d *transformationInverse = nullptr;
    java::ArrayList<TransformStep> steps{4};

    void ensureMatrices();

  public:
    SimpleBodyBuilder() {}
    SimpleBodyBuilder(Geometry *geometry, Material *material, ColorRgba *shapeColor);
    SimpleBodyBuilder(const SimpleBodyBuilder &other);
    ~SimpleBodyBuilder();

    Geometry* getGeometry() const { return geometry; }
    Material* getMaterial() const { return material; }
    ColorRgba* getShapeColor() const { return shapeColor; }
    Matrix4x4d* getTransformation() const { return transformation; }
    Matrix4x4d* getTransformationInverse() const { return transformationInverse; }
    const java::ArrayList<TransformStep>& getSteps() const { return steps; }
    Geometry* releaseGeometry();
    Material* releaseMaterial();
    ColorRgba* releaseShapeColor();
    Matrix4x4d* releaseTransformation();
    Matrix4x4d* releaseTransformationInverse();
    ColorRgba* ensureShapeColor();
    void prependMaterialLayers(Material *newHead);

    void translate(Vector3Dd *vector);
    void rotate(Vector3Dd *vector);
    void scale(Vector3Dd *vector);
    void translateOwnerOnly(Vector3Dd *vector);
    void rotateOwnerOnly(Vector3Dd *vector);
    void scaleOwnerOnly(Vector3Dd *vector);
    void invert();
};

#endif
