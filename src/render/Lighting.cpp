/****************************************************************************
 *                         lighting.c
 *
 *  This module calculates lighting properties like ambient, diffuse, specular,
 *  reflection, refraction, etc.
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

#include "render/Lighting.h"
#include "common/Frame.h"
#include "common/Matrices.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Light.h"
#include "geom/PrioQ.h"
#include "media/TxtBump.h"
#include "media/TxtColor.h"
#include "media/TxtMap.h"
#include "media/TxtTest.h"
#include "render/Render.h"

extern int traceLevel;
extern Frame globalFrame;
extern unsigned int Options;
extern int quality;
extern int shadowTestFlag;
extern long shadowRayTests, shadowRaysSucceeded;
extern long reflectedRaysTraced, refractedRaysTraced;
extern long transmittedRaysTraced;

/* "Small_Tolerance" is just too tight for higher order polynomial equations.
    this value should probably be a variable of some sort, but for now just
    use a reasonably small value.  If people render real small objects real
    close to each other then there may be some shading problems.  Otherwise
    having SHADOW_TOLERANCE as large as this won't affect images. */
#define SHADOW_TOLERANCE 0.05

static void doLight(Light *lightSource, DBL *lightSourceDepth,
    Ray *lightSourceRay, Vector3D *intersectionPoint, RGBAColor *lightColour);
static int doBlocking(Intersection *localIntersection, RGBAColor *lightColour,
    PriorityQueueNode *localQueue);
static void doPhong(Texture *texture, Ray *lightSourceRay, Vector3D eye,
    Vector3D *surfaceNormal, RGBAColor *colour, RGBAColor *lightColour,
    RGBAColor *surfaceColour);
static void doSpecular(Texture *texture, Ray *lightSourceRay, Vector3D rEye,
    Vector3D *surfaceNormal, RGBAColor *colour, RGBAColor *lightColour,
    RGBAColor *surfaceColour);
static void doDiffuse(Texture *texture, Ray *lightSourceRay,
    Vector3D *surfaceNormal, RGBAColor *colour, RGBAColor *lightColour,
    RGBAColor *surfaceColour, DBL attenuation);

void
Perturb_Normal(Vector3D *newNormal, Texture *texture,
    Vector3D *intersectionPoint, Vector3D *surfaceNormal)
{
    Vector3D transformedPoint;
    register DBL x;
    register DBL y;
    register DBL z;

    if (texture->Bump_Number == NO_BUMPS) {
        *newNormal = *surfaceNormal;
        return;
    }

    if (texture->Texture_Transformation) {
        MInverseTransformVector(&transformedPoint, intersectionPoint,
            texture->Texture_Transformation);
    } else {
        transformedPoint = *intersectionPoint;
    }

    x = transformedPoint.x;
    y = transformedPoint.y;
    z = transformedPoint.z;

    switch (texture->Bump_Number) {

    case WAVES:
        waves(x, y, z, texture, newNormal);
        break;

    case RIPPLES:
        ripples(x, y, z, texture, newNormal);
        break;

    case WRINKLES:
        wrinkles(x, y, z, texture, newNormal);
        break;

    case BUMPS:
        bumps(x, y, z, texture, newNormal);
        break;

    case DENTS:
        dents(x, y, z, texture, newNormal);
        break;

    case BUMPY1:
        bumpy1(x, y, z, texture, newNormal);
        break;

    case BUMPY2:
        bumpy2(x, y, z, texture, newNormal);
        break;

    case BUMPY3:
        bumpy3(x, y, z, texture, newNormal);
        break;

    case BUMPMAP:
        bump_map(x, y, z, texture, newNormal);
        break;
    }
}

void
Ambient(Texture *texture, RGBAColor *surfaceColour, RGBAColor *colour,
    DBL attenuation)
{
    if (texture->Object_Ambient == 0.0) {
        return;
    }

    colour->Red += surfaceColour->Red * texture->Object_Ambient * attenuation;
    colour->Green +=
        surfaceColour->Green * texture->Object_Ambient * attenuation;
    colour->Blue += surfaceColour->Blue * texture->Object_Ambient * attenuation;
}

