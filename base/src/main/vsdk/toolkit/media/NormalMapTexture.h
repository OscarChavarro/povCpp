#ifndef VSDK_NORMAL_MAP_TEXTURE_H
#define VSDK_NORMAL_MAP_TEXTURE_H
#include <vector>
#include "vsdk/toolkit/media/RGBImageUncompressed.h"
#include "vsdk/toolkit/media/NormalMap.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class NormalMapTexture : public RGBImageUncompressed {
    std::vector<float> samples;
public:
    explicit NormalMapTexture(const NormalMap& source) {
        int w = source.getXSize(), h = source.getYSize();
        init(w, h);
        samples.reserve(w * h * 3);
        for (int y = h - 1; y >= 0; --y) for (int x = 0; x < w; ++x) {
            Vector3Dd* sample = source.getNormal(x, y);
            Vector3Dd n = sample ? sample->normalized() : Vector3Dd(0, 0, 1);
            delete sample;
            float r = (n.x() + 1) * 0.5, g = (n.y() + 1) * 0.5, b = (n.z() + 1) * 0.5;
            samples.push_back(r); samples.push_back(g); samples.push_back(b);
            putPixel(x, y, (char)(r * 255), (char)(g * 255), (char)(b * 255));
        }
    }
    const float* getFloatSamples() const { return samples.data(); }
};
#endif
