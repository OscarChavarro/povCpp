#ifndef __SHADER_SELECTOR__
#define __SHADER_SELECTOR__

class RendererConfiguration;
class Shader;

class ShaderSelector {
public:
    static Shader* select(const RendererConfiguration* qualitySelection);
};

#endif
