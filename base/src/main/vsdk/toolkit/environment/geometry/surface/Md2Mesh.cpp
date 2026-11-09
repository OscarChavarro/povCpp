#include <cctype>
#include <cstddef>

#include "java/lang/String.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/surface/Md2Mesh.h"
_AnimationInfo::_AnimationInfo() : start(0), end(0) {}
_AnimationInfo::_AnimationInfo(const _AnimationInfo& ai) : name(ai.name), start(ai.start), end(ai.end) {}

Md2Mesh::Md2Mesh()
    : ident(0), version(0), skinWidth(0), skinHeight(0), frameSize(0),
      numSkins(0), numVertices(0), numTexCoords(0), numTriangles(0),
      numGlCommands(0), numFrames(0), offsetSkins(0), offsetTexCoords(0),
      offsetTriangles(0), offsetFrames(0), offsetGlCommands(0), offsetEnd(0),
      frameTimeSeg(0.25f), elapsedTimeSeg(0.0f), currentAnimationInd(0), maxAnimationInd(0)
{
}

short Md2Mesh::getMaxAnimationInd() const { return maxAnimationInd; }
void Md2Mesh::setMaxAnimationInd(short m) { maxAnimationInd = m; }
short Md2Mesh::getCurrentAnimationInd() const { return currentAnimationInd; }
void Md2Mesh::setCurrentAnimationInd(short c)
{
    short denom = (short)(maxAnimationInd + 1);
    currentAnimationInd = (denom > 0) ? (short)(c % denom) : 0;
}
float Md2Mesh::getFrameTimeSeg() const { return frameTimeSeg; }
void Md2Mesh::setFrameTimeSeg(float f) { frameTimeSeg = f; }
float Md2Mesh::getElapsedTimeSeg() const { return elapsedTimeSeg; }
void Md2Mesh::setElapsedTimeSeg(float e) { elapsedTimeSeg = e; }

void Md2Mesh::returnStartEndAnim(short inIndex, short outStartEnd[2])
{
    if (frameNames.size() == 0) {
        outStartEnd[0] = 0;
        outStartEnd[1] = 0;
        return;
    }
    if (aniInfos.size() == 0) {
        fillAniInfo();
    }
    if (inIndex < 0 || inIndex >= (short)aniInfos.size()) {
        outStartEnd[0] = 0;
        outStartEnd[1] = 0;
        return;
    }
    outStartEnd[0] = aniInfos[inIndex].start;
    outStartEnd[1] = aniInfos[inIndex].end;
}

void Md2Mesh::returnStartEndAnim(const java::String& inNameAnim, short outStartEnd[2])
{
    if (frameNames.size() == 0) {
        outStartEnd[0] = 0;
        outStartEnd[1] = 0;
        return;
    }
    if (aniInfos.size() == 0) {
        fillAniInfo();
    }
    _AnimationInfo* ai = getAniInfo(inNameAnim);
    if (ai != 0) {
        outStartEnd[0] = ai->start;
        outStartEnd[1] = ai->end;
    }
    else {
        outStartEnd[0] = 0;
        outStartEnd[1] = 0;
    }
}

_AnimationInfo* Md2Mesh::getAniInfo(const java::String& nameAnim)
{
    java::String needle = nameAnim;
    while (!needle.isEmpty() && std::isspace((unsigned char)needle[needle.size()-1])) {
        needle.erase(needle.size()-1);
    }
    for (size_t i = 0; i < aniInfos.size(); i++) {
        if (aniInfos[i].name == needle) {
            return &aniInfos[i];
        }
    }
    return 0;
}

void Md2Mesh::fillAniInfo()
{
    aniInfos.clear();
    _AnimationInfo aniInfoT;
    aniInfoT.name = "12345678901234567890";

    short i = 0;
    for (i = 0; i < (short)frameNames.size(); i++) {
        java::String fName = frameNames[i];

        while (!fName.isEmpty() && std::isspace((unsigned char)fName[fName.size()-1])) {
            fName.erase(fName.size()-1);
        }

        size_t posNull = fName.find('\0');
        if (posNull != java::String::npos) {
            fName = fName.substr(0, posNull);
        }

        int pos = (int)fName.size() - 1;
        while (pos != -1 && std::isdigit((unsigned char)fName[(size_t)pos])) {
            pos--;
        }
        fName = fName.substr(0, (size_t)(pos + 1));

        if (fName != aniInfoT.name) {
            if (i != 0) {
                aniInfoT.end = (short)(i - 1);
                aniInfos.add(aniInfoT);
            }
            aniInfoT = _AnimationInfo();
            aniInfoT.name = fName;
            aniInfoT.start = i;
        }
    }

    if (frameNames.size() > 0) {
        aniInfoT.end = (short)(i - 1);
        aniInfos.add(aniInfoT);
    }
    maxAnimationInd = (short)(aniInfos.size() == 0 ? 0 : (aniInfos.size() - 1));
}

