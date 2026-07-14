#include "rendering/resources/shader/ShaderLibrary.h"

ShaderLibrary::ShaderLibrary()
    : screen(
        "../src/shader/renderer/postprocess/screen.vs",
        "../src/shader/renderer/postprocess/screen.fs")
    , cubemap(
        "../src/shader/renderer/forward/reflection.vs",
        "../src/shader/renderer/forward/reflection.fs")
    , shadowDebug(
        "../src/shader/renderer/shadow/debug.vs",
        "../src/shader/renderer/shadow/debug.fs")
    , shadowMap(
        "../src/shader/renderer/shadow/directional.vs",
        "../src/shader/renderer/shadow/directional.fs")
    , pointShadowMap(
        "../src/shader/renderer/shadow/point.vs",
        "../src/shader/renderer/shadow/point.gs",
        "../src/shader/renderer/shadow/point.fs")
    , lightCube(
        "../src/shader/pratice/scenerender/light_cube.vs",
        "../src/shader/pratice/scenerender/light_cube.fs")
    , envCubemap(
        "../src/shader/renderer/ibl/env_cubemap.vs",
        "../src/shader/renderer/ibl/env_cubemap.fs")
    , irradiance(
        "../src/shader/renderer/ibl/env_cubemap.vs",
        "../src/shader/renderer/ibl/irradiance.fs")
    , prefilter(
        "../src/shader/renderer/ibl/env_cubemap.vs",
        "../src/shader/renderer/ibl/prefilter.fs")
    , brdf(
        "../src/shader/renderer/ibl/brdf.vs",
        "../src/shader/renderer/ibl/brdf.fs")
    , blur(
        "../src/shader/renderer/postprocess/blur.vs",
        "../src/shader/renderer/postprocess/blur.fs")
    , geometryPBR(
        "../src/shader/renderer/deferred/geometry.vs",
        "../src/shader/renderer/deferred/geometry.fs")
    , basicForward(
        "../src/shader/renderer/forward/unlit.vs",
        "../src/shader/renderer/forward/unlit.fs")
    , lightingPass(
        "../src/shader/renderer/deferred/lighting.vs",
        "../src/shader/renderer/deferred/lighting.fs")
    , ssao(
        "../src/shader/renderer/ssao/common.vs",
        "../src/shader/renderer/ssao/ssao.fs")
    , ssaoBlur(
        "../src/shader/renderer/ssao/common.vs",
        "../src/shader/renderer/ssao/ssao_bilateral_blur.fs")
{}
