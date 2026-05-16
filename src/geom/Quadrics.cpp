/****************************************************************************
 *                     quadrics.c
 *
 *  This module implements the code for the quadric shape primitive.
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

#include "geom/Quadrics.h"
#include "geom/Objects.h"

/*===========================================================================*/

Methods Quadric_Methods = {Object_Intersect, All_Quadric_Intersections,
    Inside_Quadric, Quadric_Normal, Copy_Quadric, Translate_Quadric,
    Rotate_Quadric, Scale_Quadric, Invert_Quadric};

extern Ray *vpRay;
extern long rayQuadricTests, rayQuadricTestsSucceeded;

/*===========================================================================*/

int
All_Quadric_Intersections(
    SimpleBody *object, Ray *ray, PriorityQueueNode *depthQueue)
{
    Quadric *shape = (Quadric *)object;
    DBL depth1, depth2;
    Vector3D intersectionPoint;
    Intersection localElement;
    register int intersectionFound;

    intersectionFound = FALSE;
    if (Intersect_Quadric(ray, shape, &depth1, &depth2)) {
        localElement.Depth = depth1;
        localElement.Object = shape->Parent_Object;
        VScale(intersectionPoint, ray->Direction, depth1);
        VAdd(intersectionPoint, intersectionPoint, ray->Initial);
        localElement.Point = intersectionPoint;
        localElement.Shape = (Geometry *)shape;
        depthQueue->add(&localElement);
        intersectionFound = TRUE;

        if (depth2 != depth1) {
            localElement.Depth = depth2;
            localElement.Object = shape->Parent_Object;
            VScale(intersectionPoint, ray->Direction, depth2);
            VAdd(intersectionPoint, intersectionPoint, ray->Initial);
            localElement.Point = intersectionPoint;
            localElement.Shape = (Geometry *)shape;
            depthQueue->add(&localElement);
            intersectionFound = TRUE;
        }
    }
    return (intersectionFound);
}

int
Intersect_Quadric(Ray *ray, Quadric *shape, DBL *depth1, DBL *depth2)
{
    register DBL squareTerm;
    register DBL linearTerm;
    register DBL constantTerm;
    register DBL tempTerm;
    register DBL determinant;
    register DBL determinant2;
    register DBL a2;
    register DBL bMinus;

    rayQuadricTests++;
    if (!ray->Quadric_Constants_Cached) {
        ray->makeRay();
    }

    if (shape->Non_Zero_Square_Term) {
        VDot(squareTerm, shape->Object_2_Terms, ray->Direction_2);
        VDot(tempTerm, shape->Object_Mixed_Terms, ray->Mixed_Dir_Dir);
        squareTerm += tempTerm;
    } else {
        squareTerm = 0.0;
    }

    VDot(linearTerm, shape->Object_2_Terms, ray->Initial_Direction);
    linearTerm *= 2.0;
    VDot(tempTerm, shape->Object_Terms, ray->Direction);
    linearTerm += tempTerm;
    VDot(tempTerm, shape->Object_Mixed_Terms, ray->Mixed_Init_Dir);
    linearTerm += tempTerm;

    if (ray == vpRay) {
        if (!shape->Constant_Cached) {
            VDot(constantTerm, shape->Object_2_Terms, ray->Initial_2);
            VDot(tempTerm, shape->Object_Terms, ray->Initial);
            constantTerm += tempTerm + shape->Object_Constant;
            shape->Object_VP_Constant = constantTerm;
            shape->Constant_Cached = TRUE;
        } else {
            constantTerm = shape->Object_VP_Constant;
        }
    } else {
        VDot(constantTerm, shape->Object_2_Terms, ray->Initial_2);
        VDot(tempTerm, shape->Object_Terms, ray->Initial);
        constantTerm += tempTerm + shape->Object_Constant;
    }

    VDot(tempTerm, shape->Object_Mixed_Terms, ray->Mixed_Initial_Initial);
    constantTerm += tempTerm;

    if (squareTerm != 0.0) {
        /* The equation is quadratic - find its roots */

        determinant2 =
            linearTerm * linearTerm - 4.0 * squareTerm * constantTerm;

        if (determinant2 < 0.0) {
            return (FALSE);
        }

        determinant = sqrt(determinant2);
        a2 = squareTerm * 2.0;
        bMinus = linearTerm * -1.0;

        *depth1 = (bMinus + determinant) / a2;
        *depth2 = (bMinus - determinant) / a2;
    } else {
        /* There are no quadratic terms.  Solve the linear equation instead. */
        if (linearTerm == 0.0) {
            return (FALSE);
        }

        *depth1 = constantTerm * -1.0 / linearTerm;
        *depth2 = *depth1;
    }

    if ((*depth1 < Small_Tolerance) || (*depth1 > Max_Distance)) {
        if ((*depth2 < Small_Tolerance) || (*depth2 > Max_Distance)) {
            return (FALSE);
        }
        *depth1 = *depth2;

    } else if ((*depth2 < Small_Tolerance) || (*depth2 > Max_Distance)) {
        *depth2 = *depth1;
    }

    rayQuadricTestsSucceeded++;
    return (TRUE);
}

