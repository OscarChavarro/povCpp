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

extern Ray *VP_Ray;
extern long Ray_Quadric_Tests, Ray_Quadric_Tests_Succeeded;

/*===========================================================================*/

int
All_Quadric_Intersections(
    SimpleBody *Object, Ray *ray, PriorityQueueNode *Depth_Queue)
{
    Quadric *Shape = (Quadric *)Object;
    DBL Depth1, Depth2;
    Vector3D Intersection_Point;
    Intersection Local_Element;
    register int Intersection_Found;

    Intersection_Found = FALSE;
    if (Intersect_Quadric(ray, Shape, &Depth1, &Depth2)) {
        Local_Element.Depth = Depth1;
        Local_Element.Object = Shape->Parent_Object;
        VScale(Intersection_Point, ray->Direction, Depth1);
        VAdd(Intersection_Point, Intersection_Point, ray->Initial);
        Local_Element.Point = Intersection_Point;
        Local_Element.Shape = (Geometry *)Shape;
        Depth_Queue->add(&Local_Element);
        Intersection_Found = TRUE;

        if (Depth2 != Depth1) {
            Local_Element.Depth = Depth2;
            Local_Element.Object = Shape->Parent_Object;
            VScale(Intersection_Point, ray->Direction, Depth2);
            VAdd(Intersection_Point, Intersection_Point, ray->Initial);
            Local_Element.Point = Intersection_Point;
            Local_Element.Shape = (Geometry *)Shape;
            Depth_Queue->add(&Local_Element);
            Intersection_Found = TRUE;
        }
    }
    return (Intersection_Found);
}

int
Intersect_Quadric(Ray *ray, Quadric *Shape, DBL *Depth1, DBL *Depth2)
{
    register DBL Square_Term, Linear_Term, Constant_Term, Temp_Term;
    register DBL Determinant, Determinant_2, A2, BMinus;

    Ray_Quadric_Tests++;
    if (!ray->Quadric_Constants_Cached) {
        ray->makeRay();
    }

    if (Shape->Non_Zero_Square_Term) {
        VDot(Square_Term, Shape->Object_2_Terms, ray->Direction_2);
        VDot(Temp_Term, Shape->Object_Mixed_Terms, ray->Mixed_Dir_Dir);
        Square_Term += Temp_Term;
    } else {
        Square_Term = 0.0;
    }

    VDot(Linear_Term, Shape->Object_2_Terms, ray->Initial_Direction);
    Linear_Term *= 2.0;
    VDot(Temp_Term, Shape->Object_Terms, ray->Direction);
    Linear_Term += Temp_Term;
    VDot(Temp_Term, Shape->Object_Mixed_Terms, ray->Mixed_Init_Dir);
    Linear_Term += Temp_Term;

    if (ray == VP_Ray) {
        if (!Shape->Constant_Cached) {
            VDot(Constant_Term, Shape->Object_2_Terms, ray->Initial_2);
            VDot(Temp_Term, Shape->Object_Terms, ray->Initial);
            Constant_Term += Temp_Term + Shape->Object_Constant;
            Shape->Object_VP_Constant = Constant_Term;
            Shape->Constant_Cached = TRUE;
        } else {
            Constant_Term = Shape->Object_VP_Constant;
        }
    } else {
        VDot(Constant_Term, Shape->Object_2_Terms, ray->Initial_2);
        VDot(Temp_Term, Shape->Object_Terms, ray->Initial);
        Constant_Term += Temp_Term + Shape->Object_Constant;
    }

    VDot(Temp_Term, Shape->Object_Mixed_Terms, ray->Mixed_Initial_Initial);
    Constant_Term += Temp_Term;

    if (Square_Term != 0.0) {
        /* The equation is quadratic - find its roots */

        Determinant_2 =
            Linear_Term * Linear_Term - 4.0 * Square_Term * Constant_Term;

        if (Determinant_2 < 0.0) {
            return (FALSE);
        }

        Determinant = sqrt(Determinant_2);
        A2 = Square_Term * 2.0;
        BMinus = Linear_Term * -1.0;

        *Depth1 = (BMinus + Determinant) / A2;
        *Depth2 = (BMinus - Determinant) / A2;
    } else {
        /* There are no quadratic terms.  Solve the linear equation instead. */
        if (Linear_Term == 0.0) {
            return (FALSE);
        }

        *Depth1 = Constant_Term * -1.0 / Linear_Term;
        *Depth2 = *Depth1;
    }

    if ((*Depth1 < Small_Tolerance) || (*Depth1 > Max_Distance)) {
        if ((*Depth2 < Small_Tolerance) || (*Depth2 > Max_Distance)) {
            return (FALSE);
        } else {
            *Depth1 = *Depth2;
        }
    } else if ((*Depth2 < Small_Tolerance) || (*Depth2 > Max_Distance)) {
        *Depth2 = *Depth1;
    }

    Ray_Quadric_Tests_Succeeded++;
    return (TRUE);
}

