/****************************************************************************
 *                     triangle.c
 *
 *  This module implements primitives for triangles and smooth triangles.
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

#include "geom/triangle.h"
#include "geom/objects.h"

/*===========================================================================*/

Methods Triangle_Methods = {Object_Intersect, All_Triangle_Intersections,
    Inside_Triangle, Triangle_Normal, Copy_Triangle, Translate_Triangle,
    Rotate_Triangle, Scale_Triangle, Invert_Triangle};

Methods Smooth_Triangle_Methods = {Object_Intersect, All_Triangle_Intersections,
    Inside_Triangle, Smooth_Triangle_Normal, Copy_Smooth_Triangle,
    Translate_Smooth_Triangle, Rotate_Smooth_Triangle, Scale_Smooth_Triangle,
    Invert_Smooth_Triangle};

extern Triangle *Get_Triangle_Shape();

extern Ray *VP_Ray;
extern long Ray_Triangle_Tests, Ray_Triangle_Tests_Succeeded;

#define max3(x, y, z) ((x > y) ? ((x > z) ? 1 : 3) : ((y > z) ? 2 : 3))
#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2

/*===========================================================================*/

static void
Find_Triangle_Dominant_Axis(Triangle *triangle)
{
    DBL x, y, z;

    x = fabs(triangle->Normal_Vector.x);
    y = fabs(triangle->Normal_Vector.y);
    z = fabs(triangle->Normal_Vector.z);
    switch (max3(x, y, z)) {
    case 1:
        triangle->Dominant_Axis = X_AXIS;
        break;
    case 2:
        triangle->Dominant_Axis = Y_AXIS;
        break;
    case 3:
        triangle->Dominant_Axis = Z_AXIS;
        break;
    }
}

static void
Compute_Smooth_Triangle(SmoothTriangle *triangle)
{
    Vector3D P3MinusP2, VTemp1, VTemp2;
    DBL x, y, z, uDenominator, Proj;

    VSub(P3MinusP2, triangle->P3, triangle->P2);
    x = fabs(P3MinusP2.x);
    y = fabs(P3MinusP2.y);
    z = fabs(P3MinusP2.z);

    switch (max3(x, y, z)) {
    case 1:
        triangle->vAxis = X_AXIS;
        triangle->BaseDelta = P3MinusP2.x;
        break;

    case 2:
        triangle->vAxis = Y_AXIS;
        triangle->BaseDelta = P3MinusP2.y;
        break;

    case 3:
        triangle->vAxis = Z_AXIS;
        triangle->BaseDelta = P3MinusP2.z;
        break;
    }

    VSub(VTemp1, triangle->P2, triangle->P3);
    VNormalize(VTemp1, VTemp1);
    VSub(VTemp2, triangle->P1, triangle->P3);
    VDot(Proj, VTemp2, VTemp1);
    VScale(VTemp1, VTemp1, Proj);
    VSub(triangle->Perp, VTemp1, VTemp2);
    VNormalize(triangle->Perp, triangle->Perp);
    VDot(uDenominator, VTemp2, triangle->Perp);
    uDenominator = -1.0 / uDenominator;
    VScale(triangle->Perp, triangle->Perp, uDenominator);
}

