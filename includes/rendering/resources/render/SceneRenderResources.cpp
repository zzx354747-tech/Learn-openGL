#include "rendering/resources/render/SceneRenderResources.h"

SceneRenderResources::SceneRenderResources()
{
    declareLightingPassResources();
}

void SceneRenderResources::declareLightingPassResources()
{
        lightingHandles.gPosition = registry.declareTexture("gPosition");
        lightingHandles.gNormalRoughness = registry.declareTexture("gNormalRoughness");
        lightingHandles.gAlbedoMetallic = registry.declareTexture("gAlbedoMetallic");
        lightingHandles.ao = registry.declareTexture("ao");
        lightingHandles.shadowMap = registry.declareTexture("shadowMap");
        lightingHandles.depthCubeMap = registry.declareTexture("depthCubeMap");
        lightingHandles.spotShadowMap = registry.declareTexture("spotShadowMap");
        lightingHandles.brdfLUT = registry.declareTexture("brdfLUT");
        lightingHandles.irradianceMap = registry.declareTexture("irradianceMap");
        lightingHandles.prefilterMap = registry.declareTexture("prefilterMap");
    }
