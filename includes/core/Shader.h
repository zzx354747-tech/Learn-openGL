#ifndef SHADER_H
#define SHADER_H
#include <glad/gl.h>
#include <string>
//文件流
#include <fstream>
//字符串流
#include <sstream>
#include <iostream>
#include <filesystem>
#include <stdexcept>
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
            vertexCode = loadShaderSource(vertexPath);
            fragmentCode = loadShaderSource(fragmentPath);
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
            vertexCode = loadShaderSource(vertexPath);
            geometryCode = loadShaderSource(geometryPath);
            fragmentCode = loadShaderSource(fragmentPath);
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
    void setVec2(const std::string &name, const glm::vec2 &value) const
    {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
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
    static std::string loadShaderSource(
        const std::filesystem::path& path,
        int includeDepth = 0)
    {
        if (includeDepth > 16)
            throw std::runtime_error("Shader include depth exceeded: " +
                                     path.string());

        std::ifstream file(path);
        if (!file.is_open())
            throw std::ifstream::failure(
                "Unable to open shader source: " + path.string());
        std::ostringstream output;
        std::string line;
        while (std::getline(file, line))
        {
            const std::size_t directive = line.find("#include");
            const std::size_t firstQuote = line.find('"', directive);
            const std::size_t secondQuote = firstQuote == std::string::npos
                ? std::string::npos
                : line.find('"', firstQuote + 1);
            if (directive != std::string::npos &&
                firstQuote != std::string::npos &&
                secondQuote != std::string::npos)
            {
                const std::filesystem::path includePath =
                    path.parent_path() /
                    line.substr(firstQuote + 1,
                                secondQuote - firstQuote - 1);
                output << loadShaderSource(
                    includePath.lexically_normal(), includeDepth + 1);
                continue;
            }
            output << line << '\n';
        }
        return output.str();
    }

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