void
Diffuse(Texture *texture, Vector3D *intersectionPoint, Ray *eye,
    Vector3D *surfaceNormal, RGBAColor *surfaceColour, RGBAColor *colour,
    DBL attenuation)
{
    DBL lightSourceDepth;
    Ray lightSourceRay;
    Light *lightSource;
    SimpleBody *blockingObject;
    int intersectionFound;
    Intersection *localIntersection;
    Vector3D rEye;
    RGBAColor lightColour;
    PriorityQueueNode *localQueue;

    rEye.x = 0;
    rEye.y = 0;
    rEye.z = 0;

    if ((texture->Object_Diffuse == 0.0) && (texture->Object_Specular == 0.0) &&
        (texture->Object_Phong == 0.0)) {
        return;
    }

    if (texture->Object_Specular != 0.0) {
        rEye.x = -eye->Direction.x;
        rEye.y = -eye->Direction.y;
        rEye.z = -eye->Direction.z;
    }

    localQueue = pq_pop(128);

    for (lightSource = globalFrame.Light_Sources; lightSource != nullptr;
         lightSource = lightSource->Next_Light_Source) {
        intersectionFound = FALSE;

        doLight(lightSource, &lightSourceDepth, &lightSourceRay,
            intersectionPoint, &lightColour);

        /* What objects does this ray intersect? */
        if (quality > 3) {
            shadowTestFlag = TRUE;
            for (blockingObject = globalFrame.Objects;
                 blockingObject != nullptr;
                 blockingObject = blockingObject->Next_Object) {

                shadowRayTests++;
                for (All_Intersections(
                         blockingObject, &lightSourceRay, localQueue);
                     (localIntersection = localQueue->getHighest()) != nullptr;
                     localQueue->deleteHighest()) {

                    if ((localIntersection->Depth <
                            lightSourceDepth - Small_Tolerance) &&
                        (localIntersection->Depth > SHADOW_TOLERANCE)) {

                        /* Does the object not cast a shadow? */
                        if (!localIntersection->Object->No_Shadow_Flag) {
                            if (doBlocking(localIntersection, &lightColour,
                                    localQueue)) {
                                intersectionFound = TRUE;
                                break;
                            }
                        }
                    }
                }
                if (intersectionFound) {
                    break;
                }
            }
            shadowTestFlag = FALSE;
        }

        /* If light source was not blocked by any intervening object, then
              calculate it's contribution to the object's overall illumination
         */

        if (!intersectionFound) {
            if (texture->Object_Phong > 0.0) { /* Phong Hilite */
                doPhong(texture, &lightSourceRay, eye->Direction, surfaceNormal,
                    colour, &lightColour, surfaceColour);
            }

            if (texture->Object_Specular > 0.0) { /* Specular Hilite */
                doSpecular(texture, &lightSourceRay, rEye, surfaceNormal,
                    colour, &lightColour, surfaceColour);
            }

            if (texture->Object_Diffuse > 0.0) { /* Normal Diffuse Illum. */
                doDiffuse(texture, &lightSourceRay, surfaceNormal, colour,
                    &lightColour, surfaceColour, attenuation);
            }
        }
    }
    localQueue->pushBackToPool();
}

static void
doLight(Light *lightSource, DBL *lightSourceDepth, Ray *lightSourceRay,
    Vector3D *intersectionPoint, RGBAColor *lightColour)
{
    DBL attenuation = 1.0;

    /* Get the light source colour. */
    if (lightSource->Shape_Colour == nullptr) {
        Make_Colour(lightColour, 1.0, 1.0, 1.0);
    } else {
        *lightColour = *lightSource->Shape_Colour;
    }

    lightSourceRay->Initial = *intersectionPoint;
    lightSourceRay->Quadric_Constants_Cached = FALSE;

    VSub(lightSourceRay->Direction, lightSource->Center, *intersectionPoint);

    VLength(*lightSourceDepth, lightSourceRay->Direction);

    VScale(lightSourceRay->Direction, lightSourceRay->Direction,
        1.0 / (*lightSourceDepth));

    attenuation = Attenuate_Light(lightSource, lightSourceRay);

    /* Now scale the color by the attenuation */
    lightColour->Red *= attenuation;
    lightColour->Green *= attenuation;
    lightColour->Blue *= attenuation;
}

