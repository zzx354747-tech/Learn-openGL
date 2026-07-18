"""Build pixel-stable game derivatives for the remaining Botaniq vegetation.

The source GLBs are not modified.  This produces independently-authored,
opaque geometry for grass, flowers, and conifers with four LODs each.

Run:
  blender --background --python tools/build_game_botaniq_batch.py -- \
      SOURCE_ROOT OUTPUT_ROOT
"""

from __future__ import annotations

import json
import math
import random
import sys
from pathlib import Path

import bpy
from mathutils import Vector


LOD_ORDER = ("lod0", "lod1", "lod2", "shadow")
LOD_QUALITY = {
    "lod0": 1.0,
    "lod1": 0.66,
    "lod2": 0.36,
    "shadow": 0.28,
}

ASSETS = {
    "grass_meadow": {
        "kind": "grass",
        "dimensions": (0.8783, 0.8837, 0.4940),
        "seed": 1201,
        "materials": [
            ("bq_Grass_Bromus-erectus", (0.34, 0.31, 0.12, 1.0), 0.82),
        ],
    },
    "flower_bell": {
        "kind": "bell",
        "dimensions": (0.1010, 0.0919, 0.1863),
        "seed": 2201,
        "materials": [
            ("bq_Flower_Bellflower", (0.20, 0.27, 0.72, 1.0), 0.58),
            ("bq_Grass", (0.20, 0.42, 0.12, 1.0), 0.78),
        ],
    },
    "flower_crocus": {
        "kind": "crocus",
        "dimensions": (0.0552, 0.0434, 0.1317),
        "seed": 2202,
        "materials": [
            ("bq_Leaf_Crocus-hybridus", (0.18, 0.40, 0.12, 1.0), 0.78),
            ("bq_Flower_Crocus-hybridus", (0.53, 0.28, 0.74, 1.0), 0.54),
        ],
    },
    "flower_pink": {
        "kind": "groundcover",
        "dimensions": (0.3260, 0.3416, 0.1080),
        "seed": 2203,
        "materials": [
            ("bq_Flower_Thymus-serpyllum", (0.78, 0.30, 0.52, 1.0), 0.62),
            ("bq_Leaf_Thymus-serpyllum", (0.16, 0.34, 0.10, 1.0), 0.84),
        ],
    },
    "flower_white": {
        "kind": "yarrow",
        "dimensions": (0.2183, 0.2216, 0.3270),
        "seed": 2204,
        "materials": [
            ("bq_Leaf_Achillea-millefolium", (0.22, 0.39, 0.13, 1.0), 0.82),
            ("bq_Stem_Achillea-millefolium", (0.31, 0.43, 0.18, 1.0), 0.86),
            ("bq_Flower_achillea-millefolium", (0.91, 0.88, 0.68, 1.0), 0.70),
        ],
    },
    "flower_yellow": {
        "kind": "mustard",
        "dimensions": (0.7560, 0.6951, 0.8743),
        "seed": 2205,
        "materials": [
            ("bq_Stem_Summer", (0.25, 0.43, 0.13, 1.0), 0.84),
            ("bq_Flower_Summer", (0.94, 0.68, 0.06, 1.0), 0.60),
        ],
    },
    "larix_broad": {
        "kind": "larix_broad",
        "dimensions": (3.6099, 3.8375, 5.2009),
        "seed": 3201,
        "materials": [
            ("bq_Bark_Larix-decidua", (0.20, 0.105, 0.055, 1.0), 0.92),
            ("bq_Leaf_Larix-decidua", (0.36, 0.57, 0.11, 1.0), 0.82),
        ],
    },
    "larix_sapling": {
        "kind": "larix_sparse",
        "dimensions": (3.4935, 3.6041, 5.0425),
        "seed": 3202,
        "materials": [
            ("bq_Bark_Larix-decidua", (0.19, 0.10, 0.055, 1.0), 0.93),
            ("bq_Leaf_Larix-decidua", (0.62, 0.43, 0.10, 1.0), 0.86),
        ],
    },
    "picea_tall": {
        "kind": "picea",
        "dimensions": (2.3878, 2.5366, 3.9886),
        "seed": 3203,
        "materials": [
            ("bq_Bark_Picea-abies", (0.17, 0.085, 0.045, 1.0), 0.94),
            ("bq_Leaf_Picea-abies", (0.075, 0.24, 0.085, 1.0), 0.88),
        ],
    },
}


