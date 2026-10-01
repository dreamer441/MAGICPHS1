from pathlib import Path
import unreal

SCRIPT_DIR = Path(__file__).resolve().parent
TEX_DIR = SCRIPT_DIR / "Textures"

MESH_ROOT = "/Game/Environment/Ruins/Meshes"
PILLARS = MESH_ROOT + "/Pillars"
FRAGMENTS = MESH_ROOT + "/Fragments"
DEBRIS = MESH_ROOT + "/Debris"

MAT_ROOT = "/Game/Environment/Ruins/Materials"
TEX_ROOT = MAT_ROOT + "/Textures"

MASTER = MAT_ROOT + "/M_Master_AncientStone"
MI_PALE = MAT_ROOT + "/MI_AncientStone_PaleGranite"
MI_WEATHERED = MAT_ROOT + "/MI_AncientStone_Weathered"

EXPECTED = [
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
    unreal.log("[ANCIENT_STONE_FIX_V002] " + msg)

def fail(msg):
    raise RuntimeError("[ANCIENT_STONE_FIX_V002] " + msg)

def mesh_dest(name):
    if name.startswith("SM_RuinPillar_"):
        return PILLARS
    if name.startswith("SM_RuinColumn_"):
        return FRAGMENTS
    if name.startswith("SM_RuinShard_"):
        return DEBRIS
    fail("Unknown mesh category: " + name)

def mesh_path(name):
    return mesh_dest(name) + "/" + name

# v001 already completed the UV reimport before failing. Verify those mesh assets exist.
for name in EXPECTED:
    if not unreal.EditorAssetLibrary.does_asset_exist(mesh_path(name)):
        fail("Required imported mesh is missing: " + mesh_path(name))

texture_files = {
    "T_AncientStone_BaseColor": TEX_DIR / "T_AncientStone_BaseColor.png",
    "T_AncientStone_Normal": TEX_DIR / "T_AncientStone_Normal.png",
    "T_AncientStone_Roughness": TEX_DIR / "T_AncientStone_Roughness.png",
    "T_AncientStone_AO": TEX_DIR / "T_AncientStone_AO.png",
    "T_AncientStone_Mask": TEX_DIR / "T_AncientStone_Mask.png",
}

for name, path in texture_files.items():
    if not path.exists():
        fail("Texture source file missing: " + str(path))

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# -------------------------------------------------------------------------
# Import textures. Recovery-friendly: if an earlier retry created one,
# keep it and continue instead of duplicating it.
# -------------------------------------------------------------------------
to_import = []
for name, file in texture_files.items():
    asset_path = TEX_ROOT + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        log("Texture already exists, keeping it: " + asset_path)
        continue

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(file))
    task.set_editor_property("destination_path", TEX_ROOT)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", False)
    task.set_editor_property("save", True)
    to_import.append(task)

if to_import:
    log("Importing {} stone textures...".format(len(to_import)))
    asset_tools.import_asset_tasks(to_import)

base_tex = unreal.load_asset(TEX_ROOT + "/T_AncientStone_BaseColor")
normal_tex = unreal.load_asset(TEX_ROOT + "/T_AncientStone_Normal")
rough_tex = unreal.load_asset(TEX_ROOT + "/T_AncientStone_Roughness")
ao_tex = unreal.load_asset(TEX_ROOT + "/T_AncientStone_AO")
mask_tex = unreal.load_asset(TEX_ROOT + "/T_AncientStone_Mask")

for label, tex in [
    ("BaseColor", base_tex),
    ("Normal", normal_tex),
    ("Roughness", rough_tex),
    ("AO", ao_tex),
    ("Mask", mask_tex),
]:
    if not tex:
        fail("Texture could not be loaded after import: " + label)

# Texture settings.
try:
    base_tex.set_editor_property("srgb", True)
except Exception:
    pass

for tex in [normal_tex, rough_tex, ao_tex, mask_tex]:
    try:
        tex.set_editor_property("srgb", False)
    except Exception:
        pass

