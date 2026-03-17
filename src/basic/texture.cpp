#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main(){
    //加载纹理图像
    int width,height,nrChannels;
    unsigned char *data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);
    //创建纹理对象
    unsigned int texture;
    glGenTextures(1, &texture);
    //绑定纹理对象
    glBindTexture(GL_TEXTURE_2D, texture);
    //设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //生成纹理
    if (data)    {
        //纹理图像加载成功，生成纹理
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        //生成纹理的mipmap(多级渐远纹理)
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else    {
        std::cout << "Failed to load texture" << std::endl;
    }
    //释放图像数据
    stbi_image_free(data);
    return 0;
}