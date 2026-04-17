#include "utils/uni_light.h"
#include "core/shader.h"
#include "scene/camera.h"
#include "scene/light.h"
#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void setUniLight(
    Shader &Shader,
    const glm::vec3 &viewPos,
    const glm::mat4 &view,
    const glm::mat4 &projection
)
{
    Shader.setMat4("view", view);
    Shader.setMat4("projection", projection);
    Shader.setVec3("viewPos", viewPos);
}

void setSunLight(Shader &Shader, const SunLightData &sun)
{
    Shader.setVec3("sun.direction", sun.direction);
    Shader.setVec3("sun.ambient", sun.ambient);
    Shader.setVec3("sun.diffuse", sun.diffuse);
    Shader.setVec3("sun.specular", sun.specular);
}

void setPointLight(Shader &Shader, int index, const PointLightData &point)
{
    std::string base = "pointLights[" + std::to_string(index) + "]";
    Shader.setVec3(base + ".position", point.position);
    Shader.setVec3(base + ".ambient", point.ambient);
    Shader.setVec3(base + ".diffuse", point.diffuse);
    Shader.setVec3(base + ".specular", point.specular);
    Shader.setFloat(base + ".constant", point.constant);
    Shader.setFloat(base + ".linear", point.linear);
    Shader.setFloat(base + ".quadratic", point.quadratic);
}

void setFlashLight(Shader &Shader, const FlashLightData &flash, bool flashLightOn)
{
    Shader.setVec3("flashLight.position", flash.position);
    Shader.setVec3("flashLight.direction", flash.direction);
    Shader.setVec3("flashLight.ambient", flash.ambient);
    Shader.setVec3("flashLight.diffuse", flash.diffuse);
    Shader.setVec3("flashLight.specular", flash.specular);
    Shader.setFloat("flashLight.constant", flash.constant);
    Shader.setFloat("flashLight.linear", flash.linear);
    Shader.setFloat("flashLight.quadratic", flash.quadratic);
    Shader.setFloat("flashLight.cutOff", flash.cutOff);
    Shader.setFloat("flashLight.outerCutOff", flash.outerCutOff);
    Shader.setBool("flashLightOn", flashLightOn);
}

void setMaterial(Shader &Shader, const MaterialData &material)
{
    Shader.setInt("material.diffuse", material.diffuse);
    Shader.setInt("material.specular", material.specular);
    Shader.setFloat("material.shininess", material.shininess);
}