"""Report loose-part topology for botaniq GLB vegetation assets.

Run with Blender:
  blender --background --python tools/inspect_botaniq_topology.py -- FILE.glb [...]
"""

from __future__ import annotations

import sys
from collections import Counter
from pathlib import Path

import bpy


def component_triangle_counts(mesh):
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

    mesh.calc_loop_triangles()
    for triangle in mesh.loop_triangles:
        a, b, c = triangle.vertices
        union(a, b)
        union(a, c)

    counts = Counter()
    for triangle in mesh.loop_triangles:
        counts[find(triangle.vertices[0])] += 1
    return sorted(counts.values(), reverse=True)


def main():
    paths = [Path(value) for value in sys.argv[sys.argv.index("--") + 1 :]]
    for path in paths:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.gltf(filepath=str(path.resolve()))
        print(f"TOPOLOGY {path.name}")
        for obj in bpy.context.scene.objects:
            if obj.type != "MESH":
                continue
            counts = component_triangle_counts(obj.data)
            print(
                f"  {obj.name}: triangles={sum(counts)} parts={len(counts)} "
                f"largest={counts[:8]}"
            )


if __name__ == "__main__":
    main()
