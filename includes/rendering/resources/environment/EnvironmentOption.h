#pragma once

#include "rendering/assets/texture/HDRTexture.h"
#include "rendering/resources/render/SceneRenderTypes.h"

struct EnvironmentOption
{
    const char* name;
    const char* path;
    EnvironmentSelection selection;
    HDRLoadOptions loadOptions;
};

inline const EnvironmentOption kEnvironmentOptions[] =
{
    {"Night", "../textures/skybox/night.hdr",
     EnvironmentSelection::Night, HDRLoadOptions{true, 100.0f}},

    {"Sunny", "../textures/skybox/sunny.hdr",
     EnvironmentSelection::Sunny, HDRLoadOptions{true, 100.0f}},

    {"God Rays 07 3K", "../textures/skybox/GodRays_07_3K.hdr",
     EnvironmentSelection::GodRays, HDRLoadOptions{true, 100.0f}},

    {"Night N8 3K", "../textures/skybox/Night_08_3K.hdr",
     EnvironmentSelection::NightN8_3K, HDRLoadOptions{true, 100.0f}},
};

inline int getEnvironmentIndex(EnvironmentSelection selection)
{
    for (int i = 0;
         i < static_cast<int>(
                 sizeof(kEnvironmentOptions) /
                 sizeof(kEnvironmentOptions[0]));
         ++i)
    {
        if (kEnvironmentOptions[i].selection == selection)
        {
            return i;
        }
    }

    return 0;
}
