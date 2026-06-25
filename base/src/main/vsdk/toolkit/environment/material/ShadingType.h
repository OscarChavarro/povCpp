#ifndef __VSDK_TOOLKIT_ENVIRONMENT_MATERIAL_SHADINGTYPE_H__
#define __VSDK_TOOLKIT_ENVIRONMENT_MATERIAL_SHADINGTYPE_H__

enum ShadingType {
    SHADING_NOLIGHT = 0,
    SHADING_FLAT = 1,
    SHADING_GOURAUD = 2,
    SHADING_PHONG = 3,
    SHADING_COOK_TERRANCE = 4
};

class ShadingTypeUtil {
public:
    static ShadingType fromCode(int code);
    static ShadingType next(ShadingType value);
};

#endif
