/****************************************************************************
 *                     quadrics.c
 *
 *  This module implements the code for the quadric shape primitive.
 *
 *****************************************************************************/

#include "environment/geometry/volume/Quadric.h"
#include "common/Statistics.h"
#include "common/linealAlgebra/Vector3Dd.h"
Methods Quadric::methodTable = {
    Quadric::allQuadricIntersections, Quadric::insideQuadric,
    Quadric::quadricNormal, Quadric::copyQuadric, Quadric::translateQuadric,
    Quadric::rotateQuadric, Quadric::scaleQuadric, Quadric::invertQuadric};

int
Quadric::allQuadricIntersections(
    SimpleBody *object, RayWithSegments *ray, PriorityQueueNode *depthQueue)
{
    Quadric *shape = (Quadric *)object;
    double depth1;
    double depth2;
    Vector3Dd intersectionPoint;
    Intersection localElement;
    bool intersectionFound;

    intersectionFound = false;
    if (Quadric::intersectQuadric(ray, shape, &depth1, &depth2)) {
        localElement.Depth = depth1;
        localElement.Object = nullptr;
        VectorOps::vScale(intersectionPoint, ray->direction, depth1);
        intersectionPoint.add(ray->position);
        localElement.Point = intersectionPoint;
        localElement.Shape = (Geometry *)shape;
        depthQueue->add(&localElement);
        intersectionFound = true;

        if (depth2 != depth1) {
            localElement.Depth = depth2;
            localElement.Object = nullptr;
            VectorOps::vScale(intersectionPoint, ray->direction, depth2);
            intersectionPoint.add(ray->position);
            localElement.Point = intersectionPoint;
            localElement.Shape = (Geometry *)shape;
            depthQueue->add(&localElement);
            intersectionFound = true;
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

    Statistics::global().rayQuadricTests++;
    if (!ray->quadricConstantsCached) {
        ray->makeRay();
    }

    if (shape->nonZeroSquareTerm) {
        squareTerm = shape->object2Terms.dotProduct(ray->direction2);
        tempTerm =
            shape->objectMixedTerms.dotProduct(ray->mixedDirectionDirection);
        squareTerm += tempTerm;
    } else {
        squareTerm = 0.0;
    }

    linearTerm = shape->object2Terms.dotProduct(ray->positionDirection);
    linearTerm *= 2.0;
    tempTerm = shape->objectTerms.dotProduct(ray->direction);
    linearTerm += tempTerm;
    tempTerm =
        shape->objectMixedTerms.dotProduct(ray->mixedPositionDirection);
    linearTerm += tempTerm;

    if (ray->isPrimaryRay) {
        if (!shape->constantCached) {
            constantTerm = shape->object2Terms.dotProduct(ray->position2);
            tempTerm = shape->objectTerms.dotProduct(ray->position);
            constantTerm += tempTerm + shape->objectConstant;
            shape->objectVpConstant = constantTerm;
            shape->constantCached = true;
        } else {
            constantTerm = shape->objectVpConstant;
        }
    } else {
        constantTerm = shape->object2Terms.dotProduct(ray->position2);
        tempTerm = shape->objectTerms.dotProduct(ray->position);
        constantTerm += tempTerm + shape->objectConstant;
    }

    tempTerm = shape->objectMixedTerms.dotProduct(ray->mixedPositionPosition);
    constantTerm += tempTerm;

    if (squareTerm != 0.0) {
        /* The equation is quadratic - find its roots */

        determinant2 =
            linearTerm * linearTerm - 4.0 * squareTerm * constantTerm;

        if (determinant2 < 0.0) {
            return (false);
        }

        determinant = sqrt(determinant2);
        a2 = squareTerm * 2.0;
        bMinus = linearTerm * -1.0;

        *depth1 = (bMinus + determinant) / a2;
        *depth2 = (bMinus - determinant) / a2;
    } else {
        /* There are no quadratic terms.  Solve the linear equation instead. */
        if (linearTerm == 0.0) {
            return (false);
        }

        *depth1 = constantTerm * -1.0 / linearTerm;
        *depth2 = *depth1;
    }

    if ((*depth1 < GeometryConstants::Small_Tolerance) || (*depth1 > GeometryConstants::Max_Distance)) {
        if ((*depth2 < GeometryConstants::Small_Tolerance) || (*depth2 > GeometryConstants::Max_Distance)) {
            return (false);
        }
        *depth1 = *depth2;

    } else if ((*depth2 < GeometryConstants::Small_Tolerance) || (*depth2 > GeometryConstants::Max_Distance)) {
        *depth2 = *depth1;
    }

    Statistics::global().rayQuadricTestsSucceeded++;
    return (true);
}

int
Quadric::insideQuadric(Vector3Dd *testPoint, SimpleBody *object)
{
    Quadric *shape = (Quadric *)object;
    Vector3Dd newPoint;
    double result;
    double linearTerm;
    double squareTerm;

    linearTerm = (*testPoint).dotProduct(shape->objectTerms);
    result = linearTerm + shape->objectConstant;
    VectorOps::vSquareTerms(newPoint, *testPoint);
    squareTerm = newPoint.dotProduct(shape->object2Terms);
    result += squareTerm;
    result += shape->objectMixedTerms.x * (testPoint->x) * (testPoint->y) +
              shape->objectMixedTerms.y * (testPoint->x) * (testPoint->z) +
              shape->objectMixedTerms.z * (testPoint->y) * (testPoint->z);

    if (result < GeometryConstants::Small_Tolerance) {
        return (true);
    }

    return (false);
}

void
Quadric::quadricNormal(
    Vector3Dd *result, SimpleBody *object, Vector3Dd *intersectionPoint)
{
    Quadric *intersectionShape = (Quadric *)object;
    Vector3Dd derivativeLinear;
    double len;

    VectorOps::vScale(derivativeLinear, intersectionShape->object2Terms, 2.0);
    VectorOps::vEvaluate(*result, derivativeLinear, *intersectionPoint);
    (*result).add(intersectionShape->objectTerms);

    result->x +=
        intersectionShape->objectMixedTerms.x * intersectionPoint->y +
        intersectionShape->objectMixedTerms.y * intersectionPoint->z;

    result->y +=
        intersectionShape->objectMixedTerms.x * intersectionPoint->x +
        intersectionShape->objectMixedTerms.z * intersectionPoint->z;

    result->z +=
        intersectionShape->objectMixedTerms.y * intersectionPoint->x +
        intersectionShape->objectMixedTerms.z * intersectionPoint->y;

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
    newShape->nextObject = nullptr;

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
    (*matrix)[0][0] = quadric->object2Terms.x;
    (*matrix)[1][1] = quadric->object2Terms.y;
    (*matrix)[2][2] = quadric->object2Terms.z;
    (*matrix)[0][1] = quadric->objectMixedTerms.x;
    (*matrix)[0][2] = quadric->objectMixedTerms.y;
    (*matrix)[0][3] = quadric->objectTerms.x;
    (*matrix)[1][2] = quadric->objectMixedTerms.z;
    (*matrix)[1][3] = quadric->objectTerms.y;
    (*matrix)[2][3] = quadric->objectTerms.z;
    (*matrix)[3][3] = quadric->objectConstant;
}

void
Quadric::matrixToQuadric(MATRIX *matrix, Quadric *quadric)
{
    quadric->object2Terms.x = (*matrix)[0][0];
    quadric->object2Terms.y = (*matrix)[1][1];
    quadric->object2Terms.z = (*matrix)[2][2];
    quadric->objectMixedTerms.x = (*matrix)[0][1] + (*matrix)[1][0];
    quadric->objectMixedTerms.y = (*matrix)[0][2] + (*matrix)[2][0];
    quadric->objectTerms.x = (*matrix)[0][3] + (*matrix)[3][0];
    quadric->objectMixedTerms.z = (*matrix)[1][2] + (*matrix)[2][1];
    quadric->objectTerms.y = (*matrix)[1][3] + (*matrix)[3][1];
    quadric->objectTerms.z = (*matrix)[2][3] + (*matrix)[3][2];
    quadric->objectConstant = (*matrix)[3][3];
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

    shape->object2Terms.scale(-1.0);
    shape->objectMixedTerms.scale(-1.0);
    shape->objectTerms.scale(-1.0);
    shape->objectConstant *= -1.0;
}
