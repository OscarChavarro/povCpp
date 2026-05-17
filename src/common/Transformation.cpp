/****************************************************************************
 *                     matrices.c
 *
 *  This module contains code to manipulate 4x4 matrices.
 *
 *****************************************************************************/

#include "common/Transformation.h"
#include "common/FrameConfig.h"
#include "app/PovApp.h"
#include "common/Vector3Dd.h"
#include "common/Vector3Dd.h"
#include "io/Parse.h"
void
Transformation::MZero(MATRIX *result)
{
    /* Initialize the matrix to the following values:
    0.0    0.0    0.0    0.0
    0.0    0.0    0.0    0.0
    0.0    0.0    0.0    0.0
    0.0    0.0    0.0    0.0
    */
    register int i;
    register int j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            (*result)[i][j] = 0.0;
        }
    }
}

void
Transformation::MIdentity(MATRIX *result)
{
    /* Initialize the matrix to the following values:
    1.0    0.0    0.0    0.0
    0.0    1.0    0.0    0.0
    0.0    0.0    1.0    0.0
    0.0    0.0    0.0    1.0
    */
    register int i;
    register int j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (i == j) {
                (*result)[i][j] = 1.0;
            } else {
                (*result)[i][j] = 0.0;
            }
        }
    }
}

void
Transformation::MTimes(MATRIX *result, MATRIX *matrix1, MATRIX *matrix2)
{
    register int i;
    register int j;
    register int k;
    MATRIX tempMatrix;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            tempMatrix[i][j] = 0.0;
            for (k = 0; k < 4; k++) {
                tempMatrix[i][j] += (*matrix1)[i][k] * (*matrix2)[k][j];
            }
        }
    }

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            (*result)[i][j] = tempMatrix[i][j];
        }
    }

    cooperate();
}

void
Transformation::MTranspose(MATRIX *result, MATRIX *matrix1)
{
    register int i;
    register int j;
    MATRIX tempMatrix;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            tempMatrix[i][j] = (*matrix1)[j][i];
        }
    }

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            (*result)[i][j] = tempMatrix[i][j];
        }
    }
}

void
Transformation::MTransformVector(
    Vector3Dd *result, Vector3Dd *vector, Transformation *transformation)
{
    register int i;
    double answerArray[4];
    MATRIX *matrix;

    matrix = (MATRIX *)transformation->matrix;

    for (i = 0; i < 4; i++) {
        answerArray[i] = vector->x * (*matrix)[0][i] +
                         vector->y * (*matrix)[1][i] +
                         vector->z * (*matrix)[2][i] + (*matrix)[3][i];
    }

    result->x = answerArray[0];
    result->y = answerArray[1];
    result->z = answerArray[2];
}

void
Transformation::MInverseTransformVector(
    Vector3Dd *result, Vector3Dd *vector, Transformation *transformation)
{
    register int i;
    double answerArray[4];
    MATRIX *matrix;

    matrix = (MATRIX *)transformation->inverse;

    for (i = 0; i < 4; i++) {
        answerArray[i] = vector->x * (*matrix)[0][i] +
                         vector->y * (*matrix)[1][i] +
                         vector->z * (*matrix)[2][i] + (*matrix)[3][i];
    }

    result->x = answerArray[0];
    result->y = answerArray[1];
    result->z = answerArray[2];
}

void
Transformation::MTransVector(Vector3Dd *result, Vector3Dd *vector, Transformation *transformation)
{
    register int i;
    double answerArray[4];
    MATRIX *matrix;

    matrix = (MATRIX *)transformation->matrix;

    for (i = 0; i < 4; i++) {
        answerArray[i] = vector->x * (*matrix)[0][i] +
                         vector->y * (*matrix)[1][i] +
                         vector->z * (*matrix)[2][i];
    }

    result->x = answerArray[0];
    result->y = answerArray[1];
    result->z = answerArray[2];
}

void
Transformation::MInvTransVector(
    Vector3Dd *result, Vector3Dd *vector, Transformation *transformation)
{
    register int i;
    double answerArray[4];
    MATRIX *matrix;

    matrix = (MATRIX *)transformation->inverse;

    for (i = 0; i < 4; i++) {
        answerArray[i] = vector->x * (*matrix)[0][i] +
                         vector->y * (*matrix)[1][i] +
                         vector->z * (*matrix)[2][i];
    }

    result->x = answerArray[0];
    result->y = answerArray[1];
    result->z = answerArray[2];
}