class MeshBuilder:
    def __init__(self):
        self.vertices = []
        self.faces = []
        self.uvs = []
        self.material_indices = []

    def vertex(self, position, uv=(0.5, 0.5)):
        index = len(self.vertices)
        self.vertices.append(tuple(position))
        self.uvs.append(uv)
        return index

    def face(self, indices, material_index):
        self.faces.append(tuple(indices))
        self.material_indices.append(material_index)

    def frustum(self, start, end, start_radius, end_radius, sides, material):
        start = Vector(start)
        end = Vector(end)
        axis = end - start
        if axis.length < 1.0e-6:
            return
        direction = axis.normalized()
        helper = Vector((0.0, 0.0, 1.0))
        if abs(direction.dot(helper)) > 0.88:
            helper = Vector((1.0, 0.0, 0.0))
        side = direction.cross(helper).normalized()
        up = direction.cross(side).normalized()
        first = len(self.vertices)
        for ring, (center, radius) in enumerate(
                ((start, start_radius), (end, end_radius))):
            for index in range(sides):
                angle = math.tau * index / sides
                offset = (side * math.cos(angle) + up * math.sin(angle)) * radius
                self.vertex(center + offset, (index / sides, float(ring)))
        for index in range(sides):
            following = (index + 1) % sides
            self.face(
                (first + index, first + following,
                 first + sides + following, first + sides + index),
                material,
            )
        self.face(tuple(first + i for i in reversed(range(sides))), material)
        self.face(tuple(first + sides + i for i in range(sides)), material)

    def ellipsoid(self, center, axis, length, radius, sides, rings, material):
        center = Vector(center)
        direction = Vector(axis).normalized()
        helper = Vector((0.0, 0.0, 1.0))
        if abs(direction.dot(helper)) > 0.88:
            helper = Vector((1.0, 0.0, 0.0))
        side = direction.cross(helper).normalized()
        up = direction.cross(side).normalized()
        bottom = self.vertex(center - direction * length * 0.5, (0.5, 0.0))
        ring_starts = []
        for ring_index in range(1, rings):
            phi = math.pi * ring_index / rings
            ring_starts.append(len(self.vertices))
            axial = -math.cos(phi) * length * 0.5
            radial = math.sin(phi) * radius
            for index in range(sides):
                angle = math.tau * index / sides
                point = (
                    center + direction * axial +
                    (side * math.cos(angle) + up * math.sin(angle)) * radial
                )
                self.vertex(point, (index / sides, ring_index / rings))
        top = self.vertex(center + direction * length * 0.5, (0.5, 1.0))
        first_ring = ring_starts[0]
        for index in range(sides):
            following = (index + 1) % sides
            self.face((bottom, first_ring + following, first_ring + index), material)
        for ring_index in range(len(ring_starts) - 1):
            lower = ring_starts[ring_index]
            upper = ring_starts[ring_index + 1]
            for index in range(sides):
                following = (index + 1) % sides
                self.face(
                    (lower + index, lower + following,
                     upper + following, upper + index),
                    material,
                )
        last_ring = ring_starts[-1]
        for index in range(sides):
            following = (index + 1) % sides
            self.face((last_ring + index, last_ring + following, top), material)

    def blade(self, base, yaw, height, width, bend, segments, thickness, material):
        base = Vector(base)
        outward = Vector((math.cos(yaw), math.sin(yaw), 0.0))
        side = Vector((-math.sin(yaw), math.cos(yaw), 0.0))
        first = len(self.vertices)
        for segment in range(segments + 1):
            t = segment / segments
            eased = t * t * (3.0 - 2.0 * t)
            center = base + outward * bend * eased + Vector((0.0, 0.0, height * t))
            half_width = max(width * 0.08, width * 0.5 * (1.0 - t) ** 0.58)
            half_depth = max(width * 0.04, half_width * thickness)
            depth = outward * half_depth
            for point, uv in (
                (center - side * half_width - depth, (0.0, t)),
                (center + side * half_width - depth, (0.33, t)),
                (center + side * half_width + depth, (0.66, t)),
                (center - side * half_width + depth, (1.0, t)),
            ):
                self.vertex(point, uv)
        for segment in range(segments):
            lower = first + segment * 4
            upper = lower + 4
            for edge in range(4):
                following = (edge + 1) % 4
                self.face(
                    (lower + edge, lower + following,
                     upper + following, upper + edge),
                    material,
                )
        self.face(tuple(first + i for i in reversed(range(4))), material)
        tip = first + segments * 4
        self.face(tuple(tip + i for i in range(4)), material)


