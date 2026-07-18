"""Rebuild the game crocus and bake the real Botaniq runtime material to it.

The source asset and the user's open Blender session are never modified.

Run:
  blender --background --python tools/rebuild_game_flower_crocus.py -- \
      SOURCE_ASSET_DIRECTORY OUTPUT_DIRECTORY
"""

from __future__ import annotations

import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector


TOOLS_DIR = Path(__file__).resolve().parent
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

import build_game_botaniq_batch as game_batch


ASSET_NAME = "flower_crocus"
DIMENSIONS = (0.0552, 0.0434, 0.1317)
LOD_SPECS = {
    "lod0": {
        "petals": 6, "leaves": 8, "stamens": 3,
        "petal_sides": 10, "petal_rings": 5, "leaf_segments": 4,
        "coverage_scale": 1.0,
    },
    "lod1": {
        "petals": 6, "leaves": 6, "stamens": 2,
        "petal_sides": 8, "petal_rings": 4, "leaf_segments": 3,
        "coverage_scale": 1.06,
    },
    "lod2": {
        "petals": 4, "leaves": 4, "stamens": 1,
        "petal_sides": 7, "petal_rings": 3, "leaf_segments": 3,
        "coverage_scale": 1.18,
    },
    "shadow": {
        "petals": 4, "leaves": 3, "stamens": 0,
        "petal_sides": 6, "petal_rings": 3, "leaf_segments": 2,
        "coverage_scale": 1.24,
    },
}

# Safe interior regions of the real Botaniq crocus texture.  UV V is
# bottom-origin here.  Avoiding transparent borders makes the authored solid
# geometry remain opaque while retaining the source colour/normal variation.
SOURCE_UV_RECTS = {
    0: (0.825, 0.055, 0.955, 0.950),  # green-white crocus leaf
    1: (0.300, 0.390, 0.665, 0.950),  # purple-white main petal
    2: (0.465, 0.055, 0.690, 0.430),  # yellow-orange stamen
}


def load_image(path, *, non_color=False):
    image = bpy.data.images.load(str(path.resolve()), check_existing=True)
    if non_color:
        image.colorspace_settings.name = "Non-Color"
    return image


