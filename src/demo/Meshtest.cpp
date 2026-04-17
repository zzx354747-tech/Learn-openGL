#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "shader.h"
#include "camera.h"
#include "Mesh.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

Camera camera;
bool cursorLocked = true;
bool gravePressLastFrame = false;
bool flashLightOn = true;
bool flashLightToggleLastFrame = false;
float lastFrame = 0.0f;
float currentFrame = 0.0f;
glm::vec3 lightPos1(1.2f, 1.0f, 2.0f);
glm::vec3 lightPos2(2.0f, 1.0f, 0.0f);
glm::vec3 lightPos3(1.0f, 3.0f, 2.0f);
glm::vec3 lightPos4(1.0f, 0.0f, 2.0f);

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos){
    if (!cursorLocked) return;
    camera.ProcessMouseMovement(static_cast<float>(xpos), static_cast<float>(ypos));
}
void processInput(GLFWwindow* window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);

    bool gravePressCurrentFrame = glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS;
    if (gravePressCurrentFrame && !gravePressLastFrame)
    {
        cursorLocked = !cursorLocked;
        if (cursorLocked)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            camera.Resetmouse();
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    gravePressLastFrame = gravePressCurrentFrame;

    bool flashLightToggleCurrentFrame = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
    if (flashLightToggleCurrentFrame && !flashLightToggleLastFrame)
    {
        flashLightOn = !flashLightOn;
    }
    flashLightToggleLastFrame = flashLightToggleCurrentFrame;
}

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Backpack Demo", nullptr, nullptr);
    if (!window)    
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    float vertices[] = {
    // positions          // normals           // texcoords
    -0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,  0.0f,0.0f,
     0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,  1.0f,0.0f,
     0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,  1.0f,1.0f,
     0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,  1.0f,1.0f,
    -0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,  0.0f,1.0f,
    -0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,  0.0f,0.0f,

    -0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,  0.0f,0.0f,
     0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,  1.0f,0.0f,
     0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,  1.0f,1.0f,
     0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,  1.0f,1.0f,
    -0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,  0.0f,1.0f,
    -0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,  0.0f,0.0f,

    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,  1.0f,0.0f,
    -0.5f, 0.5f,-0.5f, -1.0f, 0.0f, 0.0f,  1.0f,1.0f,
    -0.5f,-0.5f,-0.5f, -1.0f, 0.0f, 0.0f,  0.0f,1.0f,
    -0.5f,-0.5f,-0.5f, -1.0f, 0.0f, 0.0f,  0.0f,1.0f,
    -0.5f,-0.5f, 0.5f, -1.0f, 0.0f, 0.0f,  0.0f,0.0f,
    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,  1.0f,0.0f,

     0.5f, 0.5f, 0.5f,  1.0f, 0.0f, 0.0f,  1.0f,0.0f,
     0.5f, 0.5f,-0.5f,  1.0f, 0.0f, 0.0f,  1.0f,1.0f,
     0.5f,-0.5f,-0.5f,  1.0f, 0.0f, 0.0f,  0.0f,1.0f,
     0.5f,-0.5f,-0.5f,  1.0f, 0.0f, 0.0f,  0.0f,1.0f,
     0.5f,-0.5f, 0.5f,  1.0f, 0.0f, 0.0f,  0.0f,0.0f,
     0.5f, 0.5f, 0.5f,  1.0f, 0.0f, 0.0f,  1.0f,0.0f,

    -0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,  0.0f,1.0f,
     0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,  1.0f,1.0f,
     0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,  1.0f,0.0f,
     0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,  1.0f,0.0f,
    -0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,  0.0f,0.0f,
    -0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,  0.0f,1.0f,

    -0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,  0.0f,1.0f,
     0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,  1.0f,1.0f,
     0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f,  1.0f,0.0f,
     0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f,  1.0f,0.0f,
    -0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f,  0.0f,0.0f,
    -0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,  0.0f,1.0f
};

    glm::vec3 cubePositions[] = {
    glm::vec3( 0.0f,  0.0f,  0.0f), 
    glm::vec3( 2.0f,  5.0f, -15.0f), 
    glm::vec3(-1.5f, -2.2f, -2.5f),  
    glm::vec3(-3.8f, -2.0f, -12.3f),  
    glm::vec3( 2.4f, -0.4f, -3.5f),  
    glm::vec3(-1.7f,  3.0f, -7.5f),  
    glm::vec3( 1.3f, -2.0f, -2.5f),  
    glm::vec3( 1.5f,  2.0f, -2.5f), 
    glm::vec3( 1.5f,  0.2f, -1.5f), 
    glm::vec3(-1.3f,  1.0f, -1.5f)  
};

    unsigned int lightVAO, lightVBO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    glGenBuffers(1, &lightVBO);
    glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned int texture1, texture2;
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unsigned char* data1 = stbi_load("../container2.png", &width, &height, &nrChannels, 0);
    if (data1)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data1);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cerr << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data1);

    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unsigned char* data2 = stbi_load("../c2_specular.png", &width, &height, &nrChannels, 0);
    if (data2)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data2);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cerr << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data2);

    std::vector<Vertex> meshVertices;
    for (int i = 0; i < 36; ++i)
    {
        Vertex v;
        v.Position = glm::vec3(
            vertices[i * 8 + 0],
            vertices[i * 8 + 1],
            vertices[i * 8 + 2]
        );
        v.Normal = glm::vec3(
            vertices[i * 8 + 3],
            vertices[i * 8 + 4],
            vertices[i * 8 + 5]
        );
        v.TexCoords = glm::vec2(
            vertices[i * 8 + 6],
            vertices[i * 8 + 7]
        );
        meshVertices.push_back(v);
    }

    std::vector<unsigned int> meshIndices;
    for (unsigned int i = 0; i < 36; ++i)
    {
    meshIndices.push_back(i);
    }

    std::vector<Texture> meshTextures;

    Texture diffuseTex;
    diffuseTex.id = texture1;
    diffuseTex.type = "texture_diffuse";
    diffuseTex.path = "../container2.png";
    meshTextures.push_back(diffuseTex);

    Texture specularTex;
    specularTex.id = texture2;
    specularTex.type = "texture_specular";
    specularTex.path = "../c2_specular.png";
    meshTextures.push_back(specularTex);

