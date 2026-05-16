#ifndef __MATRICES_H__
#define __MATRICES_H__

#include "common/Vector.h"

class Transformation {
  public:
    MATRIX matrix;
    MATRIX inverse;
};

extern void MZero(MATRIX *result);
extern void MIdentity(MATRIX *result);
extern void MTimes(MATRIX *result, MATRIX *matrix1, MATRIX *matrix2);
extern void MAdd(MATRIX *result, MATRIX *matrix1, MATRIX *matrix2);
extern void MSub(MATRIX *result, MATRIX *matrix1, MATRIX *matrix2);
extern void MScale(MATRIX *result, MATRIX *matrix1, DBL amount);
extern void MTranspose(MATRIX *result, MATRIX *matrix1);
extern void MTransformVector(
    Vector3D *result, Vector3D *vector, Transformation *transformation);
extern void MInverseTransformVector(
    Vector3D *result, Vector3D *vector, Transformation *transformation);
extern void MTransVector(
    Vector3D *result, Vector3D *vector, Transformation *transformation);
extern void MInvTransVector(
    Vector3D *result, Vector3D *vector, Transformation *transformation);
extern void MTransNormal(
    Vector3D *result, Vector3D *vector, Transformation *transformation);
extern void getScalingTransformation(
    Transformation *result, Vector3D *vector);
extern void getInversionTransformation(Transformation *result);
extern void getTranslationTransformation(
    Transformation *transformation, Vector3D *vector);
extern void getRotationTransformation(
    Transformation *transformation, Vector3D *vector);
extern void getLookAtTransformation(Transformation *transformation,
    Vector3D *Look_At, Vector3D *Up, Vector3D *Right);
extern void composeTransformations(Transformation *Original_Transformation,
    Transformation *New_Transformation);
extern Transformation *getTransformation(void);

#endif
