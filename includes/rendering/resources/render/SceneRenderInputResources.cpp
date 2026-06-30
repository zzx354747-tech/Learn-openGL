#include "rendering/resources/render/SceneRenderInputResources.h"

bool PBRMaterialTextures::isValid() const
{
        return albedo && roughness && metallic;
    }
