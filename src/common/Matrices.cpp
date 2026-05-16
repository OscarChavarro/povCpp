/****************************************************************************
 *                     matrices.c
 *
 *  This module contains code to manipulate 4x4 matrices.
 *
 *  from Persistence of Vision Raytracer
 *  Copyright 1992 Persistence of Vision Team
 *---------------------------------------------------------------------------
 *  Copying, distribution and legal info is in the file povlegal.doc which
 *  should be distributed with this file. If povlegal.doc is not available
 *  or for more info please contact:
 *
 *         Drew Wells [POV-Team Leader]
 *         CIS: 73767,1244  Internet: 73767.1244@compuserve.com
 *         Phone: (213) 254-4041
 *
 * This program is based on the popular DKB raytracer version 2.12.
 * DKBTrace was originally written by David K. Buck.
 * DKBTrace Ver 2.0-2.12 were written by David K. Buck & Aaron A. Collins.
 *
 *****************************************************************************/

#include "common/Matrices.h"
#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"

/*===========================================================================*/

void
MZero(MATRIX *result)
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
MIdentity(MATRIX *result)
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
MTimes(MATRIX *result, MATRIX *matrix1, MATRIX *matrix2)
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

    COOPERATE
}

void
MTranspose(MATRIX *result, MATRIX *matrix1)
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
MTransformVector(
    Vector3D *result, Vector3D *vector, Transformation *transformation)
{
    register int i;
    DBL answerArray[4];
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
MInverseTransformVector(
    Vector3D *result, Vector3D *vector, Transformation *transformation)
{
    register int i;
    DBL answerArray[4];
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
MTransVector(Vector3D *result, Vector3D *vector, Transformation *transformation)
{
    register int i;
    DBL answerArray[4];
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
MInvTransVector(
    Vector3D *result, Vector3D *vector, Transformation *transformation)
{
    register int i;
    DBL answerArray[4];
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
MTransNormal(Vector3D *result, Vector3D *vector, Transformation *transformation)
{
    register int i;
    DBL answerArray[3];
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
Get_Scaling_Transformation(Transformation *result, Vector3D *vector)
{
    MIdentity((MATRIX *)result->matrix);
    (result->matrix)[0][0] = vector->x;
    (result->matrix)[1][1] = vector->y;
    (result->matrix)[2][2] = vector->z;

    MIdentity((MATRIX *)result->inverse);
    (result->inverse)[0][0] = 1.0 / vector->x;
    (result->inverse)[1][1] = 1.0 / vector->y;
    (result->inverse)[2][2] = 1.0 / vector->z;
}

void
Get_Translation_Transformation(Transformation *transformation, Vector3D *vector)
{
    MIdentity((MATRIX *)transformation->matrix);
    (transformation->matrix)[3][0] = vector->x;
    (transformation->matrix)[3][1] = vector->y;
    (transformation->matrix)[3][2] = vector->z;

    MIdentity((MATRIX *)transformation->inverse);
    (transformation->inverse)[3][0] = 0.0 - vector->x;
    (transformation->inverse)[3][1] = 0.0 - vector->y;
    (transformation->inverse)[3][2] = 0.0 - vector->z;
}

void
Get_Rotation_Transformation(Transformation *transformation, Vector3D *vector)
{
    MATRIX matrix;
    Vector3D radianVector;
    register DBL cosx;
    register DBL cosy;
    register DBL cosz;
    register DBL sinx;
    register DBL siny;
    register DBL sinz;

    VScale(radianVector, *vector, M_PI / 180.0);
    MIdentity((MATRIX *)transformation->matrix);
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
    MTranspose(
        (MATRIX *)transformation->inverse, (MATRIX *)transformation->matrix);

    MIdentity((MATRIX *)matrix);
    matrix[0][0] = cosy;
    matrix[2][2] = cosy;
    matrix[0][2] = 0.0 - siny;
    matrix[2][0] = siny;
    MTimes((MATRIX *)transformation->matrix, (MATRIX *)transformation->matrix,
        (MATRIX *)matrix);
    MTranspose((MATRIX *)matrix, (MATRIX *)matrix);
    MTimes((MATRIX *)transformation->inverse, (MATRIX *)matrix,
        (MATRIX *)transformation->inverse);

    MIdentity((MATRIX *)matrix);
    matrix[0][0] = cosz;
    matrix[1][1] = cosz;
    matrix[0][1] = sinz;
    matrix[1][0] = 0.0 - sinz;
    MTimes((MATRIX *)transformation->matrix, (MATRIX *)transformation->matrix,
        (MATRIX *)matrix);
    MTranspose((MATRIX *)matrix, (MATRIX *)matrix);
    MTimes((MATRIX *)transformation->inverse, (MATRIX *)matrix,
        (MATRIX *)transformation->inverse);
}

void
Compose_Transformations(
    Transformation *originalTransformation, Transformation *newTransformation)
{
    MTimes((MATRIX *)originalTransformation->matrix,
        (MATRIX *)originalTransformation->matrix,
        (MATRIX *)newTransformation->matrix);

    MTimes((MATRIX *)originalTransformation->inverse,
        (MATRIX *)newTransformation->inverse,
        (MATRIX *)originalTransformation->inverse);
}

Transformation *
Get_Transformation()
{
    Transformation *newTransformation;

    newTransformation = new Transformation();
    if (newTransformation == nullptr) {
        Error("Out of memory. Cannot allocate transformation");
    }

    MIdentity((MATRIX *)&(newTransformation->matrix[0][0]));
    MIdentity((MATRIX *)&(newTransformation->inverse[0][0]));
    return (newTransformation);
}

/* AAC - These are not used, so they are commented out to save code space... */
#ifdef NOT_USED_MATRIX_OPS
void
MAdd(MATRIX *result, MATRIX *matrix1, MATRIX *matrix2)
{
    register int i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            (*result)[i][j] = (*matrix1)[i][j] + (*matrix2)[i][j];
        }
    }
}

void
MSub(MATRIX *result, MATRIX *matrix1, MATRIX *matrix2)
{
    register int i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            (*result)[i][j] = (*matrix1)[i][j] - (*matrix2)[i][j];
        }
    }
}

void
MScale(MATRIX *result, MATRIX *matrix1, DBL amount)
{
    register int i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (amount == 1.0) {
                (*result)[i][j] = (*matrix1)[i][j]; /* just copy */
            } else {
                (*result)[i][j] = (*matrix1)[i][j]; /* amount */
            }
        }
    }
    return;
}

