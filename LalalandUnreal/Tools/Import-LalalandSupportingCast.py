import os
import unreal


project_dir = os.path.realpath(os.path.normpath(unreal.Paths.project_dir()))
repo_dir = os.path.dirname(project_dir)
source = os.path.join(
    repo_dir,
    "BarPrototype", "Assets", "LastCall", "Characters", "BARTENDER",
    "tripo_convert_d9f99596-e1bb-4710-ae1f-c600327822c6.fbx",
)
destination = "/Game/Characters/BARTENDER"

if not os.path.isfile(source):
    raise RuntimeError(f"Missing bartender source: {source}")

options = unreal.FbxImportUI()
options.set_editor_property("import_mesh", True)
options.set_editor_property("import_animations", False)
options.set_editor_property("import_materials", True)
options.set_editor_property("import_textures", True)
options.set_editor_property("import_as_skeletal", False)
options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
static_data = options.get_editor_property("static_mesh_import_data")
static_data.set_editor_property("import_uniform_scale", 100.0)
static_data.set_editor_property("remove_degenerates", False)

task = unreal.AssetImportTask()
task.set_editor_property("filename", source)
task.set_editor_property("destination_path", destination)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
task.set_editor_property("replace_existing", True)
task.set_editor_property("options", options)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

meshes = []
for path in task.get_editor_property("imported_object_paths"):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if isinstance(asset, unreal.StaticMesh):
        meshes.append(path)
if len(meshes) != 1:
    raise RuntimeError(f"Expected one bartender skeletal mesh, imported {meshes}")
wanted = f"{destination}/SM_BARTENDER"
if meshes[0] != wanted:
    if unreal.EditorAssetLibrary.does_asset_exist(wanted):
        unreal.EditorAssetLibrary.delete_asset(wanted)
    if not unreal.EditorAssetLibrary.rename_asset(meshes[0], wanted):
        raise RuntimeError(f"Unable to rename bartender mesh {meshes[0]}")

unreal.EditorAssetLibrary.save_directory(destination, only_if_is_dirty=False, recursive=True)
unreal.log(f"LALALAND_SUPPORTING_CAST_IMPORT_COMPLETE {wanted}")
