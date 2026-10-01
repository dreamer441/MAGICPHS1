import json
import math
import re
from pathlib import Path
import unreal

SOURCE_MAP = "/Game/Environment/Blockout/L_Phase1_Blockout_v005"
DEST_MAP = "/Game/Environment/Blockout/L_Phase1_Blockout_v005_RuinsKit_v002"

MANIFEST_REL = Path("SourceArt") / "Environment" / "Ruins" / "PillarKit_v002" / "manifest.json"

PILLAR_ROOT = "/Game/Environment/Ruins/Meshes/Pillars"
FRAGMENT_ROOT = "/Game/Environment/Ruins/Meshes/Fragments"
DEBRIS_ROOT = "/Game/Environment/Ruins/Meshes/Debris"

PILLAR_ASSETS = [
    "SM_RuinPillar_Intact_6m_A",
    "SM_RuinPillar_Intact_8m_A",
    "SM_RuinPillar_Hero_10m_A",
    "SM_RuinPillar_Hero_12m_A",
    "SM_RuinPillar_BrokenTop_6m_A",
    "SM_RuinPillar_BrokenTop_7m_B",
    "SM_RuinPillar_BrokenTop_9m_A",
]

DRUM_ASSETS = [
    "SM_RuinColumn_Drum_3m_A",
    "SM_RuinColumn_Drum_5m_A",
    "SM_RuinColumn_Drum_6m_B",
]

SHARD_ASSETS = [
    "SM_RuinShard_Large_A",
    "SM_RuinShard_Large_B",
    "SM_RuinShard_Medium_A",
    "SM_RuinShard_Medium_B",
    "SM_RuinShard_Small_A",
]

def log(msg):
    unreal.log("[RUINS_WORLD_V001] " + msg)

def warn(msg):
    unreal.log_warning("[RUINS_WORLD_V001] " + msg)

def fail(msg):
    raise RuntimeError("[RUINS_WORLD_V001] " + msg)

def load_asset_checked(path):
    obj = unreal.load_asset(path)
    if not obj:
        fail("Missing required asset: " + path)
    return obj

def mesh_path(name):
    if name.startswith("SM_RuinPillar_"):
        return PILLAR_ROOT + "/" + name
    if name.startswith("SM_RuinColumn_"):
        return FRAGMENT_ROOT + "/" + name
    if name.startswith("SM_RuinShard_"):
        return DEBRIS_ROOT + "/" + name
    fail("Unknown asset category: " + name)

def index_from_label(label):
    m = re.search(r"_(\d+)(?:_|$)", label)
    if m:
        return int(m.group(1))
    return 0

def abs_scale(v):
    return unreal.Vector(abs(v.x), abs(v.y), abs(v.z))

def rotate_vector(rot, vec):
    try:
        return rot.rotate_vector(vec)
    except Exception:
        # This fallback is only expected on API variants where Rotator.rotate_vector
        # is not exposed. It preserves correct Z anchoring for vertical actors.
        return vec

# -------------------------------------------------------------------------
# SAFETY / SOURCE DATA
# -------------------------------------------------------------------------

project_dir = Path(unreal.Paths.project_dir()).resolve()
manifest_path = (project_dir / MANIFEST_REL).resolve()

if not unreal.EditorAssetLibrary.does_asset_exist(SOURCE_MAP):
    fail("Source map missing: " + SOURCE_MAP)

if unreal.EditorAssetLibrary.does_asset_exist(DEST_MAP):
    fail(
        "Destination implementation map already exists. Refusing to overwrite: "
        + DEST_MAP
    )

if not manifest_path.exists():
    fail("PillarKit_v002 manifest missing: " + str(manifest_path))

with open(manifest_path, "r", encoding="utf-8") as f:
    manifest = json.load(f)

dims = {}
for item in manifest.get("assets", []):
    name = item.get("name")
    d = item.get("dimensions_m")
    if name and isinstance(d, list) and len(d) == 3:
        dims[name] = tuple(float(x) for x in d)

required = PILLAR_ASSETS + DRUM_ASSETS + SHARD_ASSETS
for name in required:
    path = mesh_path(name)
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        fail("Imported mesh missing. Run the Unreal import package first: " + path)
    if name not in dims:
        fail("Manifest has no dimensions for: " + name)

log("Duplicating saved v0.05 map so the original remains untouched.")
dup = unreal.EditorAssetLibrary.duplicate_asset(SOURCE_MAP, DEST_MAP)
if not dup:
    fail("Could not duplicate source map.")