def create_material(name, color, roughness):
    material = bpy.data.materials.new(name=name)
    material.use_nodes = True
    material.diffuse_color = color
    material.surface_render_method = "DITHERED"
    nodes = material.node_tree.nodes
    shader = nodes.get("Principled BSDF")
    if shader is not None:
        shader.inputs["Base Color"].default_value = color
        shader.inputs["Roughness"].default_value = roughness
        if "Alpha" in shader.inputs:
            shader.inputs["Alpha"].default_value = 1.0
    return material


def golden_points(count, radius, rng):
    angle_step = math.pi * (3.0 - math.sqrt(5.0))
    points = []
    for index in range(count):
        radial = math.sqrt((index + 0.5) / count) * radius
        angle = index * angle_step + rng.uniform(-0.20, 0.20)
        points.append((Vector((math.cos(angle) * radial,
                               math.sin(angle) * radial, 0.0)), angle, radial))
    return points


def build_grass(builder, dimensions, q, rng):
    width, depth, height = dimensions
    count = max(12, round(58 * q))
    segments = 5 if q > 0.8 else 4 if q > 0.5 else 3
    blade_width = height * (0.018 + (1.0 - q) * 0.045)
    for base, angle, radial in golden_points(count, min(width, depth) * 0.46, rng):
        normalized_radius = radial / (min(width, depth) * 0.46)
        blade_height = height * rng.uniform(0.48, 1.0) * (1.0 - normalized_radius * 0.20)
        builder.blade(
            base, angle + rng.uniform(-0.55, 0.55), blade_height,
            blade_width * rng.uniform(0.72, 1.25),
            height * rng.uniform(0.05, 0.25) * (0.5 + normalized_radius),
            segments, 0.24 + (1.0 - q) * 0.12, 0,
        )


def build_bell(builder, dimensions, q, rng):
    width, depth, height = dimensions
    flower_count = 3 if q > 0.8 else 2 if q > 0.45 else 1
    leaf_count = max(2, round(6 * q))
    stem_material = 1
    for index in range(flower_count):
        angle = math.tau * index / flower_count + 0.35
        base = Vector((math.cos(angle) * width * 0.10,
                       math.sin(angle) * depth * 0.10, 0.0))
        top = Vector((math.cos(angle) * width * 0.30,
                      math.sin(angle) * depth * 0.30,
                      height * (0.70 + 0.12 * index)))
        builder.frustum(base, top, height * 0.020, height * 0.012,
                        7 if q > 0.5 else 5, stem_material)
        bell_length = height * (0.18 + (1.0 - q) * 0.04)
        bottom = top - Vector((0.0, 0.0, bell_length))
        builder.frustum(
            top, bottom, width * 0.075, width * 0.19,
            10 if q > 0.8 else 7 if q > 0.45 else 6, 0,
        )
    for index in range(leaf_count):
        angle = math.tau * index / leaf_count + rng.uniform(-0.2, 0.2)
        builder.blade(
            Vector((0.0, 0.0, 0.0)), angle,
            height * rng.uniform(0.30, 0.56),
            width * (0.085 + (1.0 - q) * 0.055),
            width * rng.uniform(0.10, 0.30),
            3 if q > 0.45 else 2, 0.30, stem_material,
        )


