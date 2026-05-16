#ifndef __LIGHTING_H__
#define __LIGHTING_H__

extern void Perturb_Normal(Vector3D *New_Normal, Texture *Texture, Vector3D *Intersection_Point, Vector3D *Surface_Normal);
extern void Ambient(Texture *Texture, RGBAColor *Surface_Colour, RGBAColor *Colour, DBL Attenuation);
extern void Diffuse(Texture *Texture, Vector3D *Intersection_Point, Ray *Eye, Vector3D *Surface_Normal, RGBAColor *Surface_Colour, RGBAColor *Colour,DBL Attenuation);
extern void Reflect(Texture *Texture, Vector3D *Intersection_Point, Ray *Ray, Vector3D *Surface_Normal, RGBAColor *Colour);
extern void Refract(Texture *Texture, Vector3D *Intersection_Point, Ray *Ray, Vector3D *Surface_Normal, RGBAColor *Colour);
extern void Fog(DBL Distance, RGBAColor *Fog_Colour, DBL Fog_Distance, RGBAColor *Colour);
extern void Compute_Reflected_Colour(Ray *Ray, Texture *Texture, Intersection *Ray_Intersection, RGBAColor *Surface_Colour, RGBAColor *Filter_Colour,RGBAColor *Colour);
extern void Determine_Surface_Colour(Intersection *Ray_Intersection, RGBAColor *Colour, Ray *Ray, int Shadow_Ray);

#endif
