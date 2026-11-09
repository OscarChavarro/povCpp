#ifndef __MD_2_MESH__
#define __MD_2_MESH__

#include "java/lang/String.h"
#include "java/util/ArrayList.h"
class Image;

class _AnimationInfo {
public:
    java::String name;
    short start;
    short end;

    _AnimationInfo();
    _AnimationInfo(const _AnimationInfo& ai);
};

class Md2Mesh {
public:
    int ident;
    int version;
    int skinWidth;
    int skinHeight;
    int frameSize;
    int numSkins;
    int numVertices;
    int numTexCoords;
    int numTriangles;
    int numGlCommands;
    int numFrames;
    int offsetSkins;
    int offsetTexCoords;
    int offsetTriangles;
    int offsetFrames;
    int offsetGlCommands;
    int offsetEnd;

    java::ArrayList<java::String> skinNames;
    java::ArrayList<Image*> skins;
    java::ArrayList<java::String> frameNames;
    java::ArrayList< java::ArrayList<float> > frameVertices;
    java::ArrayList< java::ArrayList<short> > frameNormalIndices;
    java::ArrayList< java::ArrayList<float> > glCmdTexCoordsStrip;
    java::ArrayList< java::ArrayList<int> > glCmdVertIndexStrip;
    java::ArrayList< java::ArrayList<float> > glCmdTexCoordsFan;
    java::ArrayList< java::ArrayList<int> > glCmdVertIndexFan;
    java::ArrayList<float> texCoords;
    java::ArrayList< java::ArrayList<int> > triangles;

private:
    java::ArrayList<_AnimationInfo> aniInfos;
    float frameTimeSeg;
    float elapsedTimeSeg;
    short currentAnimationInd;
    short maxAnimationInd;

    _AnimationInfo* getAniInfo(const java::String& nameAnim);
    void fillAniInfo();

public:
    Md2Mesh();

    short getMaxAnimationInd() const;
    void setMaxAnimationInd(short maxAnimationInd);
    short getCurrentAnimationInd() const;
    void setCurrentAnimationInd(short currentAnimationInd);
    float getFrameTimeSeg() const;
    void setFrameTimeSeg(float frameTimeSeg);
    float getElapsedTimeSeg() const;
    void setElapsedTimeSeg(float elapsedTimeSeg);

    void returnStartEndAnim(short inIndex, short outStartEnd[2]);
    void returnStartEndAnim(const java::String& inNameAnim, short outStartEnd[2]);

    static const float anorms[162][3];
};

// Compressed vertex
class _md2Vertex {
public:
    unsigned char v[3];
    unsigned char normalIndex;
};

// Texture coords
class _md2TexCoord {
public:
    short s;
    short t;
};

// Triangle info
class _md2Triangle {
public:
    short vertex[3];
    short st[3];
};

// Model frame
class _md2Frame {
public:
    float scale[3];
    float translate[3];
    unsigned char name[16];
    java::ArrayList<_md2Vertex> verts;
};

#endif
