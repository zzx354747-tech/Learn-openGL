#pragma once
#include <glad/gl.h>

class SkyboxMesh
{
public:
    SkyboxMesh();

    void draw() const;

    ~SkyboxMesh();

    SkyboxMesh(const SkyboxMesh&) = delete;
    SkyboxMesh& operator=(const SkyboxMesh&) = delete;

    private:
        unsigned int skyboxVAO = 0, skyboxVBO = 0;
};