void
Get_Inversion_Transformation(Transformation *result)
{
    MIdentity((MATRIX *)result->matrix);
    (result->matrix)[0][0] = -1.0;
    (result->matrix)[1][1] = -1.0;
    (result->matrix)[2][2] = -1.0;
    (result->matrix)[3][3] = -1.0;

    (result->inverse)[0][0] = -1.0;
    (result->inverse)[1][1] = -1.0;
    (result->inverse)[2][2] = -1.0;
    (result->inverse)[3][3] = -1.0;
}

void
Get_Look_At_Transformation(
    Transformation *result, Vector3D *Look_At, Vector3D *Up, Vector3D *Right)
{
    MIdentity((MATRIX *)result->inverse);
    (result->matrix)[0][0] = Right->x;
    (result->matrix)[0][1] = Right->y;
    (result->matrix)[0][2] = Right->z;
    (result->matrix)[1][0] = Up->x;
    (result->matrix)[1][1] = Up->y;
    (result->matrix)[1][2] = Up->z;
    (result->matrix)[2][0] = Look_At->x;
    (result->matrix)[2][1] = Look_At->y;
    (result->matrix)[2][2] = Look_At->z;

    MIdentity((MATRIX *)result->matrix);
    MTranspose((MATRIX *)result->matrix, (MATRIX *)result->inverse);
}

#endif