int
Inside_Quadric(Vector3D *Test_Point, SimpleBody *Object)
{
    Quadric *Shape = (Quadric *)Object;
    Vector3D New_Point;
    register DBL Result, Linear_Term, Square_Term;

    VDot(Linear_Term, *Test_Point, Shape->Object_Terms);
    Result = Linear_Term + Shape->Object_Constant;
    VSquareTerms(New_Point, *Test_Point);
    VDot(Square_Term, New_Point, Shape->Object_2_Terms);
    Result += Square_Term;
    Result += Shape->Object_Mixed_Terms.x * (Test_Point->x) * (Test_Point->y) +
              Shape->Object_Mixed_Terms.y * (Test_Point->x) * (Test_Point->z) +
              Shape->Object_Mixed_Terms.z * (Test_Point->y) * (Test_Point->z);

    if (Result < Small_Tolerance) {
        return (TRUE);
    }

    return (FALSE);
}

void
Quadric_Normal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point)
{
    Quadric *Intersection_Shape = (Quadric *)Object;
    Vector3D Derivative_Linear;
    DBL Len;

    VScale(Derivative_Linear, Intersection_Shape->Object_2_Terms, 2.0);
    VEvaluate(*Result, Derivative_Linear, *Intersection_Point);
    VAdd(*Result, *Result, Intersection_Shape->Object_Terms);

    Result->x +=
        Intersection_Shape->Object_Mixed_Terms.x * Intersection_Point->y +
        Intersection_Shape->Object_Mixed_Terms.y * Intersection_Point->z;

    Result->y +=
        Intersection_Shape->Object_Mixed_Terms.x * Intersection_Point->x +
        Intersection_Shape->Object_Mixed_Terms.z * Intersection_Point->z;

    Result->z +=
        Intersection_Shape->Object_Mixed_Terms.y * Intersection_Point->x +
        Intersection_Shape->Object_Mixed_Terms.z * Intersection_Point->y;

    Len = Result->x * Result->x + Result->y * Result->y + Result->z * Result->z;
    Len = sqrt(Len);
    if (Len == 0.0) {
        /* The normal is not defined at this point of the surface.  Set it
            to any arbitrary direction. */
        Result->x = 1.0;
        Result->y = 0.0;
        Result->z = 0.0;
    } else {
        Result->x /= Len; /* normalize the normal */
        Result->y /= Len;
        Result->z /= Len;
    }
}

void *
Copy_Quadric(SimpleBody *Object)
{
    Quadric *New_Shape;

    New_Shape = Get_Quadric_Shape();
    *New_Shape = *((Quadric *)Object);
    New_Shape->Next_Object = NULL;

    if (New_Shape->Shape_Texture != NULL) {
        New_Shape->Shape_Texture = Copy_Texture(New_Shape->Shape_Texture);
    }

    return (New_Shape);
}

