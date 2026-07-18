"""Build a pixel-stable game derivative of the Botaniq seed-head grass.

The original asset is kept untouched.  This tool imports its material, then
authors new solid ribbon blades and coherent panicle masses for four runtime
LODs.  It intentionally does not decimate or randomly delete source strands.

Run:
  blender --background --python tools/build_game_grass_seedhead.py -- \
      SOURCE_GLB OUTPUT_DIRECTORY
"""

from __future__ import annotations

import json
import math
import random
import sys
from pathlib import Path

import bpy
from mathutils import Vector


ASSET_NAME = "grass_seedhead"
LOD_SPECS = {
    "lod0": {"blades": 48, "panicles": 12, "segments": 5,
             "width": 0.018, "thickness": 0.22},
    "lod1": {"blades": 28, "panicles": 7, "segments": 4,
             "width": 0.030, "thickness": 0.25},
    "lod2": {"blades": 12, "panicles": 3, "segments": 3,
             "width": 0.052, "thickness": 0.30},
    "shadow": {"blades": 9, "panicles": 2, "segments": 2,
               "width": 0.070, "thickness": 0.34},
}


def append_prism_blade(vertices, faces, uvs, *, base, yaw, height, width,
                       bend, segments, thickness_ratio):
    """Append a tapered four-sided ribbon with non-zero thickness."""
    first_ring = len(vertices)
    outward = Vector((math.cos(yaw), math.sin(yaw), 0.0))
    side = Vector((-math.sin(yaw), math.cos(yaw), 0.0))

    for segment in range(segments + 1):
        t = segment / segments
        eased = t * t * (3.0 - 2.0 * t)
        center = base + Vector((
            outward.x * bend * eased,
            outward.y * bend * eased,
            height * t,
        ))
        half_width = max(width * 0.06, width * 0.5 * (1.0 - t) ** 0.62)
        half_depth = max(
            width * 0.025,
            half_width * thickness_ratio,
        )
        depth = outward * half_depth
        vertices.extend([
            center - side * half_width - depth,
            center + side * half_width - depth,
            center + side * half_width + depth,
            center - side * half_width + depth,
        ])
        uvs.extend([
            (0.0, t), (0.33, t), (0.66, t), (1.0, t),
        ])

    for segment in range(segments):
        lower = first_ring + segment * 4
        upper = lower + 4
        for edge in range(4):
            next_edge = (edge + 1) % 4
            faces.append((
                lower + edge,
                lower + next_edge,
                upper + next_edge,
                upper + edge,
            ))
    faces.append(tuple(first_ring + edge for edge in reversed(range(4))))
    tip = first_ring + segments * 4
    faces.append(tuple(tip + edge for edge in range(4)))


def append_ellipsoid(vertices, faces, uvs, *, center, radius, height,
                     sides=8):
    """Append a coarse, coherent seed-head mass instead of hair-like strands."""
    bottom = len(vertices)
    vertices.append(center + Vector((0.0, 0.0, -height * 0.5)))
    uvs.append((0.5, 0.0))
    ring_starts = []
    for ring_index, (z, scale) in enumerate(
            ((-0.28, 0.68), (0.0, 1.0), (0.28, 0.72))):
        ring_starts.append(len(vertices))
        for side_index in range(sides):
            angle = math.tau * side_index / sides
            vertices.append(center + Vector((
                math.cos(angle) * radius * scale,
                math.sin(angle) * radius * scale,
                z * height,
            )))
            uvs.append((side_index / sides, (ring_index + 1) / 4.0))
    top = len(vertices)
    vertices.append(center + Vector((0.0, 0.0, height * 0.5)))
    uvs.append((0.5, 1.0))

    for side_index in range(sides):
        next_side = (side_index + 1) % sides
        faces.append((
            bottom,
            ring_starts[0] + next_side,
            ring_starts[0] + side_index,
        ))
    for ring_index in range(len(ring_starts) - 1):
        lower = ring_starts[ring_index]
        upper = ring_starts[ring_index + 1]
        for side_index in range(sides):
            next_side = (side_index + 1) % sides
            faces.append((
                lower + side_index,
                lower + next_side,
                upper + next_side,
                upper + side_index,
            ))
    for side_index in range(sides):
        next_side = (side_index + 1) % sides
        faces.append((
            ring_starts[-1] + side_index,
            ring_starts[-1] + next_side,
            top,
        ))


def candidate_layout(seed=0x51EED):
    rng = random.Random(seed)
    candidates = []
    golden_angle = math.pi * (3.0 - math.sqrt(5.0))
    for index in range(64):
        radial = math.sqrt((index + 0.5) / 64.0)
        angle = index * golden_angle + rng.uniform(-0.16, 0.16)
        candidates.append({
            "base": Vector((
                math.cos(angle) * radial * 0.22,
                math.sin(angle) * radial * 0.22,
                0.0,
            )),
            "yaw": angle + rng.uniform(-0.42, 0.42),
            "height": rng.uniform(0.54, 0.94) *
                      (1.0 - radial * rng.uniform(0.04, 0.20)),
            "width_scale": rng.uniform(0.78, 1.28),
            "bend": rng.uniform(0.05, 0.25) * (0.42 + radial),
            "panicle": index % 5 == 0,
        })
    return candidates


def choose_evenly(candidates, count):
    if count >= len(candidates):
        return list(candidates)
    return [
        candidates[min(len(candidates) - 1,
                       int((index + 0.5) * len(candidates) / count))]
        for index in range(count)
    ]