try:
    normal_tex.set_editor_property(
        "compression_settings",
        unreal.TextureCompressionSettings.TC_NORMALMAP
    )
except Exception as e:
    log("Normal compression setting skipped: " + str(e))

for tex in [rough_tex, ao_tex, mask_tex]:
    try:
        tex.set_editor_property(
            "compression_settings",
            unreal.TextureCompressionSettings.TC_MASKS
        )
    except Exception:
        pass

for tex in [base_tex, normal_tex, rough_tex, ao_tex, mask_tex]:
    unreal.EditorAssetLibrary.save_loaded_asset(tex)

# -------------------------------------------------------------------------
# Create master material.
# -------------------------------------------------------------------------
if unreal.EditorAssetLibrary.does_asset_exist(MASTER):
    fail(
        "Master material already exists from another run. "
        "Stop here so we do not overwrite it: " + MASTER
    )

factory = unreal.MaterialFactoryNew()
master = asset_tools.create_asset(
    "M_Master_AncientStone",
    MAT_ROOT,
    unreal.Material,
    factory
)
if not master:
    fail("Could not create master material.")

mel = unreal.MaterialEditingLibrary

texcoord = mel.create_material_expression(
    master, unreal.MaterialExpressionTextureCoordinate, -1200, -80
)

uv_scale = mel.create_material_expression(
    master, unreal.MaterialExpressionScalarParameter, -1200, 80
)
uv_scale.set_editor_property("parameter_name", "UVScale")
uv_scale.set_editor_property("default_value", 4.5)

uv_mul = mel.create_material_expression(
    master, unreal.MaterialExpressionMultiply, -1000, -40
)
mel.connect_material_expressions(texcoord, "", uv_mul, "A")
mel.connect_material_expressions(uv_scale, "", uv_mul, "B")

def tex_param(param_name, texture, x, y, normal=False):
    node = mel.create_material_expression(
        master, unreal.MaterialExpressionTextureSampleParameter2D, x, y
    )
    node.set_editor_property("parameter_name", param_name)
    node.set_editor_property("texture", texture)
    if normal:
        try:
            node.set_editor_property(
                "sampler_type",
                unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
            )
        except Exception:
            pass
    mel.connect_material_expressions(uv_mul, "", node, "UVs")
    return node

base_node = tex_param("BaseColorTex", base_tex, -760, -360)
normal_node = tex_param("NormalTex", normal_tex, -760, -80, True)
rough_node = tex_param("RoughnessTex", rough_tex, -760, 180)
ao_node = tex_param("AOTex", ao_tex, -760, 390)
mask_node = tex_param("MaskTex", mask_tex, -760, 600)

tint = mel.create_material_expression(
    master, unreal.MaterialExpressionVectorParameter, -510, -430
)
tint.set_editor_property("parameter_name", "BaseTint")
tint.set_editor_property(
    "default_value",
    unreal.LinearColor(1.0, 0.98, 0.92, 1.0)
)

base_tinted = mel.create_material_expression(
    master, unreal.MaterialExpressionMultiply, -290, -320
)
mel.connect_material_expressions(base_node, "RGB", base_tinted, "A")
mel.connect_material_expressions(tint, "", base_tinted, "B")

crack_strength = mel.create_material_expression(
    master, unreal.MaterialExpressionScalarParameter, -500, 670
)
crack_strength.set_editor_property("parameter_name", "CrackStrength")
crack_strength.set_editor_property("default_value", 0.27)

crack_mul = mel.create_material_expression(
    master, unreal.MaterialExpressionMultiply, -280, 620
)
mel.connect_material_expressions(mask_node, "R", crack_mul, "A")
mel.connect_material_expressions(crack_strength, "", crack_mul, "B")

one_minus = mel.create_material_expression(
    master, unreal.MaterialExpressionOneMinus, -70, 620
)
mel.connect_material_expressions(crack_mul, "", one_minus, "Input")

final_base = mel.create_material_expression(
    master, unreal.MaterialExpressionMultiply, 140, -250
)
mel.connect_material_expressions(base_tinted, "", final_base, "A")
mel.connect_material_expressions(one_minus, "", final_base, "B")

