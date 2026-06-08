/****************************************************************************
 *                     matrices.c
 *
 *  This module contains code to manipulate 4x4 matrices.
 *
 *****************************************************************************/

#include <cstdlib>

#include "common/linealAlgebra/Transformation.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/logger/Logger.h"

void
Transformation::MZero(Matrix4x4d *result)
{
    /* Initialize the matrix to the following values:
    0.0    0.0    0.0    0.0
    0.0    0.0    0.0    0.0
    0.0    0.0    0.0    0.0
    0.0    0.0    0.0    0.0
    */
    int i;
    int j;
    double tempMatrix[4][4];

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            tempMatrix[i][j] = 0.0;
        }
    }

    *result = Matrix4x4d(tempMatrix);
}

void
Transformation::MIdentity(Matrix4x4d *result)
{
    /* Initialize the matrix to the following values:
    1.0    0.0    0.0    0.0
    0.0    1.0    0.0    0.0
    0.0    0.0    1.0    0.0
    0.0    0.0    0.0    1.0
    */
    *result = Matrix4x4d::identityMatrix();
}

void
Transformation::MTimes(Matrix4x4d *result, Matrix4x4d *matrix1, Matrix4x4d *matrix2)
{
    *result = matrix1->multiply(*matrix2);
}

void
Transformation::MTranspose(Matrix4x4d *result, Matrix4x4d *matrix1)
{
    *result = matrix1->transpose();
}

void
Transformation::MTransformVector(
    Vector3Dd *result, Vector3Dd *vector, Transformation *transformation)
{
    int i;
    double answerArray[4];
    Matrix4x4d *matrix;

    matrix = &(transformation->matrix);

    for (i = 0; i < 4; i++) {
        answerArray[i] = vector->x() * matrix->get(0, i) +
                         vector->y() * matrix->get(1, i) +
                         vector->z() * matrix->get(2, i) + matrix->get(3, i);
    }

    *result = Vector3Dd(answerArray[0], answerArray[1], answerArray[2]);
}

void
Transformation::MInverseTransformVector(
    Vector3Dd *result, Vector3Dd *vector, Transformation *transformation)
{
    int i;
    double answerArray[4];
    Matrix4x4d *matrix;

    matrix = &(transformation->inverse);

    for (i = 0; i < 4; i++) {
        answerArray[i] = vector->x() * matrix->get(0, i) +
                         vector->y() * matrix->get(1, i) +
                         vector->z() * matrix->get(2, i) + matrix->get(3, i);
    }

    *result = Vector3Dd(answerArray[0], answerArray[1], answerArray[2]);
}

void
Transformation::MTransVector(
    Vector3Dd *result, Vector3Dd *vector, Transformation *transformation)
{
    int i;
    double answerArray[4];
    Matrix4x4d *matrix;

    matrix = &(transformation->matrix);

    for (i = 0; i < 4; i++) {
        answerArray[i] = vector->x() * matrix->get(0, i) +
                         vector->y() * matrix->get(1, i) +
                         vector->z() * matrix->get(2, i);
    }

    *result = Vector3Dd(answerArray[0], answerArray[1], answerArray[2]);
}

void
Transformation::MInvTransVector(
    Vector3Dd *result, Vector3Dd *vector, Transformation *transformation)
{
    int i;
    double answerArray[4];
    Matrix4x4d *matrix;

    matrix = &(transformation->inverse);

    for (i = 0; i < 4; i++) {
        answerArray[i] = vector->x() * matrix->get(0, i) +
                         vector->y() * matrix->get(1, i) +
                         vector->z() * matrix->get(2, i);
    }

    *result = Vector3Dd(answerArray[0], answerArray[1], answerArray[2]);
}

void
Transformation::MTransNormal(
    Vector3Dd *result, Vector3Dd *vector, Transformation *transformation)
{
    int i;
    double answerArray[3];
    Matrix4x4d *matrix;

    matrix = &(transformation->inverse);

    for (i = 0; i < 3; i++) {
        answerArray[i] = vector->x() * matrix->get(i, 0) +
                         vector->y() * matrix->get(i, 1) +
                         vector->z() * matrix->get(i, 2);
    }

    *result = Vector3Dd(answerArray[0], answerArray[1], answerArray[2]);
}

