import os
from pathlib import Path
import unreal

KIT_NAME = "RuinsPillarKit_v002"
SOURCE_REL = Path("SourceArt") / "Environment" / "Ruins" / "PillarKit_v002" / "FBX"

DEST_ROOT = "/Game/Environment/Ruins/Meshes"
DEST_PILLARS = DEST_ROOT + "/Pillars"
DEST_FRAGMENTS = DEST_ROOT + "/Fragments"
DEST_DEBRIS = DEST_ROOT + "/Debris"

PREVIEW_MAP = "/Game/Environment/Ruins/Preview/L_RuinsPillarKit_v002_Preview"

EXPECTED_NAMES = [
    "SM_RuinPillar_Intact_6m_A",
    "SM_RuinPillar_Intact_8m_A",
    "SM_RuinPillar_Hero_10m_A",
    "SM_RuinPillar_Hero_12m_A",
    "SM_RuinPillar_BrokenTop_6m_A",
    "SM_RuinPillar_BrokenTop_7m_B",
    "SM_RuinPillar_BrokenTop_9m_A",
    "SM_RuinPillar_Stump_3m_A",
    "SM_RuinColumn_Drum_3m_A",
    "SM_RuinColumn_Drum_5m_A",
    "SM_RuinColumn_Drum_6m_B",
    "SM_RuinShard_Large_A",
    "SM_RuinShard_Large_B",
    "SM_RuinShard_Medium_A",
    "SM_RuinShard_Medium_B",
    "SM_RuinShard_Small_A",
]

def log(msg):
    unreal.log("[RUINS_IMPORT_V001] " + msg)

def warn(msg):
    unreal.log_warning("[RUINS_IMPORT_V001] " + msg)

def fail(msg):
    raise RuntimeError("[RUINS_IMPORT_V001] " + msg)

def destination_for(name):
    if name.startswith("SM_RuinPillar_"):
        return DEST_PILLARS
    if name.startswith("SM_RuinColumn_Drum_"):
        return DEST_FRAGMENTS
    if name.startswith("SM_RuinShard_"):
        return DEST_DEBRIS
    fail("Unknown mesh naming category: " + name)

def asset_path_for(name):
    return destination_for(name) + "/" + name

project_dir = Path(unreal.Paths.project_dir()).resolve()
source_dir = (project_dir / SOURCE_REL).resolve()

log("Project: " + str(project_dir))
log("Source FBX folder: " + str(source_dir))

if not source_dir.exists():
    fail(
        "FBX source folder is missing. Generate PillarKit_v002 first: "
        + str(source_dir)
    )

fbx_files = sorted(source_dir.glob("*.fbx"))
found_names = [p.stem for p in fbx_files]

missing = [n for n in EXPECTED_NAMES if n not in found_names]
unexpected = [n for n in found_names if n not in EXPECTED_NAMES]

if missing:
    fail("Missing expected FBX files: " + ", ".join(missing))

if unexpected:
    warn("Unexpected FBX files will NOT be imported: " + ", ".join(unexpected))

# Safety: never overwrite the current import or preview map.
existing_assets = [
    asset_path_for(name)
    for name in EXPECTED_NAMES
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path_for(name))
]
if existing_assets:
    fail(
        "Target assets already exist. Refusing to overwrite:\n  "
        + "\n  ".join(existing_assets)
    )

if unreal.EditorAssetLibrary.does_asset_exist(PREVIEW_MAP):
    fail("Preview map already exists. Refusing to overwrite: " + PREVIEW_MAP)

# -------------------------------------------------------------------------
# IMPORT
# -------------------------------------------------------------------------
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
tasks = []

for name in EXPECTED_NAMES:
    fbx_path = source_dir / (name + ".fbx")
    dest = destination_for(name)

    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", False)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)

    static_data = unreal.FbxStaticMeshImportData()
    static_data.set_editor_property("combine_meshes", False)
    static_data.set_editor_property("auto_generate_collision", True)
    static_data.set_editor_property("generate_lightmap_u_vs", True)
    static_data.set_editor_property("import_uniform_scale", 1.0)

    ui.set_editor_property("static_mesh_import_data", static_data)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(fbx_path))
    task.set_editor_property("destination_path", dest)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", False)
    task.set_editor_property("replace_existing_settings", False)
    task.set_editor_property("save", True)
    task.set_editor_property("options", ui)
    tasks.append(task)

log("Importing {} static meshes...".format(len(tasks)))
asset_tools.import_asset_tasks(tasks)

failed_imports = []
for name in EXPECTED_NAMES:
    path = asset_path_for(name)
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        failed_imports.append(path)

if failed_imports:
    fail(
        "Some assets were not created:\n  "
        + "\n  ".join(failed_imports)
    )

log("All expected meshes imported.")

# -------------------------------------------------------------------------
# PREVIEW LEVEL
# -------------------------------------------------------------------------
log("Creating preview map: " + PREVIEW_MAP)

created = unreal.EditorLevelLibrary.new_level(PREVIEW_MAP)
if not created:
    fail("Could not create preview map: " + PREVIEW_MAP)

# Engine cube used only as a 2m scale reference.
cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
if not cube:
    fail("Could not load /Engine/BasicShapes/Cube.Cube")

def spawn_mesh(label, mesh, x_cm, y_cm, z_cm=0.0, scale=None, rotation=None):
    if scale is None:
        scale = unreal.Vector(1.0, 1.0, 1.0)
    if rotation is None:
        rotation = unreal.Rotator(0.0, 0.0, 0.0)

    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor,
        unreal.Vector(x_cm, y_cm, z_cm),
        rotation
    )
    if not actor:
        fail("Failed to spawn preview actor: " + label)

    actor.set_actor_label(label)
    actor.static_mesh_component.set_static_mesh(mesh)
    actor.set_actor_scale3d(scale)
    actor.static_mesh_component.set_collision_enabled(
        unreal.CollisionEnabled.QUERY_AND_PHYSICS
    )
    return actor

# 2m tall reference, 0.5m wide/deep.
spawn_mesh(
    "SCALE_REFERENCE_2M",
    cube,
    -500.0, 300.0, 100.0,
    unreal.Vector(0.5, 0.5, 2.0)
)

# Arrange assets in rows. FBX source units are meters and should import
# at their intended world scale; this map is specifically for checking that.
columns = 5
spacing_x = 550.0
spacing_y = 750.0

for i, name in enumerate(EXPECTED_NAMES):
    mesh = unreal.load_asset(asset_path_for(name))
    if not mesh:
        fail("Imported asset failed to load: " + asset_path_for(name))

    col = i % columns
    row = i // columns

    x = col * spacing_x
    y = -row * spacing_y

    spawn_mesh(
        "PREVIEW_" + name,
        mesh,
        x, y, 0.0
    )

# Add a simple floor for scale/readability.
spawn_mesh(
    "PREVIEW_FLOOR",
    cube,
    1100.0, -850.0, -25.0,
    unreal.Vector(35.0, 30.0, 0.5)
)

saved = unreal.EditorLevelLibrary.save_current_level()
if not saved:
    fail("Preview level was created but could not be saved.")

log("Preview map saved.")
log("Imported mesh root: " + DEST_ROOT)
log("Preview map: " + PREVIEW_MAP)
log("RUINS PILLAR KIT V002 UNREAL IMPORT SUCCEEDED")
