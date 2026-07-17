"""Bake botaniq Blender materials into deterministic runtime texture sets.

The botaniq source materials keep roughness, colour adjustment, translucency,
season controls, and normal strength inside custom node groups.  Blender's
glTF exporter cannot preserve those groups.  This tool converts the currently
used vegetation assets into:

* a standard GLB using baked BaseColor/Alpha and Normal textures;
* per-material packed foliage data (AO, roughness, transmission, normal scale);
* a per-asset runtime atlas with alpha-coverage-preserving mip PNGs;
* a JSON descriptor containing source parameters, atlas rectangles and files.

Run with Blender:
  blender --background --python tools/bake_botaniq_runtime_assets.py -- \
      C:/Users/<user>/polygoniq_asset_packs/botaniq_full \
      resources/source_models/botaniq_baked [ASSET_NAME ...]
"""

from __future__ import annotations

import json
import math
import re
import struct
import sys
import zlib
from pathlib import Path

import bpy
import numpy as np


ASSETS = {
    "picea_tall": (
        "blends/models/coniferous/"
        "bq_Tree_Picea-abies_B_spring-summer-autumn.blend",
        2048,
    ),
    "larix_broad": (
        "blends/models/coniferous/"
        "bq_Tree_Larix-decidua_A_spring-summer.blend",
        2048,
    ),
    "larix_sapling": (
        "blends/models/coniferous/"
        "bq_Tree_Larix-decidua_A_winter.blend",
        2048,
    ),
    "grass_meadow": (
        "blends/models/grass/bq_Grass_Bromus-erectus_A_winter.blend",
        1024,
    ),
    "grass_seedhead": (
        "blends/models/grass/bq_Grass_Panicum-virgatum_E_spring.blend",
        1024,
    ),
    "flower_bell": (
        "blends/models/flowers/"
        "bq_Flower_Campanula-scheuchzeri_C_spring-summer.blend",
        1024,
    ),
    "flower_white": (
        "blends/models/flowers/"
        "bq_Flower_Achillea-millefolium_D_spring-summer.blend",
        1024,
    ),
    "flower_yellow": (
        "blends/models/flowers/bq_Flower_Bunias-orientalis_A_summer.blend",
        1024,
    ),
    "flower_pink": (
        "blends/models/flowers/bq_Flower_Thymus-serpyllum_C_summer.blend",
        1024,
    ),
    "flower_crocus": (
        "blends/models/flowers/"
        "bq_Flower_Crocus-vernus_B_spring-summer.blend",
        1024,
    ),
}

ALPHA_CUTOFF = 0.35
ATLAS_PADDING = 16
MIP_MINIMUM_SIZE = 4


def slug(text: str) -> str:
    result = re.sub(r"[^a-z0-9]+", "_", text.lower()).strip("_")
    return result or "material"


def png_chunk(kind: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + kind
        + payload
        + struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF)
    )


def write_png(path: Path, rgba: np.ndarray) -> None:
    """Write an HxWx4 uint8 image without relying on Pillow."""
    path.parent.mkdir(parents=True, exist_ok=True)
    image = np.ascontiguousarray(rgba, dtype=np.uint8)
    height, width, channels = image.shape
    if channels != 4:
        raise ValueError("PNG output must be RGBA")
    scanlines = b"".join(
        b"\x00" + image[row].tobytes() for row in range(height)
    )
    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + png_chunk(b"IHDR", header)
        + png_chunk(b"IDAT", zlib.compress(scanlines, 7))
        + png_chunk(b"IEND", b"")
    )


def linear_to_srgb(value: np.ndarray) -> np.ndarray:
    value = np.clip(value, 0.0, 1.0)
    return np.where(
        value <= 0.0031308,
        value * 12.92,
        1.055 * np.power(value, 1.0 / 2.4) - 0.055,
    )