def build_crocus(builder, dimensions, q, rng):
    width, depth, height = dimensions
    leaf_count = max(3, round(8 * q))
    petal_count = 6 if q > 0.5 else 4 if q > 0.30 else 3
    for index in range(leaf_count):
        angle = math.tau * index / leaf_count + rng.uniform(-0.22, 0.22)
        builder.blade(
            Vector((0.0, 0.0, 0.0)), angle,
            height * rng.uniform(0.58, 0.92),
            width * (0.085 + (1.0 - q) * 0.06),
            width * rng.uniform(0.10, 0.35),
            3 if q > 0.45 else 2, 0.28, 0,
        )
    flower_center = Vector((0.0, 0.0, height * 0.73))
    builder.frustum(
        Vector((0.0, 0.0, height * 0.18)), flower_center,
        width * 0.035, width * 0.022, 6, 0,
    )
    for index in range(petal_count):
        angle = math.tau * index / petal_count
        outward = Vector((math.cos(angle), math.sin(angle), 0.56)).normalized()
        center = flower_center + outward * width * 0.16
        builder.ellipsoid(
            center, outward, height * 0.34,
            width * (0.105 + (1.0 - q) * 0.035),
            8 if q > 0.8 else 6, 4 if q > 0.45 else 3, 1,
        )
    builder.ellipsoid(
        flower_center + Vector((0.0, 0.0, height * 0.12)),
        (0.0, 0.0, 1.0), height * 0.08, width * 0.10,
        8 if q > 0.5 else 6, 3, 1,
    )


def build_groundcover(builder, dimensions, q, rng):
    width, depth, height = dimensions
    count = max(4, round(16 * q))
    for base, angle, radial in golden_points(count, min(width, depth) * 0.43, rng):
        scale = rng.uniform(0.75, 1.20)
        foliage_center = base + Vector((0.0, 0.0, height * 0.24 * scale))
        builder.ellipsoid(
            foliage_center, (0.0, 0.0, 1.0),
            height * 0.48 * scale, width * 0.070 * scale,
            8 if q > 0.7 else 6, 4 if q > 0.45 else 3, 1,
        )
        flower_center = base + Vector((0.0, 0.0, height * (0.60 + 0.25 * scale)))
        builder.ellipsoid(
            flower_center, (0.0, 0.0, 1.0),
            height * (0.12 + (1.0 - q) * 0.08),
            width * (0.048 + (1.0 - q) * 0.020),
            8 if q > 0.7 else 6, 3, 0,
        )


def build_yarrow(builder, dimensions, q, rng):
    width, depth, height = dimensions
    stem_count = max(2, round(6 * q))
    floret_count = 6 if q > 0.8 else 3 if q > 0.45 else 1
    points = golden_points(stem_count, min(width, depth) * 0.27, rng)
    for index, (base, angle, radial) in enumerate(points):
        stem_height = height * rng.uniform(0.70, 1.0)
        top = base * 1.35 + Vector((0.0, 0.0, stem_height))
        builder.frustum(
            base, top, height * 0.010, height * 0.006,
            6 if q > 0.45 else 5, 1,
        )
        builder.ellipsoid(
            top, (0.0, 0.0, 1.0),
            height * 0.035, width * (0.11 + (1.0 - q) * 0.035),
            10 if q > 0.8 else 7, 3, 2,
        )
        for floret in range(floret_count - 1):
            floret_angle = math.tau * floret / max(1, floret_count - 1)
            offset = Vector((math.cos(floret_angle), math.sin(floret_angle), 0.10))
            builder.ellipsoid(
                top + offset * width * 0.10, (0.0, 0.0, 1.0),
                height * 0.026, width * 0.055,
                7 if q > 0.7 else 5, 3, 2,
            )
    leaf_count = max(3, round(9 * q))
    for index in range(leaf_count):
        angle = math.tau * index / leaf_count
        builder.blade(
            Vector((0.0, 0.0, height * 0.02)), angle,
            height * rng.uniform(0.24, 0.46),
            width * (0.035 + (1.0 - q) * 0.025),
            width * rng.uniform(0.08, 0.22),
            3 if q > 0.45 else 2, 0.30, 0,
        )


