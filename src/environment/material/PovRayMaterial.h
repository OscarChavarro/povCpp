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
    static void applyRotationTransform(PovRayMaterial *texture, Vector3Dd *vector);
    static void applyScaleTransform(PovRayMaterial *texture, const Vector3Dd *vector);
    static void applyTranslationTransform(PovRayMaterial *texture, const Vector3Dd *vector);
    static PovRayMaterial *copyTextureNode(const PovRayMaterial *src);
    static bool needsTransform(const PovRayMaterial *texture);

    double brickMortar;

    double bumpAmount;
    double bumpFrequency;
    ControlledRGBAImageHDRUncompressed *bumpImage;
    int bumpNumberOfWaves;
    SolidTextureBumpyNames bumpPatternType;
    double bumpPhase;

    ColorRgba *checkerColor1;
    ColorRgba *checkerColor2;
    PovRayMaterial *checkerTexture1;
    PovRayMaterial *checkerTexture2;
    ControlledRGBAImageHDRUncompressed *colorImage;
    RGBAColorPalette *colorMap;
    SolidTextureColorNames colorPatternType;
    java::ArrayList<PovRayMaterial *> layers; // Ordered list of additional texture layers
    ControlledRGBAImageHDRUncompressed *materialMapImage;
    java::ArrayList<PovRayMaterial *> materialMapVariants;
    bool metallicFlag;

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
    Vector3Dd textureGradient;
    double textureRandomness;
    Matrix4x4d *textureTransformation;
    Matrix4x4d *textureTransformationInverse;
    double turbulence;

  public:
    static constexpr int DEFAULT_NUMBER_OF_WAVES = 10;

    PovRayMaterial(double objectReflection, double objectAmbient, double objectDiffuse,
                   double objectBrilliance, double objectIndexOfRefraction, double objectRefraction,
                   double objectTransmit, double objectSpecular, double objectRoughness,
                   double objectPhong, double objectPhongSize, double bumpAmount,
                   double texRandomness, double frequency, double phase,
                   SolidTextureColorNames textureNumber, SolidTextureBumpyNames bumpNumber,
                   Matrix4x4d *textureTransformation, Matrix4x4d *textureTransformationInverse,
                   ColorRgba *checkerColor1, ColorRgba *checkerColor2,
                   PovRayMaterial *checkerTexture1, PovRayMaterial *checkerTexture2, double turbulence,
                   const Vector3Dd &textureGradient, RGBAColorPalette *colorMap,
                   ControlledRGBAImageHDRUncompressed *image,
                   ControlledRGBAImageHDRUncompressed *bumpImage,
                   ControlledRGBAImageHDRUncompressed *materialImage, bool metallicFlag,
                   int numberOfWaves, int octaves, double mortar,
                   java::ArrayList<PovRayMaterial *> layers,
                   java::ArrayList<PovRayMaterial *> materials);

    PovRayMaterial *copy() override;
    static PovRayMaterial *copyTexture(const PovRayMaterial *texture);
    double getBrickMortar() const;
    double getBumpAmount() const;
    double getBumpFrequency() const;
    ControlledRGBAImageHDRUncompressed* getBumpImage() const;
    int getBumpNumberOfWaves() const;
    SolidTextureBumpyNames getBumpPatternType() const;
    double getBumpPhase() const;
    ColorRgba* getCheckerColor1() const;
    ColorRgba* getCheckerColor2() const;
    PovRayMaterial* getCheckerTexture1() const;
    PovRayMaterial* getCheckerTexture2() const;
    ControlledRGBAImageHDRUncompressed*getColorImage() const;
    RGBAColorPalette* getColorMap() const;
    SolidTextureColorNames getColorPatternType() const;
    const java::ArrayList<PovRayMaterial *>& getLayers() const;
    ControlledRGBAImageHDRUncompressed*getMaterialMapImage() const;
    const java::ArrayList<PovRayMaterial *>&getMaterialMapVariants() const;
    double getObjectAmbient() const;
    double getObjectBrilliance() const;
    double getObjectDiffuse() const;
    double getObjectIndexOfRefraction() const override;
    double getObjectPhong() const;
    double getObjectPhongSize() const;
    double getObjectReflection() const;
    double getObjectRefraction() const;
    double getObjectRoughness() const;
    double getObjectSpecular() const;
    double getObjectTransmit() const;
    int getOctaves() const;
    const Vector3Dd& getTextureGradient() const;
    double getTextureRandomness() const;
    Matrix4x4d* getTextureTransformation() const;
    Matrix4x4d* getTextureTransformationInverse() const;
    double getTurbulence() const;
    bool isMetallic() const;
    Material *prependMaterialLayers(Material *existingMaterial) override;

    // Texture-space copy/transform engine. Because `friend` is forbidden, these member
    // functions are the only writers of the private state besides the constructors; they
    // operate on PovRayMaterial instances through direct field access (transforms) or by
    // building fresh instances (copyTexture). Used at scene-parse time only.
    static void prependTextureLayers(
        PovRayMaterial *newHead, PovRayMaterial *&existingHead);
    Material *rotate(Vector3Dd *vector) override;
    static void rotateTexture(PovRayMaterial **texturePtr, Vector3Dd *vector);
    Material *scale(Vector3Dd *vector) override;
    static void scaleTexture(PovRayMaterial **texturePtr, Vector3Dd *vector);
    Material *translate(Vector3Dd *vector) override;
    static void translateTexture(PovRayMaterial **texturePtr, Vector3Dd *vector);
};

