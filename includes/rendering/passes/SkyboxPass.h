#include "rendering/uniforms/SkyboxCameraUniformSetter.h"
#include "rendering/core/SceneRenderResources.h"

class SkyboxPass
{
public:
    static void renderSkyboxPass( 
        Camera& camera, 
        SceneRenderResources1& resources, 
        SceneRenderConfig& config,
        int bfwidth, 
        int bfheight);

private:
    static void bindSkyboxTexture(CubeMap* skybox);
};