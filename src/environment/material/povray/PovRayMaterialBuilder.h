#ifndef __POV_RAY_MATERIAL_BUILDER__
#define __POV_RAY_MATERIAL_BUILDER__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"
#include "environment/material/povray/PovRayMaterial.h"
#include "environment/material/SolidTextureBumpyNames.h"
#include "environment/material/SolidTextureColorNames.h"

class SolidTexturePigment;
class SolidTextureNormal;

class PovRayMaterialBuilder {
  public:
    PovRayMaterialBuilder();
    explicit PovRayMaterialBuilder(const PovRayMaterial *base);

    PovRayMaterial *build() const;
    ColorRgba *getCheckerColor1() const;
    ColorRgba *getCheckerColor2() const;
    PovRayMaterial *getCheckerTexture1() const;
    PovRayMaterial *getCheckerTexture2() const;
    ControlledRGBAImageHDRUncompressed *getMaterialImage() const;
    java::ArrayList<PovRayMaterial *> &getMaterials();
    double getMortar() const;
    int getOctaves() const;
    PovRayMaterialBuilder &rotate(Vector3Dd &v);
    PovRayMaterialBuilder &scale(const Vector3Dd &v);
    PovRayMaterialBuilder &setBumpAmount(double v);
    PovRayMaterialBuilder &setBumpImage(ControlledRGBAImageHDRUncompressed *v);
    PovRayMaterialBuilder &setBumpNumber(SolidTextureBumpyNames v);
    PovRayMaterialBuilder &setCheckerColor1(ColorRgba *v);
    PovRayMaterialBuilder &setCheckerColor2(ColorRgba *v);
    PovRayMaterialBuilder &setCheckerTexture1(PovRayMaterial *v);
    PovRayMaterialBuilder &setCheckerTexture2(PovRayMaterial *v);
    PovRayMaterialBuilder &setColorMap(RGBAColorPalette *v);
    PovRayMaterialBuilder &setFrequency(double v);
    PovRayMaterialBuilder &setImage(ControlledRGBAImageHDRUncompressed *v);
    PovRayMaterialBuilder &setMaterialImage(ControlledRGBAImageHDRUncompressed *v);
    PovRayMaterialBuilder &setMetallicFlag(bool v);
    PovRayMaterialBuilder &setMortar(double v);
    PovRayMaterialBuilder &setObjectAmbient(double v);
    PovRayMaterialBuilder &setObjectBrilliance(double v);
    PovRayMaterialBuilder &setObjectDiffuse(double v);
    PovRayMaterialBuilder &setObjectIndexOfRefraction(double v);
    PovRayMaterialBuilder &setObjectPhong(double v);
    PovRayMaterialBuilder &setObjectPhongSize(double v);
    PovRayMaterialBuilder &setObjectReflection(double v);
    PovRayMaterialBuilder &setObjectRefraction(double v);
    PovRayMaterialBuilder &setObjectRoughness(double v);
    PovRayMaterialBuilder &setObjectSpecular(double v);
    PovRayMaterialBuilder &setObjectTransmit(double v);
    PovRayMaterialBuilder &setOctaves(int v);
    PovRayMaterialBuilder &setPhase(double v);
    PovRayMaterialBuilder &setTextureGradient(const Vector3Dd &v);
    PovRayMaterialBuilder &setTextureNumber(SolidTextureColorNames v);
    PovRayMaterialBuilder &setTextureRandomness(double v);
    PovRayMaterialBuilder &setTurbulence(double v);
    PovRayMaterialBuilder &translate(const Vector3Dd &v);

  private:
    SolidTexturePigment *buildPigment() const;
    SolidTextureNormal *buildNormal() const;

    double bumpAmount;
    ControlledRGBAImageHDRUncompressed *bumpImage;
    SolidTextureBumpyNames bumpNumber;
    ColorRgba *checkerColor1;
    ColorRgba *checkerColor2;
    PovRayMaterial *checkerTexture1;
    PovRayMaterial *checkerTexture2;
    RGBAColorPalette *colorMap;
    double frequency;
    ControlledRGBAImageHDRUncompressed *image;
    java::ArrayList<PovRayMaterial *> layers;
    ControlledRGBAImageHDRUncompressed *materialImage;
    java::ArrayList<PovRayMaterial *> materials;
    bool metallicFlag;
    double mortar;
    int numberOfWaves;
    double objectAmbient;
    double objectBrilliance;
    double objectDiffuse;
    double objectIndexOfRefraction;
    double objectPhong;
    double objectPhongSize;
    double objectReflection;
    double objectRefraction;
    double objectRoughness;
    double objectSpecular;
    double objectTransmit;
    int octaves;
    double phase;
    Vector3Dd textureGradient;
    SolidTextureColorNames textureNumber;
    double textureRandomness;
    Matrix4x4d *textureTransformation;
    Matrix4x4d *textureTransformationInverse;
    double turbulence;
};

#endif