static int
doBlocking(Intersection *localIntersection, RGBAColor *lightColour,
    PriorityQueueNode *localQueue)
{
    shadowRaysSucceeded++;

    Determine_Surface_Colour(localIntersection, lightColour, nullptr, TRUE);

    if ((lightColour->Red < 0.01) && (lightColour->Green < 0.01) &&
        (lightColour->Blue < 0.01)) {

        while (localQueue->getHighest()) {
            localQueue->deleteHighest();
        }
        return TRUE;
    }
    return FALSE;
}

static void
doPhong(Texture *texture, Ray *lightSourceRay, Vector3D eye,
    Vector3D *surfaceNormal, RGBAColor *colour, RGBAColor *lightColour,
    RGBAColor *surfaceColour)
{
    DBL cosAngleOfIncidence, normalLength, intensity;
    Vector3D localNormal;
    Vector3D normalProjection;
    Vector3D reflectDirection;

    VDot(cosAngleOfIncidence, eye, *surfaceNormal);

    if (cosAngleOfIncidence < 0.0) {
        localNormal = *surfaceNormal;
        cosAngleOfIncidence = -cosAngleOfIncidence;
    } else {
        VScale(localNormal, *surfaceNormal, -1.0);
    }

    VScale(normalProjection, localNormal, cosAngleOfIncidence);
    VScale(normalProjection, normalProjection, 2.0);
    VAdd(reflectDirection, eye, normalProjection);

    VDot(cosAngleOfIncidence, reflectDirection, lightSourceRay->Direction);
    VLength(normalLength, lightSourceRay->Direction);

    if (normalLength == 0.0) {
        cosAngleOfIncidence = 0.0;
    } else {
        cosAngleOfIncidence /= normalLength;
    }

    if (cosAngleOfIncidence < 0.0) {
        cosAngleOfIncidence = 0;
    }

    if (texture->Object_PhongSize != 1.0) {
        intensity = pow(cosAngleOfIncidence, texture->Object_PhongSize);
    } else {
        intensity = cosAngleOfIncidence;
    }

    intensity *= texture->Object_Phong;

    if (texture->Metallic_Flag) {
        colour->Red += intensity * (surfaceColour->Red);
        colour->Green += intensity * (surfaceColour->Green);
        colour->Blue += intensity * (surfaceColour->Blue);
    } else {
        colour->Red += intensity * (lightColour->Red);
        colour->Green += intensity * (lightColour->Green);
        colour->Blue += intensity * (lightColour->Blue);
    }
}

static void
doSpecular(Texture *texture, Ray *lightSourceRay, Vector3D rEye,
    Vector3D *surfaceNormal, RGBAColor *colour, RGBAColor *lightColour,
    RGBAColor *surfaceColour)
{
    DBL cosAngleOfIncidence, normalLength, intensity, halfwayLength, roughness;
    Vector3D halfway;

    VHalf(halfway, rEye, lightSourceRay->Direction);
    VLength(normalLength, *surfaceNormal);
    VLength(halfwayLength, halfway);
    VDot(cosAngleOfIncidence, halfway, *surfaceNormal);

    if (normalLength == 0.0 || halfwayLength == 0.0) {
        cosAngleOfIncidence = 0.0;
    } else {
        cosAngleOfIncidence /= (normalLength * halfwayLength);
    }

    if (cosAngleOfIncidence < 0.0) {
        cosAngleOfIncidence = 0.0;
    }

    roughness = 1.0 / texture->Object_Roughness;

    if (roughness != 1.0) {
        intensity = pow(cosAngleOfIncidence, roughness);
    } else {
        intensity = cosAngleOfIncidence;
    }
    intensity *= texture->Object_Specular;
    if (texture->Metallic_Flag) {
        colour->Red += intensity * (surfaceColour->Red);
        colour->Green += intensity * (surfaceColour->Green);
        colour->Blue += intensity * (surfaceColour->Blue);
    } else {
        colour->Red += intensity * (lightColour->Red);
        colour->Green += intensity * (lightColour->Green);
        colour->Blue += intensity * (lightColour->Blue);
    }
}

