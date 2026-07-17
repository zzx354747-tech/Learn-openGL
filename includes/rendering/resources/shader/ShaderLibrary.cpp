#include "rendering/resources/shader/ShaderLibrary.h"

ShaderLibrary::ShaderLibrary()
    : screen(
        "../src/shader/renderer/postprocess/screen.vs",
        "../src/shader/renderer/postprocess/screen.fs")
    , skyComposite(
        "../src/shader/renderer/postprocess/sky_composite.vs",
        "../src/shader/renderer/postprocess/sky_composite.fs")
    , cloudAcceleration(
        "../src/shader/renderer/postprocess/screen.vs",
        "../src/shader/renderer/postprocess/cloud_acceleration.fs")
    , volumetricLight(
        "../src/shader/renderer/postprocess/screen.vs",
        "../src/shader/renderer/postprocess/volumetric_light.fs")
    , volumetricLightComposite(
        "../src/shader/renderer/postprocess/screen.vs",
        "../src/shader/renderer/postprocess/volumetric_light_composite.fs")
    , taa(
        "../src/shader/renderer/postprocess/taa.vs",
        "../src/shader/renderer/postprocess/taa.fs")
    , cubemap(
        "../src/shader/renderer/forward/reflection.vs",
        "../src/shader/renderer/forward/reflection.fs")
    , shadowDebug(
        "../src/shader/renderer/shadow/debug.vs",
        "../src/shader/renderer/shadow/debug.fs")
    , shadowMap(
        "../src/shader/renderer/shadow/directional.vs",
        "../src/shader/renderer/shadow/directional.fs")
    , cloudOpticalDepth(
        "../src/shader/renderer/postprocess/screen.vs",
        "../src/shader/renderer/shadow/cloud_optical_depth.fs")
    , cloudOpticalDepthBlur(
        "../src/shader/renderer/postprocess/screen.vs",
        "../src/shader/renderer/shadow/cloud_optical_depth_blur.fs")
    , cloudOpticalDepthToTransmittance(
        "../src/shader/renderer/postprocess/screen.vs",
        "../src/shader/renderer/shadow/cloud_optical_depth_to_transmittance.fs")
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
    , vegetationGeometry(
        "../src/shader/renderer/deferred/vegetation.vs",
        "../src/shader/renderer/deferred/vegetation.fs")
    , vegetationShadow(
        "../src/shader/renderer/shadow/vegetation.vs",
        "../src/shader/renderer/shadow/vegetation.fs")
    , basicForward(
        "../src/shader/renderer/forward/unlit.vs",
        "../src/shader/renderer/forward/unlit.fs")
    , water(
        "../src/shader/renderer/forward/water.vs",
        "../src/shader/renderer/forward/water.fs")
    , waterCausticPhotons(
        "../src/shader/renderer/deferred/water_caustic_photons.vs",
        "../src/shader/renderer/deferred/water_caustic_photons.fs")
    , waterCausticBlur(
        "../src/shader/renderer/postprocess/screen.vs",
        "../src/shader/renderer/deferred/water_caustic_blur.fs")
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
