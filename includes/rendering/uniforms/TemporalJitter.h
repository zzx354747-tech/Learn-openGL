#pragma once

#include <glm/glm.hpp>

namespace TemporalJitter
{
    inline glm::vec2 currentPixels(0.0f);
    inline glm::ivec2 referenceResolution(0);

    inline void set(const glm::vec2& jitterPixels, int width = 0, int height = 0)
    {
        currentPixels = jitterPixels;
        if (width > 0 && height > 0)
            referenceResolution = glm::ivec2(width, height);
    }

    inline glm::vec2 get()
    {
        return currentPixels;
    }

    inline glm::mat4 apply(glm::mat4 projection, int width, int height)
    {
        // Jitter is expressed in final-output pixels. Reduced-resolution
        // passes must use the same normalized offset as the full-resolution
        // geometry/TAA passes, otherwise their image moves by a different
        // amount after upsampling.
        const int referenceWidth = referenceResolution.x > 0
            ? referenceResolution.x
            : width;
        const int referenceHeight = referenceResolution.y > 0
            ? referenceResolution.y
            : height;
        if (referenceWidth > 0 && referenceHeight > 0)
        {
            projection[2][0] += (2.0f * currentPixels.x) /
                                static_cast<float>(referenceWidth);
            projection[2][1] += (2.0f * currentPixels.y) /
                                static_cast<float>(referenceHeight);
        }
        return projection;
    }
}