def float_image_to_u8(image: np.ndarray, colour: bool) -> np.ndarray:
    result = np.clip(image, 0.0, 1.0).copy()
    if colour:
        result[..., :3] = linear_to_srgb(result[..., :3])
    return np.round(result * 255.0).astype(np.uint8)


def image_pixels(image: bpy.types.Image, width: int, height: int) -> np.ndarray:
    temporary = image.copy()
    temporary.scale(width, height)
    pixels = np.empty(width * height * 4, dtype=np.float32)
    temporary.pixels.foreach_get(pixels)
    bpy.data.images.remove(temporary)
    # Blender exposes the first row at the bottom; PNG rows start at the top.
    return pixels.reshape(height, width, 4)[::-1].copy()


def constant_image(
    width: int, height: int, colour: tuple[float, float, float, float]
) -> np.ndarray:
    result = np.empty((height, width, 4), dtype=np.float32)
    result[...] = colour
    return result


def collect_images(node_tree, output, visited) -> None:
    if not node_tree or node_tree.as_pointer() in visited:
        return
    visited.add(node_tree.as_pointer())
    for node in node_tree.nodes:
        if node.bl_idname == "ShaderNodeTexImage" and node.image:
            output.append(node.image)
        elif node.bl_idname == "ShaderNodeGroup" and node.node_tree:
            collect_images(node.node_tree, output, visited)


def unique_images(material) -> list[bpy.types.Image]:
    images = []
    collect_images(material.node_tree, images, set())
    result = []
    seen = set()
    for image in images:
        if image.as_pointer() not in seen:
            result.append(image)
            seen.add(image.as_pointer())
    return result


def choose_image(images, kind: str):
    candidates = []
    for image in images:
        name = image.name.lower()
        if kind == "normal":
            if "normal" not in name:
                continue
            score = 20
        else:
            if not any(
                token in name
                for token in ("diffuse", "albedo", "basecolor", "base_color")
            ):
                continue
            if any(
                token in name for token in ("normal", "macro", "noise", "mask")
            ):
                continue
            score = 20
            if image.file_format == "PNG":
                score += 4
        score += int(image.size[0] > 0 and image.size[1] > 0)
        candidates.append((score, image))
    return max(candidates, key=lambda item: item[0])[1] if candidates else None


def find_botaniq_group(material):
    if not material.node_tree:
        return None
    candidates = []
    for node in material.node_tree.nodes:
        if node.bl_idname != "ShaderNodeGroup" or not node.node_tree:
            continue
        name = node.node_tree.name.lower()
        score = 2 if "vegetation" in name else 1 if "bark" in name else 0
        if score:
            candidates.append((score, node))
    return max(candidates, key=lambda item: item[0])[1] if candidates else None


def socket_value(node, name: str, default: float) -> float:
    if not node:
        return default
    socket = node.inputs.get(name)
    if socket is None:
        return default
    try:
        return float(socket.default_value)
    except (TypeError, ValueError):
        return default


def material_parameters(material) -> dict:
    name = material.name.lower()
    group = find_botaniq_group(material)
    flexible = any(
        token in name for token in ("leaf", "grass", "flower", "stem")
    )
    translucency = socket_value(group, "Translucency Factor", 0.0)
    if "stem" in name:
        translucency *= 0.35
    if not flexible:
        translucency = 0.0
    return {
        "roughness": socket_value(group, "Roughness", 0.72),
        "specular": socket_value(group, "Specular", 0.35),
        "translucency": translucency,
        "hue": socket_value(group, "Hue", 0.5),
        "saturation": socket_value(group, "Saturation", 1.0),
        "value": socket_value(group, "Value", 1.0),
        "normal_strength": socket_value(group, "Normal Strength", 1.0),
        "bump_strength": socket_value(group, "Bump Strength", 0.0),
        "alpha": socket_value(group, "Alpha", 0.0),
        "snow_amount": socket_value(group, "Snow Amount", 0.0),
        "snow_directional": socket_value(group, "Snow Directional", 0.0),
        "snow_angle": socket_value(group, "Snow Angle", 0.0),
        "moss_amount": socket_value(group, "Moss Amount", 0.0),
        "moss_directional": socket_value(group, "Moss Directional", 0.0),
        "moss_angle": socket_value(group, "Moss Angle", 0.0),
        "deciduous_coniferous": socket_value(
            group, "Deciduous/Coniferous", 0.0
        ),
        "translucency_hue": socket_value(group, "Translucency Hue", 0.5),
        "translucency_saturation": socket_value(
            group, "Translucency Saturation", 1.0
        ),
        "translucency_value": socket_value(
            group, "Translucency Value", 1.0
        ),
        "season_offset": socket_value(group, "bq_season_offset", 0.0),
        "brightness": socket_value(group, "bq_brightness", 1.0),
        "random_per_branch": socket_value(group, "bq_random_per_branch", 0.0),
        "random_per_leaf": socket_value(group, "bq_random_per_leaf", 0.0),
        "flexible": flexible,
    }


