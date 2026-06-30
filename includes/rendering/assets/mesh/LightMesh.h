#pragma once
#include <glad/gl.h>

class LightMesh
{
public:
    LightMesh();

    void draw() const;

    ~LightMesh();

    LightMesh(const LightMesh&) = delete;
    LightMesh& operator=(const LightMesh&) = delete;
    private:
        unsigned int lightCubeVAO = 0, lightCubeVBO = 0;
};
