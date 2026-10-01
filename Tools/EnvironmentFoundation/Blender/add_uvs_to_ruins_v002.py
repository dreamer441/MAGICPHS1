import bpy
import sys
import json
from pathlib import Path

argv = sys.argv
if "--" not in argv:
    raise RuntimeError("Expected project directory after '--'.")

args = argv[argv.index("--") + 1:]
if not args:
    raise RuntimeError("Missing project directory.")

PROJECT = Path(args[0]).resolve()
SRC_BLEND = PROJECT / "SourceArt" / "Environment" / "Ruins" / "PillarKit_v002" / "RuinsPillarKit_v002.blend"
OUT = PROJECT / "SourceArt" / "Environment" / "Ruins" / "PillarKit_v002_UV"
FBX_DIR = OUT / "FBX"

if not SRC_BLEND.exists():
    raise RuntimeError(f"Missing v002 Blender source library: {SRC_BLEND}")

if OUT.exists():
    raise RuntimeError(f"UV output already exists. Refusing overwrite: {OUT}")

FBX_DIR.mkdir(parents=True, exist_ok=True)

bpy.ops.wm.open_mainfile(filepath=str(SRC_BLEND))

mesh_objects = [
    obj for obj in bpy.data.objects
    if obj.type == 'MESH' and obj.name.startswith("SM_Ruin")
]

if not mesh_objects:
    raise RuntimeError("No SM_Ruin* mesh objects found in v002 .blend file.")

manifest = {
    "source": str(SRC_BLEND),
    "output": str(OUT),
    "asset_count": len(mesh_objects),
    "assets": []
}

for obj in mesh_objects:
    # Add/replace primary UV map without changing geometry.
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    # Smart unwrap the complete joined mesh.
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    try:
        bpy.ops.uv.smart_project(
            angle_limit=1.15192,  # ~66 degrees
            island_margin=0.025,
            area_weight=0.35,
            correct_aspect=True,
            scale_to_bounds=True
        )
    except TypeError:
        # Blender API fallback for builds with fewer keyword arguments.
        bpy.ops.uv.smart_project(
            angle_limit=1.15192,
            island_margin=0.025
        )
    bpy.ops.object.mode_set(mode='OBJECT')

    # Save with the UVs embedded, but export with the same zeroed transform
    # convention used by PillarKit_v002.
    old_loc = obj.location.copy()
    old_rot = obj.rotation_euler.copy()
    old_scale = obj.scale.copy()

    obj.location = (0, 0, 0)
    obj.rotation_euler = (0, 0, 0)
    obj.scale = (1, 1, 1)

    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    out_fbx = FBX_DIR / f"{obj.name}.fbx"

    bpy.ops.export_scene.fbx(
        filepath=str(out_fbx),
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

    manifest["assets"].append({
        "name": obj.name,
        "fbx": str(out_fbx),
        "uv_layers": [uv.name for uv in obj.data.uv_layers]
    })

uv_blend = OUT / "RuinsPillarKit_v002_UV.blend"
bpy.ops.wm.save_as_mainfile(filepath=str(uv_blend))

with open(OUT / "manifest.json", "w", encoding="utf-8") as f:
    json.dump(manifest, f, indent=2)

print("")
print("==================================================")
print(" RUINS PILLAR KIT v002 UV PATCH COMPLETE")
print("==================================================")
print("Source:", SRC_BLEND)
print("Output:", OUT)
print("FBX:", FBX_DIR)
print("Assets:", len(mesh_objects))
print("==================================================")