if not unreal.EditorLevelLibrary.load_level(DEST_MAP):
    fail("Could not load duplicated map: " + DEST_MAP)

# -------------------------------------------------------------------------
# VARIANT SELECTION
# -------------------------------------------------------------------------

def choose_pillar(label, target_height_m, rotation):
    i = index_from_label(label)

    leaning = abs(rotation.pitch) > 7.0 or abs(rotation.roll) > 7.0

    if target_height_m >= 10.5:
        choices = [
            "SM_RuinPillar_Hero_12m_A",
            "SM_RuinPillar_BrokenTop_9m_A",
            "SM_RuinPillar_Hero_10m_A",
        ]
    elif target_height_m >= 8.5:
        choices = [
            "SM_RuinPillar_Hero_10m_A",
            "SM_RuinPillar_BrokenTop_9m_A",
            "SM_RuinPillar_Intact_8m_A",
        ]
    elif target_height_m >= 7.0:
        choices = [
            "SM_RuinPillar_Intact_8m_A",
            "SM_RuinPillar_BrokenTop_7m_B",
            "SM_RuinPillar_Intact_6m_A",
        ]
    else:
        choices = [
            "SM_RuinPillar_Intact_6m_A",
            "SM_RuinPillar_BrokenTop_6m_A",
            "SM_RuinPillar_BrokenTop_7m_B",
        ]

    # Leaning pillars should usually read as damaged.
    if leaning:
        broken = [c for c in choices if "BrokenTop" in c]
        if broken:
            return broken[i % len(broken)]

    return choices[i % len(choices)]

def choose_shard(label):
    i = index_from_label(label)
    return SHARD_ASSETS[i % len(SHARD_ASSETS)]