def hue_rotate(rgb: np.ndarray, angle: float) -> np.ndarray:
    if abs(angle) < 1e-5:
        return rgb
    axis = np.array([1.0, 1.0, 1.0], dtype=np.float32)
    axis /= np.linalg.norm(axis)
    cosine = math.cos(angle)
    sine = math.sin(angle)
    cross = np.cross(np.broadcast_to(axis, rgb.shape), rgb)
    dot = np.sum(rgb * axis, axis=-1, keepdims=True)
    return rgb * cosine + cross * sine + axis * dot * (1.0 - cosine)


def adjust_base_colour(base: np.ndarray, parameters: dict) -> np.ndarray:
    result = base.copy()
    rgb = result[..., :3]
    rgb = hue_rotate(rgb, (parameters["hue"] - 0.5) * 2.0 * math.pi)
    luminance = np.sum(
        rgb * np.array([0.2126, 0.7152, 0.0722], dtype=np.float32),
        axis=-1,
        keepdims=True,
    )
    rgb = luminance + (rgb - luminance) * parameters["saturation"]
    result[..., :3] = np.clip(rgb * parameters["value"], 0.0, 1.0)
    return result


def dilate_transparent_rgb(base: np.ndarray, iterations: int = 16) -> np.ndarray:
    result = base.copy()
    valid = result[..., 3] > 1.0 / 255.0
    for _ in range(iterations):
        if valid.all():
            break
        accumulated = np.zeros_like(result[..., :3])
        weight = np.zeros(valid.shape, dtype=np.float32)
        for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            shifted_valid = np.roll(valid, (dy, dx), axis=(0, 1))
            shifted_rgb = np.roll(result[..., :3], (dy, dx), axis=(0, 1))
            accumulated += shifted_rgb * shifted_valid[..., None]
            weight += shifted_valid
        fill = (~valid) & (weight > 0.0)
        result[..., :3][fill] = accumulated[fill] / weight[fill, None]
        valid |= fill
    return result


def normalize_normal_map(normal: np.ndarray, strength: float) -> np.ndarray:
    result = normal.copy()
    vector = result[..., :3] * 2.0 - 1.0
    vector[..., :2] *= strength
    length = np.linalg.norm(vector, axis=-1, keepdims=True)
    vector /= np.maximum(length, 1e-6)
    result[..., :3] = vector * 0.5 + 0.5
    result[..., 3] = 1.0
    return result


