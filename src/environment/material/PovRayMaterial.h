#ifndef __POV_RAY_MATERIAL__
#define __POV_RAY_MATERIAL__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"
#include "environment/material/Material.h"
#include "environment/material/SolidTextureBumpyNames.h"
#include "environment/material/SolidTextureColorNames.h"

class PovRayMaterial : public Material {
  public:
    static constexpr int DEFAULT_NUMBER_OF_WAVES = 10;

    PovRayMaterial();

    java::ArrayList<PovRayMaterial *>& getLayers();
    const java::ArrayList<PovRayMaterial *>& getLayers() const;
    java::ArrayList<PovRayMaterial *>& getMaterials();
    const java::ArrayList<PovRayMaterial *>& getMaterials() const;
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
    PovRayMaterial *copy() override;
    void translate(Vector3Dd *vector) override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    double getObjectIndexOfRefraction() const override;
    void setObjectIndexOfRefraction(double value);

  private:
    java::ArrayList<PovRayMaterial *> layers; // Ordered list of additional texture layers
    java::ArrayList<PovRayMaterial *> materials; // PovrayMaterial map variants
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
    int octaves;
    double mortar;
};

inline java::ArrayList<PovRayMaterial *>&
PovRayMaterial::getLayers() { return layers; }
inline const java::ArrayList<PovRayMaterial *>&
PovRayMaterial::getLayers() const { return layers; }
inline java::ArrayList<PovRayMaterial *>&
PovRayMaterial::getMaterials() { return materials; }
inline const java::ArrayList<PovRayMaterial *>&
PovRayMaterial::getMaterials() const { return materials; }
inline double
PovRayMaterial::getObjectReflection() const { return objectReflection; }
inline void
PovRayMaterial::setObjectReflection(double value) { objectReflection = value; }
inline double
PovRayMaterial::getObjectAmbient() const { return objectAmbient; }
inline void
PovRayMaterial::setObjectAmbient(double value) { objectAmbient = value; }
inline double
PovRayMaterial::getObjectDiffuse() const { return objectDiffuse; }
inline void
PovRayMaterial::setObjectDiffuse(double value) { objectDiffuse = value; }
inline double
PovRayMaterial::getObjectBrilliance() const { return objectBrilliance; }
inline void
PovRayMaterial::setObjectBrilliance(double value) { objectBrilliance = value; }
inline double
PovRayMaterial::getObjectIndexOfRefraction() const { return objectIndexOfRefraction; }
inline void
PovRayMaterial::setObjectIndexOfRefraction(double value) { objectIndexOfRefraction = value; }
inline double
PovRayMaterial::getObjectRefraction() const { return objectRefraction; }
inline void
PovRayMaterial::setObjectRefraction(double value) { objectRefraction = value; }
inline double
PovRayMaterial::getObjectTransmit() const { return objectTransmit; }
inline void
PovRayMaterial::setObjectTransmit(double value) { objectTransmit = value; }
inline double
PovRayMaterial::getObjectSpecular() const { return objectSpecular; }
inline void
PovRayMaterial::setObjectSpecular(double value) { objectSpecular = value; }
inline double
PovRayMaterial::getObjectRoughness() const { return objectRoughness; }
inline void
PovRayMaterial::setObjectRoughness(double value) { objectRoughness = value; }
inline double
PovRayMaterial::getObjectPhong() const { return objectPhong; }
inline void
PovRayMaterial::setObjectPhong(double value) { objectPhong = value; }
inline double
PovRayMaterial::getObjectPhongSize() const { return objectPhongSize; }
inline void
PovRayMaterial::setObjectPhongSize(double value) { objectPhongSize = value; }
inline double
PovRayMaterial::getBumpAmount() const { return bumpAmount; }
inline void
PovRayMaterial::setBumpAmount(double amount) { bumpAmount = amount; }
inline double
PovRayMaterial::getTextureRandomness() const { return textureRandomness; }
inline void
PovRayMaterial::setTextureRandomness(double value) { textureRandomness = value; }
inline double
PovRayMaterial::getFrequency() const { return frequency; }
inline void
PovRayMaterial::setFrequency(double value) { frequency = value; }
inline double
PovRayMaterial::getPhase() const { return phase; }
inline void
PovRayMaterial::setPhase(double value) { phase = value; }
inline SolidTextureColorNames
PovRayMaterial::getTextureNumber() const { return textureNumber; }
inline void
PovRayMaterial::setTextureNumber(SolidTextureColorNames num) { textureNumber = num; }
inline SolidTextureBumpyNames
PovRayMaterial::getBumpNumber() const { return bumpNumber; }
inline void
PovRayMaterial::setBumpNumber(SolidTextureBumpyNames num) { bumpNumber = num; }
inline Matrix4x4d*
PovRayMaterial::getTextureTransformation() const { return textureTransformation; }
inline void
PovRayMaterial::setTextureTransformation(Matrix4x4d *value) { textureTransformation = value; }
inline Matrix4x4d*
PovRayMaterial::getTextureTransformationInverse() const { return textureTransformationInverse; }
inline void
PovRayMaterial::setTextureTransformationInverse(Matrix4x4d *value) { textureTransformationInverse = value; }
inline ColorRgba*&
PovRayMaterial::getColor1() { return color1; }
inline ColorRgba*
PovRayMaterial::getColor1() const { return color1; }
inline void
PovRayMaterial::setColor1(ColorRgba* color) { color1 = color; }
inline ColorRgba*&
PovRayMaterial::getColor2() { return color2; }
inline ColorRgba*
PovRayMaterial::getColor2() const { return color2; }
inline void
PovRayMaterial::setColor2(ColorRgba* color) { color2 = color; }
inline double
PovRayMaterial::getTurbulence() const { return turbulence; }
inline void
PovRayMaterial::setTurbulence(double value) { turbulence = value; }
inline Vector3Dd&
PovRayMaterial::getTextureGradient() { return textureGradient; }
inline const Vector3Dd&
PovRayMaterial::getTextureGradient() const { return textureGradient; }
inline void
PovRayMaterial::setTextureGradient(const Vector3Dd &value) { textureGradient = value; }
inline RGBAColorPalette*
PovRayMaterial::getColorMap() const { return colorMap; }
inline void
PovRayMaterial::setColorMap(RGBAColorPalette* map) { colorMap = map; }
inline ControlledRGBAImageHDRUncompressed*
PovRayMaterial::getImage() const { return image; }
inline void
PovRayMaterial::setImage(ControlledRGBAImageHDRUncompressed* img) { image = img; }
inline ControlledRGBAImageHDRUncompressed*
PovRayMaterial::getBumpImage() const { return bumpImage; }
inline void
PovRayMaterial::setBumpImage(ControlledRGBAImageHDRUncompressed* img) { bumpImage = img; }
inline ControlledRGBAImageHDRUncompressed*
PovRayMaterial::getMaterialImage() const { return materialImage; }
inline void
PovRayMaterial::setMaterialImage(ControlledRGBAImageHDRUncompressed* img) { materialImage = img; }
inline bool
PovRayMaterial::isMetallic() const { return metallicFlag; }
inline void
PovRayMaterial::setMetallicFlag(bool value) { metallicFlag = value; }
inline bool
PovRayMaterial::getOnceFlag() const { return onceFlag; }
inline void
PovRayMaterial::setOnceFlag(bool value) { onceFlag = value; }
inline bool
PovRayMaterial::isConstant() const { return constantFlag; }
inline void
PovRayMaterial::setConstant(bool value) { constantFlag = value; }
inline int
PovRayMaterial::getNumberOfWaves() const { return numberOfWaves; }
inline void
PovRayMaterial::setNumberOfWaves(int value) { numberOfWaves = value; }
inline int
PovRayMaterial::getOctaves() const { return octaves; }
inline void
PovRayMaterial::setOctaves(int value) { octaves = value; }
inline double
PovRayMaterial::getMortar() const { return mortar; }
inline void
PovRayMaterial::setMortar(double value) { mortar = value; }

#endif
