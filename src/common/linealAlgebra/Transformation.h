#ifndef __TRANSFORMATION_H__
#define __TRANSFORMATION_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"

class Transformation {
  public:
    Matrix4x4d matrix;
    Matrix4x4d inverse;

    static void MZero(Matrix4x4d *result);
    static void MIdentity(Matrix4x4d *result);
    static void MTimes(Matrix4x4d *result, Matrix4x4d *matrix1, Matrix4x4d *matrix2);
    static void MTranspose(Matrix4x4d *result, Matrix4x4d *matrix1);
    static void MTransformVector(
        Vector3Dd *result, Vector3Dd *vector, Transformation *transformation);
    static void MInverseTransformVector(
        Vector3Dd *result, Vector3Dd *vector, Transformation *transformation);
    static void MTransVector(
        Vector3Dd *result, Vector3Dd *vector, Transformation *transformation);
    static void MInvTransVector(
        Vector3Dd *result, Vector3Dd *vector, Transformation *transformation);
    static void MTransNormal(
        Vector3Dd *result, Vector3Dd *vector, Transformation *transformation);
    static void getScalingTransformation(
        Transformation *result, Vector3Dd *vector);
    static void getTranslationTransformation(
        Transformation *transformation, Vector3Dd *vector);
    static void getRotationTransformation(
        Transformation *transformation, Vector3Dd *vector);
    static void composeTransformations(Transformation *originalTransformation,
        Transformation *newTransformation);
    static Transformation *getTransformation(void);
};

#endif