def choose_drum(label):
    i = index_from_label(label)
    return DRUM_ASSETS[(i // 5) % len(DRUM_ASSETS)]

# -------------------------------------------------------------------------
# REPLACEMENT HELPERS
# -------------------------------------------------------------------------

def set_mesh(actor, mesh):
    if not isinstance(actor, unreal.StaticMeshActor):
        fail("Expected StaticMeshActor: " + actor.get_actor_label())
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.static_mesh_component.set_collision_enabled(
        unreal.CollisionEnabled.QUERY_AND_PHYSICS
    )

def replace_pillar(actor):
    label = actor.get_actor_label()

    old_loc = actor.get_actor_location()
    old_rot = actor.get_actor_rotation()
    old_scale = abs_scale(actor.get_actor_scale3d())

    # Original basic cylinder is ~1m tall / 1m diameter, so its actor scale
    # directly represents the current desired world dimensions in meters.
    target_x_m = max(0.20, old_scale.x)
    target_y_m = max(0.20, old_scale.y)
    target_z_m = max(0.50, old_scale.z)

    selected = choose_pillar(label, target_z_m, old_rot)
    source_x_m, source_y_m, source_z_m = dims[selected]

    new_scale = unreal.Vector(
        target_x_m / max(source_x_m, 0.001),
        target_y_m / max(source_y_m, 0.001),
        target_z_m / max(source_z_m, 0.001),
    )

    # Current placeholder cylinder pivot is at its center.
    # Production pillar pivot is at its base. Recover the original local base
    # position so manual location/rotation work is preserved.
    local_base_offset = unreal.Vector(0.0, 0.0, -target_z_m * 50.0)
    world_offset = rotate_vector(old_rot, local_base_offset)
    base_loc = unreal.Vector(
        old_loc.x + world_offset.x,
        old_loc.y + world_offset.y,
        old_loc.z + world_offset.z,
    )

    mesh = load_asset_checked(mesh_path(selected))
    set_mesh(actor, mesh)
    actor.set_actor_scale3d(new_scale)
    actor.set_actor_rotation(old_rot, False)
    actor.set_actor_location(base_loc, False, False)

    return selected

def replace_shard(actor):
    label = actor.get_actor_label()

    old_loc = actor.get_actor_location()
    old_rot = actor.get_actor_rotation()
    old_scale = abs_scale(actor.get_actor_scale3d())

    # Original rock placeholder is the 1m Engine cube, therefore the three
    # actor-scale values directly encode the intended local dimensions in m.
    target_x_m = max(0.50, old_scale.x)
    target_y_m = max(0.50, old_scale.y)
    target_z_m = max(0.40, old_scale.z)

    selected = choose_shard(label)
    source_x_m, source_y_m, source_z_m = dims[selected]

    new_scale = unreal.Vector(
        target_x_m / max(source_x_m, 0.001),
        target_y_m / max(source_y_m, 0.001),
        target_z_m / max(source_z_m, 0.001),
    )

    # Cube pivot was centered; shard pivot sits at ground/base.
    bottom_z = old_loc.z - target_z_m * 50.0
    new_loc = unreal.Vector(old_loc.x, old_loc.y, bottom_z)

    mesh = load_asset_checked(mesh_path(selected))
    set_mesh(actor, mesh)
    actor.set_actor_scale3d(new_scale)
    actor.set_actor_rotation(old_rot, False)
    actor.set_actor_location(new_loc, False, False)

    return selected

def replace_with_fallen_drum(actor):
    label = actor.get_actor_label()

    old_loc = actor.get_actor_location()
    old_rot = actor.get_actor_rotation()
    old_scale = abs_scale(actor.get_actor_scale3d())

    target_x_m = max(0.50, old_scale.x)
    target_y_m = max(0.50, old_scale.y)
    target_z_m = max(0.40, old_scale.z)

    target_length_m = max(target_x_m, target_y_m)
    target_thickness_m = max(
        1.0,
        min(target_z_m, min(target_x_m, target_y_m) * 0.70)
    )

    selected = choose_drum(label)
    source_x_m, source_y_m, source_z_m = dims[selected]

    # Drum source length is along local Z. Rotate it onto the ground.
    new_scale = unreal.Vector(
        target_thickness_m / max(source_x_m, 0.001),
        target_thickness_m / max(source_y_m, 0.001),
        target_length_m / max(source_z_m, 0.001),
    )

    yaw = old_rot.yaw
    new_rot = unreal.Rotator(90.0, yaw, 0.0)

    # Keep the fallen drum centered around the old rock placeholder.
    half_length_cm = target_length_m * 50.0
    center_offset = rotate_vector(
        new_rot,
        unreal.Vector(0.0, 0.0, half_length_cm)
    )

    bottom_z = old_loc.z - target_z_m * 50.0
    new_loc = unreal.Vector(
        old_loc.x - center_offset.x,
        old_loc.y - center_offset.y,
        bottom_z + target_thickness_m * 50.0 - center_offset.z,
    )

    mesh = load_asset_checked(mesh_path(selected))
    set_mesh(actor, mesh)
    actor.set_actor_scale3d(new_scale)
    actor.set_actor_rotation(new_rot, False)
    actor.set_actor_location(new_loc, False, False)

    return selected

# -------------------------------------------------------------------------
# EXECUTE ON STATE 2 ONLY
# -------------------------------------------------------------------------

actors = unreal.EditorLevelLibrary.get_all_level_actors()

pillar_actors = []
rock_actors = []

for actor in actors:
    label = actor.get_actor_label()
    if label.startswith("S2_Pillar_"):
        pillar_actors.append(actor)
    elif label.startswith("S2_Rock_"):
        rock_actors.append(actor)

if not pillar_actors:
    fail("No S2_Pillar_* actors were found in the duplicated v0.05 map.")

if not rock_actors:
    warn("No S2_Rock_* actors were found; only pillars will be replaced.")

pillar_actors.sort(key=lambda a: a.get_actor_label())
rock_actors.sort(key=lambda a: a.get_actor_label())

pillar_counts = {}
for actor in pillar_actors:
    selected = replace_pillar(actor)
    pillar_counts[selected] = pillar_counts.get(selected, 0) + 1

shard_count = 0
drum_count = 0

for actor in rock_actors:
    i = index_from_label(actor.get_actor_label())

    # About 20% of the old blockout rocks become fallen architectural
    # column sections. The rest become irregular ruin shards.
    if i % 5 == 0:
        replace_with_fallen_drum(actor)
        drum_count += 1
    else:
        replace_shard(actor)
        shard_count += 1

if not unreal.EditorLevelLibrary.save_current_level():
    fail("Replacement succeeded in memory, but duplicated map could not be saved.")

log("State 2 replacement complete.")
log("Pillars replaced: {}".format(len(pillar_actors)))
log("Rock placeholders -> shards: {}".format(shard_count))
log("Rock placeholders -> fallen drums: {}".format(drum_count))

for name in sorted(pillar_counts):
    log("  {} x {}".format(pillar_counts[name], name))

log("Unchanged: State 1, State 3, State 4, connectors, and blockout arches.")
log("Original v0.05 remains untouched: " + SOURCE_MAP)
log("Implementation map: " + DEST_MAP)
log("RUINS WORLD PLACEMENT V001 SUCCEEDED")