static void
Quadric_To_Matrix(Quadric *Quadric, MATRIX *Matrix)
{
    MZero(Matrix);
    (*Matrix)[0][0] = Quadric->Object_2_Terms.x;
    (*Matrix)[1][1] = Quadric->Object_2_Terms.y;
    (*Matrix)[2][2] = Quadric->Object_2_Terms.z;
    (*Matrix)[0][1] = Quadric->Object_Mixed_Terms.x;
    (*Matrix)[0][2] = Quadric->Object_Mixed_Terms.y;
    (*Matrix)[0][3] = Quadric->Object_Terms.x;
    (*Matrix)[1][2] = Quadric->Object_Mixed_Terms.z;
    (*Matrix)[1][3] = Quadric->Object_Terms.y;
    (*Matrix)[2][3] = Quadric->Object_Terms.z;
    (*Matrix)[3][3] = Quadric->Object_Constant;
}

static void
Matrix_To_Quadric(MATRIX *Matrix, Quadric *Quadric)
{
    Quadric->Object_2_Terms.x = (*Matrix)[0][0];
    Quadric->Object_2_Terms.y = (*Matrix)[1][1];
    Quadric->Object_2_Terms.z = (*Matrix)[2][2];
    Quadric->Object_Mixed_Terms.x = (*Matrix)[0][1] + (*Matrix)[1][0];
    Quadric->Object_Mixed_Terms.y = (*Matrix)[0][2] + (*Matrix)[2][0];
    Quadric->Object_Terms.x = (*Matrix)[0][3] + (*Matrix)[3][0];
    Quadric->Object_Mixed_Terms.z = (*Matrix)[1][2] + (*Matrix)[2][1];
    Quadric->Object_Terms.y = (*Matrix)[1][3] + (*Matrix)[3][1];
    Quadric->Object_Terms.z = (*Matrix)[2][3] + (*Matrix)[3][2];
    Quadric->Object_Constant = (*Matrix)[3][3];
}

static void
Transform_Quadric(Quadric *Shape, Transformation *transformation)
{
    MATRIX Quadric_Matrix, Transform_Transposed;

    Quadric_To_Matrix(Shape, (MATRIX *)&Quadric_Matrix[0][0]);
    MTimes((MATRIX *)&Quadric_Matrix[0][0],
        (MATRIX *)&(transformation->inverse[0][0]),
        (MATRIX *)&Quadric_Matrix[0][0]);
    MTranspose((MATRIX *)&Transform_Transposed[0][0],
        (MATRIX *)&(transformation->inverse[0][0]));
    MTimes((MATRIX *)&Quadric_Matrix[0][0], (MATRIX *)&Quadric_Matrix[0][0],
        (MATRIX *)&Transform_Transposed[0][0]);
    Matrix_To_Quadric((MATRIX *)&Quadric_Matrix[0][0], Shape);
}

void
Translate_Quadric(SimpleBody *Object, Vector3D *Vector)
{
    Transformation transformation;

    Get_Translation_Transformation(&transformation, Vector);
    Transform_Quadric((Quadric *)Object, &transformation);

    Translate_Texture(&((Quadric *)Object)->Shape_Texture, Vector);
}

void
Rotate_Quadric(SimpleBody *Object, Vector3D *Vector)
{
    Transformation transformation;

    Get_Rotation_Transformation(&transformation, Vector);
    Transform_Quadric((Quadric *)Object, &transformation);

    Rotate_Texture(&((Quadric *)Object)->Shape_Texture, Vector);
}

void
Scale_Quadric(SimpleBody *Object, Vector3D *Vector)
{
    Transformation transformation;

    Get_Scaling_Transformation(&transformation, Vector);
    Transform_Quadric((Quadric *)Object, &transformation);

    Scale_Texture(&((Quadric *)Object)->Shape_Texture, Vector);
}

void
Invert_Quadric(SimpleBody *Object)
{
    Quadric *Shape = (Quadric *)Object;

    VScale(Shape->Object_2_Terms, Shape->Object_2_Terms, -1.0);
    VScale(Shape->Object_Mixed_Terms, Shape->Object_Mixed_Terms, -1.0);
    VScale(Shape->Object_Terms, Shape->Object_Terms, -1.0);
    Shape->Object_Constant *= -1.0;
}