def make_source_material(name, texture_dir):
    material = bpy.data.materials.new(name=name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    nodes.clear()
    links = material.node_tree.links

    output = nodes.new("ShaderNodeOutputMaterial")
    shader = nodes.new("ShaderNodeBsdfPrincipled")
    uv_map = nodes.new("ShaderNodeUVMap")
    uv_map.uv_map = "SourceUV"
    base_node = nodes.new("ShaderNodeTexImage")
    normal_node = nodes.new("ShaderNodeTexImage")
    roughness_node = nodes.new("ShaderNodeTexImage")
    normal_map = nodes.new("ShaderNodeNormalMap")

    base_node.image = load_image(texture_dir / "basecolor.png")
    normal_node.image = load_image(texture_dir / "normal.png", non_color=True)
    roughness_node.image = load_image(
        texture_dir / "roughness.png", non_color=True)
    base_node.extension = "EXTEND"
    normal_node.extension = "EXTEND"
    roughness_node.extension = "EXTEND"

    links.new(uv_map.outputs["UV"], base_node.inputs["Vector"])
    links.new(uv_map.outputs["UV"], normal_node.inputs["Vector"])
    links.new(uv_map.outputs["UV"], roughness_node.inputs["Vector"])
    links.new(base_node.outputs["Color"], shader.inputs["Base Color"])
    links.new(normal_node.outputs["Color"], normal_map.inputs["Color"])
    links.new(normal_map.outputs["Normal"], shader.inputs["Normal"])
    links.new(roughness_node.outputs["Color"], shader.inputs["Roughness"])
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    material.diffuse_color = (0.65, 0.48, 0.70, 1.0)
    return material


def build_crocus(spec):
    builder = game_batch.MeshBuilder()
    width, depth, height = DIMENSIONS
    coverage = spec["coverage_scale"]

    leaf_count = spec["leaves"]
    for index in range(leaf_count):
        angle = math.tau * index / leaf_count + (index % 2) * 0.13
        radial = width * (0.055 + 0.035 * (index % 3))
        base = Vector((
            math.cos(angle) * radial,
            math.sin(angle) * radial * depth / width,
            0.0,
        ))
        builder.blade(
            base,
            angle + (-0.24 if index % 2 else 0.24),
            height * (0.70 + 0.035 * (index % 4)),
            width * (0.085 + (1.0 - spec["leaves"] / 8.0) * 0.035),
            width * (0.10 + 0.035 * (index % 3)),
            spec["leaf_segments"],
            0.30,
            0,
        )

    petal_count = spec["petals"]
    petal_base_z = height * 0.34
    petal_center_z = height * 0.68
    builder.frustum(
        (0.0, 0.0, height * 0.08),
        (0.0, 0.0, petal_base_z),
        width * 0.040,
        width * 0.030,
        7 if petal_count > 4 else 6,
        0,
    )

    for index in range(petal_count):
        angle = math.tau * index / petal_count + (index % 2) * 0.12
        inner = index % 2 == 1
        outward_tilt = 0.17 if inner else 0.27
        axis = Vector((
            math.cos(angle) * outward_tilt,
            math.sin(angle) * outward_tilt,
            1.0,
        )).normalized()
        radial = width * (0.040 if inner else 0.075) * coverage
        center = Vector((
            math.cos(angle) * radial,
            math.sin(angle) * radial * depth / width,
            petal_center_z + (height * 0.012 if inner else 0.0),
        ))
        builder.ellipsoid(
            center,
            axis,
            height * (0.61 if inner else 0.65),
            width * (0.340 if inner else 0.380) * coverage,
            spec["petal_sides"],
            spec["petal_rings"],
            1,
        )

    for index in range(spec["stamens"]):
        angle = math.tau * index / max(1, spec["stamens"]) + 0.25
        center = Vector((
            math.cos(angle) * width * 0.040,
            math.sin(angle) * depth * 0.040,
            height * 0.74,
        ))
        axis = Vector((
            math.cos(angle) * 0.16,
            math.sin(angle) * 0.16,
            1.0,
        )).normalized()
        builder.ellipsoid(
            center,
            axis,
            height * 0.25,
            width * (0.050 + (1.0 - spec["stamens"] / 3.0) * 0.020),
            7 if spec["stamens"] > 1 else 6,
            3,
            2,
        )
    return builder


def add_uv_layers(obj, builder):
    source_uv = obj.data.uv_layers.get("UVMap")
    source_uv.name = "SourceUV"
    bake_uv = obj.data.uv_layers.new(name="BakeUV")

    for polygon in obj.data.polygons:
        rect = SOURCE_UV_RECTS[polygon.material_index]
        u0, v0, u1, v1 = rect
        for loop_index in polygon.loop_indices:
            vertex_index = obj.data.loops[loop_index].vertex_index
            generic_u, generic_v = builder.uvs[vertex_index]
            source_uv.data[loop_index].uv = (
                u0 + (u1 - u0) * generic_u,
                v0 + (v1 - v0) * generic_v,
            )
            bake_uv.data[loop_index].uv = (generic_u, generic_v)

    obj.data.uv_layers.active = bake_uv
    bake_uv.active_render = True
    return source_uv, bake_uv


def create_target_images(output_dir, material_names, pass_name, resolution):
    images = []
    for material_name in material_names:
        slug = material_name.lower().replace(" ", "_").replace("-", "_")
        image = bpy.data.images.new(
            f"{slug}_{pass_name}",
            width=resolution,
            height=resolution,
            alpha=False,
            float_buffer=False,
        )
        image.generated_color = (
            (0.5, 0.5, 1.0, 1.0) if pass_name == "normal"
            else (0.35, 0.35, 0.35, 1.0) if pass_name == "roughness"
            else (0.0, 0.0, 0.0, 1.0)
        )
        image.file_format = "PNG"
        image.filepath_raw = str(
            output_dir / "textures" / f"{slug}_{pass_name}.png")
        images.append(image)
    return images


def bake_pass(obj, materials, images, bake_type, pass_name):
    target_nodes = []
    for material, image in zip(materials, images):
        nodes = material.node_tree.nodes
        for node in nodes:
            node.select = False
        target = nodes.new("ShaderNodeTexImage")
        target.name = f"BAKE_TARGET_{pass_name}"
        target.image = image
        target.select = True
        nodes.active = target
        target_nodes.append(target)

    bpy.ops.object.select_all(action="DESELECT")
    obj.hide_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    if bake_type == "DIFFUSE":
        bpy.ops.object.bake(
            type="DIFFUSE",
            pass_filter={"COLOR"},
            use_clear=True,
            margin=12,
        )
    else:
        bpy.ops.object.bake(
            type=bake_type,
            use_clear=True,
            margin=12,
        )
    for image in images:
        image.save()
    for material, target in zip(materials, target_nodes):
        material.node_tree.nodes.remove(target)


def make_baked_material(name, base_image, normal_image, roughness_image):
    material = bpy.data.materials.new(name=f"{name}_Baked")
    material.use_nodes = True
    nodes = material.node_tree.nodes
    nodes.clear()
    links = material.node_tree.links
    output = nodes.new("ShaderNodeOutputMaterial")
    shader = nodes.new("ShaderNodeBsdfPrincipled")
    base = nodes.new("ShaderNodeTexImage")
    normal = nodes.new("ShaderNodeTexImage")
    roughness = nodes.new("ShaderNodeTexImage")
    normal_map = nodes.new("ShaderNodeNormalMap")
    base.image = base_image
    normal.image = normal_image
    roughness.image = roughness_image
    normal.image.colorspace_settings.name = "Non-Color"
    roughness.image.colorspace_settings.name = "Non-Color"
    links.new(base.outputs["Color"], shader.inputs["Base Color"])
    links.new(normal.outputs["Color"], normal_map.inputs["Color"])
    links.new(normal_map.outputs["Normal"], shader.inputs["Normal"])
    links.new(roughness.outputs["Color"], shader.inputs["Roughness"])
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    material.diffuse_color = (0.65, 0.48, 0.70, 1.0)
    return material


def replace_materials(obj, baked_materials):
    material_indices = [
        polygon.material_index for polygon in obj.data.polygons
    ]
    obj.data.materials.clear()
    for material in baked_materials:
        obj.data.materials.append(material)
    for polygon, material_index in zip(
            obj.data.polygons, material_indices):
        polygon.material_index = material_index
    source_uv = obj.data.uv_layers.get("SourceUV")
    if source_uv is not None:
        obj.data.uv_layers.remove(source_uv)
    bake_uv = obj.data.uv_layers.get("BakeUV")
    bake_uv.name = "UVMap"
    obj.data.uv_layers.active = bake_uv
    bake_uv.active_render = True


def main():
    arguments = sys.argv[sys.argv.index("--") + 1 :]
    if len(arguments) != 2:
        raise SystemExit("Expected SOURCE_ASSET_DIRECTORY OUTPUT_DIRECTORY")
    source_dir = Path(arguments[0]).resolve()
    output_dir = Path(arguments[1]).resolve()
    (output_dir / "textures").mkdir(parents=True, exist_ok=True)

    bpy.ops.wm.read_factory_settings(use_empty=True)
    leaf_textures = (
        source_dir / "materials" / "bq_leaf_crocus_hybridus")
    flower_textures = (
        source_dir / "materials" / "bq_flower_crocus_hybridus")
    material_names = [
        "bq_Leaf_Crocus-hybridus",
        "bq_Flower_Crocus-hybridus",
        "bq_Flower_Crocus-hybridus_Stamen",
    ]
    source_materials = [
        make_source_material(material_names[0], leaf_textures),
        make_source_material(material_names[1], flower_textures),
        make_source_material(material_names[2], flower_textures),
    ]

    objects = {}
    builders = {}
    for lod_name, spec in LOD_SPECS.items():
        builder = build_crocus(spec)
        obj = game_batch.finalize_object(
            ASSET_NAME, lod_name, builder, source_materials)
        add_uv_layers(obj, builder)
        objects[lod_name] = obj
        builders[lod_name] = builder

    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 8
    # Material-to-texture baking is deterministic here and does not need
    # selected-to-active ray projection.
    resolution = 512
    base_images = create_target_images(
        output_dir, material_names, "basecolor", resolution)
    normal_images = create_target_images(
        output_dir, material_names, "normal", resolution)
    roughness_images = create_target_images(
        output_dir, material_names, "roughness", resolution)
    bake_pass(objects["lod0"], source_materials, base_images, "DIFFUSE", "basecolor")
    bake_pass(objects["lod0"], source_materials, normal_images, "NORMAL", "normal")
    bake_pass(
        objects["lod0"], source_materials, roughness_images,
        "ROUGHNESS", "roughness")

    baked_materials = [
        make_baked_material(name, base, normal, roughness)
        for name, base, normal, roughness in zip(
            material_names, base_images, normal_images, roughness_images)
    ]
    for obj in objects.values():
        replace_materials(obj, baked_materials)

    report = {
        "schema": "openai.game-vegetation-art/2",
        "asset": ASSET_NAME,
        "source": str(source_dir / "flower_crocus.glb"),
        "method": "solid_crocus_with_botaniq_material_rebake",
        "baked_channels": ["base_color", "normal", "roughness"],
        "bake_resolution_per_material": resolution,
        "source_uv_regions": SOURCE_UV_RECTS,
        "lods": {},
    }
    for lod_name, obj in objects.items():
        output_path = output_dir / f"{ASSET_NAME}_{lod_name}.glb"
        game_batch.export_selected(obj, output_path)
        builder = builders[lod_name]
        report["lods"][lod_name] = {
            "triangles": sum(
                max(len(face) - 2, 0) for face in builder.faces),
            "vertices": len(builder.vertices),
            "materials": len(baked_materials),
            "path": output_path.name,
        }

    for lod_name, obj in objects.items():
        obj.hide_render = lod_name != "lod0"
        obj.hide_viewport = lod_name != "lod0"
    bpy.ops.wm.save_as_mainfile(
        filepath=str(output_dir / "flower_crocus_game_master.blend"))
    game_batch.render_preview(ASSET_NAME, output_dir, objects["lod0"])
    with (output_dir / "asset_report.json").open(
            "w", encoding="utf-8") as stream:
        json.dump(report, stream, indent=2)
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
