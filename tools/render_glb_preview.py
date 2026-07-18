"""Render a neutral preview of a GLB without modifying the source asset.

Run with Blender:
  blender --background --python tools/render_glb_preview.py -- INPUT.glb OUTPUT.png
"""

from __future__ import annotations

import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector


def look_at(obj, target):
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat("-Z", "Y").to_euler()


def scene_bounds(objects):
    corners = [
        obj.matrix_world @ Vector(corner)
        for obj in objects
        for corner in obj.bound_box
    ]
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


def main():
    args = sys.argv[sys.argv.index("--") + 1 :]
    if len(args) != 2:
        raise SystemExit("Expected INPUT.glb OUTPUT.png")

    input_path = Path(args[0]).resolve()
    output_path = Path(args[1]).resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)

    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(input_path))
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if not meshes:
        raise RuntimeError(f"No mesh objects found in {input_path}")

    minimum, maximum = scene_bounds(meshes)
    center = (minimum + maximum) * 0.5
    extent = maximum - minimum
    radius = max(extent.x, extent.y, extent.z) * 0.5

    world = bpy.data.worlds.new("NeutralPreviewWorld")
    world.color = (0.055, 0.065, 0.08)
    bpy.context.scene.world = world

    camera_data = bpy.data.cameras.new("PreviewCamera")
    camera = bpy.data.objects.new("PreviewCamera", camera_data)
    bpy.context.collection.objects.link(camera)
    camera_data.lens = 58
    camera.location = center + Vector((1.35, -1.75, 0.78)).normalized() * radius * 3.15
    look_at(camera, center + Vector((0.0, 0.0, extent.z * 0.03)))
    bpy.context.scene.camera = camera

    key_data = bpy.data.lights.new("Key", "AREA")
    key_data.energy = 950
    key_data.shape = "DISK"
    key_data.size = radius * 2.8
    key = bpy.data.objects.new("Key", key_data)
    bpy.context.collection.objects.link(key)
    key.location = center + Vector((-1.5, -1.4, 2.4)).normalized() * radius * 3.2
    look_at(key, center)

    fill_data = bpy.data.lights.new("Fill", "AREA")
    fill_data.energy = 420
    fill_data.size = radius * 2.4
    fill = bpy.data.objects.new("Fill", fill_data)
    bpy.context.collection.objects.link(fill)
    fill.location = center + Vector((1.8, 0.6, 1.2)).normalized() * radius * 2.8
    look_at(fill, center)

    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = 768
    scene.render.resolution_y = 768
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.film_transparent = False
    scene.render.filepath = str(output_path)
    scene.render.image_settings.color_depth = "8"
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.render.fps = 24
    scene.camera.data.dof.use_dof = False
    scene.render.image_settings.compression = 15

    bpy.ops.render.render(write_still=True)
    print(
        f"Rendered {input_path.name}: "
        f"bounds=({extent.x:.3f}, {extent.y:.3f}, {extent.z:.3f}), "
        f"output={output_path}"
    )


if __name__ == "__main__":
    main()
