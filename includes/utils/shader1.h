#ifndef SHADER1_H
#define SHADER1_H
#include <glad/gl.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader1 {
    unsigned int ID1, ID2;
    public:
    Shader1(const char* vertexPath, const char* fragment1Path, const char* fragment2Path)
    {
        std::string vertexCode;
        std::string fragment1Code;
        std::string fragment2Code;
        //创建文件流对象
        std::ifstream vShaderFile;
        std::ifstream fShader1File;
        std::ifstream fShader2File;
        //保证ifstream对象可以抛出异常
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShader1File.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShader2File.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try 
        {
            //打开文件
            vShaderFile.open(vertexPath);
            fShader1File.open(fragment1Path);
            fShader2File.open(fragment2Path);
            //定义数据流对象(用于存储读取的文件内容)
            std::stringstream vShaderStream, fShader1Stream, fShader2Stream;
            //读取文件的缓冲内容到数据流中
            vShaderStream << vShaderFile.rdbuf();
            fShader1Stream << fShader1File.rdbuf();
            fShader2Stream << fShader2File.rdbuf();
            //关闭文件处理器
            vShaderFile.close();
            fShader1File.close();
            fShader2File.close();
            //转换数据流为字符串
            vertexCode = vShaderStream.str();
            fragment1Code = fShader1Stream.str();
            fragment2Code = fShader2Stream.str();
            }
            //捕获异常
            catch (std::ifstream::failure& e)
            {
                std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
            }
        
            //将string转换为char*
            const char* vShaderCode = vertexCode.c_str();
            const char* fShader1Code = fragment1Code.c_str();
            const char* fShader2Code = fragment2Code.c_str();

            //编译着色器
            unsigned int vertex, fragment1, fragment2;
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
            if (!success)            {
                glGetShaderInfoLog(vertex, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
            }
            //创建片段着色器1
            fragment1 = glCreateShader(GL_FRAGMENT_SHADER);
            //将着色器源码附加到着色器对象
            glShaderSource(fragment1, 1, &fShader1Code, NULL);
            //编译着色器
            glCompileShader(fragment1);
            //检查编译是否成功
            glGetShaderiv(fragment1, GL_COMPILE_STATUS, &success);
            if (!success)            {
                glGetShaderInfoLog(fragment1, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::FRAGMENT1::COMPILATION_FAILED\n" << infoLog << std::endl;
            }
            //创建片段着色器2
            fragment2 = glCreateShader(GL_FRAGMENT_SHADER);
            //将着色器源码附加到着色器对象
            glShaderSource(fragment2, 1, &fShader2Code, NULL);
            //编译着色器
            glCompileShader(fragment2);
            //检查编译是否成功
            glGetShaderiv(fragment2, GL_COMPILE_STATUS, &success);
            if (!success)            {
                glGetShaderInfoLog(fragment2, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::FRAGMENT2::COMPILATION_FAILED\n" << infoLog << std::endl;
            }
            //创建着色器程序1
            ID1 = glCreateProgram();
            //将着色器对象附加到程序对象上
            glAttachShader(ID1, vertex);
            glAttachShader(ID1, fragment1);
            glLinkProgram(ID1);
            //检查链接是否成功
            glGetProgramiv(ID1, GL_LINK_STATUS, &success);
            if (!success)            {
                glGetProgramInfoLog(ID1, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::PROGRAM1::LINKING_FAILED\n" << infoLog << std::endl;
            }
            //创建着色器程序2
            ID2 = glCreateProgram();
            //将着色器对象附加到程序对象上
            glAttachShader(ID2, vertex);
            glAttachShader(ID2, fragment2);
            glLinkProgram(ID2);
            //检查链接是否成功
            glGetProgramiv(ID2, GL_LINK_STATUS, &success);
            if (!success)            {
                glGetProgramInfoLog(ID2, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::PROGRAM2::LINKING_FAILED\n" << infoLog << std::endl;
            }
            //删除着色器对象（已链接到程序对象上，不再需要）
            glDeleteShader(vertex);
            glDeleteShader(fragment1);
            glDeleteShader(fragment2);
        }
        //使用/激活程序
        void use1()
        {
            glUseProgram(ID1);
        }
        void use2()
        {
            glUseProgram(ID2);
        }
        //uniform工具函数
        //找到uniform变量位置并传入值
        void setBool1(const std::string &name, bool value) const
        {
            glUniform1i(glGetUniformLocation(ID1, name.c_str()), (int)value);
        }
        void setInt1(const std::string &name, int value) const
        {
            glUniform1i(glGetUniformLocation(ID1, name.c_str()), value);
        }
        void setFloat1(const std::string &name, float value) const
        {
            glUniform1f(glGetUniformLocation(ID1, name.c_str()), value);
        }
        void setMat4_1(const std::string &name, const glm::mat4 &mat) const
        {
            glUniformMatrix4fv(glGetUniformLocation(ID1, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }
        void setBool2(const std::string &name, bool value) const
        {
            glUniform1i(glGetUniformLocation(ID2, name.c_str()), (int)value);
        }
        void setInt2(const std::string &name, int value) const
        {
            glUniform1i(glGetUniformLocation(ID2, name.c_str()), value);
        }
        void setFloat2(const std::string &name, float value) const
        {
            glUniform1f(glGetUniformLocation(ID2, name.c_str()), value);
        }
        void setMat4_2(const std::string &name, const glm::mat4 &mat) const
        {
            glUniformMatrix4fv(glGetUniformLocation(ID2, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }
};

#endif
        
    
