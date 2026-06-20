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
    java::ArrayList<PovRayMaterial *> materialMapVariants;
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
    double bumpFrequency;
    double bumpPhase;
    SolidTextureColorNames colorPatternType;
    SolidTextureBumpyNames bumpPatternType;
    Matrix4x4d *textureTransformation;
    Matrix4x4d *textureTransformationInverse;
    ColorRgba *color1;
    ColorRgba *color2;
    double turbulence;
    Vector3Dd textureGradient;
    RGBAColorPalette *colorMap;
    ControlledRGBAImageHDRUncompressed *colorImage;
    ControlledRGBAImageHDRUncompressed *bumpImage;
    ControlledRGBAImageHDRUncompressed *materialMapImage;
    bool metallicFlag;
    int bumpNumberOfWaves;
    int octaves;
    double brickMortar;

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
                   int numberOfWaves, int octaves, double mortar,
                   java::ArrayList<PovRayMaterial *> layers,
                   java::ArrayList<PovRayMaterial *> materials);

    const java::ArrayList<PovRayMaterial *>& getLayers() const;
    const java::ArrayList<PovRayMaterial *>&getMaterialMapVariants() const;
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
    double getBumpFrequency() const;
    double getBumpPhase() const;
    SolidTextureColorNames getColorPatternType() const;
    SolidTextureBumpyNames getBumpPatternType() const;
    Matrix4x4d* getTextureTransformation() const;
    Matrix4x4d* getTextureTransformationInverse() const;
    ColorRgba* getColor1() const;
    ColorRgba* getColor2() const;
    double getTurbulence() const;
    const Vector3Dd& getTextureGradient() const;
    RGBAColorPalette* getColorMap() const;
    ControlledRGBAImageHDRUncompressed*getColorImage() const;
    ControlledRGBAImageHDRUncompressed* getBumpImage() const;
    ControlledRGBAImageHDRUncompressed*getMaterialMapImage() const;
    bool isMetallic() const;
    int getBumpNumberOfWaves() const;
    int getOctaves() const;
    double getBrickMortar() const;
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
PovRayMaterial::getMaterialMapVariants() const { return materialMapVariants; }
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
PovRayMaterial::getBumpFrequency() const { return bumpFrequency; }
inline double
PovRayMaterial::getBumpPhase() const { return bumpPhase; }
inline SolidTextureColorNames
PovRayMaterial::getColorPatternType() const { return colorPatternType; }
inline SolidTextureBumpyNames
PovRayMaterial::getBumpPatternType() const { return bumpPatternType; }
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
PovRayMaterial::getColorImage() const { return colorImage; }
inline ControlledRGBAImageHDRUncompressed*
PovRayMaterial::getBumpImage() const { return bumpImage; }
inline ControlledRGBAImageHDRUncompressed*
PovRayMaterial::getMaterialMapImage() const { return materialMapImage; }
inline bool
PovRayMaterial::isMetallic() const { return metallicFlag; }
inline int
PovRayMaterial::getBumpNumberOfWaves() const { return bumpNumberOfWaves; }
inline int
PovRayMaterial::getOctaves() const { return octaves; }
inline double
PovRayMaterial::getBrickMortar() const { return brickMortar; }

#endif
