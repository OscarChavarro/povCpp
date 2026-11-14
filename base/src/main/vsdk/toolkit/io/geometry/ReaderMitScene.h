#ifndef __READER_MIT_SCENE__
#define __READER_MIT_SCENE__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class Image;
class NormalMap;
class SimpleBody;
class SimpleMaterial;
class SimpleScene;

// Reader of the MIT 6.837 scene format as extended by VITRAL. Relative resource
// files (textures, bump maps, altimetry, OBJ models, cubemap faces) are looked
// up from the scene file's directory first and then from the working directory.
// Textures and normal maps are scene resources shared by the bodies using them.
class ReaderMitScene {
private:
    java::String sceneDirectory;
    // Euler angles of the last `rotation` line, applied to each later primitive.
    double yaw;
    double pitch;
    double roll;
    Image* currentTexture;
    NormalMap* currentNormalMap;
    java::ArrayList<java::String> texturePaths;
    java::ArrayList<Image*> textures;
    java::ArrayList<java::String> normalMapKeys;
    java::ArrayList<NormalMap*> normalMaps;

    void resetState(const char* fileName);
    bool hasRotation() const;
    // Resolved path of a relative resource, or the path as given when missing.
    java::String resolveResource(const java::String& path) const;
    Image* loadTexture(const java::String& path, SimpleScene* scene);
    NormalMap* loadNormalMap(const java::String& path, const Vector3Dd& scale,
        SimpleScene* scene);
    // Applies the current rotation about the body's origin.
    void applyRotation(SimpleBody* body) const;
    // Applies the current rotation, texture and normal map.
    void dressBody(SimpleBody* body) const;
    // Moves the bodies of an OBJ file into the scene, placed by the current
    // rotation, the translation and a uniform scale; false when unreadable.
    bool importObj(const java::String& path, const Vector3Dd& translation,
        double uniformScale, const SimpleMaterial* fallbackMaterial, SimpleScene* scene) const;
public:
    ReaderMitScene();
    virtual ~ReaderMitScene() {}

    void importEnvironment(const char* fileName, SimpleScene* outScene);
};

#endif
