#include "core/shader.h"
#include "vector"
#include "scene/camera.h"
#include "rendering/Model.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

enum class VertexLayoutType
{
    PositionOnly,
    PositionNormal, 
    PositionTexcoord,
    PositionNormalTex,
};

struct FrameData
{
    std::vector<glm::vec3> lightPos;
    bool flashLightOn = false;
    int bfwidth = 800;
    int bfheight = 600;
};

struct RenderSettings
{
    bool enableLight;
    bool enableAssimp;
};

struct RenderContext
{
    Camera* camera = nullptr;
    Model* assimpModel = nullptr;
};

struct RenderShaders
{
    Shader* lightingShader = nullptr;
    Shader* lightCubeShader = nullptr;
    Shader* singleColorShader = nullptr;
};

struct RawMeshDesc
{
    const float* vertices = nullptr;
    size_t vertexSize = 0;
    VertexLayoutType layoutType = VertexLayoutType::PositionOnly;
};

struct RenderGeometry
{
    RawMeshDesc lightingCube;
    RawMeshDesc cube;
    RawMeshDesc lightCube;
    RawMeshDesc floor;
    RawMeshDesc glass;
    RawMeshDesc grass;
};

struct DrawData
{
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    unsigned int vertexCount = 0;
    GLenum primitive = GL_TRIANGLES;
};

class Renderer
{
public:

Renderer(
    const RenderContext& context,
    const RenderShaders& shaders,
    const RenderGeometry& geometry
)
    : 
        camera(context.camera),
        assimpModel(context.assimpModel),
        lightingShader(shaders.lightingShader),
        lightCubeShader(shaders.lightCubeShader),
        singleColorShader(shaders.singleColorShader)
    {
        glEnable(GL_DEPTH_TEST);
        createRawMesh(
            lightingData,
            geometry.lightingCube
        );

        createRawMesh(
            cubeData,
            geometry.cube
        );

        createRawMesh(
            lightCubeData,
            geometry.lightCube
        );

        createRawMesh(
            floorData,
            geometry.floor
        );

        createRawMesh(
            grassData,
            geometry.grass
        );

        createRawMesh(
            glassData,
            geometry.glass
        );
    };

    void beginFrame()
    {
        clear();
    }

    void drawAssimpModel(const FrameData& frameData, const RenderSettings& settings)
    {
        if (assimpModel == nullptr)
            return;

        glm::mat4 model = glm::mat4(1.0f);

        if (lightingShader == nullptr)
            return;

        drawModel(*lightingShader,
            frameData,
            settings,
            lightingData,
            model
        );
    }

    void drawLight(const FrameData& frameData)
    {
        drawLightCube(frameData, lightCubeData);
    }

~Renderer()
    {
        destroyRawMesh(lightingData);
        destroyRawMesh(lightCubeData);
        destroyRawMesh(floorData);
        destroyRawMesh(noLightFloorData);
        destroyRawMesh(cubeData); 
        destroyRawMesh(grassData);
        destroyRawMesh(glassData);  
    };

private:
    Camera* camera;
    Model* assimpModel;
    Shader* lightingShader;
    Shader* lightCubeShader;
    Shader* singleColorShader;
    DrawData lightingData;
    DrawData cubeData;
    DrawData lightCubeData; 
    DrawData floorData;
    DrawData noLightFloorData;
    DrawData glassData;
    DrawData grassData;

    // 给VAO,VBO提供stride，提供给createRawMesh使用
    int getStride(VertexLayoutType layout)
    {
        switch (layout)
        {
        case VertexLayoutType::PositionOnly:
            return 3;
        case VertexLayoutType::PositionNormal:
            return 6;
        case VertexLayoutType::PositionTexcoord:
            return 5;
        case VertexLayoutType::PositionNormalTex:
            return 8;
        default:
            return 0;
        }
    }

    // 根据传入的type,为当前绑定的VAO设置顶点属性指针
    void setupVertexLayout(VertexLayoutType type)
    {
        switch (type)
        {
            case VertexLayoutType::PositionOnly:
            {
                int stride = 3 * sizeof(float);

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
                glEnableVertexAttribArray(0);
                break;
            }

            case VertexLayoutType::PositionNormal:
            {
                int stride = 6 * sizeof(float);

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
                glEnableVertexAttribArray(0);

                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
                glEnableVertexAttribArray(1);
                break;
            }

            case VertexLayoutType::PositionTexcoord:
            {
                int stride = 5 * sizeof(float);

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
                glEnableVertexAttribArray(0);

                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
                glEnableVertexAttribArray(2);
                break;
            }

            case VertexLayoutType::PositionNormalTex:
            {
                int stride = 8 * sizeof(float);

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
                glEnableVertexAttribArray(0);

                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
                glEnableVertexAttribArray(1);

                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
                glEnableVertexAttribArray(2);
                break;
            }
        }
    }

    // 销毁VAO,VBO,EBO，提供给析构函数使用
    void destroyRawMesh(DrawData& data)
    {
        if (data.VAO != 0)
            glDeleteVertexArrays(1, &data.VAO);
        if (data.VBO != 0)
            glDeleteBuffers(1, &data.VBO);
        if (data.EBO != 0)
            glDeleteBuffers(1, &data.EBO);
        data.VAO = 0;
        data.VBO = 0;
        data.EBO = 0;
    }

