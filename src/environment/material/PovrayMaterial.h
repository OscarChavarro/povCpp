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

    java::ArrayList<PovrayMaterial*>& getLayers();
    const java::ArrayList<PovrayMaterial*>& getLayers() const;
    java::ArrayList<PovrayMaterial*>& getMaterials();
    const java::ArrayList<PovrayMaterial*>& getMaterials() const;
    double getObjectReflection() const;
    void setObjectReflection(double value);
    double getObjectAmbient() const;
    void setObjectAmbient(double value);
    double getObjectDiffuse() const;
    void setObjectDiffuse(double value);
    double getObjectBrilliance() const;
    void setObjectBrilliance(double value);
    double getObjectRefraction() const;
    void setObjectRefraction(double value);
    double getObjectTransmit() const;
    void setObjectTransmit(double value);
    double getObjectSpecular() const;
    void setObjectSpecular(double value);
    double getObjectRoughness() const;
    void setObjectRoughness(double value);
    double getObjectPhong() const;
    void setObjectPhong(double value);
    double getObjectPhongSize() const;
    void setObjectPhongSize(double value);
    double getBumpAmount() const;
    void setBumpAmount(double amount);
    double getTextureRandomness() const;
    void setTextureRandomness(double value);
    double getFrequency() const;
    void setFrequency(double value);
    double getPhase() const;
    void setPhase(double value);
    SolidTextureColorNames getTextureNumber() const;
    void setTextureNumber(SolidTextureColorNames num);
    SolidTextureBumpyNames getBumpNumber() const;
    void setBumpNumber(SolidTextureBumpyNames num);
    Matrix4x4d* getTextureTransformation() const;
    void setTextureTransformation(Matrix4x4d *value);
    Matrix4x4d* getTextureTransformationInverse() const;
    void setTextureTransformationInverse(Matrix4x4d *value);
    ColorRgba*& getColor1();
    ColorRgba* getColor1() const;
    void setColor1(ColorRgba* color);
    ColorRgba*& getColor2();
    ColorRgba* getColor2() const;
    void setColor2(ColorRgba* color);
    double getTurbulence() const;
    void setTurbulence(double value);
    Vector3Dd& getTextureGradient();
    const Vector3Dd& getTextureGradient() const;
    void setTextureGradient(const Vector3Dd &value);
    RGBAColorPalette* getColorMap() const;
    void setColorMap(RGBAColorPalette* map);
    ControlledRGBAImageHDRUncompressed* getImage() const;
    void setImage(ControlledRGBAImageHDRUncompressed* img);
    ControlledRGBAImageHDRUncompressed* getBumpImage() const;
    void setBumpImage(ControlledRGBAImageHDRUncompressed* img);
    ControlledRGBAImageHDRUncompressed* getMaterialImage() const;
    void setMaterialImage(ControlledRGBAImageHDRUncompressed* img);
    bool isMetallic() const;
    void setMetallicFlag(bool value);
    bool getOnceFlag() const;
    void setOnceFlag(bool value);
    bool isConstant() const;
    void setConstant(bool value);
    int getNumberOfWaves() const;
    void setNumberOfWaves(int value);
    int getOctaves() const;
    void setOctaves(int value);
    double getMortar() const;
    void setMortar(double value);
    PovrayMaterial *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    double getObjectIndexOfRefraction() const override;
    void setObjectIndexOfRefraction(double value);

  private:
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
};

