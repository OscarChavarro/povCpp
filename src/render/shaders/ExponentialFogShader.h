#ifndef __EXPONENTIAL_FOG_SHADER_H__
#define __EXPONENTIAL_FOG_SHADER_H__

class RGBAColor;

class ExponentialFogShader {
public:
    static void shade(double distance, RGBAColor *fogColor, double fogDistance,
        RGBAColor *color);
};

#endif
