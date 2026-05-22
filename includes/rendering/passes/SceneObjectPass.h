#pragma once
#include "core/Shader.h"  

class SceneObjectPass
{
public:
    void renderCube();
    void renderPlane();
    void renderModel();

private:
    void setupCubeUniforms();
    void setupPlaneUniforms();  
    void setupModelUniforms();

    void bindCubeTexture();
    void bindPlaneTexture();

    Shader* getCubeShader();
    Shader* getPlaneShader();
    Shader* getModelShader();

};
    
