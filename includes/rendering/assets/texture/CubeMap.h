#pragma once
#include <glad/gl.h>
#include <string>
#include <vector>
#include "stb_image.h"
#include <iostream>

class CubeMap
{
public:
    CubeMap(const std::vector<std::string>& faces);

    void bind(unsigned int unit = 0) const;

    void unbind() const;

    ~CubeMap();

    CubeMap(const CubeMap&) = delete;
    CubeMap& operator=(const CubeMap&) = delete;

private:
    unsigned int id = 0;
    int width = 0;
    int height = 0;
    int nrChannels = 0;

    void initSkyboxTex();

    void initSkyboxData(const std::vector<std::string>& skyboxFaces);
};
    
