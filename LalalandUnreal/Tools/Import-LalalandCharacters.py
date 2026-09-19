import os
import shutil
import zipfile
import unreal

PROJECT_DIR = os.path.realpath(os.path.normpath(unreal.Paths.project_dir()))
REPO_DIR = os.path.dirname(PROJECT_DIR)
LIBRARY_DIR = os.path.join(REPO_DIR, "新置换模型素材库")
EXTRACT_DIR = os.path.join(PROJECT_DIR, "Intermediate", "CharacterImport")

ARCHIVES = {
    "A": "kiko+model+(动效12+控制速度）.zip",
    "B": "x+model(含动效+控制速度）.zip",
    "C": "万塞+model(25动效+控制速度）.zip",
    "D": "一桐model(大幅度动效21+控制速度）.zip",
}


def extract_source(actor, archive_name):
    archive = os.path.join(LIBRARY_DIR, archive_name)
    if not os.path.isfile(archive):
        raise RuntimeError(f"Missing character archive: {archive}")
    target = os.path.join(EXTRACT_DIR, actor)
    if os.path.isdir(target):
        shutil.rmtree(target)
    os.makedirs(target, exist_ok=True)
    with zipfile.ZipFile(archive, "r") as bundle:
        bundle.extractall(target)
    fbx_files = []
    for root, _, files in os.walk(target):
        fbx_files.extend(os.path.join(root, name) for name in files if name.lower().endswith(".fbx"))
    if len(fbx_files) != 1:
        raise RuntimeError(f"Expected exactly one FBX for {actor}, found {len(fbx_files)}")
    return fbx_files[0]


def import_actor(actor, fbx_path):
    destination = f"/Game/Characters/{actor}"
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_animations", True)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", True)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    skeletal_data = options.get_editor_property("skeletal_mesh_import_data")
    skeletal_data.set_editor_property("import_morph_targets", True)
    skeletal_data.set_editor_property("use_t0_as_ref_pose", False)
    animation_data = options.get_editor_property("anim_sequence_import_data")
    animation_data.set_editor_property("snap_to_closest_frame_boundary", True)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", fbx_path)
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    skeletal_assets = []
    for path in task.get_editor_property("imported_object_paths"):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(asset, unreal.SkeletalMesh):
            skeletal_assets.append(path)
    if len(skeletal_assets) != 1:
        raise RuntimeError(f"Expected one SkeletalMesh for {actor}, imported {skeletal_assets}")
    wanted = f"{destination}/SK_{actor}"
    if skeletal_assets[0] != wanted:
        if unreal.EditorAssetLibrary.does_asset_exist(wanted):
            unreal.EditorAssetLibrary.delete_asset(wanted)
        if not unreal.EditorAssetLibrary.rename_asset(skeletal_assets[0], wanted):
            raise RuntimeError(f"Unable to rename {skeletal_assets[0]} to {wanted}")
    unreal.EditorAssetLibrary.save_directory(destination, only_if_is_dirty=False, recursive=True)
    unreal.log(f"LALALAND_IMPORT_OK {actor} {wanted}")


def import_audio():
    audio_root = os.path.join(REPO_DIR, "BarPrototype", "Assets", "LastCall", "SceneZero", "Audio")
    files = {
        "arrival.wav": "SW_Arrival",
        "cup.wav": "SW_Cup",
        "door.wav": "SW_Door",
        "elevator.wav": "SW_Elevator",
        "lounge.wav": "SW_Lounge",
        "phone.wav": "SW_Phone",
    }
    for source_name, asset_name in files.items():
        source = os.path.join(audio_root, source_name)
        if not os.path.isfile(source):
            raise RuntimeError(f"Missing audio source: {source}")
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", source)
        task.set_editor_property("destination_path", "/Game/Audio")
        task.set_editor_property("destination_name", asset_name)
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)
        task.set_editor_property("replace_existing", True)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    unreal.EditorAssetLibrary.save_directory("/Game/Audio", only_if_is_dirty=False, recursive=True)
    unreal.log("LALALAND_IMPORT_OK Scene0 audio")


def main():
    for actor, archive in ARCHIVES.items():
        import_actor(actor, extract_source(actor, archive))
    import_audio()
    unreal.log("LALALAND_IMPORT_COMPLETE A-D")


main()
