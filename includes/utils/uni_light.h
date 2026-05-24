#pragma once
#include "core/Shader.h"
#include "scene/Light.h"
#include <glm/glm.hpp>

void setUniLight(
    Shader &Shader,
    const glm::vec3 &viewPos,
    const glm::mat4 &view,
    const glm::mat4 &projection
);

void setSunLight(Shader &Shader, const SunLightData &sun);
void setPointLight(Shader &Shader, int index, const PointLightData &point);
void setFlashLight(Shader &Shader, const FlashLightData &flash, bool flashLightOn);
void setMaterial(Shader &Shader, const MaterialData &material);