static void
doDiffuse(Texture *texture, Ray *lightSourceRay, Vector3D *surfaceNormal,
    RGBAColor *colour, RGBAColor *lightColour, RGBAColor *surfaceColour,
    DBL attenuation)
{
    DBL cosAngleOfIncidence, intensity, randomNumber;

    VDot(cosAngleOfIncidence, *surfaceNormal, lightSourceRay->Direction);
    if (cosAngleOfIncidence < 0.0) {
        cosAngleOfIncidence = -cosAngleOfIncidence;
    }

    if (texture->Object_Brilliance != 1.0) {
        intensity = pow(cosAngleOfIncidence, texture->Object_Brilliance);
    } else {
        intensity = cosAngleOfIncidence;
    }

    intensity *= texture->Object_Diffuse * attenuation;

    randomNumber = (rand() & 0x7FFF) / (DBL)0x7FFF;

    intensity -= randomNumber * texture->Texture_Randomness;

    colour->Red += intensity * (surfaceColour->Red) * (lightColour->Red);
    colour->Green += intensity * (surfaceColour->Green) * (lightColour->Green);
    colour->Blue += intensity * (surfaceColour->Blue) * (lightColour->Blue);
}

void
Reflect(Texture *texture, Vector3D *intersectionPoint, Ray *ray,
    Vector3D *surfaceNormal, RGBAColor *colour)
{
    Ray newRay;
    RGBAColor tempColour;
    Vector3D localNormal;
    Vector3D normalProjection;
    Vector3D surfaceOffset;
    register DBL normalComponent;

    if (texture->Object_Reflection != 0.0) {
        reflectedRaysTraced++;
        VDot(normalComponent, ray->Direction, *surfaceNormal);
        if (normalComponent < 0.0) {
            localNormal = *surfaceNormal;
            normalComponent *= -1.0;
        } else
            VScale(localNormal, *surfaceNormal, -1.0);

        VScale(normalProjection, localNormal, normalComponent);
        VScale(normalProjection, normalProjection, 2.0);
        VAdd(newRay.Direction, ray->Direction, normalProjection);
        newRay.Initial = *intersectionPoint;

        /* ARE 08/25/91 */

        VScale(surfaceOffset, newRay.Direction, 2.0 * Small_Tolerance);
        VAdd(newRay.Initial, newRay.Initial, surfaceOffset);

        Copy_Ray_Containers(&newRay, ray);
        traceLevel++;
        Make_Colour(&tempColour, 0.0, 0.0, 0.0);
        newRay.Quadric_Constants_Cached = FALSE;
        Trace(&newRay, &tempColour);
        traceLevel--;

        colour->Red += tempColour.Red * texture->Object_Reflection;
        colour->Green += tempColour.Green * texture->Object_Reflection;
        colour->Blue += tempColour.Blue * texture->Object_Reflection;
    }
}

