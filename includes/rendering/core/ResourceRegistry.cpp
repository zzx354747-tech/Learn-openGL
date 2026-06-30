#include "rendering/core/ResourceRegistry.h"

ResourceHandle ResourceRegistry::declareTexture(const std::string& name)
{
        // 发起哈希表查找，返回一个迭代器
        // 你可以用it->first访问键，用it->second访问值
        // it 为std::unordered_map<std::string, ResourceHandle>::iterator类型
        auto it = nameToHandle.find(name);

        // 如果找到了，就返回对应的ResourceHandle
        if (it != nameToHandle.end())
        {
            return it->second;
        }

        // 将texture.size()转换为uint32_t类型，并且把它当作纹理ID
        ResourceHandle handle = static_cast<ResourceHandle>(textures.size());

        // 键值配对
        nameToHandle[name] = handle;
        // 目前还没有加载纹理，所以先把纹理ID设为0,用来占位
        // 数组只存纹理ID
        textures.push_back(0);

        // 给外部对象赋值ID
        return handle;
    }

void ResourceRegistry::setTexture(ResourceHandle handle, GLuint textureID)
{
        // 如果传入的句柄超过了textures的大小，说明这个句柄是无效的，抛出异常
        if (handle >= textures.size())
        {
            throw std::runtime_error("ResourceRegistry::setTexture invalid handle");
        }

        // 如果句柄有效，就把对应的纹理ID存入textures数组中
        // 把已经括容的数组进行赋值
        // 由于令handle为textures.size,所以handle就可以代表texture纹理ID索引
        textures[handle] = textureID;
    }

GLuint ResourceRegistry::resolveTexture(ResourceHandle handle) const
{
        // 同样检测句柄是否有效，如果无效就抛出异常
        if (handle >= textures.size())
        {
            throw std::runtime_error("ResourceRegistry::resolveTexture invalid handle");
        }

        return textures[handle];
    }