void
Transformation::MTransNormal(Vector3Dd *result, Vector3Dd *vector, Transformation *transformation)
{
    register int i;
    double answerArray[3];
    MATRIX *matrix;

    matrix = (MATRIX *)transformation->inverse;

    for (i = 0; i < 3; i++) {
        answerArray[i] = vector->x * (*matrix)[i][0] +
                         vector->y * (*matrix)[i][1] +
                         vector->z * (*matrix)[i][2];
    }

    result->x = answerArray[0];
    result->y = answerArray[1];
    result->z = answerArray[2];
}

void
Transformation::getScalingTransformation(Transformation *result, Vector3Dd *vector)
{
    Transformation::MIdentity((MATRIX *)result->matrix);
    (result->matrix)[0][0] = vector->x;
    (result->matrix)[1][1] = vector->y;
    (result->matrix)[2][2] = vector->z;

    Transformation::MIdentity((MATRIX *)result->inverse);
    (result->inverse)[0][0] = 1.0 / vector->x;
    (result->inverse)[1][1] = 1.0 / vector->y;
    (result->inverse)[2][2] = 1.0 / vector->z;
}

void
Transformation::getTranslationTransformation(Transformation *transformation, Vector3Dd *vector)
{
    Transformation::MIdentity((MATRIX *)transformation->matrix);
    (transformation->matrix)[3][0] = vector->x;
    (transformation->matrix)[3][1] = vector->y;
    (transformation->matrix)[3][2] = vector->z;

    Transformation::MIdentity((MATRIX *)transformation->inverse);
    (transformation->inverse)[3][0] = 0.0 - vector->x;
    (transformation->inverse)[3][1] = 0.0 - vector->y;
    (transformation->inverse)[3][2] = 0.0 - vector->z;
}

void
Transformation::getRotationTransformation(Transformation *transformation, Vector3Dd *vector)
{
    MATRIX matrix;
    Vector3Dd radianVector;
    register double cosx;
    register double cosy;
    register double cosz;
    register double sinx;
    register double siny;
    register double sinz;

    VectorOps::vScale(radianVector, *vector, M_PI / 180.0);
    Transformation::MIdentity((MATRIX *)transformation->matrix);
    cosx = cos(radianVector.x);
    sinx = sin(radianVector.x);
    cosy = cos(radianVector.y);
    siny = sin(radianVector.y);
    cosz = cos(radianVector.z);
    sinz = sin(radianVector.z);

    (transformation->matrix)[1][1] = cosx;
    (transformation->matrix)[2][2] = cosx;
    (transformation->matrix)[1][2] = sinx;
    (transformation->matrix)[2][1] = 0.0 - sinx;
    Transformation::MTranspose(
        (MATRIX *)transformation->inverse, (MATRIX *)transformation->matrix);

    Transformation::MIdentity((MATRIX *)matrix);
    matrix[0][0] = cosy;
    matrix[2][2] = cosy;
    matrix[0][2] = 0.0 - siny;
    matrix[2][0] = siny;
    Transformation::MTimes((MATRIX *)transformation->matrix, (MATRIX *)transformation->matrix,
        (MATRIX *)matrix);
    Transformation::MTranspose((MATRIX *)matrix, (MATRIX *)matrix);
    Transformation::MTimes((MATRIX *)transformation->inverse, (MATRIX *)matrix,
        (MATRIX *)transformation->inverse);

    Transformation::MIdentity((MATRIX *)matrix);
    matrix[0][0] = cosz;
    matrix[1][1] = cosz;
    matrix[0][1] = sinz;
    matrix[1][0] = 0.0 - sinz;
    Transformation::MTimes((MATRIX *)transformation->matrix, (MATRIX *)transformation->matrix,
        (MATRIX *)matrix);
    Transformation::MTranspose((MATRIX *)matrix, (MATRIX *)matrix);
    Transformation::MTimes((MATRIX *)transformation->inverse, (MATRIX *)matrix,
        (MATRIX *)transformation->inverse);
}

void
Transformation::composeTransformations(
    Transformation *originalTransformation, Transformation *newTransformation)
{
    Transformation::MTimes((MATRIX *)originalTransformation->matrix,
        (MATRIX *)originalTransformation->matrix,
        (MATRIX *)newTransformation->matrix);

    Transformation::MTimes((MATRIX *)originalTransformation->inverse,
        (MATRIX *)newTransformation->inverse,
        (MATRIX *)originalTransformation->inverse);
}

Transformation *
Transformation::getTransformation()
{
    Transformation *newTransformation;

    newTransformation = new Transformation();
    if (newTransformation == nullptr) {
        ParseErrorReporter::Error("Out of memory. Cannot allocate transformation");
    }

    Transformation::MIdentity((MATRIX *)&(newTransformation->matrix[0][0]));
    Transformation::MIdentity((MATRIX *)&(newTransformation->inverse[0][0]));
    return (newTransformation);
}