rough_mult = mel.create_material_expression(
    master, unreal.MaterialExpressionScalarParameter, -510, 255
)
rough_mult.set_editor_property("parameter_name", "RoughnessMultiplier")
rough_mult.set_editor_property("default_value", 1.0)

rough_final = mel.create_material_expression(
    master, unreal.MaterialExpressionMultiply, -280, 225
)
mel.connect_material_expressions(rough_node, "R", rough_final, "A")
mel.connect_material_expressions(rough_mult, "", rough_final, "B")

mel.connect_material_property(
    final_base, "", unreal.MaterialProperty.MP_BASE_COLOR
)
mel.connect_material_property(
    normal_node, "RGB", unreal.MaterialProperty.MP_NORMAL
)
mel.connect_material_property(
    rough_final, "", unreal.MaterialProperty.MP_ROUGHNESS
)
mel.connect_material_property(
    ao_node, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION
)

mel.layout_material_expressions(master)
mel.recompile_material(master)
unreal.EditorAssetLibrary.save_loaded_asset(master)

# -------------------------------------------------------------------------
# Material instances.
# -------------------------------------------------------------------------
def make_instance(asset_name, tint_color, roughness_mult, uv_value, crack_value):
    asset_path = MAT_ROOT + "/" + asset_name
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        fail("Material instance already exists; refusing overwrite: " + asset_path)

    mi_factory = unreal.MaterialInstanceConstantFactoryNew()
    mi = asset_tools.create_asset(
        asset_name,
        MAT_ROOT,
        unreal.MaterialInstanceConstant,
        mi_factory
    )
    if not mi:
        fail("Could not create material instance: " + asset_name)

    mel.set_material_instance_parent(mi, master)
    mel.set_material_instance_texture_parameter_value(mi, "BaseColorTex", base_tex)
    mel.set_material_instance_texture_parameter_value(mi, "NormalTex", normal_tex)
    mel.set_material_instance_texture_parameter_value(mi, "RoughnessTex", rough_tex)
    mel.set_material_instance_texture_parameter_value(mi, "AOTex", ao_tex)
    mel.set_material_instance_texture_parameter_value(mi, "MaskTex", mask_tex)

    mel.set_material_instance_scalar_parameter_value(mi, "UVScale", uv_value)
    mel.set_material_instance_scalar_parameter_value(
        mi, "RoughnessMultiplier", roughness_mult
    )
    mel.set_material_instance_scalar_parameter_value(
        mi, "CrackStrength", crack_value
    )
    mel.set_material_instance_vector_parameter_value(
        mi, "BaseTint", tint_color
    )

    try:
        mel.update_material_instance(mi)
    except Exception:
        pass

    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    return mi

pale = make_instance(
    "MI_AncientStone_PaleGranite",
    unreal.LinearColor(1.00, 0.98, 0.93, 1.0),
    1.00,
    4.5,
    0.27
)

weathered = make_instance(
    "MI_AncientStone_Weathered",
    unreal.LinearColor(0.86, 0.84, 0.78, 1.0),
    1.06,
    4.2,
    0.36
)

# -------------------------------------------------------------------------
# Assign to all ruin kit meshes. World actors already reference these assets,
# so they update without replacing the actors again.
# -------------------------------------------------------------------------
for name in EXPECTED:
    mesh = unreal.load_asset(mesh_path(name))
    if not mesh:
        fail("Mesh failed to load: " + mesh_path(name))

    material = pale if name.startswith("SM_RuinPillar_") else weathered

    try:
        mesh.set_material(0, material)
    except Exception as e:
        fail("Could not assign material to {}: {}".format(name, e))

    unreal.EditorAssetLibrary.save_loaded_asset(mesh)

log("Texture folder used: " + str(TEX_DIR))
log("Textures imported and configured.")
log("M_Master_AncientStone created.")
log("PaleGranite / Weathered instances created.")
log("Materials assigned to all RuinsPillarKit_v002 meshes.")
log("ANCIENT STONE FIX V002 SUCCEEDED")
