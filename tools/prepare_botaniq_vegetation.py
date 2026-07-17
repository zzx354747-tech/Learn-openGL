"""Prepare user-exported botaniq GLBs for the OpenGL vegetation renderer.

Run with Blender:
  blender --background --python tools/prepare_botaniq_vegetation.py -- \
      D:/HDRI/model resources/models/vegetation

The source exports are presentation meshes, not real-time LODs.  This script
keeps their materials and UVs while producing bounded triangle-count variants.
Runtime normalization preserves the vegetation system's existing world sizes.
"""

from __future__ import annotations

import json
import math
import os
import sys
from collections import defaultdict
from pathlib import Path

import bmesh
import bpy


ASSETS = {
    # Three genuinely different conifer silhouettes.
    # The old 7-17% LOD0 collapse destroyed foliage topology.  These targets
    # keep enough branch and leaf-cluster structure for the near/mid field.
    "picea_tall": ("planet_tree.glb", (60000, 18000, 5000, 8000)),
    "larix_broad": ("planet_treeA.glb", (36000, 14000, 4000, 6500)),
    "larix_sapling": ("snow_treeA.glb", (28000, 10000, 2800, 4500)),
    # Two meadow layers.  grass_meadow is deliberately reused for GrassC.
    "grass_meadow": ("snow_grass.glb", (800, 320, 120, 180)),
    "grass_seedhead": ("planet_grass.glb", (4000, 1400, 450, 700)),
    # Ecologically useful flower accents.
    "flower_bell": ("purple_flower.glb", (240, 140, 80, 100)),
    "flower_white": ("white_flower.glb", (1200, 450, 140, 200)),
    "flower_yellow": ("yellow_flower.glb", (700, 280, 100, 140)),
    "flower_pink": ("pink_flower.glb", (1800, 650, 180, 260)),
    "flower_crocus": ("crocus_flower.glb", (900, 360, 120, 180)),
}

LOD_NAMES = ("lod0", "lod1", "lod2", "shadow")


def triangle_count(mesh_objects):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    total = 0
    for obj in mesh_objects:
        evaluated = obj.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh()
        total += sum(max(len(poly.vertices) - 2, 0) for poly in mesh.polygons)
        evaluated.to_mesh_clear()
    return total


def stable_component_hash(center, root):
    """Return a deterministic pseudo-random key without frame-to-frame noise."""
    x = int(round(center.x * 4096.0))
    y = int(round(center.y * 4096.0))
    z = int(round(center.z * 4096.0))
    value = (
        (x * 73856093)
        ^ (y * 19349663)
        ^ (z * 83492791)
        ^ (root * 2654435761)
    ) & 0xFFFFFFFF
    value ^= value >> 16
    value = (value * 0x7FEB352D) & 0xFFFFFFFF
    value ^= value >> 15
    return value


def loose_components(obj):
    """Collect complete connected mesh islands and their triangle budgets."""
    mesh = obj.data
    mesh.calc_loop_triangles()
    parent = list(range(len(mesh.vertices)))

    def find(index):
        while parent[index] != index:
            parent[index] = parent[parent[index]]
            index = parent[index]
        return index

    def union(a, b):
        a = find(a)
        b = find(b)
        if a != b:
            parent[b] = a

    for triangle in mesh.loop_triangles:
        a, b, c = triangle.vertices
        union(a, b)
        union(a, c)

    faces_by_root = defaultdict(list)
    triangles_by_root = defaultdict(int)
    vertices_by_root = defaultdict(set)
    for polygon in mesh.polygons:
        root = find(polygon.vertices[0])
        faces_by_root[root].append(polygon.index)
        triangles_by_root[root] += max(len(polygon.vertices) - 2, 0)
        vertices_by_root[root].update(polygon.vertices)

    components = []
    for root, faces in faces_by_root.items():
        vertex_indices = vertices_by_root[root]
        center = sum(
            (mesh.vertices[index].co for index in vertex_indices),
            mesh.vertices[next(iter(vertex_indices))].co * 0.0,
        ) / max(len(vertex_indices), 1)
        components.append(
            {
                "object": obj,
                "root": root,
                "faces": faces,
                "triangles": triangles_by_root[root],
                "center": obj.matrix_world @ center,
            }
        )
    return components