def make_lod_object(name, material, spec, candidates):
    vertices = []
    faces = []
    uvs = []
    chosen = choose_evenly(candidates, spec["blades"])
    for candidate in chosen:
        append_prism_blade(
            vertices, faces, uvs,
            base=candidate["base"],
            yaw=candidate["yaw"],
            height=candidate["height"],
            width=spec["width"] * candidate["width_scale"],
            bend=candidate["bend"],
            segments=spec["segments"],
            thickness_ratio=spec["thickness"],
        )

    panicle_candidates = [
        candidate for candidate in candidates if candidate["panicle"]
    ]
    for candidate in choose_evenly(
            panicle_candidates, spec["panicles"]):
        stem_height = min(1.0, candidate["height"] + 0.12)
        append_prism_blade(
            vertices, faces, uvs,
            base=candidate["base"] * 0.82,
            yaw=candidate["yaw"],
            height=stem_height,
            width=spec["width"] * 0.42,
            bend=candidate["bend"] * 0.45,
            segments=max(2, spec["segments"] - 1),
            thickness_ratio=max(0.32, spec["thickness"]),
        )
        outward = Vector((
            math.cos(candidate["yaw"]),
            math.sin(candidate["yaw"]),
            0.0,
        ))
        head_center = candidate["base"] * 0.82 + Vector((
            outward.x * candidate["bend"] * 0.45,
            outward.y * candidate["bend"] * 0.45,
            stem_height + 0.025,
        ))
        append_ellipsoid(
            vertices, faces, uvs,
            center=head_center,
            radius=spec["width"] * 1.45,
            height=spec["width"] * 4.8,
            sides=8 if name == "lod0" else 6,
        )

    mesh = bpy.data.meshes.new(f"{ASSET_NAME}_{name}_mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    uv_layer = mesh.uv_layers.new(name="UVMap")
    for loop in mesh.loops:
        uv_layer.data[loop.index].uv = uvs[loop.vertex_index]

    obj = bpy.data.objects.new(f"{ASSET_NAME}_{name}", mesh)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(material)
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    obj["game_asset"] = True
    obj["lod"] = name
    return obj


def export_selected(obj, path):
    bpy.ops.object.select_all(action="DESELECT")
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


def configure_preview(output_dir, lod0):
    for obj in bpy.context.scene.objects:
        if obj.type == "MESH":
            obj.hide_render = obj != lod0

    world = bpy.context.scene.world
    if world is None:
        world = bpy.data.worlds.new("grass_seedhead_preview_world")
        bpy.context.scene.world = world
    world.color = (0.055, 0.065, 0.08)

    camera_data = bpy.data.cameras.new("PreviewCamera")
    camera = bpy.data.objects.new("PreviewCamera", camera_data)
    bpy.context.collection.objects.link(camera)
    camera.location = (1.6, -1.8, 1.05)
    target = Vector((0.0, 0.0, 0.52))
    camera.rotation_euler = (target - camera.location).to_track_quat(
        "-Z", "Y").to_euler()
    camera_data.type = "ORTHO"
    camera_data.ortho_scale = 1.28
    bpy.context.scene.camera = camera

    key_data = bpy.data.lights.new("PreviewKey", "AREA")
    key_data.energy = 750.0
    key_data.shape = "DISK"
    key_data.size = 3.0
    key = bpy.data.objects.new("PreviewKey", key_data)
    bpy.context.collection.objects.link(key)
    key.location = (-2.0, -2.0, 3.0)

    fill_data = bpy.data.lights.new("PreviewFill", "AREA")
    fill_data.energy = 380.0
    fill_data.size = 2.0
    fill = bpy.data.objects.new("PreviewFill", fill_data)
    bpy.context.collection.objects.link(fill)
    fill.location = (2.0, 0.5, 1.5)

    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 768
    scene.render.resolution_y = 768
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = str(output_dir / "grass_seedhead_preview.png")
    scene.render.film_transparent = False
    scene.render.image_settings.color_mode = "RGBA"
    bpy.ops.render.render(write_still=True)


def main():
    arguments = sys.argv[sys.argv.index("--") + 1:]
    if len(arguments) != 2:
        raise SystemExit("Expected SOURCE_GLB OUTPUT_DIRECTORY")
    source = Path(arguments[0]).resolve()
    output_dir = Path(arguments[1]).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(source))
    source_objects = [
        obj for obj in bpy.context.scene.objects if obj.type == "MESH"
    ]
    if not source_objects or not source_objects[0].data.materials:
        raise RuntimeError("Source GLB has no reusable grass material")
    material = source_objects[0].data.materials[0].copy()
    material.name = "bq_Grass_Panicum-virgatum"
    for obj in source_objects:
        bpy.data.objects.remove(obj, do_unlink=True)

    candidates = candidate_layout()
    lod_objects = {}
    report = {
        "schema": "openai.game-vegetation-art/1",
        "asset": ASSET_NAME,
        "source": str(source),
        "method": "solid_prism_blades_and_coherent_panicles",
        "lods": {},
    }
    for lod_name, spec in LOD_SPECS.items():
        obj = make_lod_object(lod_name, material, spec, candidates)
        lod_objects[lod_name] = obj
        output_path = output_dir / f"{ASSET_NAME}_{lod_name}.glb"
        export_selected(obj, output_path)
        report["lods"][lod_name] = {
            "triangles": sum(
                max(len(polygon.vertices) - 2, 0)
                for polygon in obj.data.polygons
            ),
            "vertices": len(obj.data.vertices),
            "blades": spec["blades"],
            "panicles": spec["panicles"],
            "base_width_relative_to_height": spec["width"],
            "path": output_path.name,
        }

    master_path = output_dir / "grass_seedhead_game_master.blend"
    bpy.ops.wm.save_as_mainfile(filepath=str(master_path))
    configure_preview(output_dir, lod_objects["lod0"])
    with (output_dir / "asset_report.json").open(
            "w", encoding="utf-8") as stream:
        json.dump(report, stream, indent=2)
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