int
Inside_Quadric(Vector3D *testPoint, SimpleBody *object)
{
    Quadric *shape = (Quadric *)object;
    Vector3D newPoint;
    register DBL result;
    register DBL linearTerm;
    register DBL squareTerm;

    VDot(linearTerm, *testPoint, shape->Object_Terms);
    result = linearTerm + shape->Object_Constant;
    VSquareTerms(newPoint, *testPoint);
    VDot(squareTerm, newPoint, shape->Object_2_Terms);
    result += squareTerm;
    result += shape->Object_Mixed_Terms.x * (testPoint->x) * (testPoint->y) +
              shape->Object_Mixed_Terms.y * (testPoint->x) * (testPoint->z) +
              shape->Object_Mixed_Terms.z * (testPoint->y) * (testPoint->z);

    if (result < Small_Tolerance) {
        return (TRUE);
    }

    return (FALSE);
}

void
Quadric_Normal(
    Vector3D *result, SimpleBody *object, Vector3D *intersectionPoint)
{
    Quadric *intersectionShape = (Quadric *)object;
    Vector3D derivativeLinear;
    DBL len;

    VScale(derivativeLinear, intersectionShape->Object_2_Terms, 2.0);
    VEvaluate(*result, derivativeLinear, *intersectionPoint);
    VAdd(*result, *result, intersectionShape->Object_Terms);

    result->x +=
        intersectionShape->Object_Mixed_Terms.x * intersectionPoint->y +
        intersectionShape->Object_Mixed_Terms.y * intersectionPoint->z;

    result->y +=
        intersectionShape->Object_Mixed_Terms.x * intersectionPoint->x +
        intersectionShape->Object_Mixed_Terms.z * intersectionPoint->z;

    result->z +=
        intersectionShape->Object_Mixed_Terms.y * intersectionPoint->x +
        intersectionShape->Object_Mixed_Terms.z * intersectionPoint->y;

    len = result->x * result->x + result->y * result->y + result->z * result->z;
    len = sqrt(len);
    if (len == 0.0) {
        /* The normal is not defined at this point of the surface.  Set it
            to any arbitrary direction. */
        result->x = 1.0;
        result->y = 0.0;
        result->z = 0.0;
    } else {
        result->x /= len; /* normalize the normal */
        result->y /= len;
        result->z /= len;
    }
}

void *
Copy_Quadric(SimpleBody *object)
{
    Quadric *newShape;

    newShape = Get_Quadric_Shape();
    *newShape = *((Quadric *)object);
    newShape->Next_Object = nullptr;

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture = Copy_Texture(newShape->Shape_Texture);
    }

    return (newShape);
}

