"""Export selected botaniq source blends as glTF-compatible GLBs.

botaniq materials route their images through custom node groups. Blender's
glTF exporter cannot translate those groups and falls back to a neutral
baseColorFactor. This script preserves the original geometry and images but
rebuilds every material as:

    Image Texture -> Principled BSDF -> Material Output
    Normal Image -> Normal Map -> Principled BSDF

Run with Blender:
  blender --background --python tools/export_botaniq_glb.py -- \
      C:/Users/<user>/polygoniq_asset_packs/botaniq_full \
      resources/source_models/botaniq_corrected
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

import bpy


EXPORTS = {
    "planet_tree.glb": "blends/models/coniferous/bq_Tree_Picea-abies_B_spring-summer-autumn.blend",
    "planet_treeA.glb": "blends/models/coniferous/bq_Tree_Larix-decidua_A_spring-summer.blend",
    "snow_tree.glb": "blends/models/coniferous/bq_Tree_Picea-abies_A_winter.blend",
    "snow_treeA.glb": "blends/models/coniferous/bq_Tree_Larix-decidua_A_winter.blend",
    "planet_grass.glb": "blends/models/grass/bq_Grass_Panicum-virgatum_E_spring.blend",
    "snow_grass.glb": "blends/models/grass/bq_Grass_Bromus-erectus_A_winter.blend",
    "purple_flower.glb": "blends/models/flowers/bq_Flower_Campanula-scheuchzeri_C_spring-summer.blend",
    "white_flower.glb": "blends/models/flowers/bq_Flower_Achillea-millefolium_D_spring-summer.blend",
    "yellow_flower.glb": "blends/models/flowers/bq_Flower_Bunias-orientalis_A_summer.blend",
    "red_flower.glb": "blends/models/flowers/bq_Flower_Dahlia-pinnata_F_summer-autumn.blend",
    # Two additional meadow species chosen for Alpine plausibility and a
    # readable colour range: creeping thyme and spring crocus.
    "pink_flower.glb": "blends/models/flowers/bq_Flower_Thymus-serpyllum_C_summer.blend",
    "crocus_flower.glb": "blends/models/flowers/bq_Flower_Crocus-vernus_B_spring-summer.blend",
}


def collect_images(node_tree, output, visited):
    if not node_tree or node_tree.as_pointer() in visited:
        return
    visited.add(node_tree.as_pointer())
    for node in node_tree.nodes:
        if node.bl_idname == "ShaderNodeTexImage" and node.image:
            output.append(node.image)
        elif node.bl_idname == "ShaderNodeGroup" and node.node_tree:
            collect_images(node.node_tree, output, visited)


def unique_images(material):
    images = []
    collect_images(material.node_tree, images, set())
    unique = []
    seen = set()
    for image in images:
        if image.as_pointer() not in seen:
            seen.add(image.as_pointer())
            unique.append(image)
    return unique


def choose_image(images, kind):
    candidates = []
    for image in images:
        name = image.name.lower()
        if kind == "normal":
            if "normal" not in name:
                continue
            score = 10
        else:
            if not any(token in name for token in ("diffuse", "albedo", "basecolor", "base_color")):
                continue
            if any(token in name for token in ("normal", "macro", "noise", "mask")):
                continue
            score = 10
            if image.file_format == "PNG":
                score += 2
        if image.size[0] > 0 and image.size[1] > 0:
            score += 1
        candidates.append((score, image))
    return max(candidates, key=lambda item: item[0])[1] if candidates else None


def fallback_color(material_name):
    name = material_name.lower()
    if "bark" in name:
        return (0.12, 0.055, 0.022, 1.0)
    if "leaf_picea" in name:
        return (0.035, 0.13, 0.025, 1.0)
    if "leaf_larix" in name:
        return (0.11, 0.24, 0.04, 1.0)
    if "leaf" in name:
        return (0.08, 0.22, 0.035, 1.0)
    if "stem" in name:
        return (0.10, 0.20, 0.035, 1.0)
    if "grass" in name:
        return (0.10, 0.28, 0.045, 1.0)
    if "bellflower" in name:
        return (0.19, 0.23, 0.78, 1.0)
    if "achillea" in name and "flower" in name:
        return (0.88, 0.84, 0.68, 1.0)
    if "dahlia" in name and "flower" in name:
        return (0.72, 0.025, 0.018, 1.0)
    if "flower_summer" in name:
        return (0.95, 0.50, 0.025, 1.0)
    if "snow" in name:
        return (0.78, 0.82, 0.88, 1.0)
    return (0.15, 0.32, 0.055, 1.0)


def socket(node, name, fallback_index=None):
    result = node.inputs.get(name)
    if result is None and fallback_index is not None and fallback_index < len(node.inputs):
        result = node.inputs[fallback_index]
    return result


def rebuild_material(material):
    images = unique_images(material)
    diffuse = choose_image(images, "diffuse")
    normal = choose_image(images, "normal")

    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    nodes.clear()

    output = nodes.new("ShaderNodeOutputMaterial")
    output.location = (520, 0)
    principled = nodes.new("ShaderNodeBsdfPrincipled")
    principled.location = (220, 0)
    principled.inputs["Base Color"].default_value = fallback_color(material.name)
    principled.inputs["Roughness"].default_value = 0.82
    principled.inputs["Metallic"].default_value = 0.0
    links.new(principled.outputs["BSDF"], output.inputs["Surface"])

    alpha_used = False
    if diffuse:
        diffuse.colorspace_settings.name = "sRGB"
        texture = nodes.new("ShaderNodeTexImage")
        texture.name = "glTF Base Color"
        texture.label = diffuse.name
        texture.image = diffuse
        texture.location = (-380, 100)
        links.new(texture.outputs["Color"], principled.inputs["Base Color"])
        if diffuse.channels == 4:
            links.new(texture.outputs["Alpha"], principled.inputs["Alpha"])
            alpha_used = True

    if normal:
        normal.colorspace_settings.name = "Non-Color"
        texture = nodes.new("ShaderNodeTexImage")
        texture.name = "glTF Normal"
        texture.label = normal.name
        texture.image = normal
        texture.location = (-380, -240)
        normal_map = nodes.new("ShaderNodeNormalMap")
        normal_map.location = (-80, -220)
        links.new(texture.outputs["Color"], normal_map.inputs["Color"])
        links.new(normal_map.outputs["Normal"], principled.inputs["Normal"])

    flexible = any(
        token in material.name.lower()
        for token in ("leaf", "grass", "flower", "stem")
    )
    material.use_backface_culling = not flexible
    if alpha_used and hasattr(material, "surface_render_method"):
        material.surface_render_method = "DITHERED"
    return {
        "material": material.name,
        "diffuse": diffuse.name if diffuse else None,
        "normal": normal.name if normal else None,
        "alpha": alpha_used,
    }


def make_materials_local_and_convert():
    converted = {}
    for obj in bpy.context.scene.objects:
        if obj.type != "MESH":
            continue
        if obj.data.library:
            obj.data = obj.data.copy()
        for slot in obj.material_slots:
            material = slot.material
            if not material:
                continue
            if material.library:
                material = material.copy()
                slot.material = material
            key = material.as_pointer()
            if key not in converted:
                converted[key] = rebuild_material(material)
    return list(converted.values())


def export_one(source, destination):
    bpy.ops.wm.open_mainfile(filepath=str(source))
    material_report = make_materials_local_and_convert()

    bpy.ops.object.select_all(action="DESELECT")
    mesh_objects = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    for obj in mesh_objects:
        obj.hide_set(False)
        obj.hide_render = False
        obj.select_set(True)
    if not mesh_objects:
        raise RuntimeError(f"No meshes found in {source}")
    bpy.context.view_layer.objects.active = mesh_objects[0]

    destination.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.export_scene.gltf(
        filepath=str(destination),
        export_format="GLB",
        use_selection=True,
        export_apply=True,
        export_animations=False,
        export_skins=False,
        export_morph=False,
        export_lights=False,
        export_cameras=False,
        export_materials="EXPORT",
    )
    return {
        "source": str(source),
        "output": str(destination),
        "materials": material_report,
    }


def main():
    args = sys.argv[sys.argv.index("--") + 1 :]
    if len(args) < 2:
        raise SystemExit(
            "Expected BOTANIQ_ROOT OUTPUT_DIRECTORY [OUTPUT_NAME ...]"
        )
    botaniq_root = Path(args[0]).resolve()
    output_directory = Path(args[1]).resolve()
    selected = set(args[2:])
    unknown = selected.difference(EXPORTS)
    if unknown:
        raise SystemExit(f"Unknown export names: {sorted(unknown)}")
    report_path = output_directory / "export_report.json"
    report = {}
    if selected and report_path.exists():
        with report_path.open("r", encoding="utf-8") as stream:
            report = json.load(stream)
    for output_name, source_relative in EXPORTS.items():
        if selected and output_name not in selected:
            continue
        source = botaniq_root / source_relative
        if not source.exists():
            raise FileNotFoundError(source)
        report[output_name] = export_one(source, output_directory / output_name)
        print(f"EXPORTED {output_name}")
    with report_path.open("w", encoding="utf-8") as stream:
        json.dump(report, stream, indent=2, ensure_ascii=False)


if __name__ == "__main__":
    main()
