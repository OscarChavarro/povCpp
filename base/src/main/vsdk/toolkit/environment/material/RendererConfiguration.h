#ifndef __RENDERERCONFIGURATION__
#define __RENDERERCONFIGURATION__

#include "java/lang/String.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/environment/material/ShadingType.h"
class RendererConfiguration {
public:
    static const int SHADING_TYPE_NOLIGHT = 0;
    static const int SHADING_TYPE_FLAT = 1;
    static const int SHADING_TYPE_GOURAUD = 2;
    static const int SHADING_TYPE_PHONG = 3;
    static const int SHADING_TYPE_COOK_TERRANCE = 4;

private:
    int shadingType;
    bool surfaces;
    bool wires;
    bool boundingVolume;
    bool selectionCorners;
    bool texture;
    bool bumpMap;
    bool points;
    bool normals;
    bool trianglesNormals;
    bool useVertexColors;
    double vertexNormalSmoothingThresholdDegrees;
    ColorRgb wireColor;
    ColorRgb boundingVolumeColor;
    int lodHint;

public:
    RendererConfiguration();
    virtual ~RendererConfiguration() {}

    int compareTo(const RendererConfiguration& other) const;
    void cloneFrom(const RendererConfiguration& other);
    RendererConfiguration clone() const;

    void setLodHint(int l);
    int getLodHint() const;

    void setWireColor(const ColorRgb& c);
    void setWireColor(double r, double g, double b);
    ColorRgb getWireColor() const;

    void setSurfaces(bool b);
    void setWires(bool b);
    void setBoundingVolume(bool b);
    void setSelectionCorners(bool c);
    void setTexture(bool b);
    void setBumpMap(bool b);
    void setPoints(bool b);
    void setNormals(bool b);
    void setTrianglesNormals(bool b);
    void setShadingType(int type);
    void setShadingType(ShadingType type);
    void setVertexNormalSmoothingThresholdDegrees(double thresholdDegrees);

    bool isSurfacesSet() const;
    bool isWiresSet() const;
    bool isBoundingVolumeSet() const;
    bool isSelectionCornersSet() const;
    bool isTextureSet() const;
    bool isBumpMapSet() const;
    bool isPointsSet() const;
    bool isNormalsSet() const;
    bool isTrianglesNormalsSet() const;

    int getShadingType() const;
    ShadingType getShadingTypeEnum() const;
    double getVertexNormalSmoothingThresholdDegrees() const;

    void changeSurfaces();
    void changeWires();
    void changeBoundingVolume();
    void changeSelectionCorners();
    void changeTexture();
    void changeBumpMap();
    void changePoints();
    void changeNormals();
    void changeTrianglesNormals();
    void changeShadingType();

    bool getUseVertexColors() const;
    void setUseVertexColors(bool useVertexColors);
    ColorRgb getBoundingVolumeColor() const;
    void setBoundingVolumeColor(const ColorRgb& boundingVolumeColor);

    java::String toString() const;
};

#endif