    // 根据DrawData中的信息创建VAO,VBO,EBO，并设置顶点属性指针，提供给构造函数使用。
    // 这里的size由外部传入：sizeof(xxx vertices)
    void createRawMesh(DrawData& target, const RawMeshDesc& desc)
    {
        const float* vertices = desc.vertices;
        size_t size = desc.vertexSize;
        VertexLayoutType type = desc.layoutType;

        if (vertices == nullptr || size == 0)
        {
            target.VAO = 0;
            target.VBO = 0;
            target.EBO = 0;
            target.vertexCount = 0;
            return;
        }
        
        glGenVertexArrays(1, &target.VAO);
        glGenBuffers(1, &target.VBO);

        glBindVertexArray(target.VAO);

        glBindBuffer(GL_ARRAY_BUFFER, target.VBO);
        glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

        setupVertexLayout(type);

        int stride = getStride(type);
        target.vertexCount = size / (stride * sizeof(float));

        glBindVertexArray(0);
    }

    void clear()
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    // 设置projection和view矩阵，提供给drawModel使用
    void setupCameraData(Shader &shader,const FrameData& frameData)
    {
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)frameData.bfwidth / (float)frameData.bfheight, 0.1f, 100.0f);
        glm::mat4 view = camera -> GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
    }

    // 设置viewPos，提供给drawModel使用
    void setupViewPos(Shader &shader)
    {
        glm::vec3 viewPos = camera -> Getposition();
        shader.setVec3("viewPos", viewPos);
    }

    // 设置model矩阵和normalMatrix，提供给drawModel使用
    void setupModel(Shader &shader, const glm::mat4& model, const RenderSettings& settings)
    {
        shader.setMat4("model", model);
        if (settings.enableLight)
        {
            shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        }
    }

    // 设置光照相关的uniform，提供给drawModel使用
    void setupLightData(Shader& shader,
                    const FrameData& frameData,
                    const RenderSettings& settings)
    {
        if (!settings.enableLight)
            return;

        // point lights
        for (int i = 0; i < frameData.lightPos.size(); i++)
        {
            std::string index = std::to_string(i);

            shader.setVec3("pointLights[" + index + "].position", frameData.lightPos[i]);

            shader.setVec3("pointLights[" + index + "].ambient",  glm::vec3(0.1f, 0.1f, 0.1f));
            shader.setVec3("pointLights[" + index + "].diffuse",  glm::vec3(0.1f, 0.1f, 0.1f));
            shader.setVec3("pointLights[" + index + "].specular", glm::vec3(1.0f, 1.0f, 1.0f));

            shader.setFloat("pointLights[" + index + "].constant",  1.0f);
            shader.setFloat("pointLights[" + index + "].linear",    0.09f);
            shader.setFloat("pointLights[" + index + "].quadratic", 0.032f);
        }

        // flashlight
        shader.setVec3("flashlight.position", camera->Getposition());
        shader.setVec3("flashlight.direction", camera->GetFront());

        shader.setVec3("flashlight.diffuse",  glm::vec3(0.5f, 0.5f, 0.5f));
        shader.setVec3("flashlight.specular", glm::vec3(1.0f, 1.0f, 1.0f));

        shader.setFloat("flashlight.constant",  1.0f);
        shader.setFloat("flashlight.linear",    0.09f);
        shader.setFloat("flashlight.quadratic", 0.032f);

        shader.setFloat("flashlight.cutOff", glm::cos(glm::radians(12.5f)));
        shader.setFloat("flashlight.outerCutOff", glm::cos(glm::radians(17.5f)));

        shader.setInt("flashlightOn", frameData.flashLightOn);

        // dir light
        shader.setVec3("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
        shader.setVec3("dirLight.ambient",   glm::vec3(0.1f, 0.1f, 0.1f));
        shader.setVec3("dirLight.diffuse",   glm::vec3(0.1f, 0.1f, 0.1f));
        shader.setVec3("dirLight.specular",  glm::vec3(0.5f, 0.5f, 0.5f));

        // material
        shader.setFloat("material.shininess", 32.0f);
    }

    // 直接根据DrawData中的信息绘制，不使用Assimp加载的模型，提供给drawModel使用
    void drawRaw(const DrawData& drawData)
    {
        glBindVertexArray(drawData.VAO);
        glDrawArrays(drawData.primitive, 0, drawData.vertexCount);
        glBindVertexArray(0);
    }
        
    void drawModel(Shader& shader,
                const FrameData& frameData, 
                const RenderSettings& settings, 
                const DrawData& drawData,
                const glm::mat4 &model
                )
    {
        shader.use();
        setupCameraData(shader, frameData);
        setupModel(shader, model, settings);

        if (settings.enableLight)
        {
            setupLightData(shader, frameData, settings);
            setupViewPos(shader);
        }

        if (settings.enableAssimp)
        {
            assimpModel -> Draw(shader);
        }
        else 
        {
            drawRaw(drawData);
        }
    }

        void drawLightCube(const FrameData& frameData, const DrawData& drawData)
    {
        if (lightCubeShader == nullptr)
            return;

        lightCubeShader->use();
        setupCameraData(*lightCubeShader, frameData);

        for (int i = 0; i < (int)frameData.lightPos.size(); i++)
        {
            glm::mat4 Lightmodel = glm::mat4(1.0f);
            Lightmodel = glm::translate(Lightmodel, frameData.lightPos[i]);
            Lightmodel = glm::scale(Lightmodel, glm::vec3(0.2f)); 
            lightCubeShader->setMat4("model", Lightmodel);
            drawRaw(drawData);
        }
    }

};