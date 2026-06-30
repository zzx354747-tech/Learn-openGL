#pragma once
#include <glad/gl.h>

class PlaneMesh
{
public:
    PlaneMesh();

    void draw() const;

    ~PlaneMesh();

    PlaneMesh(const PlaneMesh&) = delete;
    PlaneMesh& operator=(const PlaneMesh&) = delete;

    private:
        unsigned int planeVAO = 0, planeVBO = 0;
};
