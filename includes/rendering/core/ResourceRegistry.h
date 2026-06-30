#pragma once
// 纹理资源注册表

#include <glad/gl.h>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

using ResourceHandle = uint32_t;

class ResourceRegistry
{
public:
    // 此函数负责查和存纹理资源
    ResourceHandle declareTexture(const std::string& name);

    void setTexture(ResourceHandle handle, GLuint textureID);

    GLuint resolveTexture(ResourceHandle handle) const;

private:
    std::unordered_map<std::string, ResourceHandle> nameToHandle;
    std::vector<GLuint> textures;
};
