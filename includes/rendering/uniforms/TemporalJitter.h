#pragma once

#include <glm/glm.hpp>

namespace TemporalJitter
{
    inline glm::vec2 currentPixels(0.0f);

    inline void set(const glm::vec2& jitterPixels)
    {
        currentPixels = jitterPixels;
    }

    inline glm::vec2 get()
    {
        return currentPixels;
    }

    inline glm::mat4 apply(glm::mat4 projection, int width, int height)
    {
        if (width > 0 && height > 0)
        {
            projection[2][0] += (2.0f * currentPixels.x) / static_cast<float>(width);
            projection[2][1] += (2.0f * currentPixels.y) / static_cast<float>(height);
        }
        return projection;
    }
}
