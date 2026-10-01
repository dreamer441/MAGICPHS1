import bpy
import math
import random
import json
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# RUINS PILLAR KIT v0.02
# Blender 5.2.x
#
# Changes from v0.01:
#   - thicker multi-layered bases and capitals
#   - clearer stepped transitions
#   - broken tops shifted from soft/crumpled to snapped vertical-slice language
#   - same overall family, scale range, and clean early-production style
# ---------------------------------------------------------------------------

argv = sys.argv
if "--" not in argv:
    raise RuntimeError("Expected output directory after '--'.")

args = argv[argv.index("--") + 1:]
if not args:
    raise RuntimeError("Missing output directory.")

OUT = Path(args[0]).resolve()
FBX_DIR = OUT / "FBX"
OUT.mkdir(parents=True, exist_ok=True)
FBX_DIR.mkdir(parents=True, exist_ok=True)

# ---------- scene ----------
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.length_unit = 'METERS'
scene.unit_settings.scale_length = 1.0

# ---------- material ----------
mat = bpy.data.materials.new("M_Preview_AncientStone")
mat.diffuse_color = (0.48, 0.46, 0.42, 1.0)
mat.use_nodes = True
bsdf = mat.node_tree.nodes.get("Principled BSDF")
if bsdf:
    bsdf.inputs["Base Color"].default_value = (0.46, 0.44, 0.40, 1.0)
    bsdf.inputs["Roughness"].default_value = 0.72

# ---------- helpers ----------
def clear_selection():
    bpy.ops.object.select_all(action='DESELECT')


def add_bevel(obj, width=0.035, segments=2):
    mod = obj.modifiers.new(name="SoftStoneEdges", type='BEVEL')
    mod.width = width
    mod.segments = segments
    mod.limit_method = 'ANGLE'
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=mod.name)


def set_smooth(obj):
    if obj.type == 'MESH':
        for p in obj.data.polygons:
            p.use_smooth = True


def assign_material(obj):
    if obj.type == 'MESH' and len(obj.data.materials) == 0:
        obj.data.materials.append(mat)


def origin_to_base(obj):
    scene.cursor.location = (0.0, 0.0, 0.0)
    clear_selection()
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR', center='MEDIAN')


def join_parts(parts, name):
    clear_selection()
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    assign_material(obj)
    return obj


def add_cylinder_part(name, radius, depth, z, vertices=48):
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=vertices,
        radius=radius,
        depth=depth,
        location=(0, 0, z)
    )
    obj = bpy.context.object
    obj.name = name
    assign_material(obj)
    add_bevel(obj, width=min(0.05, radius * 0.06), segments=2)
    return obj


def add_square_plinth(name, size, height, z):
    bpy.ops.mesh.primitive_cube_add(
        location=(0, 0, z),
        scale=(size * 0.5, size * 0.5, height * 0.5)
    )
    obj = bpy.context.object
    obj.name = name
    assign_material(obj)
    add_bevel(obj, width=min(0.08, height * 0.12), segments=2)
    return obj


def create_vertical_slice_profile(radial_segments, flute_count, seed, break_depth):
    """
    Generates a top fracture profile that feels like snapped vertical slices
    instead of a melted/crumpled tube.
    """
    rng = random.Random(seed)
    slice_count = flute_count * 2  # enough density to echo fluting
    slice_values = []

    base_levels = [0.02, 0.07, 0.13, 0.20, 0.28, 0.36, 0.46, 0.56]
    for i in range(slice_count):
        level = rng.choice(base_levels)
        # occasional taller surviving tooth / lower cleft
        if rng.random() < 0.18:
            level *= 0.45
        if rng.random() < 0.16:
            level *= 1.22
        # a tiny alternating rhythm helps make it read as snapped slices
        if i % 2 == 0:
            level *= 0.92
        else:
            level *= 1.04
        level = max(0.0, min(level, 0.85))
        slice_values.append(-break_depth * level)

    offsets = []
    for r_i in range(radial_segments):
        t = r_i / radial_segments
        idx = min(slice_count - 1, int(t * slice_count))
        offset = slice_values[idx]

        # Slightly emphasize groove positions so the broken top reads as fluted slices.
        theta = 2.0 * math.pi * t
        groove = 0.5 + 0.5 * math.cos(flute_count * theta)
        offset -= groove * break_depth * 0.045

        # A very small notch noise, still sharp/quantized overall.
        offset += rng.uniform(-0.015, 0.015) * break_depth
        offsets.append(offset)

    return offsets