def build_mustard(builder, dimensions, q, rng):
    width, depth, height = dimensions
    stem_count = max(2, round(5 * q))
    branch_count = 2 if q > 0.45 else 1
    for index, (base, angle, radial) in enumerate(
            golden_points(stem_count, min(width, depth) * 0.20, rng)):
        stem_height = height * rng.uniform(0.72, 1.0)
        top = Vector((
            math.cos(angle) * width * rng.uniform(0.16, 0.34),
            math.sin(angle) * depth * rng.uniform(0.16, 0.34),
            stem_height,
        ))
        builder.frustum(
            base, top, height * 0.010, height * 0.005,
            7 if q > 0.65 else 5, 0,
        )
        head_points = [top]
        for branch in range(branch_count):
            branch_angle = angle + (branch * 2.0 - 0.5) * 0.75
            joint = base.lerp(top, 0.62 + branch * 0.13)
            end = joint + Vector((
                math.cos(branch_angle) * width * 0.17,
                math.sin(branch_angle) * depth * 0.17,
                height * 0.14,
            ))
            builder.frustum(
                joint, end, height * 0.006, height * 0.0035,
                6 if q > 0.55 else 5, 0,
            )
            head_points.append(end)
        for head in head_points:
            builder.ellipsoid(
                head, (0.0, 0.0, 1.0),
                height * (0.035 + (1.0 - q) * 0.018),
                width * (0.025 + (1.0 - q) * 0.012),
                8 if q > 0.7 else 6, 3, 1,
            )
    leaf_count = max(4, round(10 * q))
    for index in range(leaf_count):
        angle = math.tau * index / leaf_count
        builder.blade(
            Vector((0.0, 0.0, 0.0)), angle,
            height * rng.uniform(0.22, 0.46),
            width * (0.026 + (1.0 - q) * 0.018),
            width * rng.uniform(0.05, 0.16),
            3 if q > 0.45 else 2, 0.34, 0,
        )


def conifer_envelope(kind, t):
    if kind == "picea":
        return max(0.05, (1.0 - t) ** 0.72)
    if kind == "larix_broad":
        return max(0.08, math.sin(min(1.0, t + 0.11) * math.pi) ** 0.52)
    return max(0.06, math.sin(min(1.0, t + 0.08) * math.pi) ** 0.72)


