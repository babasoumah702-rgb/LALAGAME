import os
import unreal


PROJECT_DIR = os.path.realpath(os.path.normpath(unreal.Paths.project_dir()))
REPO_DIR = os.path.dirname(PROJECT_DIR)
SOURCE_DIR = os.path.join(REPO_DIR, "BarPrototype", "Assets", "LastCall", "Stage")
TEXTURE_DIR = "/Game/Environment/Textures"
MATERIAL_DIR = "/Game/Environment/Materials"


TEXTURES = {
    "skin-bar.png": "T_BackbarWarm",
    "otome-bar.png": "T_BackbarModern",
    "skin-floor.png": "T_Floor",
    "skin-leather.png": "T_Leather",
    "skin-plaster.png": "T_Plaster",
    "skin-wood.png": "T_Wood",
    "skin-photos.png": "T_Photos",
}


def import_textures():
    for source_name, asset_name in TEXTURES.items():
        source = os.path.join(SOURCE_DIR, source_name)
        if not os.path.isfile(source):
            raise RuntimeError(f"Missing environment source: {source}")
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", source)
        task.set_editor_property("destination_path", TEXTURE_DIR)
        task.set_editor_property("destination_name", asset_name)
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)
        task.set_editor_property("replace_existing", True)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])


def fresh_material(name):
    path = f"{MATERIAL_DIR}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, MATERIAL_DIR, unreal.Material, unreal.MaterialFactoryNew()
    )


def make_tint_material():
    material = fresh_material("M_Tint")
    tint = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -420, -20
    )
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.18, 0.12, 0.08, 1.0))
    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -420, 140
    )
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.72)
    unreal.MaterialEditingLibrary.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)


def make_texture_material(name, texture_name, roughness_value=0.72, emissive=False):
    material = fresh_material(name)
    texture = unreal.EditorAssetLibrary.load_asset(f"{TEXTURE_DIR}/{texture_name}")
    if not texture:
        raise RuntimeError(f"Texture was not imported: {texture_name}")
    sample = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, -460, -40
    )
    sample.set_editor_property("texture", texture)
    if emissive:
        material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
        strength = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionMultiply, -180, -40
        )
        constant = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionConstant, -420, 180
        )
        constant.set_editor_property("r", 0.7)
        unreal.MaterialEditingLibrary.connect_material_expressions(sample, "RGB", strength, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(constant, "", strength, "B")
        unreal.MaterialEditingLibrary.connect_material_property(strength, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        roughness = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionConstant, -220, 150
        )
        roughness.set_editor_property("r", roughness_value)
        unreal.MaterialEditingLibrary.connect_material_property(sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
        unreal.MaterialEditingLibrary.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)


def main():
    import_textures()
    make_tint_material()
    make_texture_material("M_BackbarWarm", "T_BackbarWarm", emissive=True)
    make_texture_material("M_BackbarModern", "T_BackbarModern", emissive=True)
    make_texture_material("M_Floor", "T_Floor", 0.62)
    make_texture_material("M_Leather", "T_Leather", 0.48)
    make_texture_material("M_Plaster", "T_Plaster", 0.9)
    make_texture_material("M_Wood", "T_Wood", 0.58)
    make_texture_material("M_Photos", "T_Photos", 0.75)
    unreal.EditorAssetLibrary.save_directory("/Game/Environment", only_if_is_dirty=False, recursive=True)
    unreal.log("LALALAND_ENVIRONMENT_IMPORT_COMPLETE")


main()
