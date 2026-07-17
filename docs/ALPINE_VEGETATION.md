# Alpine Vegetation System (OpenGL 3.3)

## Runtime data flow

`TerrainMesh` owns the immutable height field and Terrain Data Map (TDM). At
startup, `AlpineVegetationSystem` samples the rendered surface, slope, aspect,
curvature, water distance and shared CPU biome classifier. It creates
deterministic 32-byte instances, sorts each species into 256 m chunks, uploads
one static instance VBO per species, and writes a binary cache keyed by both
terrain and vegetation settings.

The renderer uses OpenGL 3.3 instanced attributes only:

- location 5: `vec4 posScale` (world position and uniform scale)
- location 6: `vec4 rotColor` (yaw, tilt, tilt azimuth, packed variation)

There are no SSBOs, compute shaders, runtime insertions, or GPU distribution.
Near chunk ranges are drawn by resetting the instance attribute offset before
`glDrawElementsInstanced`, which is the 3.3 replacement for base-instance
drawing. All instances remain resident: beyond the geometric range, each
species is emitted in one `GL_POINTS` instanced draw as a circular point sprite.
The point path performs no wind evaluation and writes zero object velocity.
Grass and flowers are two-sided; tree shadow chunks use the silhouette-matched
ultra-low LOD and are clipped at 300 m.

The 60-call geometry budget reserves 47 near-geometry draws plus thirteen
whole-map point-sprite draws. Near slots are scheduled fairly per species
before remaining slots are filled by normalized camera distance. A dense
tall-tree bucket can therefore never starve broad trees, saplings, grass or
flowers.

## Mesh library

`AlpineVegetationMeshFactory` generates thirteen deterministic mesh families:

- 45 m tall, 34 m broad and 18 m sapling conifers with irregular foliage tiers;
- round and wind-swept shrubs built from overlapping warped low-poly lobes;
- upright, fan and wind-swept grass tufts made from curved solid leaves;
- large, saturated star, bell and spike flowers;
- compact and elongated alpine cushion plants.

Each family provides LOD0, LOD1, LOD2 and shadow geometry where applicable.
The vertex format stores linear base color, roughness, wind weight and stable
shape/color variation weight. Instance-world hashing changes the silhouette
without increasing instance memory and is evaluated identically by geometry
and shadow shaders.

## Distribution rules and future interfaces

Trees and shrubs use minimum-distance rejection (6 m and 3 m). Grass and flower
budgets use low-discrepancy jittered candidates. `alpineBiomeWeights` mirrors
`terrain_biomes.glsl`; startup runs a deterministic 1000-sample parity check
with a `1e-3` failure threshold. Tree line shifts with sun-facing aspect, all
species reject submerged points, and water distance/curvature modulate flowers.

Grass uses a 75% biome-wide base layer and 25% habitat-patch modulation across
72 overlapping deterministic meadow domains. Flowers use a 25% biome-wide base
and 75% overlapping habitat fields, including water/valley weighting. This
produces continuous grassland and visible flower seas without restricting them
to a small preview patch.

`sampleDensityField(worldXZ)` is intentionally the only clustering interface.
It currently returns threshold-softened 150-300 m value noise. A later moisture
stage can replace the function with a texture sample without changing the
distribution pipeline. Moisture-driven flowers, accumulated sunlight, runtime
vegetation changes, GPU distribution, and wind-field textures remain explicitly
out of scope until the OpenGL 4.3 migration.

## Temporal rendering and terrain coupling

`TerrainMesh::sampleSurface()` is the only attachment contract. It reconstructs
the exact 256 x 256 render triangle (`a-c-b` or `b-c-d`) under a world XZ point
and returns that plane's height and normal together with TDM and hydrology.
Sampling the denser 1024 x 1024 source height field directly is intentionally
forbidden for attachment because it can differ from a rendered steep face by
several metres. Roots are then sunk slightly below the reconstructed plane.

The geometry pass explicitly clears and populates terrain/model depth before
vegetation. It then restates depth test, depth write, `GL_LESS`, full color mask,
default depth range and disabled blending at the vegetation boundary. Hidden
tree triangles and far point sprites therefore fail against the same populated
GBuffer depth surface.

Terrain hydrology currently authors seven coherent regions: two primary lakes,
two additional alpine tarns (haizi), and three long, procedurally meandered
grassland river reaches. Height sculpting, water mesh regions and water-distance
vegetation modulation all consume the same signed-distance definitions.

Wind is evaluated at current and previous time. Vegetation writes an RG16F
object-motion delta to the GBuffer; TAA adds that delta to its existing camera
reprojection, avoiding dynamic foliage ghosting without double-counting camera
motion. A 256 x 256 startup-baked grass density texture darkens only the terrain
grass biome from 1.0 to 0.9, supplying inexpensive contact/self-occlusion while
grass and flowers remain outside the shadow pass.

Run `fbo_demo.exe --fuji-preview` from the build directory for a clean Fuji
visual-validation view without the ImGui panel.