def build_conifer(builder, dimensions, q, rng, kind):
    width, depth, height = dimensions
    if kind == "picea":
        base_tiers, base_branches, crown_start = 13, 7, 0.16
        foliage_scale, droop = 0.065, -0.075
    elif kind == "larix_broad":
        base_tiers, base_branches, crown_start = 12, 7, 0.20
        foliage_scale, droop = 0.055, -0.025
    else:
        base_tiers, base_branches, crown_start = 10, 6, 0.22
        foliage_scale, droop = 0.044, -0.015

    tiers = max(5, round(base_tiers * (0.48 + q * 0.52)))
    branches = max(4, round(base_branches * (0.55 + q * 0.45)))
    sides = 9 if q > 0.8 else 7 if q > 0.45 else 6
    builder.frustum(
        (0.0, 0.0, 0.0), (0.0, 0.0, height),
        height * 0.028, height * 0.0045, sides, 0,
    )

    for tier in range(tiers):
        t = crown_start + (tier + 0.35) / tiers * (0.91 - crown_start)
        z = height * t
        envelope = conifer_envelope(kind, t)
        radius_x = width * 0.48 * envelope
        radius_y = depth * 0.48 * envelope
        phase = tier * 0.47 + rng.uniform(-0.12, 0.12)
        for branch in range(branches):
            angle = math.tau * branch / branches + phase
            length_scale = rng.uniform(0.78, 1.05)
            end = Vector((
                math.cos(angle) * radius_x * length_scale,
                math.sin(angle) * radius_y * length_scale,
                z + height * droop * envelope + rng.uniform(-0.012, 0.018) * height,
            ))
            start = Vector((0.0, 0.0, z))
            builder.frustum(
                start, end,
                height * (0.0075 + 0.0025 * envelope),
                height * 0.0028,
                6 if q > 0.45 else 5, 0,
            )
            direction = end - start
            cluster_count = 2 if q > 0.75 else 1
            if kind == "larix_sparse" and (tier + branch) % 3 == 0:
                cluster_count = 0
            for cluster in range(cluster_count):
                factor = 0.52 + cluster * 0.30
                center = start.lerp(end, factor)
                cluster_length = direction.length * (0.52 if cluster_count == 1 else 0.34)
                cluster_radius = height * foliage_scale * (
                    0.72 + 0.30 * envelope + (1.0 - q) * 0.28
                )
                builder.ellipsoid(
                    center, direction, cluster_length, cluster_radius,
                    9 if q > 0.8 else 7 if q > 0.45 else 6,
                    5 if q > 0.8 else 4 if q > 0.45 else 3, 1,
                )

    top_length = height * (0.20 if kind == "picea" else 0.14)
    builder.ellipsoid(
        (0.0, 0.0, height - top_length * 0.46), (0.0, 0.0, 1.0),
        top_length, height * foliage_scale * (1.25 + (1.0 - q) * 0.2),
        9 if q > 0.8 else 7 if q > 0.45 else 6,
        5 if q > 0.8 else 4, 1,
    )


def build_asset_geometry(config, lod_name):
    q = LOD_QUALITY[lod_name]
    rng = random.Random(config["seed"])
    builder = MeshBuilder()
    kind = config["kind"]
    dimensions = config["dimensions"]
    if kind == "grass":
        build_grass(builder, dimensions, q, rng)
    elif kind == "bell":
        build_bell(builder, dimensions, q, rng)
    elif kind == "crocus":
        build_crocus(builder, dimensions, q, rng)
    elif kind == "groundcover":
        build_groundcover(builder, dimensions, q, rng)
    elif kind == "yarrow":
        build_yarrow(builder, dimensions, q, rng)
    elif kind == "mustard":
        build_mustard(builder, dimensions, q, rng)
    else:
        build_conifer(builder, dimensions, q, rng, kind)
    return builder


def finalize_object(asset_name, lod_name, builder, materials):
    mesh = bpy.data.meshes.new(f"{asset_name}_{lod_name}_mesh")
    mesh.from_pydata(builder.vertices, [], builder.faces)
    mesh.update()
    uv_layer = mesh.uv_layers.new(name="UVMap")
    for loop in mesh.loops:
        uv_layer.data[loop.index].uv = builder.uvs[loop.vertex_index]
    for material in materials:
        mesh.materials.append(material)
    for polygon, material_index in zip(mesh.polygons, builder.material_indices):
        polygon.material_index = material_index
        polygon.use_smooth = True
    obj = bpy.data.objects.new(f"{asset_name}_{lod_name}", mesh)
    bpy.context.collection.objects.link(obj)
    obj["game_asset"] = True
    obj["lod"] = lod_name
    obj["pixel_stable_geometry"] = True
    return obj