int
Compute_Triangle(Triangle *triangle)
{
    Vector3D V1, V2, Temp;
    DBL Length;

    VSub(V1, triangle->P1, triangle->P2);
    VSub(V2, triangle->P3, triangle->P2);
    VCross(triangle->Normal_Vector, V1, V2);
    VLength(Length, triangle->Normal_Vector);
    /* Set up a flag so we can ignore degenerate triangles */
    if (Length < 1.0e-9) {
        triangle->Degenerate_Flag = TRUE;
        return (0);
    }

    /* Normalize the normal vector. */
    VScale(triangle->Normal_Vector, triangle->Normal_Vector, 1.0 / Length);

    VDot(triangle->Distance, triangle->Normal_Vector, triangle->P1);
    triangle->Distance *= -1.0;
    Find_Triangle_Dominant_Axis(triangle);

    switch (triangle->Dominant_Axis) {
    case X_AXIS:
        if ((triangle->P2.y - triangle->P3.y) *
                (triangle->P2.z - triangle->P1.z) <
            (triangle->P2.z - triangle->P3.z) *
                (triangle->P2.y - triangle->P1.y)) {

            Temp = triangle->P2;
            triangle->P2 = triangle->P1;
            triangle->P1 = Temp;
            if (triangle->Type == SMOOTH_TRIANGLE_TYPE) {
                Temp = ((SmoothTriangle *)triangle)->N2;
                ((SmoothTriangle *)triangle)->N2 =
                    ((SmoothTriangle *)triangle)->N1;
                ((SmoothTriangle *)triangle)->N1 = Temp;
            }
        }
        break;

    case Y_AXIS:
        if ((triangle->P2.x - triangle->P3.x) *
                (triangle->P2.z - triangle->P1.z) <
            (triangle->P2.z - triangle->P3.z) *
                (triangle->P2.x - triangle->P1.x)) {

            Temp = triangle->P2;
            triangle->P2 = triangle->P1;
            triangle->P1 = Temp;
            if (triangle->Type == SMOOTH_TRIANGLE_TYPE) {
                Temp = ((SmoothTriangle *)triangle)->N2;
                ((SmoothTriangle *)triangle)->N2 =
                    ((SmoothTriangle *)triangle)->N1;
                ((SmoothTriangle *)triangle)->N1 = Temp;
            }
        }
        break;

    case Z_AXIS:
        if ((triangle->P2.x - triangle->P3.x) *
                (triangle->P2.y - triangle->P1.y) <
            (triangle->P2.y - triangle->P3.y) *
                (triangle->P2.x - triangle->P1.x)) {

            Temp = triangle->P2;
            triangle->P2 = triangle->P1;
            triangle->P1 = Temp;
            if (triangle->Type == SMOOTH_TRIANGLE_TYPE) {
                Temp = ((SmoothTriangle *)triangle)->N2;
                ((SmoothTriangle *)triangle)->N2 =
                    ((SmoothTriangle *)triangle)->N1;
                ((SmoothTriangle *)triangle)->N1 = Temp;
            }
        }
        break;
    }

    if (triangle->Type == SMOOTH_TRIANGLE_TYPE) {
        Compute_Smooth_Triangle((SmoothTriangle *)triangle);
    }
    return (1);
}

int
All_Triangle_Intersections(
    SimpleBody *Object, Ray *Ray, PriorityQueueNode *Depth_Queue)
{
    Triangle *Shape = (Triangle *)Object;
    DBL Depth;
    Vector3D Intersection_Point;
    Intersection Local_Element;

    if (Shape->Degenerate_Flag) {
        return (FALSE);
    }

    if (Intersect_Triangle(Ray, Shape, &Depth)) {
        Local_Element.Depth = Depth;
        Local_Element.Object = Shape->Parent_Object;
        VScale(Intersection_Point, Ray->Direction, Depth);
        VAdd(Intersection_Point, Intersection_Point, Ray->Initial);
        Local_Element.Point = Intersection_Point;
        Local_Element.Shape = (Geometry *)Shape;
        Depth_Queue->add(&Local_Element);
        return (TRUE);
    }
    return (FALSE);
}

