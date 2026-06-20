#ifndef __POV_RAY_MATERIAL_BUILDER__
#define __POV_RAY_MATERIAL_BUILDER__

#include "environment/material/PovRayMaterial.h"
#include "environment/material/SolidTextureBumpyNames.h"
#include "environment/material/SolidTextureColorNames.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"

class PovRayMaterialBuilder {
  public:
    PovRayMaterialBuilder();
    explicit PovRayMaterialBuilder(const PovRayMaterial *base);

    PovRayMaterial *build() const;

    PovRayMaterialBuilder &setObjectReflection(double v);
    PovRayMaterialBuilder &setObjectAmbient(double v);
    PovRayMaterialBuilder &setObjectDiffuse(double v);
    PovRayMaterialBuilder &setObjectBrilliance(double v);
    PovRayMaterialBuilder &setObjectIndexOfRefraction(double v);
    PovRayMaterialBuilder &setObjectRefraction(double v);
    PovRayMaterialBuilder &setObjectTransmit(double v);
    PovRayMaterialBuilder &setObjectSpecular(double v);
    PovRayMaterialBuilder &setObjectRoughness(double v);
    PovRayMaterialBuilder &setObjectPhong(double v);
    PovRayMaterialBuilder &setObjectPhongSize(double v);
    PovRayMaterialBuilder &setBumpAmount(double v);
    PovRayMaterialBuilder &setTextureRandomness(double v);
    PovRayMaterialBuilder &setFrequency(double v);
    PovRayMaterialBuilder &setPhase(double v);
    PovRayMaterialBuilder &setTextureNumber(SolidTextureColorNames v);
    PovRayMaterialBuilder &setBumpNumber(SolidTextureBumpyNames v);
    PovRayMaterialBuilder &setTextureTransformation(Matrix4x4d *v);
    PovRayMaterialBuilder &setTextureTransformationInverse(Matrix4x4d *v);
    PovRayMaterialBuilder &setColor1(ColorRgba *v);
    PovRayMaterialBuilder &setColor2(ColorRgba *v);
    PovRayMaterialBuilder &setTurbulence(double v);
    PovRayMaterialBuilder &setTextureGradient(const Vector3Dd &v);
    PovRayMaterialBuilder &setColorMap(RGBAColorPalette *v);
    PovRayMaterialBuilder &setImage(ControlledRGBAImageHDRUncompressed *v);
    PovRayMaterialBuilder &setBumpImage(ControlledRGBAImageHDRUncompressed *v);
    PovRayMaterialBuilder &setMaterialImage(ControlledRGBAImageHDRUncompressed *v);
    PovRayMaterialBuilder &setMetallicFlag(bool v);
    PovRayMaterialBuilder &setNumberOfWaves(int v);
    PovRayMaterialBuilder &setOctaves(int v);
    PovRayMaterialBuilder &setMortar(double v);

    SolidTextureColorNames getTextureNumber() const;
    ControlledRGBAImageHDRUncompressed *getImage() const;
    ControlledRGBAImageHDRUncompressed *getBumpImage() const;
    ControlledRGBAImageHDRUncompressed *getMaterialImage() const;
    ColorRgba *getColor1() const;
    ColorRgba *getColor2() const;
    int getOctaves() const;
    double getMortar() const;
    const Vector3Dd &getTextureGradient() const;
    java::ArrayList<PovRayMaterial *> &layers();
    java::ArrayList<PovRayMaterial *> &materials();

    PovRayMaterialBuilder &translate(const Vector3Dd &v);
    PovRayMaterialBuilder &rotate(Vector3Dd &v);
    PovRayMaterialBuilder &scale(const Vector3Dd &v);

  private:
    java::ArrayList<PovRayMaterial *> layers_;
    java::ArrayList<PovRayMaterial *> materials_;
    double objectReflection_;
    double objectAmbient_;
    double objectDiffuse_;
    double objectBrilliance_;
    double objectIndexOfRefraction_;
    double objectRefraction_;
    double objectTransmit_;
    double objectSpecular_;
    double objectRoughness_;
    double objectPhong_;
    double objectPhongSize_;
    double bumpAmount_;
    double textureRandomness_;
    double frequency_;
    double phase_;
    SolidTextureColorNames textureNumber_;
    SolidTextureBumpyNames bumpNumber_;
    Matrix4x4d *textureTransformation_;
    Matrix4x4d *textureTransformationInverse_;
    ColorRgba *color1_;
    ColorRgba *color2_;
    double turbulence_;
    Vector3Dd textureGradient_;
    RGBAColorPalette *colorMap_;
    ControlledRGBAImageHDRUncompressed *image_;
    ControlledRGBAImageHDRUncompressed *bumpImage_;
    ControlledRGBAImageHDRUncompressed *materialImage_;
    bool metallicFlag_;
    int numberOfWaves_;
    int octaves_;
    double mortar_;
};

#endif