def preserve_loose_parts(mesh_objects, target_triangles):
    """Reduce density by removing whole mesh islands, never collapsed fragments.

    Grass blades, leaf cards, flower petals and branch clusters remain exactly
    connected. Spatial bins are sampled round-robin so the reduced model keeps
    the source silhouette instead of collapsing toward a few random specks.
    """
    components = [
        component
        for obj in mesh_objects
        for component in loose_components(obj)
        if component["triangles"] > 0
    ]
    source_triangles = sum(component["triangles"] for component in components)
    if source_triangles <= target_triangles or not components:
        return {
            "source_parts": len(components),
            "output_parts": len(components),
            "method": "full_mesh",
        }

    minimum = [min(component["center"][axis] for component in components)
               for axis in range(3)]
    maximum = [max(component["center"][axis] for component in components)
               for axis in range(3)]

    def bin_key(component):
        coordinates = []
        for axis, bins in ((0, 4), (2, 4), (1, 3)):
            extent = maximum[axis] - minimum[axis]
            normalized = (
                (component["center"][axis] - minimum[axis]) / extent
                if extent > 1e-6 else 0.5
            )
            coordinates.append(min(bins - 1, int(normalized * bins)))
        return tuple(coordinates)

    bins = defaultdict(list)
    for component in components:
        bins[bin_key(component)].append(component)
    for bucket in bins.values():
        bucket.sort(
            key=lambda component: (
                -math.log2(component["triangles"] + 1.0),
                stable_component_hash(component["center"], component["root"]),
            )
        )

    selected = []
    selected_ids = set()
    output_triangles = 0

    # Structural trunks/stems can be much larger than foliage islands. Keep
    # the largest complete part of every mesh before spatial density sampling.
    for obj in mesh_objects:
        candidates = [component for component in components
                      if component["object"] == obj]
        if not candidates:
            continue
        component = max(candidates, key=lambda item: item["triangles"])
        identity = (id(component["object"]), component["root"])
        if identity not in selected_ids:
            selected.append(component)
            selected_ids.add(identity)
            output_triangles += component["triangles"]

    ordered_bins = sorted(bins)
    while output_triangles < target_triangles:
        made_progress = False
        for key in ordered_bins:
            bucket = bins[key]
            while bucket:
                component = bucket.pop(0)
                identity = (id(component["object"]), component["root"])
                if identity in selected_ids:
                    continue
                # Never split a blade/card/branch to hit an exact number.
                if (output_triangles + component["triangles"] >
                        target_triangles and selected):
                    continue
                selected.append(component)
                selected_ids.add(identity)
                output_triangles += component["triangles"]
                made_progress = True
                break
            if output_triangles >= target_triangles:
                break
        if not made_progress:
            break

    keep_faces = defaultdict(set)
    for component in selected:
        keep_faces[component["object"]].update(component["faces"])

    for obj in mesh_objects:
        mesh = obj.data
        body = bmesh.new()
        body.from_mesh(mesh)
        body.faces.ensure_lookup_table()
        discard = [
            face for face in body.faces
            if face.index not in keep_faces[obj]
        ]
        if discard:
            bmesh.ops.delete(body, geom=discard, context="FACES")
        unused = [vertex for vertex in body.verts if not vertex.link_faces]
        if unused:
            bmesh.ops.delete(body, geom=unused, context="VERTS")
        body.to_mesh(mesh)
        body.free()
        mesh.update()

    return {
        "source_parts": len(components),
        "output_parts": len(selected),
        "method": "whole_loose_parts",
    }


def prepare_one(source: Path, destination: Path, target_triangles: int):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(source))

    mesh_objects = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if not mesh_objects:
        raise RuntimeError(f"No mesh objects in {source}")

    original_triangles = triangle_count(mesh_objects)
    topology = preserve_loose_parts(mesh_objects, target_triangles)

    bpy.ops.object.select_all(action="DESELECT")
    for obj in mesh_objects:
        obj.select_set(True)
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
        "source_triangles": original_triangles,
        "target_triangles": target_triangles,
        "output_triangles": triangle_count(mesh_objects),
        **topology,
    }


def main():
    args = sys.argv[sys.argv.index("--") + 1 :]
    if len(args) != 2:
        raise SystemExit("Expected SOURCE_DIRECTORY OUTPUT_DIRECTORY")

    source_directory = Path(args[0]).resolve()
    output_directory = Path(args[1]).resolve()
    report = {}
    for asset_name, (source_name, targets) in ASSETS.items():
        source = source_directory / source_name
        baked_source = (
            source_directory / asset_name / f"{asset_name}.glb"
        )
        if not source.exists() and baked_source.exists():
            source = baked_source
        if not source.exists():
            raise FileNotFoundError(source)
        report[asset_name] = {}
        for lod_name, target in zip(LOD_NAMES, targets):
            destination = output_directory / f"{asset_name}_{lod_name}.glb"
            report[asset_name][lod_name] = prepare_one(source, destination, target)
            print(
                f"PREPARED {asset_name}/{lod_name}: "
                f"{report[asset_name][lod_name]['output_triangles']} triangles"
            )

    with (output_directory / "asset_report.json").open("w", encoding="utf-8") as stream:
        json.dump(report, stream, indent=2)


if __name__ == "__main__":
    main()
