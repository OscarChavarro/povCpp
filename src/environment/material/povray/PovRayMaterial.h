#ifndef __POV_RAY_MATERIAL__
#define __POV_RAY_MATERIAL__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"
#include "environment/material/Material.h"

class SolidTexturePigment;
class SolidTextureNormal;

class PovRayMaterial : public Material {
  private:
    static void applyRotationTransform(PovRayMaterial *texture, Vector3Dd *vector);
    static void applyScaleTransform(PovRayMaterial *texture, const Vector3Dd *vector);
    static void applyTranslationTransform(PovRayMaterial *texture, const Vector3Dd *vector);
    static PovRayMaterial *copyTextureNode(const PovRayMaterial *src);
    static bool needsTransform(const PovRayMaterial *texture);

    // TextureParser builds a texture incrementally, one POV attribute token at a time,
    // round-tripping through PovRayMaterialBuilder on every token (turbulence/octaves/...
    // can appear before the pattern keyword that actually selects a pigment, e.g. "turbulence
    // 0.8 bozo"). These fields exist purely so that pending attribute values survive a
    // build() round trip even while pigment/normal is still nullptr; once the pattern is
    // known they are folded into the concrete pigment/normal object and are not consulted
    // again at render time.
    double pendingBumpAmount;
    double pendingFrequency;
    double pendingMortar;
    double pendingPhase;
    double pendingTurbulence;
    int pendingNumberOfWaves;
    int pendingOctaves;
    ControlledRGBAImageHDRUncompressed *pendingBumpImage;
    RGBAColorPalette *pendingColorMap;

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

    SolidTexturePigment *pigment; // nullptr == NO_TEXTURE
    SolidTextureNormal *normal;   // nullptr == NO_BUMPS

    double textureRandomness;
    Matrix4x4d *textureTransformation;
    Matrix4x4d *textureTransformationInverse;

  public:
    static constexpr int DEFAULT_NUMBER_OF_WAVES = 10;

    PovRayMaterial(double objectReflection, double objectAmbient, double objectDiffuse,
                   double objectBrilliance, double objectIndexOfRefraction, double objectRefraction,
                   double objectTransmit, double objectSpecular, double objectRoughness,
                   double objectPhong, double objectPhongSize, double texRandomness,
                   Matrix4x4d *textureTransformation, Matrix4x4d *textureTransformationInverse,
                   SolidTexturePigment *pigment, SolidTextureNormal *normal,
                   ControlledRGBAImageHDRUncompressed *materialImage, bool metallicFlag,
                   java::ArrayList<PovRayMaterial *> layers,
                   java::ArrayList<PovRayMaterial *> materials,
                   double pendingTurbulence, int pendingOctaves, RGBAColorPalette *pendingColorMap,
                   double pendingMortar, double pendingBumpAmount, double pendingFrequency,
                   double pendingPhase, int pendingNumberOfWaves,
                   ControlledRGBAImageHDRUncompressed *pendingBumpImage);
    ~PovRayMaterial() override;

    PovRayMaterial *copy() override;
    static PovRayMaterial *copyTexture(const PovRayMaterial *texture);
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
    SolidTexturePigment *getPigment() const;
    SolidTextureNormal *getNormal() const;
    double getPendingTurbulence() const;
    int getPendingOctaves() const;
    RGBAColorPalette *getPendingColorMap() const;
    double getPendingMortar() const;
    double getPendingBumpAmount() const;
    double getPendingFrequency() const;
    double getPendingPhase() const;
    int getPendingNumberOfWaves() const;
    ControlledRGBAImageHDRUncompressed *getPendingBumpImage() const;
    double getTextureRandomness() const;
    Matrix4x4d* getTextureTransformation() const;
    Matrix4x4d* getTextureTransformationInverse() const;
    bool isMetallic() const;
    Material *prependMaterialLayers(Material *existingMaterial) override;
    void releaseFromOwner() override;

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
inline SolidTexturePigment *
PovRayMaterial::getPigment() const { return pigment; }
inline SolidTextureNormal *
PovRayMaterial::getNormal() const { return normal; }
inline double
PovRayMaterial::getPendingTurbulence() const { return pendingTurbulence; }
inline int
PovRayMaterial::getPendingOctaves() const { return pendingOctaves; }
inline RGBAColorPalette *
PovRayMaterial::getPendingColorMap() const { return pendingColorMap; }
inline double
PovRayMaterial::getPendingMortar() const { return pendingMortar; }
inline double
PovRayMaterial::getPendingBumpAmount() const { return pendingBumpAmount; }
inline double
PovRayMaterial::getPendingFrequency() const { return pendingFrequency; }
inline double
PovRayMaterial::getPendingPhase() const { return pendingPhase; }
inline int
PovRayMaterial::getPendingNumberOfWaves() const { return pendingNumberOfWaves; }
inline ControlledRGBAImageHDRUncompressed *
PovRayMaterial::getPendingBumpImage() const { return pendingBumpImage; }
inline double
PovRayMaterial::getTextureRandomness() const { return textureRandomness; }
inline Matrix4x4d*
PovRayMaterial::getTextureTransformation() const { return textureTransformation; }
inline Matrix4x4d*
PovRayMaterial::getTextureTransformationInverse() const { return textureTransformationInverse; }
inline bool
PovRayMaterial::isMetallic() const { return metallicFlag; }

#endif