def export_selected(obj, path):
    bpy.ops.object.select_all(action="DESELECT")
    obj.hide_set(False)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.gltf(
        filepath=str(path),
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


def object_bounds(obj):
    corners = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    minimum = Vector((
        min(point.x for point in corners),
        min(point.y for point in corners),
        min(point.z for point in corners),
    ))
    maximum = Vector((
        max(point.x for point in corners),
        max(point.y for point in corners),
        max(point.z for point in corners),
    ))
    return minimum, maximum


def look_at(obj, target):
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat("-Z", "Y").to_euler()


def render_preview(asset_name, output_dir, lod0):
    for obj in bpy.context.scene.objects:
        if obj.type == "MESH":
            obj.hide_render = obj != lod0
            obj.hide_set(obj != lod0)
    minimum, maximum = object_bounds(lod0)
    center = (minimum + maximum) * 0.5
    extent = maximum - minimum
    radius = max(extent) * 0.5

    world = bpy.data.worlds.new(f"{asset_name}_preview_world")
    world.color = (0.055, 0.065, 0.08)
    bpy.context.scene.world = world

    camera_data = bpy.data.cameras.new("PreviewCamera")
    camera = bpy.data.objects.new("PreviewCamera", camera_data)
    bpy.context.collection.objects.link(camera)
    camera_data.lens = 58
    camera.location = center + Vector((1.35, -1.75, 0.78)).normalized() * radius * 3.2
    look_at(camera, center + Vector((0.0, 0.0, extent.z * 0.03)))
    bpy.context.scene.camera = camera

    for name, energy, location, size in (
        ("Key", 950, (-1.5, -1.4, 2.4), 2.8),
        ("Fill", 420, (1.8, 0.6, 1.2), 2.4),
    ):
        light_data = bpy.data.lights.new(name, "AREA")
        light_data.energy = energy
        light_data.size = radius * size
        light = bpy.data.objects.new(name, light_data)
        bpy.context.collection.objects.link(light)
        light.location = center + Vector(location).normalized() * radius * 3.0
        look_at(light, center)

    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 640
    scene.render.resolution_y = 640
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.film_transparent = False
    scene.render.filepath = str(output_dir / f"{asset_name}_preview.png")
    try:
        scene.view_settings.look = "AgX - Medium High Contrast"
    except TypeError:
        pass
    bpy.ops.render.render(write_still=True)


def build_one(asset_name, config, source_root, output_root):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    output_dir = output_root / asset_name
    output_dir.mkdir(parents=True, exist_ok=True)
    materials = [
        create_material(name, color, roughness)
        for name, color, roughness in config["materials"]
    ]
    source_path = source_root / asset_name / f"{asset_name}.glb"
    report = {
        "schema": "openai.game-vegetation-art/1",
        "asset": asset_name,
        "source": str(source_path),
        "method": f"opaque_{config['kind']}_reconstruction",
        "source_dimensions": config["dimensions"],
        "lods": {},
    }
    lod_objects = {}
    for lod_name in LOD_ORDER:
        builder = build_asset_geometry(config, lod_name)
        obj = finalize_object(asset_name, lod_name, builder, materials)
        lod_objects[lod_name] = obj
        output_path = output_dir / f"{asset_name}_{lod_name}.glb"
        export_selected(obj, output_path)
        report["lods"][lod_name] = {
            "triangles": sum(max(len(face) - 2, 0) for face in builder.faces),
            "vertices": len(builder.vertices),
            "connected_design_elements": len(builder.faces),
            "path": output_path.name,
        }

    for lod_name, obj in lod_objects.items():
        obj.hide_render = lod_name != "lod0"
        obj.hide_viewport = lod_name != "lod0"
    master_path = output_dir / f"{asset_name}_game_master.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master_path))
    render_preview(asset_name, output_dir, lod_objects["lod0"])
    with (output_dir / "asset_report.json").open("w", encoding="utf-8") as stream:
        json.dump(report, stream, indent=2)
    print(json.dumps(report))


def main():
    arguments = sys.argv[sys.argv.index("--") + 1 :]
    if len(arguments) != 2:
        raise SystemExit("Expected SOURCE_ROOT OUTPUT_ROOT")
    source_root = Path(arguments[0]).resolve()
    output_root = Path(arguments[1]).resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    for asset_name, config in ASSETS.items():
        build_one(asset_name, config, source_root, output_root)


if __name__ == "__main__":
    main()
