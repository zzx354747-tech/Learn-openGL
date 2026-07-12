#ifndef SHADER_H
#define SHADER_H
#include <glad/gl.h>
#include <string>
//文件流
#include <fstream>
//字符串流
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>

class Shader {
public:
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    //程序ID
    unsigned int ID;
    //构造函数读取并构建着色器（构造函数即调用时就执行）

    Shader(const char* vertexPath, const char* fragmentPath)
    {
        std::string vertexCode;
        std::string fragmentCode;

        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);

            std::stringstream vShaderStream;
            std::stringstream fShaderStream;

            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            vShaderFile.close();
            fShaderFile.close();

            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }

        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        unsigned int vertex;
        unsigned int fragment;

        int success;

        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);

        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            printShaderCompileError(vertex, "VERTEX", vertexPath);
        }

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);

        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            printShaderCompileError(fragment, "FRAGMENT", fragmentPath);
        }

        ID = glCreateProgram();

        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);

        glLinkProgram(ID);

        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success)
        {
            printProgramLinkError(ID, vertexPath, nullptr, fragmentPath);
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    Shader(const char* vertexPath, const char* geometryPath, const char* fragmentPath)
    {
        std::string vertexCode;
        std::string geometryCode;
        std::string fragmentCode;

        std::ifstream vShaderFile;
        std::ifstream gShaderFile;
        std::ifstream fShaderFile;

        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            vShaderFile.open(vertexPath);
            gShaderFile.open(geometryPath);
            fShaderFile.open(fragmentPath);

            std::stringstream vShaderStream, gShaderStream, fShaderStream;

            vShaderStream << vShaderFile.rdbuf();
            gShaderStream << gShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            vShaderFile.close();
            gShaderFile.close();
            fShaderFile.close();

            vertexCode = vShaderStream.str();
            geometryCode = gShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
        }

        const char* vShaderCode = vertexCode.c_str();
        const char* gShaderCode = geometryCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        unsigned int vertex, geometry, fragment;
        int success;

        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            printShaderCompileError(vertex, "VERTEX", vertexPath);
        }

        geometry = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geometry, 1, &gShaderCode, NULL);
        glCompileShader(geometry);
        glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            printShaderCompileError(geometry, "GEOMETRY", geometryPath);
        }

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            printShaderCompileError(fragment, "FRAGMENT", fragmentPath);
        }

        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, geometry);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);

        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success)
        {
            printProgramLinkError(ID, vertexPath, geometryPath, fragmentPath);
        }

        glDeleteShader(vertex);
        glDeleteShader(geometry);
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
    void setMat4(const std::string &name, const glm::mat4 &mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    void setVec3(const std::string &name, const glm::vec3 &value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec4(const std::string &name, const glm::vec4 &value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setMat3(const std::string &name, const glm::mat3 &mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:
    static std::string getShaderInfoLog(unsigned int shader)
    {
        int logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength <= 1)
            return {};

        std::string infoLog(logLength, '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, &infoLog[0]);
        return infoLog;
    }

    static std::string getProgramInfoLog(unsigned int program)
    {
        int logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength <= 1)
            return {};

        std::string infoLog(logLength, '\0');
        glGetProgramInfoLog(program, logLength, nullptr, &infoLog[0]);
        return infoLog;
    }

    static void printShaderCompileError(unsigned int shader, const char* shaderType, const char* path)
    {
        std::cout
            << "ERROR::SHADER::" << shaderType << "::COMPILATION_FAILED\n"
            << "PATH: " << (path ? path : "<unknown>") << "\n"
            << getShaderInfoLog(shader)
            << std::endl;
    }

    static void printProgramLinkError(unsigned int program,
        const char* vertexPath,
        const char* geometryPath,
        const char* fragmentPath)
    {
        std::cout
            << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
            << "VERTEX: " << (vertexPath ? vertexPath : "<unknown>") << "\n";

        if (geometryPath)
            std::cout << "GEOMETRY: " << geometryPath << "\n";

        std::cout
            << "FRAGMENT: " << (fragmentPath ? fragmentPath : "<unknown>") << "\n"
            << getProgramInfoLog(program)
            << std::endl;
    }
};

#endif