inline java::ArrayList<PovrayMaterial*>& PovrayMaterial::getLayers() { return layers; }
inline const java::ArrayList<PovrayMaterial*>& PovrayMaterial::getLayers() const { return layers; }
inline java::ArrayList<PovrayMaterial*>& PovrayMaterial::getMaterials() { return materials; }
inline const java::ArrayList<PovrayMaterial*>& PovrayMaterial::getMaterials() const { return materials; }
inline double PovrayMaterial::getObjectReflection() const { return objectReflection; }
inline void PovrayMaterial::setObjectReflection(double value) { objectReflection = value; }
inline double PovrayMaterial::getObjectAmbient() const { return objectAmbient; }
inline void PovrayMaterial::setObjectAmbient(double value) { objectAmbient = value; }
inline double PovrayMaterial::getObjectDiffuse() const { return objectDiffuse; }
inline void PovrayMaterial::setObjectDiffuse(double value) { objectDiffuse = value; }
inline double PovrayMaterial::getObjectBrilliance() const { return objectBrilliance; }
inline void PovrayMaterial::setObjectBrilliance(double value) { objectBrilliance = value; }
inline double PovrayMaterial::getObjectIndexOfRefraction() const { return objectIndexOfRefraction; }
inline void PovrayMaterial::setObjectIndexOfRefraction(double value) { objectIndexOfRefraction = value; }
inline double PovrayMaterial::getObjectRefraction() const { return objectRefraction; }
inline void PovrayMaterial::setObjectRefraction(double value) { objectRefraction = value; }
inline double PovrayMaterial::getObjectTransmit() const { return objectTransmit; }
inline void PovrayMaterial::setObjectTransmit(double value) { objectTransmit = value; }
inline double PovrayMaterial::getObjectSpecular() const { return objectSpecular; }
inline void PovrayMaterial::setObjectSpecular(double value) { objectSpecular = value; }
inline double PovrayMaterial::getObjectRoughness() const { return objectRoughness; }
inline void PovrayMaterial::setObjectRoughness(double value) { objectRoughness = value; }
inline double PovrayMaterial::getObjectPhong() const { return objectPhong; }
inline void PovrayMaterial::setObjectPhong(double value) { objectPhong = value; }
inline double PovrayMaterial::getObjectPhongSize() const { return objectPhongSize; }
inline void PovrayMaterial::setObjectPhongSize(double value) { objectPhongSize = value; }
inline double PovrayMaterial::getBumpAmount() const { return bumpAmount; }
inline void PovrayMaterial::setBumpAmount(double amount) { bumpAmount = amount; }
inline double PovrayMaterial::getTextureRandomness() const { return textureRandomness; }
inline void PovrayMaterial::setTextureRandomness(double value) { textureRandomness = value; }
inline double PovrayMaterial::getFrequency() const { return frequency; }
inline void PovrayMaterial::setFrequency(double value) { frequency = value; }
inline double PovrayMaterial::getPhase() const { return phase; }
inline void PovrayMaterial::setPhase(double value) { phase = value; }
inline SolidTextureColorNames PovrayMaterial::getTextureNumber() const { return textureNumber; }
inline void PovrayMaterial::setTextureNumber(SolidTextureColorNames num) { textureNumber = num; }
inline SolidTextureBumpyNames PovrayMaterial::getBumpNumber() const { return bumpNumber; }
inline void PovrayMaterial::setBumpNumber(SolidTextureBumpyNames num) { bumpNumber = num; }
inline Matrix4x4d* PovrayMaterial::getTextureTransformation() const { return textureTransformation; }
inline void PovrayMaterial::setTextureTransformation(Matrix4x4d *value) { textureTransformation = value; }
inline Matrix4x4d* PovrayMaterial::getTextureTransformationInverse() const { return textureTransformationInverse; }
inline void PovrayMaterial::setTextureTransformationInverse(Matrix4x4d *value) { textureTransformationInverse = value; }
inline ColorRgba*& PovrayMaterial::getColor1() { return color1; }
inline ColorRgba* PovrayMaterial::getColor1() const { return color1; }
inline void PovrayMaterial::setColor1(ColorRgba* color) { color1 = color; }
inline ColorRgba*& PovrayMaterial::getColor2() { return color2; }
inline ColorRgba* PovrayMaterial::getColor2() const { return color2; }
inline void PovrayMaterial::setColor2(ColorRgba* color) { color2 = color; }
inline double PovrayMaterial::getTurbulence() const { return turbulence; }
inline void PovrayMaterial::setTurbulence(double value) { turbulence = value; }
inline Vector3Dd& PovrayMaterial::getTextureGradient() { return textureGradient; }
inline const Vector3Dd& PovrayMaterial::getTextureGradient() const { return textureGradient; }
inline void PovrayMaterial::setTextureGradient(const Vector3Dd &value) { textureGradient = value; }
inline RGBAColorPalette* PovrayMaterial::getColorMap() const { return colorMap; }
inline void PovrayMaterial::setColorMap(RGBAColorPalette* map) { colorMap = map; }
inline ControlledRGBAImageHDRUncompressed* PovrayMaterial::getImage() const { return image; }
inline void PovrayMaterial::setImage(ControlledRGBAImageHDRUncompressed* img) { image = img; }
inline ControlledRGBAImageHDRUncompressed* PovrayMaterial::getBumpImage() const { return bumpImage; }
inline void PovrayMaterial::setBumpImage(ControlledRGBAImageHDRUncompressed* img) { bumpImage = img; }
inline ControlledRGBAImageHDRUncompressed* PovrayMaterial::getMaterialImage() const { return materialImage; }
inline void PovrayMaterial::setMaterialImage(ControlledRGBAImageHDRUncompressed* img) { materialImage = img; }
inline bool PovrayMaterial::isMetallic() const { return metallicFlag; }
inline void PovrayMaterial::setMetallicFlag(bool value) { metallicFlag = value; }
inline bool PovrayMaterial::getOnceFlag() const { return onceFlag; }
inline void PovrayMaterial::setOnceFlag(bool value) { onceFlag = value; }
inline bool PovrayMaterial::isConstant() const { return constantFlag; }
inline void PovrayMaterial::setConstant(bool value) { constantFlag = value; }
inline int PovrayMaterial::getNumberOfWaves() const { return numberOfWaves; }
inline void PovrayMaterial::setNumberOfWaves(int value) { numberOfWaves = value; }
inline int PovrayMaterial::getOctaves() const { return octaves; }
inline void PovrayMaterial::setOctaves(int value) { octaves = value; }
inline double PovrayMaterial::getMortar() const { return mortar; }
inline void PovrayMaterial::setMortar(double value) { mortar = value; }

#endif