def create_fluted_shaft(name, height, radius, seed, broken_top=False,
                        break_depth=0.40, radial_segments=48,
                        z_segments=10, flute_count=12):
    verts = []
    faces = []

    if broken_top:
        top_offsets = create_vertical_slice_profile(
            radial_segments=radial_segments,
            flute_count=flute_count,
            seed=seed,
            break_depth=break_depth,
        )
    else:
        top_offsets = [0.0] * radial_segments

    for z_i in range(z_segments + 1):
        t = z_i / z_segments
        z = height * t
        taper = 1.0 - 0.055 * t

        for r_i in range(radial_segments):
            theta = 2.0 * math.pi * r_i / radial_segments
            flute = 1.0 - 0.028 * (0.5 + 0.5 * math.cos(flute_count * theta))
            micro = 1.0 + 0.003 * math.sin(theta * 7.0 + seed)
            rr = radius * taper * flute * micro

            zz = z
            if broken_top and z_i == z_segments:
                zz += top_offsets[r_i]
            elif broken_top and z_i == z_segments - 1:
                # keep the final ring slightly influenced so breaks read as vertical slices
                zz += top_offsets[r_i] * 0.08

            verts.append((rr * math.cos(theta), rr * math.sin(theta), zz))

    for z_i in range(z_segments):
        for r_i in range(radial_segments):
            a = z_i * radial_segments + r_i
            b = z_i * radial_segments + (r_i + 1) % radial_segments
            c = (z_i + 1) * radial_segments + (r_i + 1) % radial_segments
            d = (z_i + 1) * radial_segments + r_i
            faces.append((a, b, c, d))

    bottom_center = len(verts)
    verts.append((0.0, 0.0, 0.0))
    for r_i in range(radial_segments):
        a = r_i
        b = (r_i + 1) % radial_segments
        faces.append((bottom_center, b, a))

    top_ring_start = z_segments * radial_segments
    top_center_z = height + (sum(top_offsets) / len(top_offsets))
    top_center = len(verts)
    verts.append((0.0, 0.0, top_center_z))
    for r_i in range(radial_segments):
        a = top_ring_start + r_i
        b = top_ring_start + (r_i + 1) % radial_segments
        faces.append((top_center, a, b))

    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update()

    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    assign_material(obj)
    set_smooth(obj)
    return obj


def create_base_stack(name, radius):
    parts = []
    h1 = max(0.18, radius * 0.18)
    h2 = max(0.18, radius * 0.17)
    h3 = max(0.16, radius * 0.15)
    h4 = max(0.16, radius * 0.15)

    z = h1 * 0.5
    parts.append(add_square_plinth(f"{name}_BaseSlab_A", size=radius * 3.10, height=h1, z=z))
    z += (h1 * 0.5 + h2 * 0.5)
    parts.append(add_square_plinth(f"{name}_BaseSlab_B", size=radius * 2.75, height=h2, z=z))
    z += (h2 * 0.5 + h3 * 0.5)
    parts.append(add_square_plinth(f"{name}_BaseSlab_C", size=radius * 2.35, height=h3, z=z))
    z += (h3 * 0.5 + h4 * 0.5)
    parts.append(add_cylinder_part(f"{name}_BaseCollar", radius=radius * 1.18, depth=h4, z=z))

    base_total = h1 + h2 + h3 + h4
    return parts, base_total


