#ifndef __EXPONENTIAL_FOG_SHADER_H__
#define __EXPONENTIAL_FOG_SHADER_H__

class ColorRgba;

class ExponentialFogShader {
public:
    static void shade(double distance, const ColorRgba *fogColor, double fogDistance,
        ColorRgba *color);
};

#endif
