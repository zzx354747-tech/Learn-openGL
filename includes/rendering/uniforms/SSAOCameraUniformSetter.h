#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/Shader.h"
#include "scene/Camera.h"

class SSAOCameraUniformSetter
{
public:
    static void apply( Shader& shader, Camera& camera, float width, float height );
};
