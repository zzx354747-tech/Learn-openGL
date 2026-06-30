#pragma once
#include <glad/gl.h>

class Screenquad
{
public:
    Screenquad();

    void draw();

    ~Screenquad();

private:
    unsigned int VAO, VBO;
};
