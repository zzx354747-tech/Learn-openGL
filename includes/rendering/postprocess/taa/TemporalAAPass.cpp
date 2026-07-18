#include "rendering/postprocess/taa/TemporalAAPass.h"

#include <glm/gtc/matrix_transform.hpp>

#include "rendering/uniforms/TemporalJitter.h"

TemporalAAPass::TemporalAAPass(int width, int height, Camera& camera,
                               SceneRenderConfig& config)
    : camera_(camera), config_(config), history_(width, height)
{}

float TemporalAAPass::halton(unsigned int index, unsigned int base)
{
    float result = 0.0f;
    float fraction = 1.0f;
    while (index > 0)
    {
        fraction /= static_cast<float>(base);
        result += fraction * static_cast<float>(index % base);
        index /= base;
    }
    return result;
}

void TemporalAAPass::beginFrame(int width, int height)
{
    const bool stateChanged = stateInitialized_ &&
        (previousTAAEnabled_ != config_.enableTAA ||
         previousRenderMode_ != config_.renderMode);
    if (stateChanged)
        historyValid_ = false;
    previousTAAEnabled_ = config_.enableTAA;
    previousRenderMode_ = config_.renderMode;
    stateInitialized_ = true;

    glm::vec2 jitter(0.0f);
    if (config_.enableTAA)
    {
        const unsigned int sample = (frameIndex_ % 16u) + 1u;
        jitter = glm::vec2(halton(sample, 2u), halton(sample, 3u)) - 0.5f;
    }
    TemporalJitter::set(jitter, width, height);

    glm::mat4 projection = glm::perspective(
        glm::radians(Camera::DefaultFieldOfViewDegrees),
        static_cast<float>(width) / static_cast<float>(height),
        0.1f,
        20000.0f);
    const glm::mat4 view = camera_.GetViewMatrix();
    // History is stored on the stable, unjittered output grid. Render passes
    // still consume TemporalJitter, but reprojection matrices must describe
    // that stable grid or low-confidence pixels expose the camera jitter.
    currentViewProjection_ = projection * view;
    currentSkyViewProjection_ = projection * glm::mat4(glm::mat3(view));
}

GLuint TemporalAAPass::resolve(int width, int height, Framebuffer& source,
                               Screenquad& screenQuad, Shader& shader,
                               GLuint currentPositionTexture,
                               GLuint currentVelocityTexture,
                               GLuint currentCoverageReactiveTexture)
{
    history_.bindWrite(writeIndex_);
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    shader.use();
    shader.setInt("currentColor", 0);
    shader.setInt("historyColor", 1);
    shader.setInt("currentDepth", 2);
    shader.setInt("currentPosition", 3);
    shader.setInt("currentVelocity", 4);
    shader.setInt("currentCoverageReactive", 5);
    shader.setBool("hasCurrentPosition", currentPositionTexture != 0);
    shader.setBool("hasCurrentVelocity", currentVelocityTexture != 0);
    shader.setBool("hasCoverageReactive", currentCoverageReactiveTexture != 0);
    shader.setBool("historyValid", historyValid_);
    shader.setFloat("historyWeight", config_.taaHistoryWeight);
    shader.setFloat("sharpness", config_.taaSharpness);
    shader.setVec2("invResolution", glm::vec2(1.0f / width, 1.0f / height));
    shader.setVec2("currentJitterPixels", TemporalJitter::get());
    shader.setMat4("inverseCurrentViewProjection", glm::inverse(currentViewProjection_));
    shader.setMat4("previousViewProjection", previousViewProjection_);
    shader.setMat4("inverseCurrentSkyViewProjection",
                   glm::inverse(currentSkyViewProjection_));
    shader.setMat4("previousSkyViewProjection", previousSkyViewProjection_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, source.getTextureID(0));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, history_.getTextureID(1 - writeIndex_));
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, source.getDepthTextureID());
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, currentPositionTexture);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, currentVelocityTexture);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, currentCoverageReactiveTexture);
    screenQuad.draw();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    const GLuint resolved = history_.getTextureID(writeIndex_);
    writeIndex_ = 1 - writeIndex_;
    previousViewProjection_ = currentViewProjection_;
    previousSkyViewProjection_ = currentSkyViewProjection_;
    historyValid_ = true;
    ++frameIndex_;
    return resolved;
}

void TemporalAAPass::resize(int width, int height)
{
    history_.resize(width, height);
    historyValid_ = false;
    writeIndex_ = 0;
    frameIndex_ = 0;
    TemporalJitter::set(glm::vec2(0.0f), width, height);
}
