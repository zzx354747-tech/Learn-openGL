#pragma once
#include "stb_image.h"
#include <string>
#include <glad/gl.h>
#include <iostream>

class GLTexture
{
public:
    GLTexture(const std::string& path);

    GLTexture(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);

    ~GLTexture();

    void bind(unsigned int unit = 0) const;

private:
    unsigned int id;
    int width, height, nrChannels;
    std::string path;

    void initTex();

    void initData(const std::string& path);


};
