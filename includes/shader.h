#ifndef SHADER_H
#define SHADER_H
#include <glad/gl.h>
#include <string>
//文件流
#include <fstream>
//字符串流
#include <sstream>
#include <iostream>

class Shader {
public:
    //程序ID
    unsigned int ID;
    //构造函数读取并构建着色器（构造函数即调用时就执行）
    Shader(const char* vertexPath, const char* fragmentPath)
    {
        std::string vertexCode;
        std::string fragmentCode;
        //创建文件流对象（用于读取文件）
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        //保证ifstream对象可以抛出异常
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try 
        {
            //打开文件
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            //定义数据流对象（用于存储读取的文件内容）
            std::stringstream vShaderstream, fShaderstream;
            //读取文件的缓冲内容到数据流中
            vShaderstream << vShaderFile.rdbuf();
            fShaderstream << fShaderFile.rdbuf();
            //关闭文件处理器
            vShaderFile.close();
            fShaderFile.close();
            //转换数据流为字符串
            vertexCode = vShaderstream.str();
            fragmentCode = fShaderstream.str();
        }
        //捕获异常
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }
            //将string转换为char*
            const char* vShaderCode = vertexCode.c_str();
            const char* fShaderCode = fragmentCode.c_str();
        //编译着色器
        unsigned int vertex, fragment;
        int success;
        char infoLog[512];
        //创建顶点着色器
        vertex = glCreateShader(GL_VERTEX_SHADER);
        //将着色器源码附加到着色器对象
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        //编译着色器
        glCompileShader(vertex);
        //检查编译是否成功
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success)        {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        //创建片段着色器
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        //将着色器源码附加到着色器对象
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        //编译着色器
        glCompileShader(fragment);
        //检查编译是否成功
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success)        {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        //创建程序对象
        ID = glCreateProgram();
        //将着色器对象附加到程序对象上
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        //链接程序对象
        glLinkProgram(ID);
        //检查链接是否成功
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success)        {
            glGetProgramInfoLog(ID, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }
        //删除着色器对象（已链接到程序对象上，不再需要）
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }
    
    //使用/激活程序
    void use()
    { 
        glUseProgram(ID);
    }
    //uniform工具函数
    //找到uniform变量位置并传入值
    void setBool(const std::string &name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    void setInt(const std::string &name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    void setFloat(const std::string &name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
};

#endif