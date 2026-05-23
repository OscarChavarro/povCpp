/****************************************************************************
 *                     quadrics.c
 *
 *  This module implements the code for the quadric shape primitive.
 *
 *****************************************************************************/

#include "environment/geometry/volume/Quadric.h"
#include "common/Statistics.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/volume/compound/Composite.h"
Methods Quadric_Methods = {Composite::objectIntersect,
    Quadric::allQuadricIntersections, Quadric::insideQuadric,
    Quadric::quadricNormal, Quadric::copyQuadric, Quadric::translateQuadric,
    Quadric::rotateQuadric, Quadric::scaleQuadric, Quadric::invertQuadric};

extern RayWithSegments *vpRay;
int
Quadric::allQuadricIntersections(
    SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue)
{
    Quadric *shape = (Quadric *)object;
    double depth1;
    double depth2;
    Vector3Dd intersectionPoint;
    Intersection localElement;
    int intersectionFound;

    intersectionFound = FALSE;
    if (Quadric::intersectQuadric(ray, shape, &depth1, &depth2)) {
        localElement.Depth = depth1;
        localElement.Object = nullptr;
        VectorOps::vScale(intersectionPoint, ray->direction, depth1);
        intersectionPoint.add(ray->position);
        localElement.Point = intersectionPoint;
        localElement.Shape = (Geometry *)shape;
        depthQueue->add(&localElement);
        intersectionFound = TRUE;

        if (depth2 != depth1) {
            localElement.Depth = depth2;
            localElement.Object = nullptr;
            VectorOps::vScale(intersectionPoint, ray->direction, depth2);
            intersectionPoint.add(ray->position);
            localElement.Point = intersectionPoint;
            localElement.Shape = (Geometry *)shape;
            depthQueue->add(&localElement);
            intersectionFound = TRUE;
        }
    }
    return (intersectionFound);
}

int
Quadric::intersectQuadric(
    RayWithSegments *ray, Quadric *shape, double *depth1, double *depth2)
{
    double squareTerm;
    double linearTerm;
    double constantTerm;
    double tempTerm;
    double determinant;
    double determinant2;
    double a2;
    double bMinus;

    globalStatistics.rayQuadricTests++;
    if (!ray->quadricConstantsCached) {
        ray->makeRay();
    }

    if (shape->Non_Zero_Square_Term) {
        squareTerm = shape->Object_2_Terms.dotProduct(ray->direction2);
        tempTerm =
            shape->Object_Mixed_Terms.dotProduct(ray->mixedDirectionDirection);
        squareTerm += tempTerm;
    } else {
        squareTerm = 0.0;
    }

    linearTerm = shape->Object_2_Terms.dotProduct(ray->positionDirection);
    linearTerm *= 2.0;
    tempTerm = shape->Object_Terms.dotProduct(ray->direction);
    linearTerm += tempTerm;
    tempTerm =
        shape->Object_Mixed_Terms.dotProduct(ray->mixedPositionDirection);
    linearTerm += tempTerm;

    if (ray == vpRay) {
        if (!shape->Constant_Cached) {
            constantTerm = shape->Object_2_Terms.dotProduct(ray->position2);
            tempTerm = shape->Object_Terms.dotProduct(ray->position);
            constantTerm += tempTerm + shape->Object_Constant;
            shape->Object_VP_Constant = constantTerm;
            shape->Constant_Cached = TRUE;
        } else {
            constantTerm = shape->Object_VP_Constant;
        }
    } else {
        constantTerm = shape->Object_2_Terms.dotProduct(ray->position2);
        tempTerm = shape->Object_Terms.dotProduct(ray->position);
        constantTerm += tempTerm + shape->Object_Constant;
    }

    tempTerm = shape->Object_Mixed_Terms.dotProduct(ray->mixedPositionPosition);
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

    globalStatistics.rayQuadricTestsSucceeded++;
    return (TRUE);
}

int
Quadric::insideQuadric(Vector3Dd *testPoint, SimpleBody *object)
{
    Quadric *shape = (Quadric *)object;
    Vector3Dd newPoint;
    double result;
    double linearTerm;
    double squareTerm;

    linearTerm = (*testPoint).dotProduct(shape->Object_Terms);
    result = linearTerm + shape->Object_Constant;
    VectorOps::vSquareTerms(newPoint, *testPoint);
    squareTerm = newPoint.dotProduct(shape->Object_2_Terms);
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
Quadric::quadricNormal(
    Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint)
{
    Quadric *intersectionShape = (Quadric *)object;
    Vector3Dd derivativeLinear;
    double len;

    VectorOps::vScale(derivativeLinear, intersectionShape->Object_2_Terms, 2.0);
    VectorOps::vEvaluate(*result, derivativeLinear, *intersectionPoint);
    (*result).add(intersectionShape->Object_Terms);

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
Quadric::copyQuadric(SimpleBody *object)
{
    Quadric *newShape;

    newShape = new Quadric;
    *newShape = *((Quadric *)object);
    newShape->Next_Object = nullptr;

    if (newShape->Shape_Texture != nullptr) {
        newShape->Shape_Texture =
            TextureUtils::copyTexture(newShape->Shape_Texture);
    }

    return (newShape);
}

void
Quadric::quadricToMatrix(Quadric *quadric, MATRIX *matrix)
{
    Transformation::MZero(matrix);
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

void
Quadric::matrixToQuadric(MATRIX *matrix, Quadric *quadric)
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

void
Quadric::transformQuadric(Quadric *shape, Transformation *transformation)
{
    MATRIX quadricMatrix;
    MATRIX transformTransposed;

    Quadric::quadricToMatrix(shape, (MATRIX *)&quadricMatrix[0][0]);
    Transformation::MTimes((MATRIX *)&quadricMatrix[0][0],
        (MATRIX *)&(transformation->inverse[0][0]),
        (MATRIX *)&quadricMatrix[0][0]);
    Transformation::MTranspose((MATRIX *)&transformTransposed[0][0],
        (MATRIX *)&(transformation->inverse[0][0]));
    Transformation::MTimes((MATRIX *)&quadricMatrix[0][0],
        (MATRIX *)&quadricMatrix[0][0], (MATRIX *)&transformTransposed[0][0]);
    Quadric::matrixToQuadric((MATRIX *)&quadricMatrix[0][0], shape);
}

void
Quadric::translateQuadric(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transformation;

    Transformation::getTranslationTransformation(&transformation, vector);
    Quadric::transformQuadric((Quadric *)object, &transformation);

    TextureUtils::translateTexture(&((Quadric *)object)->Shape_Texture, vector);
}

void
Quadric::rotateQuadric(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transformation;

    Transformation::getRotationTransformation(&transformation, vector);
    Quadric::transformQuadric((Quadric *)object, &transformation);

    TextureUtils::rotateTexture(&((Quadric *)object)->Shape_Texture, vector);
}

void
Quadric::scaleQuadric(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transformation;

    Transformation::getScalingTransformation(&transformation, vector);
    Quadric::transformQuadric((Quadric *)object, &transformation);

    TextureUtils::scaleTexture(&((Quadric *)object)->Shape_Texture, vector);
}

void
Quadric::invertQuadric(SimpleBody *object)
{
    Quadric *shape = (Quadric *)object;

    shape->Object_2_Terms.scale(-1.0);
    shape->Object_Mixed_Terms.scale(-1.0);
    shape->Object_Terms.scale(-1.0);
    shape->Object_Constant *= -1.0;
}