void
Refract(Texture *texture, Vector3D *intersectionPoint, Ray *ray,
    Vector3D *surfaceNormal, RGBAColor *colour)
{
    Ray newRay;
    RGBAColor tempColour;
    Vector3D localNormal;
    Vector3D rayDirection;
    register DBL normalComponent;
    register DBL tempIor;
    DBL temp, ior;

    if (surfaceNormal == nullptr) {
        newRay.Initial = *intersectionPoint;
        newRay.Direction = ray->Direction;

        Copy_Ray_Containers(&newRay, ray);
        traceLevel++;
        transmittedRaysTraced++;
        Make_Colour(&tempColour, 0.0, 0.0, 0.0);
        newRay.Quadric_Constants_Cached = FALSE;
        Trace(&newRay, &tempColour);
        traceLevel--;
        (colour->Red) += tempColour.Red;
        (colour->Green) += tempColour.Green;
        (colour->Blue) += tempColour.Blue;
    } else {
        refractedRaysTraced++;
        VDot(normalComponent, ray->Direction, *surfaceNormal);
        if (normalComponent <= 0.0) {
            localNormal.x = surfaceNormal->x;
            localNormal.y = surfaceNormal->y;
            localNormal.z = surfaceNormal->z;
            normalComponent *= -1.0;
        } else {
            VScale(localNormal, *surfaceNormal, -1.0);
        }

        Copy_Ray_Containers(&newRay, ray);

        if (ray->Containing_Index == -1) {
            /* The ray is entering from the atmosphere */
            Ray_Enter(&newRay, texture);
            ior = (globalFrame.Atmosphere_IOR) /
                  (texture->Object_Index_Of_Refraction);
        } else {
            /* The ray is currently inside an object */
            if (newRay.Containing_Textures[newRay.Containing_Index] == texture)
            /*            if (inside) */
            {
                /* The ray is leaving the current object */
                newRay.exitContainingMedium();
                if (newRay.Containing_Index == -1) {
                    /* The ray is leaving into the atmosphere */
                    tempIor = globalFrame.Atmosphere_IOR;
                } else {
                    /* The ray is leaving into another object */
                    tempIor = newRay.Containing_IORs[newRay.Containing_Index];
                }

                ior = (texture->Object_Index_Of_Refraction) / tempIor;
            } else {
                /* The ray is entering a new object */
                tempIor = newRay.Containing_IORs[newRay.Containing_Index];
                Ray_Enter(&newRay, texture);

                ior = tempIor / (texture->Object_Index_Of_Refraction);
            }
        }

        temp = 1.0 + ior * ior * (normalComponent * normalComponent - 1.0);
        if (temp < 0.0) {
            /* Total internal reflection - not yet implemented.
    Reflect (Texture, Intersection_Point, ray, Surface_Normal, Color);
    */
            return;
        }

        temp = ior * normalComponent - sqrt(temp);
        VScale(localNormal, localNormal, temp);
        VScale(rayDirection, ray->Direction, ior);
        VAdd(newRay.Direction, localNormal, rayDirection);
        VNormalize(newRay.Direction, newRay.Direction);

        newRay.Initial = *intersectionPoint;
        traceLevel++;
        Make_Colour(&tempColour, 0.0, 0.0, 0.0);
        newRay.Quadric_Constants_Cached = FALSE;

        Trace(&newRay, &tempColour);
        traceLevel--;

        (colour->Red) += (tempColour.Red) * (texture->Object_Refraction);
        (colour->Green) += (tempColour.Green) * (texture->Object_Refraction);
        (colour->Blue) += (tempColour.Blue) * (texture->Object_Refraction);
    }
}

void
Fog(DBL distance, RGBAColor *fogColour, DBL fogDistance, RGBAColor *colour)
{
    DBL fogFactor, fogFactorInverse;

    fogFactor = exp(-1.0 * distance / fogDistance);
    fogFactorInverse = 1.0 - fogFactor;
    colour->Red = colour->Red * fogFactor + fogColour->Red * fogFactorInverse;
    colour->Green =
        colour->Green * fogFactor + fogColour->Green * fogFactorInverse;
    colour->Blue =
        colour->Blue * fogFactor + fogColour->Blue * fogFactorInverse;
}

