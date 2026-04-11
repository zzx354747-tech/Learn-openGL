#pragma once
#include "shader.h"
#include "camera.h"
#include "light.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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