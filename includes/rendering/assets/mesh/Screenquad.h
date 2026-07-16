#pragma once
#include <glad/gl.h>

class Screenquad
{
public:
    Screenquad();

    void draw();
    void drawTriangle();

    ~Screenquad();

private:
    unsigned int VAO, VBO;
    unsigned int triangleVAO, triangleVBO;
};