inline double
PovRayMaterial::getBrickMortar() const { return brickMortar; }
inline double
PovRayMaterial::getBumpAmount() const { return bumpAmount; }
inline double
PovRayMaterial::getBumpFrequency() const { return bumpFrequency; }
inline ControlledRGBAImageHDRUncompressed*
PovRayMaterial::getBumpImage() const { return bumpImage; }
inline int
PovRayMaterial::getBumpNumberOfWaves() const { return bumpNumberOfWaves; }
inline SolidTextureBumpyNames
PovRayMaterial::getBumpPatternType() const { return bumpPatternType; }
inline double
PovRayMaterial::getBumpPhase() const { return bumpPhase; }
inline ColorRgba*
PovRayMaterial::getCheckerColor1() const { return checkerColor1; }
inline ColorRgba*
PovRayMaterial::getCheckerColor2() const { return checkerColor2; }
inline PovRayMaterial*
PovRayMaterial::getCheckerTexture1() const { return checkerTexture1; }
inline PovRayMaterial*
PovRayMaterial::getCheckerTexture2() const { return checkerTexture2; }
inline ControlledRGBAImageHDRUncompressed*
PovRayMaterial::getColorImage() const { return colorImage; }
inline RGBAColorPalette*
PovRayMaterial::getColorMap() const { return colorMap; }
inline SolidTextureColorNames
PovRayMaterial::getColorPatternType() const { return colorPatternType; }
inline const java::ArrayList<PovRayMaterial *>&
PovRayMaterial::getLayers() const { return layers; }
inline ControlledRGBAImageHDRUncompressed*
PovRayMaterial::getMaterialMapImage() const { return materialMapImage; }
inline const java::ArrayList<PovRayMaterial *>&
PovRayMaterial::getMaterialMapVariants() const { return materialMapVariants; }
inline double
PovRayMaterial::getObjectAmbient() const { return objectAmbient; }
inline double
PovRayMaterial::getObjectBrilliance() const { return objectBrilliance; }
inline double
PovRayMaterial::getObjectDiffuse() const { return objectDiffuse; }
inline double
PovRayMaterial::getObjectIndexOfRefraction() const { return objectIndexOfRefraction; }
inline double
PovRayMaterial::getObjectPhong() const { return objectPhong; }
inline double
PovRayMaterial::getObjectPhongSize() const { return objectPhongSize; }
inline double
PovRayMaterial::getObjectReflection() const { return objectReflection; }
inline double
PovRayMaterial::getObjectRefraction() const { return objectRefraction; }
inline double
PovRayMaterial::getObjectRoughness() const { return objectRoughness; }
inline double
PovRayMaterial::getObjectSpecular() const { return objectSpecular; }
inline double
PovRayMaterial::getObjectTransmit() const { return objectTransmit; }
inline int
PovRayMaterial::getOctaves() const { return octaves; }
inline const Vector3Dd&
PovRayMaterial::getTextureGradient() const { return textureGradient; }
inline double
PovRayMaterial::getTextureRandomness() const { return textureRandomness; }
inline Matrix4x4d*
PovRayMaterial::getTextureTransformation() const { return textureTransformation; }
inline Matrix4x4d*
PovRayMaterial::getTextureTransformationInverse() const { return textureTransformationInverse; }
inline double
PovRayMaterial::getTurbulence() const { return turbulence; }
inline bool
PovRayMaterial::isMetallic() const { return metallicFlag; }

#endif