void
Compute_Reflected_Colour(Ray *ray, Texture *texture,
    Intersection *rayIntersection, RGBAColor *surfaceColour,
    RGBAColor *filterColour, RGBAColor *colour)
{
    Vector3D surfaceNormal;
    DBL normalDirection, attenuation;
    RGBAColor emittedColour;

    /* This variable keeps track of how much colour comes from the surface
of the object and how much is transmited through. */

    Make_Colour(&emittedColour, 0.0, 0.0, 0.0);

    if (texture == nullptr) {
        texture = rayIntersection->Object->Object_Texture;
    }

    if (quality <= 1) {
        surfaceColour->Alpha = 0.0;

        colour->Red += surfaceColour->Red * filterColour->Alpha;
        colour->Green += surfaceColour->Green * filterColour->Alpha;
        colour->Blue += surfaceColour->Blue * filterColour->Alpha;
        return;
    }

    Normal(&surfaceNormal, (SimpleBody *)rayIntersection->Shape,
        &rayIntersection->Point);

    if (quality >= 8) {
        Perturb_Normal(
            &surfaceNormal, texture, &rayIntersection->Point, &surfaceNormal);
    }

    /* If the surface normal points away, flip its direction. */
    VDot(normalDirection, surfaceNormal, ray->Direction);
    if (normalDirection > 0.0) {
        VScale(surfaceNormal, surfaceNormal, -1.0);
    }

    attenuation = filterColour->Alpha * (1.0 - surfaceColour->Alpha);

    Ambient(texture, surfaceColour, &emittedColour, attenuation);
    Diffuse(texture, &rayIntersection->Point, ray, &surfaceNormal,
        surfaceColour, &emittedColour, attenuation);
    colour->Red += emittedColour.Red;
    colour->Green += emittedColour.Green;
    colour->Blue += emittedColour.Blue;
    if (quality >= 8) {
        Reflect(texture, &rayIntersection->Point, ray, &surfaceNormal, colour);
    }
}

