#ifndef __TRANSFORMATION_H__
#define __TRANSFORMATION_H__

#include "common/Vector3D.h"

class Transformation {
  public:
    MATRIX matrix;
    MATRIX inverse;

    static void MZero(MATRIX *result);
    static void MIdentity(MATRIX *result);
    static void MTimes(MATRIX *result, MATRIX *matrix1, MATRIX *matrix2);
    static void MTranspose(MATRIX *result, MATRIX *matrix1);
    static void MTransformVector(
        Vector3D *result, Vector3D *vector, Transformation *transformation);
    static void MInverseTransformVector(
        Vector3D *result, Vector3D *vector, Transformation *transformation);
    static void MTransVector(
        Vector3D *result, Vector3D *vector, Transformation *transformation);
    static void MInvTransVector(
        Vector3D *result, Vector3D *vector, Transformation *transformation);
    static void MTransNormal(
        Vector3D *result, Vector3D *vector, Transformation *transformation);
    static void getScalingTransformation(
        Transformation *result, Vector3D *vector);
    static void getTranslationTransformation(
        Transformation *transformation, Vector3D *vector);
    static void getRotationTransformation(
        Transformation *transformation, Vector3D *vector);
    static void composeTransformations(
        Transformation *originalTransformation, Transformation *newTransformation);
    static Transformation *getTransformation(void);
};

#endif