def create_capital_stack(name, radius, top_of_shaft):
    parts = []

    h0 = max(0.16, radius * 0.15)
    h1 = max(0.17, radius * 0.16)
    h2 = max(0.17, radius * 0.16)
    h3 = max(0.19, radius * 0.18)

    z = top_of_shaft - h0 * 0.55
    parts.append(add_cylinder_part(f"{name}_CapitalCollar", radius=radius * 1.16, depth=h0, z=z))
    z = top_of_shaft + h1 * 0.50
    parts.append(add_square_plinth(f"{name}_CapitalSlab_A", size=radius * 2.35, height=h1, z=z))
    z += (h1 * 0.5 + h2 * 0.5)
    parts.append(add_square_plinth(f"{name}_CapitalSlab_B", size=radius * 2.65, height=h2, z=z))
    z += (h2 * 0.5 + h3 * 0.5)
    parts.append(add_square_plinth(f"{name}_CapitalSlab_C", size=radius * 3.05, height=h3, z=z))

    return parts


def create_pillar(name, height, radius, seed, broken_top=False, with_capital=True, with_base=True):
    parts = []

    base_total = 0.0
    if with_base:
        base_parts, base_total = create_base_stack(name, radius)
        parts.extend(base_parts)

    shaft = create_fluted_shaft(
        name + "_Shaft",
        height=height,
        radius=radius,
        seed=seed,
        broken_top=broken_top,
        break_depth=max(0.28, min(0.68, radius * 0.55))
    )
    shaft.location.z = base_total
    parts.append(shaft)

    # Subtle carved bands: preserved from v0.01 family.
    for band_i, t in enumerate((0.20, 0.74)):
        parts.append(add_cylinder_part(
            f"{name}_Band_{band_i}",
            radius=radius * 1.06,
            depth=max(0.11, radius * 0.12),
            z=base_total + height * t
        ))

    if with_capital and not broken_top:
        parts.extend(create_capital_stack(name, radius, base_total + height))

    obj = join_parts(parts, name)

    clear_selection()
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    origin_to_base(obj)
    return obj


def create_irregular_shard(name, dimensions, seed):
    rng = random.Random(seed)
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=1.0, location=(0,0,0))
    obj = bpy.context.object
    obj.name = name

    sx, sy, sz = dimensions
    for v in obj.data.vertices:
        co = v.co
        jitter = 1.0 + rng.uniform(-0.24, 0.24)
        co.x = (co.x * sx * 0.5 * jitter) + co.z * rng.uniform(-0.20, 0.20)
        co.y = (co.y * sy * 0.5 * jitter) + co.x * rng.uniform(-0.10, 0.10)
        co.z = (co.z * sz * 0.5 * jitter) + rng.uniform(-0.08, 0.08) * sz

    min_z = min(v.co.z for v in obj.data.vertices)
    for v in obj.data.vertices:
        v.co.z -= min_z

    assign_material(obj)
    set_smooth(obj)
    add_bevel(obj, width=0.025, segments=1)
    origin_to_base(obj)
    return obj


def create_broken_drum(name, length, radius, seed):
    obj = create_fluted_shaft(
        name,
        height=length,
        radius=radius,
        seed=seed,
        broken_top=True,
        break_depth=max(0.25, radius * 0.48),
        radial_segments=48,
        z_segments=6,
        flute_count=12
    )
    band = add_cylinder_part(
        name + "_Band",
        radius=radius * 1.07,
        depth=max(0.10, radius * 0.11),
        z=length * 0.43
    )
    obj = join_parts([obj, band], name)
    origin_to_base(obj)
    return obj


def export_fbx(obj, filepath):
    old_loc = obj.location.copy()
    old_rot = obj.rotation_euler.copy()
    old_scale = obj.scale.copy()

    obj.location = (0,0,0)
    obj.rotation_euler = (0,0,0)
    obj.scale = (1,1,1)

    clear_selection()
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    bpy.ops.export_scene.fbx(
        filepath=str(filepath),
        use_selection=True,
        apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_ALL',
        object_types={'MESH'},
        use_mesh_modifiers=True,
        mesh_smooth_type='FACE',
        add_leaf_bones=False,
        bake_anim=False,
        axis_forward='-Z',
        axis_up='Y'
    )

    obj.location = old_loc
    obj.rotation_euler = old_rot
    obj.scale = old_scale