int
Intersect_Triangle(Ray *Ray, Triangle *triangle, DBL *Depth)
{
    DBL NormalDotOrigin, NormalDotDirection;
    DBL s, t;

    Ray_Triangle_Tests++;
    if (triangle->Degenerate_Flag) {
        return (FALSE);
    }

    if (Ray == VP_Ray) {
        if (!triangle->VPCached) {
            VDot(triangle->VPNormDotOrigin, triangle->Normal_Vector,
                Ray->Initial);
            triangle->VPNormDotOrigin += triangle->Distance;
            triangle->VPNormDotOrigin *= -1.0;
            triangle->VPCached = TRUE;
        }

        VDot(NormalDotDirection, triangle->Normal_Vector, Ray->Direction);
        if ((NormalDotDirection < Small_Tolerance) &&
            (NormalDotDirection > -Small_Tolerance)) {
            return (FALSE);
        }

        *Depth = triangle->VPNormDotOrigin / NormalDotDirection;
    } else {
        VDot(NormalDotOrigin, triangle->Normal_Vector, Ray->Initial);
        NormalDotOrigin += triangle->Distance;
        NormalDotOrigin *= -1.0;

        VDot(NormalDotDirection, triangle->Normal_Vector, Ray->Direction);
        if ((NormalDotDirection < Small_Tolerance) &&
            (NormalDotDirection > -Small_Tolerance)) {
            return (FALSE);
        }

        *Depth = NormalDotOrigin / NormalDotDirection;
    }

    if ((*Depth < Small_Tolerance) || (*Depth > Max_Distance)) {
        return (FALSE);
    }

    switch (triangle->Dominant_Axis) {
    case X_AXIS:
        s = Ray->Initial.y + *Depth * Ray->Direction.y;
        t = Ray->Initial.z + *Depth * Ray->Direction.z;

        if (((triangle->P2.y - s) * (triangle->P2.z - triangle->P1.z)) <
            ((triangle->P2.z - t) * (triangle->P2.y - triangle->P1.y))) {
            if ((int)triangle->Inverted) {
                Ray_Triangle_Tests_Succeeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if (((triangle->P3.y - s) * (triangle->P3.z - triangle->P2.z)) <
            ((triangle->P3.z - t) * (triangle->P3.y - triangle->P2.y))) {
            if ((int)triangle->Inverted) {
                Ray_Triangle_Tests_Succeeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if (((triangle->P1.y - s) * (triangle->P1.z - triangle->P3.z)) <
            ((triangle->P1.z - t) * (triangle->P1.y - triangle->P3.y))) {
            if ((int)triangle->Inverted) {
                Ray_Triangle_Tests_Succeeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if (!(int)triangle->Inverted) {
            Ray_Triangle_Tests_Succeeded++;
            return (TRUE);
        }
        return (FALSE);

    case Y_AXIS:
        s = Ray->Initial.x + *Depth * Ray->Direction.x;
        t = Ray->Initial.z + *Depth * Ray->Direction.z;

        if ((triangle->P2.x - s) * (triangle->P2.z - triangle->P1.z) <
            (triangle->P2.z - t) * (triangle->P2.x - triangle->P1.x)) {
            if ((int)triangle->Inverted) {
                Ray_Triangle_Tests_Succeeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if ((triangle->P3.x - s) * (triangle->P3.z - triangle->P2.z) <
            (triangle->P3.z - t) * (triangle->P3.x - triangle->P2.x)) {
            if ((int)triangle->Inverted) {
                Ray_Triangle_Tests_Succeeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if ((triangle->P1.x - s) * (triangle->P1.z - triangle->P3.z) <
            (triangle->P1.z - t) * (triangle->P1.x - triangle->P3.x)) {
            if ((int)triangle->Inverted) {
                Ray_Triangle_Tests_Succeeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if (!(int)triangle->Inverted) {
            Ray_Triangle_Tests_Succeeded++;
            return (TRUE);
        }
        return (FALSE);

    case Z_AXIS:
        s = Ray->Initial.x + *Depth * Ray->Direction.x;
        t = Ray->Initial.y + *Depth * Ray->Direction.y;

        if ((triangle->P2.x - s) * (triangle->P2.y - triangle->P1.y) <
            (triangle->P2.y - t) * (triangle->P2.x - triangle->P1.x)) {
            if ((int)triangle->Inverted) {
                Ray_Triangle_Tests_Succeeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if ((triangle->P3.x - s) * (triangle->P3.y - triangle->P2.y) <
            (triangle->P3.y - t) * (triangle->P3.x - triangle->P2.x)) {
            if ((int)triangle->Inverted) {
                Ray_Triangle_Tests_Succeeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if ((triangle->P1.x - s) * (triangle->P1.y - triangle->P3.y) <
            (triangle->P1.y - t) * (triangle->P1.x - triangle->P3.x)) {
            if ((int)triangle->Inverted) {
                Ray_Triangle_Tests_Succeeded++;
                return (TRUE);
            }
            return (FALSE);
        }

        if (!(int)triangle->Inverted) {
            Ray_Triangle_Tests_Succeeded++;
            return (TRUE);
        }
        return (FALSE);
    }
    return (FALSE);
}

int
Inside_Triangle(Vector3D *Test_Point, SimpleBody *Object)
{
    return (FALSE);
}

void
Triangle_Normal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point)
{
    Triangle *triangle = (Triangle *)Object;

    *Result = triangle->Normal_Vector;
}

void *
Copy_Triangle(SimpleBody *Object)
{
    Triangle *New_Shape;

    New_Shape = Get_Triangle_Shape();
    *New_Shape = *((Triangle *)Object);
    New_Shape->Next_Object = NULL;

    if (New_Shape->Shape_Texture != NULL) {
        New_Shape->Shape_Texture = Copy_Texture(New_Shape->Shape_Texture);
    }

    return (New_Shape);
}

void
Translate_Triangle(SimpleBody *Object, Vector3D *Vector)
{
    Triangle *triangle = (Triangle *)Object;
    Vector3D Translation;

    VEvaluate(Translation, triangle->Normal_Vector, *Vector);
    triangle->Distance -= Translation.x + Translation.y + Translation.z;
    VAdd(triangle->P1, triangle->P1, *Vector)
        VAdd(triangle->P2, triangle->P2, *Vector)
            VAdd(triangle->P3, triangle->P3, *Vector)

                Translate_Texture(&((Triangle *)Object)->Shape_Texture, Vector);
}

void
Rotate_Triangle(SimpleBody *Object, Vector3D *Vector)
{
    Transformation transformation;
    Triangle *triangle = (Triangle *)Object;

    Get_Rotation_Transformation(&transformation, Vector);
    MTransformVector(
        &triangle->Normal_Vector, &triangle->Normal_Vector, &transformation);
    MTransformVector(&triangle->P1, &triangle->P1, &transformation);
    MTransformVector(&triangle->P2, &triangle->P2, &transformation);
    MTransformVector(&triangle->P3, &triangle->P3, &transformation);
    Compute_Triangle(triangle);

    Rotate_Texture(&((Triangle *)Object)->Shape_Texture, Vector);
}

void
Scale_Triangle(SimpleBody *Object, Vector3D *Vector)
{
    Triangle *triangle = (Triangle *)Object;
    DBL Length;

    triangle->Normal_Vector.x = triangle->Normal_Vector.x / Vector->x;
    triangle->Normal_Vector.y = triangle->Normal_Vector.y / Vector->y;
    triangle->Normal_Vector.z = triangle->Normal_Vector.z / Vector->z;

    VLength(Length, triangle->Normal_Vector);
    VScale(triangle->Normal_Vector, triangle->Normal_Vector, 1.0 / Length);
    triangle->Distance /= Length;

    VEvaluate(triangle->P1, triangle->P1, *Vector);
    VEvaluate(triangle->P2, triangle->P2, *Vector);
    VEvaluate(triangle->P3, triangle->P3, *Vector);

    Scale_Texture(&((Triangle *)Object)->Shape_Texture, Vector);
}

void
Invert_Triangle(SimpleBody *Object)
{
    Triangle *triangle = (Triangle *)Object;

    triangle->Inverted ^= TRUE;
}

/* Calculate the Phong-interpolated vector within the triangle
    at the given intersection point. The math for this is a bit
    bizarre:

     -            P1
     |          /|\ \
     |         / |Perp\
     |        /  V  \    \
     |      /    |     \    \
  u |     /____|_____PI___\
     |    /      |         \     \
     -  P2-----|--------|----P3
                  Pbase     PIntersect
          |-------------------|
                              v

    triangle->Perp is a unit vector from P1 to Pbase. We calculate

    u = (PI - P1) DOT Perp / ((P3 - P1) DOT Perp).

    We then calculate where the line from P1 to PI intersects the line P2 to P3:
    PIntersect = (PI - P1)/u.

    We really only need one coordinate of PIntersect.  We then calculate v as:

        v = PIntersect.x / (P3.x - P2.x)
 or    v = PIntersect.y / (P3.y - P2.y)
 or    v = PIntersect.z / (P3.z - P2.z)

    depending on which calculation will give us the best answers.

    Once we have u and v, we can perform the normal interpolation as:

      NTemp1 = N1 + u(N2 - N1);
      NTemp2 = N1 + u(N3 - N1);
      Result = normalize (NTemp1 + v(NTemp2 - NTemp1))

    As always, any values which are constant for the triangle are cached
    in the triangle.
*/

void
Smooth_Triangle_Normal(
    Vector3D *Result, SimpleBody *Object, Vector3D *Intersection_Point)
{
    SmoothTriangle *triangle = (SmoothTriangle *)Object;
    Vector3D PIMinusP1, NTemp1, NTemp2;
    DBL u = 0.0, v = 0.0;

    VSub(PIMinusP1, *Intersection_Point, triangle->P1);
    VDot(u, PIMinusP1, triangle->Perp);
    if (u < 1.0e-9) {
        *Result = triangle->N1;
        return;
    }

    /* BaseDelta contains P3.x-P2.x,  P3.y-P2.y, or P3.z-P2.z depending on the
        value of vAxis. */

    switch (triangle->vAxis) {
    case X_AXIS:
        v = (PIMinusP1.x / u + triangle->P1.x - triangle->P2.x) /
            triangle->BaseDelta;
        break;

    case Y_AXIS:
        v = (PIMinusP1.y / u + triangle->P1.y - triangle->P2.y) /
            triangle->BaseDelta;
        break;

    case Z_AXIS:
        v = (PIMinusP1.z / u + triangle->P1.z - triangle->P2.z) /
            triangle->BaseDelta;
        break;
    }

    VSub(NTemp1, triangle->N2, triangle->N1);
    VScale(NTemp1, NTemp1, u);
    VAdd(NTemp1, NTemp1, triangle->N1);
    VSub(NTemp2, triangle->N3, triangle->N1);
    VScale(NTemp2, NTemp2, u);
    VAdd(NTemp2, NTemp2, triangle->N1);
    VSub(*Result, NTemp2, NTemp1);
    VScale(*Result, *Result, v);
    VAdd(*Result, *Result, NTemp1);
    VNormalize(*Result, *Result);
}

void *
Copy_Smooth_Triangle(SimpleBody *Object)
{
    SmoothTriangle *New_Shape;

    New_Shape = Get_Smooth_Triangle_Shape();
    *New_Shape = *((SmoothTriangle *)Object);
    New_Shape->Next_Object = NULL;

    if (New_Shape->Shape_Texture != NULL) {
        New_Shape->Shape_Texture = Copy_Texture(New_Shape->Shape_Texture);
    }

    return (New_Shape);
}

void
Rotate_Smooth_Triangle(SimpleBody *Object, Vector3D *Vector)
{
    Transformation transformation;
    SmoothTriangle *triangle = (SmoothTriangle *)Object;

    Get_Rotation_Transformation(&transformation, Vector);
    MTransformVector(
        &triangle->Normal_Vector, &triangle->Normal_Vector, &transformation);
    MTransformVector(&triangle->P1, &triangle->P1, &transformation);
    MTransformVector(&triangle->P2, &triangle->P2, &transformation);
    MTransformVector(&triangle->P3, &triangle->P3, &transformation);
    MTransformVector(&triangle->N1, &triangle->N1, &transformation);
    MTransformVector(&triangle->N2, &triangle->N2, &transformation);
    MTransformVector(&triangle->N3, &triangle->N3, &transformation);
    Compute_Triangle((Triangle *)triangle);

    Rotate_Texture(&((Triangle *)Object)->Shape_Texture, Vector);
}

void
Translate_Smooth_Triangle(SimpleBody *Object, Vector3D *Vector)
{
    SmoothTriangle *triangle = (SmoothTriangle *)Object;
    Vector3D Translation;

    VEvaluate(Translation, triangle->Normal_Vector, *Vector);
    triangle->Distance -= Translation.x + Translation.y + Translation.z;
    VAdd(triangle->P1, triangle->P1, *Vector)
        VAdd(triangle->P2, triangle->P2, *Vector)
            VAdd(triangle->P3, triangle->P3, *Vector)
                Compute_Triangle((Triangle *)triangle);

    Translate_Texture(&((Triangle *)Object)->Shape_Texture, Vector);
}

void
Scale_Smooth_Triangle(SimpleBody *Object, Vector3D *Vector)
{
    SmoothTriangle *triangle = (SmoothTriangle *)Object;
    DBL Length;

    triangle->Normal_Vector.x = triangle->Normal_Vector.x / Vector->x;
    triangle->Normal_Vector.y = triangle->Normal_Vector.y / Vector->y;
    triangle->Normal_Vector.z = triangle->Normal_Vector.z / Vector->z;

    VLength(Length, triangle->Normal_Vector);
    VScale(triangle->Normal_Vector, triangle->Normal_Vector, 1.0 / Length);
    triangle->Distance /= Length;

    VEvaluate(triangle->P1, triangle->P1, *Vector);
    VEvaluate(triangle->P2, triangle->P2, *Vector);
    VEvaluate(triangle->P3, triangle->P3, *Vector);
    Compute_Triangle((Triangle *)triangle);

    Scale_Texture(&((SmoothTriangle *)Object)->Shape_Texture, Vector);
}

void
Invert_Smooth_Triangle(SimpleBody *Object)
{
    SmoothTriangle *triangle = (SmoothTriangle *)Object;

    triangle->Inverted ^= TRUE;
}
