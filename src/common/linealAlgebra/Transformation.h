#ifndef __TRANSFORMATION_H__
#define __TRANSFORMATION_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

typedef double MATRIX[4][4];

class Transformation {
  public:
    MATRIX matrix;
    MATRIX inverse;

    static void MZero(MATRIX *result);
    static void MIdentity(MATRIX *result);
    static void MTimes(MATRIX *result, MATRIX *matrix1, MATRIX *matrix2);
    static void MTranspose(MATRIX *result, MATRIX *matrix1);
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