def mesh_stats(obj):
    dims = obj.dimensions
    return {
        "name": obj.name,
        "dimensions_m": [round(dims.x, 3), round(dims.y, 3), round(dims.z, 3)],
        "vertices": len(obj.data.vertices),
        "polygons": len(obj.data.polygons)
    }


assets = []

specs = [
    ("SM_RuinPillar_Intact_6m_A",       6.0, 0.62, 101, False, True, True),
    ("SM_RuinPillar_Intact_8m_A",       8.0, 0.72, 102, False, True, True),
    ("SM_RuinPillar_Hero_10m_A",       10.0, 0.88, 103, False, True, True),
    ("SM_RuinPillar_Hero_12m_A",       12.0, 0.98, 104, False, True, True),
    ("SM_RuinPillar_BrokenTop_6m_A",    6.2, 0.68, 201, True,  False, True),
    ("SM_RuinPillar_BrokenTop_7m_B",    7.3, 0.74, 202, True,  False, True),
    ("SM_RuinPillar_BrokenTop_9m_A",    9.1, 0.86, 203, True,  False, True),
    ("SM_RuinPillar_Stump_3m_A",        3.0, 0.76, 204, True,  False, True),
]

for spec in specs:
    obj = create_pillar(
        name=spec[0],
        height=spec[1],
        radius=spec[2],
        seed=spec[3],
        broken_top=spec[4],
        with_capital=spec[5],
        with_base=spec[6]
    )
    assets.append(obj)

for name, length, radius, seed in [
    ("SM_RuinColumn_Drum_3m_A", 3.0, 0.72, 301),
    ("SM_RuinColumn_Drum_5m_A", 5.0, 0.82, 302),
    ("SM_RuinColumn_Drum_6m_B", 6.0, 0.90, 303),
]:
    assets.append(create_broken_drum(name, length, radius, seed))

for name, dims, seed in [
    ("SM_RuinShard_Large_A",  (4.5, 3.1, 2.4), 401),
    ("SM_RuinShard_Large_B",  (5.2, 2.6, 3.0), 402),
    ("SM_RuinShard_Medium_A", (2.8, 2.0, 1.7), 403),
    ("SM_RuinShard_Medium_B", (3.3, 1.8, 2.2), 404),
    ("SM_RuinShard_Small_A",  (1.6, 1.2, 0.9), 405),
]:
    assets.append(create_irregular_shard(name, dims, seed))

manifest = {
    "kit": "RuinsPillarKit_v002",
    "blender_version": bpy.app.version_string,
    "units": "meters",
    "asset_count": len(assets),
    "notes": [
        "Thicker multi-layer bases/capitals.",
        "Broken tops revised toward snapped vertical slices.",
        "Same family/style as v001, with only targeted geometry fixes."
    ],
    "assets": []
}

for obj in assets:
    export_fbx(obj, FBX_DIR / f"{obj.name}.fbx")
    manifest["assets"].append(mesh_stats(obj))

spacing_x = 5.5
spacing_y = 7.0
for i, obj in enumerate(assets):
    col = i % 5
    row = i // 5
    obj.location = (col * spacing_x, -row * spacing_y, 0.0)

for i, obj in enumerate(assets):
    col = i % 5
    row = i // 5
    bpy.ops.object.text_add(location=(col*spacing_x, -row*spacing_y - 2.2, 0.05))
    txt = bpy.context.object
    txt.data.body = obj.name.replace("SM_Ruin", "")
    txt.data.size = 0.28
    txt.data.extrude = 0.002

blend_path = OUT / "RuinsPillarKit_v002.blend"
bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))

with open(OUT / "manifest.json", "w", encoding="utf-8") as f:
    json.dump(manifest, f, indent=2)

print("")
print("==================================================")
print(" RUINS PILLAR KIT v0.02 GENERATED")
print("==================================================")
print("Output:", OUT)
print("Blend :", blend_path)
print("FBX   :", FBX_DIR)
print("Assets:", len(assets))
print("==================================================")