Mesh cubeMesh(meshVertices, meshIndices, meshTextures);

    int bfwidth, bfheight;
    glfwGetFramebufferSize(window, &bfwidth, &bfheight);
    glViewport(0, 0, bfwidth, bfheight);

    Shader lightingShader("../src/shader/backpack/model.vs", "../src/shader/backpack/model.fs");
    Shader lightCubeshader("../src/shader/backpack/lightcube.vs", "../src/shader/backpack/lightcube.fs");
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        float time = glfwGetTime() * 0.2f;
        lightPos1.x = sin(time) * 2.0f;
        lightPos1.y = cos(time) * 2.0f;
        lightPos1.z = 2.0f;

        lightPos2.x = sin(time * 1.3f) * 2.0f;
        lightPos2.y = cos(time * 1.3f) * 2.0f;
        lightPos2.z = 2.0f;

        lightPos3.x = sin(time * 1.7f) * 2.0f;
        lightPos3.y = cos(time * 1.7f) * 2.0f;
        lightPos3.z = 2.0f;

        lightPos4.x = sin(time * 2.0f) * 2.0f;
        lightPos4.y = cos(time * 2.0f) * 2.0f;
        lightPos4.z = 2.0f;

        processInput(window, deltaTime);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)bfwidth / (float)bfheight, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::vec3 viewPos = camera.Getposition();

        lightingShader.use();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);
        lightingShader.setVec3("viewPos", viewPos);
        lightingShader.setVec3("pointLights[0].position", lightPos1);
        lightingShader.setVec3("pointLights[0].ambient", glm::vec3(0.2f, 0.2f, 0.2f));
        lightingShader.setVec3("pointLights[0].diffuse", glm::vec3(0.4f, 0.4f, 0.4f));
        lightingShader.setVec3("pointLights[0].specular", glm::vec3(1.0f, 1.0f, 1.0f));
        lightingShader.setFloat("pointLights[0].constant", 1.0f);
        lightingShader.setFloat("pointLights[0].linear", 0.09f);
        lightingShader.setFloat("pointLights[0].quadratic", 0.032f);

        lightingShader.setVec3("pointLights[1].position", lightPos2);
        lightingShader.setVec3("pointLights[1].ambient", glm::vec3(0.2f, 0.2f, 0.2f));
        lightingShader.setVec3("pointLights[1].diffuse", glm::vec3(0.4f, 0.4f, 0.4f));
        lightingShader.setVec3("pointLights[1].specular", glm::vec3(1.0f, 1.0f, 1.0f));
        lightingShader.setFloat("pointLights[1].constant", 1.0f);
        lightingShader.setFloat("pointLights[1].linear", 0.09f);
        lightingShader.setFloat("pointLights[1].quadratic", 0.032f);

        lightingShader.setVec3("pointLights[2].position", lightPos3);
        lightingShader.setVec3("pointLights[2].ambient", glm::vec3(0.2f, 0.2f, 0.2f));
        lightingShader.setVec3("pointLights[2].diffuse", glm::vec3(0.4f, 0.4f, 0.4f));
        lightingShader.setVec3("pointLights[2].specular", glm::vec3(1.0f, 1.0f, 1.0f));
        lightingShader.setFloat("pointLights[2].constant", 1.0f);
        lightingShader.setFloat("pointLights[2].linear", 0.09f);
        lightingShader.setFloat("pointLights[2].quadratic", 0.032f);

        lightingShader.setVec3("pointLights[3].position", lightPos4);
        lightingShader.setVec3("pointLights[3].ambient", glm::vec3(0.2f, 0.2f, 0.2f));
        lightingShader.setVec3("pointLights[3].diffuse", glm::vec3(0.4f, 0.4f, 0.4f));
        lightingShader.setVec3("pointLights[3].specular", glm::vec3(1.0f, 1.0f, 1.0f));
        lightingShader.setFloat("pointLights[3].constant", 1.0f);
        lightingShader.setFloat("pointLights[3].linear", 0.09f);
        lightingShader.setFloat("pointLights[3].quadratic", 0.032f);

        lightingShader.setVec3("flashlight.position", camera.Getposition());
        lightingShader.setVec3("flashlight.direction", camera.GetFront());
        lightingShader.setVec3("flashlight.diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
        lightingShader.setVec3("flashlight.specular", glm::vec3(1.0f, 1.0f, 1.0f));
        lightingShader.setFloat("flashlight.constant", 1.0f);
        lightingShader.setFloat("flashlight.linear", 0.09f);
        lightingShader.setFloat("flashlight.quadratic", 0.032f);
        lightingShader.setInt("flashlightOn", flashLightOn);

        lightingShader.setVec3("dirLight.ambient", glm::vec3(0.2f, 0.2f, 0.2f));
        lightingShader.setVec3("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
        lightingShader.setVec3("dirLight.diffuse", glm::vec3(0.4f, 0.4f, 0.4f));
        lightingShader.setVec3("dirLight.specular", glm::vec3(0.5f, 0.5f, 0.5f));

        lightingShader.setFloat("material.shininess", 32.0f);

        for (unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            lightingShader.setMat4("model", model);
            lightingShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
            cubeMesh.Draw(lightingShader);
        };

        lightCubeshader.use();
        lightCubeshader.setMat4("projection", projection);
        lightCubeshader.setMat4("view", view);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos1);
        model = glm::scale(model, glm::vec3(0.2f)); 
        lightCubeshader.setMat4("model", model);
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glm::mat4 model2 = glm::mat4(1.0f);
        model2 = glm::translate(model2, lightPos2);
        model2 = glm::scale(model2, glm::vec3(0.2f)); 
        lightCubeshader.setMat4("model", model2);
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glm::mat4 model3 = glm::mat4(1.0f);
        model3 = glm::translate(model3, lightPos3);
        model3 = glm::scale(model3, glm::vec3(0.2f)); 
        lightCubeshader.setMat4("model", model3);
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glm::mat4 model4 = glm::mat4(1.0f);
        model4 = glm::translate(model4, lightPos4);
        model4 = glm::scale(model4, glm::vec3(0.2f)); 
        lightCubeshader.setMat4("model", model4);
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &lightVAO); 
    glDeleteBuffers(1, &lightVBO);
    glDeleteTextures(1, &texture1);
    glDeleteTextures(1, &texture2);
    glfwTerminate();
    return 0;
}
        
     
        



    