void
Transformation::getScalingTransformation(
    Transformation *result, Vector3Dd *vector)
{
    result->matrix = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    result->inverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z());
}

void
Transformation::getTranslationTransformation(
    Transformation *transformation, Vector3Dd *vector)
{
    Transformation::MIdentity(&(transformation->matrix));
    transformation->matrix = transformation->matrix.withVal(3, 0, vector->x());
    transformation->matrix = transformation->matrix.withVal(3, 1, vector->y());
    transformation->matrix = transformation->matrix.withVal(3, 2, vector->z());

    Transformation::MIdentity(&(transformation->inverse));
    transformation->inverse = transformation->inverse.withVal(3, 0, 0.0 - vector->x());
    transformation->inverse = transformation->inverse.withVal(3, 1, 0.0 - vector->y());
    transformation->inverse = transformation->inverse.withVal(3, 2, 0.0 - vector->z());
}

void
Transformation::getRotationTransformation(
    Transformation *transformation, Vector3Dd *vector)
{
    Matrix4x4d matrix;
    Vector3Dd radianVector;
    double cosx;
    double cosy;
    double cosz;
    double sinx;
    double siny;
    double sinz;

    radianVector = (*vector).multiply(M_PI / 180.0);
    Transformation::MIdentity(&(transformation->matrix));
    cosx = cos(radianVector.x());
    sinx = sin(radianVector.x());
    cosy = cos(radianVector.y());
    siny = sin(radianVector.y());
    cosz = cos(radianVector.z());
    sinz = sin(radianVector.z());

    transformation->matrix = transformation->matrix.withVal(1, 1, cosx);
    transformation->matrix = transformation->matrix.withVal(2, 2, cosx);
    transformation->matrix = transformation->matrix.withVal(1, 2, sinx);
    transformation->matrix = transformation->matrix.withVal(2, 1, 0.0 - sinx);
    Transformation::MTranspose(
        &(transformation->inverse), &(transformation->matrix));

    Transformation::MIdentity(&matrix);
    matrix = matrix.withVal(0, 0, cosy);
    matrix = matrix.withVal(2, 2, cosy);
    matrix = matrix.withVal(0, 2, 0.0 - siny);
    matrix = matrix.withVal(2, 0, siny);
    Transformation::MTimes(&(transformation->matrix),
        &(transformation->matrix), &matrix);
    Transformation::MTranspose(&matrix, &matrix);
    Transformation::MTimes(&(transformation->inverse), &matrix,
        &(transformation->inverse));

    Transformation::MIdentity(&matrix);
    matrix = matrix.withVal(0, 0, cosz);
    matrix = matrix.withVal(1, 1, cosz);
    matrix = matrix.withVal(0, 1, sinz);
    matrix = matrix.withVal(1, 0, 0.0 - sinz);
    Transformation::MTimes(&(transformation->matrix),
        &(transformation->matrix), &matrix);
    Transformation::MTranspose(&matrix, &matrix);
    Transformation::MTimes(&(transformation->inverse), &matrix,
        &(transformation->inverse));
}

void
Transformation::composeTransformations(
    Transformation *originalTransformation, Transformation *newTransformation)
{
    Transformation::MTimes(&(originalTransformation->matrix),
        &(originalTransformation->matrix),
        &(newTransformation->matrix));

    Transformation::MTimes(&(originalTransformation->inverse),
        &(newTransformation->inverse),
        &(originalTransformation->inverse));
}

Transformation *
Transformation::getTransformation()
{
    Transformation *newTransformation;

    newTransformation = new Transformation();
    if (newTransformation == nullptr) {
        Logger::error(
            "Out of memory. Cannot allocate transformation");
        exit(1);
    }

    Transformation::MIdentity(&(newTransformation->matrix));
    Transformation::MIdentity(&(newTransformation->inverse));
    return (newTransformation);
}
