#include "vsdk/toolkit/environment/material/ShadingType.h"
ShadingType ShadingTypeUtil::fromCode(int code)
{
    switch ( code ) {
        case SHADING_NOLIGHT: return SHADING_NOLIGHT;
        case SHADING_FLAT: return SHADING_FLAT;
        case SHADING_GOURAUD: return SHADING_GOURAUD;
        case SHADING_PHONG: return SHADING_PHONG;
        case SHADING_COOK_TERRANCE: return SHADING_COOK_TERRANCE;
        default: return SHADING_GOURAUD;
    }
}

ShadingType ShadingTypeUtil::next(ShadingType value)
{
    return static_cast<ShadingType>((static_cast<int>(value) + 1) % 5);
}

const char* ShadingTypeUtil::name(ShadingType value)
{
    switch ( value ) {
        case SHADING_NOLIGHT: return "NOLIGHT";
        case SHADING_FLAT: return "FLAT";
        case SHADING_GOURAUD: return "GOURAUD";
        case SHADING_PHONG: return "PHONG";
        case SHADING_COOK_TERRANCE: return "COOK_TERRANCE";
        default: return "GOURAUD";
    }
}
