#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"
#include "environment/material/Material.h"
#include "environment/material/SolidTextureBumpyNames.h"
#include "environment/material/SolidTextureColorNames.h"

class PovrayMaterial : public Material {
  public:
    static constexpr int DEFAULT_NUMBER_OF_WAVES = 10;

    PovrayMaterial();

    java::ArrayList<PovrayMaterial*> layers; // Ordered list of additional texture layers
    java::ArrayList<PovrayMaterial*> materials; // PovrayMaterial map variants
    double objectReflection;
    double objectAmbient;
    double objectDiffuse;
    double objectBrilliance;
    double objectIndexOfRefraction;
    double objectRefraction;
    double objectTransmit;
    double objectSpecular;
    double objectRoughness;
    double objectPhong;
    double objectPhongSize;
    double bumpAmount;
    double textureRandomness;
    double frequency;
    double phase;
    SolidTextureColorNames textureNumber;
    SolidTextureBumpyNames bumpNumber;
    Matrix4x4d *textureTransformation;
    Matrix4x4d *textureTransformationInverse;
    ColorRgba *color1;
    ColorRgba *color2;
    double turbulence;
    Vector3Dd textureGradient;
    RGBAColorPalette *colorMap;
    ControlledRGBAImageHDRUncompressed *image;
    ControlledRGBAImageHDRUncompressed *bumpImage;
    ControlledRGBAImageHDRUncompressed *materialImage;
    bool metallicFlag;
    bool onceFlag;
    bool constantFlag;
    int numberOfWaves;
    int octaves; // dmf, 1/92 for turbulence functions
    double mortar; // rha, 2/92 for brick texture

    bool isConstant() const { return constantFlag; }
    void setConstant(bool value) { constantFlag = value; }
    ControlledRGBAImageHDRUncompressed* getImage() const { return image; }
    void setImage(ControlledRGBAImageHDRUncompressed* img) { image = img; }
    ControlledRGBAImageHDRUncompressed* getBumpImage() const { return bumpImage; }
    void setBumpImage(ControlledRGBAImageHDRUncompressed* img) { bumpImage = img; }
    ControlledRGBAImageHDRUncompressed* getMaterialImage() const { return materialImage; }
    void setMaterialImage(ControlledRGBAImageHDRUncompressed* img) { materialImage = img; }
    Matrix4x4d* getTextureTransformation() const { return textureTransformation; }
    SolidTextureColorNames getTextureNumber() const { return textureNumber; }
    void setTextureNumber(SolidTextureColorNames num) { textureNumber = num; }
    SolidTextureBumpyNames getBumpNumber() const { return bumpNumber; }
    void setBumpNumber(SolidTextureBumpyNames num) { bumpNumber = num; }
    double getBumpAmount() const { return bumpAmount; }
    void setBumpAmount(double amount) { bumpAmount = amount; }
    ColorRgba* getColor1() const { return color1; }
    void setColor1(ColorRgba* color) { color1 = color; }
    ColorRgba* getColor2() const { return color2; }
    void setColor2(ColorRgba* color) { color2 = color; }
    int getOctaves() const { return octaves; }
    double getPhase() const { return phase; }
    double getMortar() const { return mortar; }
    double getObjectAmbient() const { return objectAmbient; }
    double getObjectDiffuse() const { return objectDiffuse; }
    double getObjectBrilliance() const { return objectBrilliance; }
    double getObjectReflection() const { return objectReflection; }
    double getTurbulence() const { return turbulence; }
    double getFrequency() const { return frequency; }
    Vector3Dd& getTextureGradient() { return textureGradient; }
    const Vector3Dd& getTextureGradient() const { return textureGradient; }
    java::ArrayList<PovrayMaterial*>& getLayers() { return layers; }
    PovrayMaterial *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    double getObjectIndexOfRefraction() const override { return objectIndexOfRefraction; }
};

#endif
