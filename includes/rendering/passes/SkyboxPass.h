#include "rendering/uniforms/SkyboxCameraUniformSetter.h"
#include "rendering/core/SceneRenderResources.h"

class SkyboxPass
{
public:
    static void renderSkyboxPass( 
        Camera& camera,
        SceneRenderResources& resources,
        SceneRenderConfig& config,
        int bfwidth, 
        int bfheight);

private:
    static void bindSkyboxTexture(EnvCubemap* skybox);
};