static void
quadricToMatrix(Quadric *quadric, MATRIX *matrix)
{
    MZero(matrix);
    (*matrix)[0][0] = quadric->Object_2_Terms.x;
    (*matrix)[1][1] = quadric->Object_2_Terms.y;
    (*matrix)[2][2] = quadric->Object_2_Terms.z;
    (*matrix)[0][1] = quadric->Object_Mixed_Terms.x;
    (*matrix)[0][2] = quadric->Object_Mixed_Terms.y;
    (*matrix)[0][3] = quadric->Object_Terms.x;
    (*matrix)[1][2] = quadric->Object_Mixed_Terms.z;
    (*matrix)[1][3] = quadric->Object_Terms.y;
    (*matrix)[2][3] = quadric->Object_Terms.z;
    (*matrix)[3][3] = quadric->Object_Constant;
}

static void
matrixToQuadric(MATRIX *matrix, Quadric *quadric)
{
    quadric->Object_2_Terms.x = (*matrix)[0][0];
    quadric->Object_2_Terms.y = (*matrix)[1][1];
    quadric->Object_2_Terms.z = (*matrix)[2][2];
    quadric->Object_Mixed_Terms.x = (*matrix)[0][1] + (*matrix)[1][0];
    quadric->Object_Mixed_Terms.y = (*matrix)[0][2] + (*matrix)[2][0];
    quadric->Object_Terms.x = (*matrix)[0][3] + (*matrix)[3][0];
    quadric->Object_Mixed_Terms.z = (*matrix)[1][2] + (*matrix)[2][1];
    quadric->Object_Terms.y = (*matrix)[1][3] + (*matrix)[3][1];
    quadric->Object_Terms.z = (*matrix)[2][3] + (*matrix)[3][2];
    quadric->Object_Constant = (*matrix)[3][3];
}

static void
transformQuadric(Quadric *shape, Transformation *transformation)
{
    MATRIX quadricMatrix;
    MATRIX transformTransposed;

    quadricToMatrix(shape, (MATRIX *)&quadricMatrix[0][0]);
    MTimes((MATRIX *)&quadricMatrix[0][0],
        (MATRIX *)&(transformation->inverse[0][0]),
        (MATRIX *)&quadricMatrix[0][0]);
    MTranspose((MATRIX *)&transformTransposed[0][0],
        (MATRIX *)&(transformation->inverse[0][0]));
    MTimes((MATRIX *)&quadricMatrix[0][0], (MATRIX *)&quadricMatrix[0][0],
        (MATRIX *)&transformTransposed[0][0]);
    matrixToQuadric((MATRIX *)&quadricMatrix[0][0], shape);
}

void
Translate_Quadric(SimpleBody *object, Vector3D *vector)
{
    Transformation transformation;

    Get_Translation_Transformation(&transformation, vector);
    transformQuadric((Quadric *)object, &transformation);

    Translate_Texture(&((Quadric *)object)->Shape_Texture, vector);
}

void
Rotate_Quadric(SimpleBody *object, Vector3D *vector)
{
    Transformation transformation;

    Get_Rotation_Transformation(&transformation, vector);
    transformQuadric((Quadric *)object, &transformation);

    Rotate_Texture(&((Quadric *)object)->Shape_Texture, vector);
}

void
Scale_Quadric(SimpleBody *object, Vector3D *vector)
{
    Transformation transformation;

    Get_Scaling_Transformation(&transformation, vector);
    transformQuadric((Quadric *)object, &transformation);

    Scale_Texture(&((Quadric *)object)->Shape_Texture, vector);
}

void
Invert_Quadric(SimpleBody *object)
{
    Quadric *shape = (Quadric *)object;

    VScale(shape->Object_2_Terms, shape->Object_2_Terms, -1.0);
    VScale(shape->Object_Mixed_Terms, shape->Object_Mixed_Terms, -1.0);
    VScale(shape->Object_Terms, shape->Object_Terms, -1.0);
    shape->Object_Constant *= -1.0;
}