void
Determine_Surface_Colour(
    Intersection *rayIntersection, RGBAColor *colour, Ray *ray, int shadowRay)
{
    RGBAColor surfaceColour;
    RGBAColor refractedColour;
    RGBAColor filterColour;
    Texture *tempTexture;
    Texture *texture;
    Vector3D surfaceNormal;
    DBL normalDirection;
    int surface;

    if (!shadowRay)
        Make_Colour(colour, 0.0, 0.0, 0.0);

    if (Options & DEBUGGING) {
        if (rayIntersection->Shape->Shape_Colour) {
            printf("Depth: %f Object %d Colour %f %f %f ",
                rayIntersection->Depth, rayIntersection->Shape->Type,
                rayIntersection->Shape->Shape_Colour->Red,
                rayIntersection->Shape->Shape_Colour->Green,
                rayIntersection->Shape->Shape_Colour->Blue);
        } else {
            printf("Depth: %f Object %d Colour NIL ", rayIntersection->Depth,
                rayIntersection->Shape->Type);
        }
    }

    Make_Colour(&surfaceColour, 0.0, 0.0, 0.0);

    /* Is there a texture in the shape?  If not, use the one in the object. */
    if ((texture = rayIntersection->Shape->Shape_Texture) == nullptr) {
        texture = rayIntersection->Object->Object_Texture;
    }
    /* Check to see if this object/shape has a material_map texture, if so */
    /* then change the texture pointer to point to the mapped texture - CdW 7/91
     */
    if (texture->Texture_Number == MATERIAL_MAP_TEXTURE) {
        texture = material_map(&rayIntersection->Point, texture);
    }

    /* If this is just a shadow ray and we're rendering low quality, then return
     */

    if (shadowRay && (quality <= 5)) {
        return;
    }

    Make_Colour(&filterColour, 1.0, 1.0, 1.0);
    filterColour.Alpha = 1.0;

    /* Now, we perform the lighting calculations. */
    for (surface = 1, tempTexture = texture;
         (tempTexture != nullptr) && (filterColour.Alpha > 0.01);
         surface++, tempTexture = tempTexture->Next_Texture) {

        Make_Colour(&surfaceColour, 0.0, 0.0, 0.0);
        if (quality <= 5) {
            if (rayIntersection->Shape->Shape_Colour != nullptr) {
                surfaceColour = *rayIntersection->Shape->Shape_Colour;
            } else if (rayIntersection->Object->Object_Colour != nullptr) {
                surfaceColour = *rayIntersection->Object->Object_Colour;
            } else {
                Make_Colour(&surfaceColour, 0.5, 0.5, 0.5);
            }
        } else {
            Colour_At(&surfaceColour, tempTexture, &rayIntersection->Point);
        }
        /* We don't need to compute the lighting characteristics for shadow
         * rays. */
        if (!shadowRay) {
            Compute_Reflected_Colour(ray, tempTexture, rayIntersection,
                &surfaceColour, &filterColour, colour);
        }

        if (Options & DEBUGGING) {
            printf("Surface %d\n", surface);
            printf("    Surf: %6.4f %6.4f %6.4f %6.4f\n", surfaceColour.Red,
                surfaceColour.Green, surfaceColour.Blue, surfaceColour.Alpha);
            printf("    Filter_Colour:    %6.4f %6.4f %6.4f %6.4f  Final "
                   "Colour: %6.4f %6.4f %6.4f %6.4f  \n",
                filterColour.Red, filterColour.Green, filterColour.Blue,
                filterColour.Alpha, colour->Red, colour->Green, colour->Blue,
                colour->Alpha);
        }

        filterColour.Red *= surfaceColour.Red;
        filterColour.Green *= surfaceColour.Green;
        filterColour.Blue *= surfaceColour.Blue;

        filterColour.Alpha *= surfaceColour.Alpha;
    }

    /* For shadow rays, we have the filter colour now - time to return */
    if (shadowRay) {

        if (filterColour.Alpha < 0.01) {
            Make_Colour(colour, 0.0, 0.0, 0.0);
            return;
        }

        if (texture->Object_Refraction > 0.0) {
            colour->Red *= filterColour.Red * texture->Object_Refraction *
                           filterColour.Alpha;
            colour->Green *= filterColour.Green * texture->Object_Refraction *
                             filterColour.Alpha;
            colour->Blue *= filterColour.Blue * texture->Object_Refraction *
                            filterColour.Alpha;
        } else {
            colour->Red *= filterColour.Red * filterColour.Alpha;
            colour->Green *= filterColour.Green * filterColour.Alpha;
            colour->Blue *= filterColour.Blue * filterColour.Alpha;
        }
        return;
    }

    if ((filterColour.Alpha > 0.01) && (quality > 5)) {
        Make_Colour(&refractedColour, 0.0, 0.0, 0.0);

        if (texture->Object_Refraction > 0.0) {
            Normal(&surfaceNormal, (SimpleBody *)rayIntersection->Shape,
                &rayIntersection->Point);

            if (quality > 7) {
                Perturb_Normal(&surfaceNormal, texture, &rayIntersection->Point,
                    &surfaceNormal);
            }

            /* If the surface normal points away, flip its direction. */
            VDot(normalDirection, surfaceNormal, ray->Direction);
            if (normalDirection > 0.0) {
                VScale(surfaceNormal, surfaceNormal, -1.0);
            }

            Refract(texture, &rayIntersection->Point, ray, &surfaceNormal,
                &refractedColour);
        } else {
            Refract(texture, &rayIntersection->Point, ray, nullptr,
                &refractedColour);
        }

        colour->Red +=
            filterColour.Red * refractedColour.Red * filterColour.Alpha;
        colour->Green +=
            filterColour.Green * refractedColour.Green * filterColour.Alpha;
        colour->Blue +=
            filterColour.Blue * refractedColour.Blue * filterColour.Alpha;

        if (texture->Object_Refraction > 0.0 &&
            texture->Object_Transmit > 0.0) {
            Make_Colour(&refractedColour, 0.0, 0.0, 0.0);
            Refract(texture, &rayIntersection->Point, ray, nullptr,
                &refractedColour);
            colour->Red +=
                filterColour.Red * refractedColour.Red * filterColour.Alpha;
            colour->Green +=
                filterColour.Green * refractedColour.Green * filterColour.Alpha;
            colour->Blue +=
                filterColour.Blue * refractedColour.Blue * filterColour.Alpha;
        }
    }

    if (globalFrame.Fog_Distance != 0.0) {
        Fog(rayIntersection->Depth, &globalFrame.Fog_Colour,
            globalFrame.Fog_Distance, colour);
    }
}
