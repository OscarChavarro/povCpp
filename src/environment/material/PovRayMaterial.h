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
  private:
    static bool needsTransform(const PovRayMaterial *texture);
    static void applyTranslationTransform(PovRayMaterial *texture, const Vector3Dd *vector);
    static void applyRotationTransform(PovRayMaterial *texture, Vector3Dd *vector);
    static void applyScaleTransform(PovRayMaterial *texture, const Vector3Dd *vector);
    static PovRayMaterial *copyTextureNode(const PovRayMaterial *src);

    java::ArrayList<PovRayMaterial *> layers; // Ordered list of additional texture layers
    java::ArrayList<PovRayMaterial *> materials; // PovRayMaterial map variants
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

  public:
    static constexpr int DEFAULT_NUMBER_OF_WAVES = 10;

    PovRayMaterial(double objectReflection, double objectAmbient, double objectDiffuse,
                   double objectBrilliance, double objectIndexOfRefraction, double objectRefraction,
                   double objectTransmit, double objectSpecular, double objectRoughness,
                   double objectPhong, double objectPhongSize, double bumpAmount,
                   double texRandomness, double frequency, double phase,
                   SolidTextureColorNames textureNumber, SolidTextureBumpyNames bumpNumber,
                   Matrix4x4d *textureTransformation, Matrix4x4d *textureTransformationInverse,
                   ColorRgba *color1, ColorRgba *color2, double turbulence,
                   const Vector3Dd &textureGradient, RGBAColorPalette *colorMap,
                   ControlledRGBAImageHDRUncompressed *image,
                   ControlledRGBAImageHDRUncompressed *bumpImage,
                   ControlledRGBAImageHDRUncompressed *materialImage, bool metallicFlag,
                   bool onceFlag, bool constantFlag, int numberOfWaves, int octaves, double mortar,
                   java::ArrayList<PovRayMaterial *> layers,
                   java::ArrayList<PovRayMaterial *> materials);

    const java::ArrayList<PovRayMaterial *>& getLayers() const;
    const java::ArrayList<PovRayMaterial *>& getMaterials() const;
    double getObjectReflection() const;
    double getObjectAmbient() const;
    double getObjectDiffuse() const;
    double getObjectBrilliance() const;
    double getObjectRefraction() const;
    double getObjectTransmit() const;
    double getObjectSpecular() const;
    double getObjectRoughness() const;
    double getObjectPhong() const;
    double getObjectPhongSize() const;
    double getBumpAmount() const;
    double getTextureRandomness() const;
    double getFrequency() const;
    double getPhase() const;
    SolidTextureColorNames getTextureNumber() const;
    SolidTextureBumpyNames getBumpNumber() const;
    Matrix4x4d* getTextureTransformation() const;
    Matrix4x4d* getTextureTransformationInverse() const;
    ColorRgba* getColor1() const;
    ColorRgba* getColor2() const;
    double getTurbulence() const;
    const Vector3Dd& getTextureGradient() const;
    RGBAColorPalette* getColorMap() const;
    ControlledRGBAImageHDRUncompressed* getImage() const;
    ControlledRGBAImageHDRUncompressed* getBumpImage() const;
    ControlledRGBAImageHDRUncompressed* getMaterialImage() const;
    bool isMetallic() const;
    bool getOnceFlag() const;
    bool isConstant() const;
    int getNumberOfWaves() const;
    int getOctaves() const;
    double getMortar() const;
    PovRayMaterial *copy() override;
    Material *translate(Vector3Dd *vector) override;
    Material *rotate(Vector3Dd *vector) override;
    Material *scale(Vector3Dd *vector) override;
    double getObjectIndexOfRefraction() const override;
    Material *prependMaterialLayers(Material *existingMaterial) override;

    // Texture-space copy/transform engine. Because `friend` is forbidden, these member
    // functions are the only writers of the private state besides the constructors; they
    // operate on PovRayMaterial instances through direct field access (transforms) or by
    // building fresh instances (copyTexture). Used at scene-parse time only.
    static PovRayMaterial *copyTexture(const PovRayMaterial *texture);
    static void translateTexture(PovRayMaterial **texturePtr, Vector3Dd *vector);
    static void rotateTexture(PovRayMaterial **texturePtr, Vector3Dd *vector);
    static void scaleTexture(PovRayMaterial **texturePtr, Vector3Dd *vector);
    static void prependTextureLayers(
        PovRayMaterial *newHead, PovRayMaterial *&existingHead);
};

inline const java::ArrayList<PovRayMaterial *>&
PovRayMaterial::getLayers() const { return layers; }
inline const java::ArrayList<PovRayMaterial *>&
PovRayMaterial::getMaterials() const { return materials; }
inline double
PovRayMaterial::getObjectReflection() const { return objectReflection; }
inline double
PovRayMaterial::getObjectAmbient() const { return objectAmbient; }
inline double
PovRayMaterial::getObjectDiffuse() const { return objectDiffuse; }
inline double
PovRayMaterial::getObjectBrilliance() const { return objectBrilliance; }
inline double
PovRayMaterial::getObjectIndexOfRefraction() const { return objectIndexOfRefraction; }
inline double
PovRayMaterial::getObjectRefraction() const { return objectRefraction; }
inline double
PovRayMaterial::getObjectTransmit() const { return objectTransmit; }
inline double
PovRayMaterial::getObjectSpecular() const { return objectSpecular; }
inline double
PovRayMaterial::getObjectRoughness() const { return objectRoughness; }
inline double
PovRayMaterial::getObjectPhong() const { return objectPhong; }
inline double
PovRayMaterial::getObjectPhongSize() const { return objectPhongSize; }
inline double
PovRayMaterial::getBumpAmount() const { return bumpAmount; }
inline double
PovRayMaterial::getTextureRandomness() const { return textureRandomness; }
inline double
PovRayMaterial::getFrequency() const { return frequency; }
inline double
PovRayMaterial::getPhase() const { return phase; }
inline SolidTextureColorNames
PovRayMaterial::getTextureNumber() const { return textureNumber; }
inline SolidTextureBumpyNames
PovRayMaterial::getBumpNumber() const { return bumpNumber; }
inline Matrix4x4d*
PovRayMaterial::getTextureTransformation() const { return textureTransformation; }
inline Matrix4x4d*
PovRayMaterial::getTextureTransformationInverse() const { return textureTransformationInverse; }
inline ColorRgba*
PovRayMaterial::getColor1() const { return color1; }
inline ColorRgba*
PovRayMaterial::getColor2() const { return color2; }
inline double
PovRayMaterial::getTurbulence() const { return turbulence; }
inline const Vector3Dd&
PovRayMaterial::getTextureGradient() const { return textureGradient; }
inline RGBAColorPalette*
PovRayMaterial::getColorMap() const { return colorMap; }
inline ControlledRGBAImageHDRUncompressed*
PovRayMaterial::getImage() const { return image; }
inline ControlledRGBAImageHDRUncompressed*
PovRayMaterial::getBumpImage() const { return bumpImage; }
inline ControlledRGBAImageHDRUncompressed*
PovRayMaterial::getMaterialImage() const { return materialImage; }
inline bool
PovRayMaterial::isMetallic() const { return metallicFlag; }
inline bool
PovRayMaterial::getOnceFlag() const { return onceFlag; }
inline bool
PovRayMaterial::isConstant() const { return constantFlag; }
inline int
PovRayMaterial::getNumberOfWaves() const { return numberOfWaves; }
inline int
PovRayMaterial::getOctaves() const { return octaves; }
inline double
PovRayMaterial::getMortar() const { return mortar; }

#endif