def downsample_base(image: np.ndarray) -> np.ndarray:
    height = max(1, image.shape[0] // 2)
    width = max(1, image.shape[1] // 2)
    cropped = image[: height * 2, : width * 2]
    blocks = cropped.reshape(height, 2, width, 2, 4)
    alpha = blocks[..., 3:4]
    alpha_average = alpha.mean(axis=(1, 3))
    premultiplied = (blocks[..., :3] * alpha).mean(axis=(1, 3))
    rgb = premultiplied / np.maximum(alpha_average, 1e-6)
    return np.concatenate((rgb, alpha_average), axis=-1)


def downsample_normal(image: np.ndarray) -> np.ndarray:
    height = max(1, image.shape[0] // 2)
    width = max(1, image.shape[1] // 2)
    cropped = image[: height * 2, : width * 2]
    vector = cropped[..., :3] * 2.0 - 1.0
    vector = vector.reshape(height, 2, width, 2, 3).mean(axis=(1, 3))
    vector /= np.maximum(np.linalg.norm(vector, axis=-1, keepdims=True), 1e-6)
    alpha = np.ones((height, width, 1), dtype=np.float32)
    return np.concatenate((vector * 0.5 + 0.5, alpha), axis=-1)


def downsample_linear(image: np.ndarray) -> np.ndarray:
    height = max(1, image.shape[0] // 2)
    width = max(1, image.shape[1] // 2)
    cropped = image[: height * 2, : width * 2]
    return cropped.reshape(height, 2, width, 2, 4).mean(axis=(1, 3))


def preserve_alpha_coverage(
    alpha: np.ndarray, target_coverage: float, cutoff: float
) -> np.ndarray:
    if target_coverage <= 0.0 or target_coverage >= 0.999999:
        return alpha
    low, high = 0.0, 8.0
    for _ in range(18):
        middle = (low + high) * 0.5
        coverage = np.mean(np.clip(alpha * middle, 0.0, 1.0) >= cutoff)
        if coverage < target_coverage:
            low = middle
        else:
            high = middle
    return np.clip(alpha * ((low + high) * 0.5), 0.0, 1.0)


def pad_tile(image: np.ndarray, cell_size: int, padding: int) -> np.ndarray:
    result = np.empty((cell_size, cell_size, 4), dtype=np.float32)
    inner = cell_size - padding * 2
    if image.shape[:2] != (inner, inner):
        raise ValueError("Tile image has the wrong size")
    result[padding : padding + inner, padding : padding + inner] = image
    result[:padding, padding : padding + inner] = image[0:1]
    result[padding + inner :, padding : padding + inner] = image[-1:]
    result[:, :padding] = result[:, padding : padding + 1]
    result[:, padding + inner :] = result[:, padding + inner - 1 : padding + inner]
    return result


def material_records(mesh_objects, inner_size: int, output_directory: Path):
    records = []
    seen = {}
    for obj in mesh_objects:
        if obj.data.library:
            obj.data = obj.data.copy()
        for slot in obj.material_slots:
            material = slot.material
            if not material:
                continue
            if material.library:
                material = material.copy()
                slot.material = material
            pointer = material.as_pointer()
            if pointer in seen:
                continue
            parameters = material_parameters(material)
            images = unique_images(material)
            diffuse = choose_image(images, "diffuse")
            normal_source = choose_image(images, "normal")
            if diffuse:
                base = image_pixels(diffuse, inner_size, inner_size)
            else:
                base = constant_image(inner_size, inner_size, (0.18, 0.32, 0.08, 1.0))
            base = dilate_transparent_rgb(adjust_base_colour(base, parameters))
            has_alpha = bool(np.min(base[..., 3]) < 0.999)
            if not has_alpha:
                base[..., 3] = 1.0

            if normal_source:
                normal = image_pixels(normal_source, inner_size, inner_size)
            else:
                normal = constant_image(inner_size, inner_size, (0.5, 0.5, 1.0, 1.0))
            normal = normalize_normal_map(normal, parameters["normal_strength"])

            luminance = np.sum(
                base[..., :3]
                * np.array([0.2126, 0.7152, 0.0722], dtype=np.float32),
                axis=-1,
            )
            transmission = (
                parameters["translucency"]
                * np.sqrt(np.clip(luminance, 0.0, 1.0))
                * (base[..., 3] if has_alpha else 1.0)
            )
            data = np.empty_like(base)
            data[..., 0] = 1.0
            data[..., 1] = np.clip(parameters["roughness"], 0.04, 1.0)
            data[..., 2] = np.clip(transmission, 0.0, 1.0)
            data[..., 3] = np.clip(parameters["normal_strength"], 0.0, 1.0)
            roughness = constant_image(
                inner_size,
                inner_size,
                (
                    np.clip(parameters["roughness"], 0.04, 1.0),
                    np.clip(parameters["roughness"], 0.04, 1.0),
                    np.clip(parameters["roughness"], 0.04, 1.0),
                    1.0,
                ),
            )
            thickness = np.empty_like(base)
            thickness[..., :3] = np.clip(transmission[..., None], 0.0, 1.0)
            thickness[..., 3] = base[..., 3] if has_alpha else 1.0

            material_slug = slug(material.name)
            material_directory = output_directory / "materials" / material_slug
            base_path = material_directory / "basecolor.png"
            normal_path = material_directory / "normal.png"
            roughness_path = material_directory / "roughness.png"
            thickness_path = material_directory / "thickness.png"
            data_path = material_directory / "foliage_data.png"
            write_png(base_path, float_image_to_u8(base, True))
            write_png(normal_path, float_image_to_u8(normal, False))
            write_png(roughness_path, float_image_to_u8(roughness, False))
            write_png(thickness_path, float_image_to_u8(thickness, False))
            write_png(data_path, float_image_to_u8(data, False))

            record = {
                "material": material,
                "name": material.name,
                "slug": material_slug,
                "parameters": parameters,
                "diffuse_source": diffuse.name if diffuse else None,
                "normal_source": normal_source.name if normal_source else None,
                "has_alpha": has_alpha,
                "base": base,
                "normal": normal,
                "roughness": roughness,
                "thickness": thickness,
                "data": data,
                "base_path": base_path,
                "normal_path": normal_path,
                "roughness_path": roughness_path,
                "thickness_path": thickness_path,
                "data_path": data_path,
            }
            seen[pointer] = len(records)
            records.append(record)
    return records


def build_atlas(records, cell_size: int, output_directory: Path) -> dict:
    count = len(records)
    columns = max(1, int(math.ceil(math.sqrt(count))))
    rows = int(math.ceil(count / columns))
    inner_size = cell_size - ATLAS_PADDING * 2
    base = np.zeros((rows * cell_size, columns * cell_size, 4), dtype=np.float32)
    normal = constant_image(
        columns * cell_size, rows * cell_size, (0.5, 0.5, 1.0, 1.0)
    )
    data = constant_image(
        columns * cell_size, rows * cell_size, (1.0, 0.8, 0.0, 1.0)
    )
    roughness = constant_image(
        columns * cell_size, rows * cell_size, (0.8, 0.8, 0.8, 1.0)
    )
    thickness = np.zeros(
        (rows * cell_size, columns * cell_size, 4), dtype=np.float32
    )
    thickness[..., 3] = 1.0
    coverage_targets = []
    for index, record in enumerate(records):
        column = index % columns
        row = index // columns
        y0, x0 = row * cell_size, column * cell_size
        base[y0 : y0 + cell_size, x0 : x0 + cell_size] = pad_tile(
            record["base"], cell_size, ATLAS_PADDING
        )
        normal[y0 : y0 + cell_size, x0 : x0 + cell_size] = pad_tile(
            record["normal"], cell_size, ATLAS_PADDING
        )
        data[y0 : y0 + cell_size, x0 : x0 + cell_size] = pad_tile(
            record["data"], cell_size, ATLAS_PADDING
        )
        roughness[y0 : y0 + cell_size, x0 : x0 + cell_size] = pad_tile(
            record["roughness"], cell_size, ATLAS_PADDING
        )
        thickness[y0 : y0 + cell_size, x0 : x0 + cell_size] = pad_tile(
            record["thickness"], cell_size, ATLAS_PADDING
        )
        coverage_targets.append(
            float(np.mean(record["base"][..., 3] >= ALPHA_CUTOFF))
        )
        record["tile_index"] = index
        record["tile_rect"] = [
            (x0 + ATLAS_PADDING) / base.shape[1],
            1.0
            - (y0 + ATLAS_PADDING + inner_size) / base.shape[0],
            inner_size / base.shape[1],
            inner_size / base.shape[0],
        ]

    mip_files = {
        "base_color": [],
        "normal": [],
        "roughness": [],
        "thickness": [],
        "foliage_data": [],
    }
    level = 0
    current_base = base
    current_normal = normal
    current_roughness = roughness
    current_thickness = thickness
    current_data = data
    while min(current_base.shape[:2]) >= MIP_MINIMUM_SIZE:
        if level > 0:
            level_cell = max(1, cell_size >> level)
            level_padding = max(0, ATLAS_PADDING >> level)
            for index, target in enumerate(coverage_targets):
                column = index % columns
                row = index // columns
                x0, y0 = column * level_cell, row * level_cell
                x1 = min(current_base.shape[1], x0 + level_cell)
                y1 = min(current_base.shape[0], y0 + level_cell)
                px0 = min(x1, x0 + level_padding)
                py0 = min(y1, y0 + level_padding)
                px1 = max(px0, x1 - level_padding)
                py1 = max(py0, y1 - level_padding)
                alpha = current_base[py0:py1, px0:px1, 3]
                current_base[py0:py1, px0:px1, 3] = preserve_alpha_coverage(
                    alpha, target, ALPHA_CUTOFF
                )

        base_path = output_directory / "atlas" / f"basecolor_mip{level}.png"
        normal_path = output_directory / "atlas" / f"normal_mip{level}.png"
        roughness_path = output_directory / "atlas" / f"roughness_mip{level}.png"
        thickness_path = output_directory / "atlas" / f"thickness_mip{level}.png"
        data_path = output_directory / "atlas" / f"foliage_data_mip{level}.png"
        write_png(base_path, float_image_to_u8(current_base, True))
        write_png(normal_path, float_image_to_u8(current_normal, False))
        write_png(
            roughness_path, float_image_to_u8(current_roughness, False)
        )
        write_png(
            thickness_path, float_image_to_u8(current_thickness, False)
        )
        write_png(data_path, float_image_to_u8(current_data, False))
        mip_files["base_color"].append(str(base_path.relative_to(output_directory)))
        mip_files["normal"].append(str(normal_path.relative_to(output_directory)))
        mip_files["roughness"].append(
            str(roughness_path.relative_to(output_directory))
        )
        mip_files["thickness"].append(
            str(thickness_path.relative_to(output_directory))
        )
        mip_files["foliage_data"].append(str(data_path.relative_to(output_directory)))

        if min(current_base.shape[:2]) <= MIP_MINIMUM_SIZE:
            break
        current_base = downsample_base(current_base)
        current_normal = downsample_normal(current_normal)
        current_roughness = downsample_linear(current_roughness)
        current_thickness = downsample_linear(current_thickness)
        current_data = downsample_linear(current_data)
        level += 1

    return {
        "dimensions": [base.shape[1], base.shape[0]],
        "cell_size": cell_size,
        "padding": ATLAS_PADDING,
        "columns": columns,
        "rows": rows,
        "alpha_cutoff": ALPHA_CUTOFF,
        "uv_addressing": "repeat-within-tile",
        "tile_rect_origin": "bottom-left",
        "channels": {
            "base_color": "RGBA = linearised botaniq colour converted to sRGB + opacity",
            "normal": "RGB tangent-space normal",
            "roughness": "RGB = scalar roughness",
            "thickness": (
                "RGB = derived thin-leaf transmission proxy; "
                "white means more back-light transmission"
            ),
            "foliage_data": "R=AO, G=roughness, B=transmission, A=normal strength",
        },
        "mips": mip_files,
    }


def rebuild_standard_material(record) -> None:
    material = record["material"]
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    nodes.clear()

    output = nodes.new("ShaderNodeOutputMaterial")
    output.location = (520, 0)
    principled = nodes.new("ShaderNodeBsdfPrincipled")
    principled.location = (220, 0)
    principled.inputs["Roughness"].default_value = record["parameters"]["roughness"]
    principled.inputs["Metallic"].default_value = 0.0
    links.new(principled.outputs["BSDF"], output.inputs["Surface"])

    base_image = bpy.data.images.load(str(record["base_path"]), check_existing=False)
    base_image.colorspace_settings.name = "sRGB"
    base_node = nodes.new("ShaderNodeTexImage")
    base_node.name = "Runtime Base Color"
    base_node.image = base_image
    base_node.location = (-420, 120)
    links.new(base_node.outputs["Color"], principled.inputs["Base Color"])
    if record["has_alpha"]:
        links.new(base_node.outputs["Alpha"], principled.inputs["Alpha"])

    normal_image = bpy.data.images.load(str(record["normal_path"]), check_existing=False)
    normal_image.colorspace_settings.name = "Non-Color"
    normal_node = nodes.new("ShaderNodeTexImage")
    normal_node.name = "Runtime Normal"
    normal_node.image = normal_image
    normal_node.location = (-420, -230)
    normal_map = nodes.new("ShaderNodeNormalMap")
    normal_map.inputs["Strength"].default_value = 1.0
    normal_map.location = (-80, -210)
    links.new(normal_node.outputs["Color"], normal_map.inputs["Color"])
    links.new(normal_map.outputs["Normal"], principled.inputs["Normal"])

    material.use_backface_culling = not record["parameters"]["flexible"]
    if record["has_alpha"] and hasattr(material, "surface_render_method"):
        material.surface_render_method = "DITHERED"


def patch_glb_materials(path: Path, records) -> None:
    raw = path.read_bytes()
    if raw[:4] != b"glTF":
        raise ValueError(f"Not a GLB: {path}")
    json_length, json_type = struct.unpack_from("<II", raw, 12)
    if json_type != 0x4E4F534A:
        raise ValueError("First GLB chunk is not JSON")
    document = json.loads(raw[20 : 20 + json_length].decode("utf-8").rstrip(" \0"))
    by_name = {record["name"]: record for record in records}
    for material in document.get("materials", []):
        record = by_name.get(material.get("name", ""))
        if not record:
            continue
        material["doubleSided"] = bool(record["parameters"]["flexible"])
        material.setdefault("extras", {})["botaniqRuntimeMaterial"] = record["slug"]
        if record["has_alpha"]:
            material["alphaMode"] = "MASK"
            material["alphaCutoff"] = ALPHA_CUTOFF
        else:
            material.pop("alphaMode", None)
            material.pop("alphaCutoff", None)

    encoded = json.dumps(
        document, separators=(",", ":"), ensure_ascii=False
    ).encode("utf-8")
    encoded += b" " * ((4 - len(encoded) % 4) % 4)
    remainder = raw[20 + json_length :]
    total_length = 12 + 8 + len(encoded) + len(remainder)
    rebuilt = (
        b"glTF"
        + struct.pack("<II", 2, total_length)
        + struct.pack("<II", len(encoded), 0x4E4F534A)
        + encoded
        + remainder
    )
    path.write_bytes(rebuilt)


def export_glb(mesh_objects, destination: Path) -> None:
    bpy.ops.object.select_all(action="DESELECT")
    for obj in mesh_objects:
        obj.hide_set(False)
        obj.hide_render = False
        obj.select_set(True)
    bpy.context.view_layer.objects.active = mesh_objects[0]
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


def bake_asset(asset_name: str, source: Path, output_root: Path, cell_size: int):
    bpy.ops.wm.open_mainfile(filepath=str(source))
    mesh_objects = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if not mesh_objects:
        raise RuntimeError(f"No mesh objects in {source}")

    output_directory = output_root / asset_name
    output_directory.mkdir(parents=True, exist_ok=True)
    inner_size = cell_size - ATLAS_PADDING * 2
    records = material_records(mesh_objects, inner_size, output_directory)
    atlas = build_atlas(records, cell_size, output_directory)
    for record in records:
        rebuild_standard_material(record)

    glb_path = output_directory / f"{asset_name}.glb"
    export_glb(mesh_objects, glb_path)
    patch_glb_materials(glb_path, records)

    descriptor = {
        "schema": "openai.botaniq-runtime-material/1",
        "asset": asset_name,
        "source_blend": str(source),
        "glb": glb_path.name,
        "bake_scope": {
            "rasterized": [
                "base_color_and_opacity",
                "normal",
                "roughness",
                "derived_thickness_transmission",
            ],
            "base_color_evaluation": (
                "source diffuse plus top-level Hue, Saturation and Value"
            ),
            "thickness_source": (
                "derived from Bonatiq Translucency Factor, base-color "
                "luminance and opacity; the source library has no dedicated "
                "thickness bitmap"
            ),
            "procedural_controls": (
                "season, snow, moss and per-branch/per-leaf controls are "
                "recorded below but not collapsed into the atlas because the "
                "source meshes use overlapping/repeating UVs"
            ),
        },
        "atlas": atlas,
        "materials": [
            {
                "name": record["name"],
                "slug": record["slug"],
                "tile_index": record["tile_index"],
                "tile_rect": record["tile_rect"],
                "alpha_mode": "MASK" if record["has_alpha"] else "OPAQUE",
                "alpha_cutoff": ALPHA_CUTOFF if record["has_alpha"] else None,
                "source_textures": {
                    "diffuse": record["diffuse_source"],
                    "normal": record["normal_source"],
                },
                "runtime_textures": {
                    "base_color": str(
                        record["base_path"].relative_to(output_directory)
                    ),
                    "normal": str(
                        record["normal_path"].relative_to(output_directory)
                    ),
                    "roughness": str(
                        record["roughness_path"].relative_to(output_directory)
                    ),
                    "thickness": str(
                        record["thickness_path"].relative_to(output_directory)
                    ),
                    "foliage_data": str(
                        record["data_path"].relative_to(output_directory)
                    ),
                },
                "parameters": record["parameters"],
            }
            for record in records
        ],
    }
    descriptor_path = output_directory / "material.json"
    descriptor_path.write_text(
        json.dumps(descriptor, indent=2, ensure_ascii=False), encoding="utf-8"
    )
    print(
        f"BAKED {asset_name}: {len(records)} materials, "
        f"{atlas['dimensions'][0]}x{atlas['dimensions'][1]} atlas, "
        f"{len(atlas['mips']['base_color'])} mips"
    )
    return descriptor


def main() -> None:
    args = sys.argv[sys.argv.index("--") + 1 :]
    if len(args) < 2:
        raise SystemExit(
            "Expected BOTANIQ_ROOT OUTPUT_DIRECTORY [ASSET_NAME ...]"
        )
    botaniq_root = Path(args[0]).resolve()
    output_root = Path(args[1]).resolve()
    selected = args[2:] or list(ASSETS)
    unknown = set(selected).difference(ASSETS)
    if unknown:
        raise SystemExit(f"Unknown assets: {sorted(unknown)}")

    for asset_name in selected:
        source_relative, cell_size = ASSETS[asset_name]
        source = botaniq_root / source_relative
        if not source.exists():
            raise FileNotFoundError(source)
        bake_asset(asset_name, source, output_root, cell_size)
    output_root.mkdir(parents=True, exist_ok=True)
    report = {}
    for asset_name in ASSETS:
        descriptor_path = output_root / asset_name / "material.json"
        if descriptor_path.exists():
            report[asset_name] = json.loads(
                descriptor_path.read_text(encoding="utf-8")
            )
    (output_root / "bake_report.json").write_text(
        json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8"
    )


if __name__ == "__main__":
    main()