const float Md2Mesh::anorms[162][3] =
    {
        {-0.525731f, 0.000000f, 0.850651f}, 
        {-0.442863f, 0.238856f, 0.864188f}, 
        {-0.295242f, 0.000000f, 0.955423f}, 
        {-0.309017f, 0.500000f, 0.809017f}, 
        {-0.162460f, 0.262866f, 0.951056f}, 
        {0.000000f, 0.000000f, 1.000000f}, 
        {0.000000f, 0.850651f, 0.525731f}, 
        {-0.147621f, 0.716567f, 0.681718f}, 
        {0.147621f, 0.716567f, 0.681718f}, 
        {0.000000f, 0.525731f, 0.850651f}, 
        {0.309017f, 0.500000f, 0.809017f}, 
        {0.525731f, 0.000000f, 0.850651f}, 
        {0.295242f, 0.000000f, 0.955423f}, 
        {0.442863f, 0.238856f, 0.864188f}, 
        {0.162460f, 0.262866f, 0.951056f}, 
        {-0.681718f, 0.147621f, 0.716567f}, 
        {-0.809017f, 0.309017f, 0.500000f}, 
        {-0.587785f, 0.425325f, 0.688191f}, 
        {-0.850651f, 0.525731f, 0.000000f}, 
        {-0.864188f, 0.442863f, 0.238856f}, 
        {-0.716567f, 0.681718f, 0.147621f}, 
        {-0.688191f, 0.587785f, 0.425325f}, 
        {-0.500000f, 0.809017f, 0.309017f}, 
        {-0.238856f, 0.864188f, 0.442863f}, 
        {-0.425325f, 0.688191f, 0.587785f}, 
        {-0.716567f, 0.681718f, -0.147621f}, 
        {-0.500000f, 0.809017f, -0.309017f}, 
        {-0.525731f, 0.850651f, 0.000000f}, 
        {0.000000f, 0.850651f, -0.525731f}, 
        {-0.238856f, 0.864188f, -0.442863f}, 
        {0.000000f, 0.955423f, -0.295242f}, 
        {-0.262866f, 0.951056f, -0.162460f}, 
        {0.000000f, 1.000000f, 0.000000f}, 
        {0.000000f, 0.955423f, 0.295242f}, 
        {-0.262866f, 0.951056f, 0.162460f}, 
        {0.238856f, 0.864188f, 0.442863f}, 
        {0.262866f, 0.951056f, 0.162460f}, 
        {0.500000f, 0.809017f, 0.309017f}, 
        {0.238856f, 0.864188f, -0.442863f}, 
        {0.262866f, 0.951056f, -0.162460f}, 
        {0.500000f, 0.809017f, -0.309017f}, 
        {0.850651f, 0.525731f, 0.000000f}, 
        {0.716567f, 0.681718f, 0.147621f}, 
        {0.716567f, 0.681718f, -0.147621f}, 
        {0.525731f, 0.850651f, 0.000000f}, 
        {0.425325f, 0.688191f, 0.587785f}, 
        {0.864188f, 0.442863f, 0.238856f}, 
        {0.688191f, 0.587785f, 0.425325f}, 
        {0.809017f, 0.309017f, 0.500000f}, 
        {0.681718f, 0.147621f, 0.716567f}, 
        {0.587785f, 0.425325f, 0.688191f}, 
        {0.955423f, 0.295242f, 0.000000f}, 
        {1.000000f, 0.000000f, 0.000000f}, 
        {0.951056f, 0.162460f, 0.262866f}, 
        {0.850651f, -0.525731f, 0.000000f}, 
        {0.955423f, -0.295242f, 0.000000f}, 
        {0.864188f, -0.442863f, 0.238856f}, 
        {0.951056f, -0.162460f, 0.262866f}, 
        {0.809017f, -0.309017f, 0.500000f}, 
        {0.681718f, -0.147621f, 0.716567f}, 
        {0.850651f, 0.000000f, 0.525731f}, 
        {0.864188f, 0.442863f, -0.238856f}, 
        {0.809017f, 0.309017f, -0.500000f}, 
        {0.951056f, 0.162460f, -0.262866f}, 
        {0.525731f, 0.000000f, -0.850651f}, 
        {0.681718f, 0.147621f, -0.716567f}, 
        {0.681718f, -0.147621f, -0.716567f}, 
        {0.850651f, 0.000000f, -0.525731f}, 
        {0.809017f, -0.309017f, -0.500000f}, 
        {0.864188f, -0.442863f, -0.238856f}, 
        {0.951056f, -0.162460f, -0.262866f}, 
        {0.147621f, 0.716567f, -0.681718f}, 
        {0.309017f, 0.500000f, -0.809017f}, 
        {0.425325f, 0.688191f, -0.587785f}, 
        {0.442863f, 0.238856f, -0.864188f}, 
        {0.587785f, 0.425325f, -0.688191f}, 
        {0.688191f, 0.587785f, -0.425325f}, 
        {-0.147621f, 0.716567f, -0.681718f}, 
        {-0.309017f, 0.500000f, -0.809017f}, 
        {0.000000f, 0.525731f, -0.850651f}, 
        {-0.525731f, 0.000000f, -0.850651f}, 
        {-0.442863f, 0.238856f, -0.864188f}, 
        {-0.295242f, 0.000000f, -0.955423f}, 
        {-0.162460f, 0.262866f, -0.951056f}, 
        {0.000000f, 0.000000f, -1.000000f}, 
        {0.295242f, 0.000000f, -0.955423f}, 
        {0.162460f, 0.262866f, -0.951056f}, 
        {-0.442863f, -0.238856f, -0.864188f}, 
        {-0.309017f, -0.500000f, -0.809017f}, 
        {-0.162460f, -0.262866f, -0.951056f}, 
        {0.000000f, -0.850651f, -0.525731f}, 
        {-0.147621f, -0.716567f, -0.681718f}, 
        {0.147621f, -0.716567f, -0.681718f}, 
        {0.000000f, -0.525731f, -0.850651f}, 
        {0.309017f, -0.500000f, -0.809017f}, 
        {0.442863f, -0.238856f, -0.864188f}, 
        {0.162460f, -0.262866f, -0.951056f}, 
        {0.238856f, -0.864188f, -0.442863f}, 
        {0.500000f, -0.809017f, -0.309017f}, 
        {0.425325f, -0.688191f, -0.587785f}, 
        {0.716567f, -0.681718f, -0.147621f}, 
        {0.688191f, -0.587785f, -0.425325f}, 
        {0.587785f, -0.425325f, -0.688191f}, 
        {0.000000f, -0.955423f, -0.295242f}, 
        {0.000000f, -1.000000f, 0.000000f}, 
        {0.262866f, -0.951056f, -0.162460f}, 
        {0.000000f, -0.850651f, 0.525731f}, 
        {0.000000f, -0.955423f, 0.295242f}, 
        {0.238856f, -0.864188f, 0.442863f}, 
        {0.262866f, -0.951056f, 0.162460f}, 
        {0.500000f, -0.809017f, 0.309017f}, 
        {0.716567f, -0.681718f, 0.147621f}, 
        {0.525731f, -0.850651f, 0.000000f}, 
        {-0.238856f, -0.864188f, -0.442863f}, 
        {-0.500000f, -0.809017f, -0.309017f}, 
        {-0.262866f, -0.951056f, -0.162460f}, 
        {-0.850651f, -0.525731f, 0.000000f}, 
        {-0.716567f, -0.681718f, -0.147621f}, 
        {-0.716567f, -0.681718f, 0.147621f}, 
        {-0.525731f, -0.850651f, 0.000000f}, 
        {-0.500000f, -0.809017f, 0.309017f}, 
        {-0.238856f, -0.864188f, 0.442863f}, 
        {-0.262866f, -0.951056f, 0.162460f}, 
        {-0.864188f, -0.442863f, 0.238856f}, 
        {-0.809017f, -0.309017f, 0.500000f}, 
        {-0.688191f, -0.587785f, 0.425325f}, 
        {-0.681718f, -0.147621f, 0.716567f}, 
        {-0.442863f, -0.238856f, 0.864188f}, 
        {-0.587785f, -0.425325f, 0.688191f}, 
        {-0.309017f, -0.500000f, 0.809017f}, 
        {-0.147621f, -0.716567f, 0.681718f}, 
        {-0.425325f, -0.688191f, 0.587785f}, 
        {-0.162460f, -0.262866f, 0.951056f}, 
        {0.442863f, -0.238856f, 0.864188f}, 
        {0.162460f, -0.262866f, 0.951056f}, 
        {0.309017f, -0.500000f, 0.809017f}, 
        {0.147621f, -0.716567f, 0.681718f}, 
        {0.000000f, -0.525731f, 0.850651f}, 
        {0.425325f, -0.688191f, 0.587785f}, 
        {0.587785f, -0.425325f, 0.688191f}, 
        {0.688191f, -0.587785f, 0.425325f}, 
        {-0.955423f, 0.295242f, 0.000000f}, 
        {-0.951056f, 0.162460f, 0.262866f}, 
        {-1.000000f, 0.000000f, 0.000000f}, 
        {-0.850651f, 0.000000f, 0.525731f}, 
        {-0.955423f, -0.295242f, 0.000000f}, 
        {-0.951056f, -0.162460f, 0.262866f}, 
        {-0.864188f, 0.442863f, -0.238856f}, 
        {-0.951056f, 0.162460f, -0.262866f}, 
        {-0.809017f, 0.309017f, -0.500000f}, 
        {-0.864188f, -0.442863f, -0.238856f}, 
        {-0.951056f, -0.162460f, -0.262866f}, 
        {-0.809017f, -0.309017f, -0.500000f}, 
        {-0.681718f, 0.147621f, -0.716567f}, 
        {-0.681718f, -0.147621f, -0.716567f}, 
        {-0.850651f, 0.000000f, -0.525731f}, 
        {-0.688191f, 0.587785f, -0.425325f}, 
        {-0.587785f, 0.425325f, -0.688191f}, 
        {-0.425325f, 0.688191f, -0.587785f}, 
        {-0.425325f, -0.688191f, -0.587785f}, 
        {-0.587785f, -0.425325f, -0.688191f}, 
        {-0.688191f, -0.587785f, -0.425325f}
    };
