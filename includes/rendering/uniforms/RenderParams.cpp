#include "rendering/uniforms/RenderParams.h"

void RenderParams::apply(Shader& shader) const
{
    shader.setBool ("enableNormalMapping",   enableNormalMapping);
    shader.setBool ("enableParallaxMapping", enableParallaxMapping);
    shader.setFloat("bumpNormalStrength",    bumpNormalStrength);
    shader.setInt  ("numLayers",             numLayers);
    shader.setFloat("parallaxHeightScale",   parallaxHeightScale);
}