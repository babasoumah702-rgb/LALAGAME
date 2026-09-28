"""Import the September 2026 B-D models and retarget the existing action library.

The replacement FBXs use a different 61-bone hierarchy from the original
41-bone characters. They must keep their own skeleton: assigning the legacy
skeleton during FBX import produces a saved mesh that collapses at runtime.
"""

import os
import shutil
import zipfile
import unreal

PROJECT_DIR = os.path.realpath(os.path.normpath(unreal.Paths.project_dir()))
REPO_DIR = os.path.dirname(PROJECT_DIR)
LIBRARY_DIR = os.path.join(REPO_DIR, "新置换模型素材库")
EXTRACT_DIR = os.path.join(PROJECT_DIR, "Intermediate", "ReplacementCharacterImport")

ARCHIVES = {
    "B": "X-new-model-2026-09.zip",
    "C": "WanSai-new-model-2026-09.zip",
    "D": "Yitong-new-model-2026-09.zip",
}

SOURCE_CHAINS = [
    ("Root", "root", "root"),
    ("Spine", "Waist", "Spine02"),
    ("Head", "NeckTwist01", "Head"),
    ("LeftArm", "L_Clavicle", "L_Hand"),
    ("RightArm", "R_Clavicle", "R_Hand"),
    ("LeftLeg", "L_Thigh", "L_ToeBase"),
    ("RightLeg", "R_Thigh", "R_ToeBase"),
]

TARGET_CHAINS = [
    ("Root", "root", "root"),
    ("Spine", "spine_01", "spine_03"),
    ("Head", "neck_01", "head"),
    ("LeftArm", "clavicle_l", "hand_l"),
    ("RightArm", "clavicle_r", "hand_r"),
    ("LeftLeg", "thigh_l", "ball_l"),
    ("RightLeg", "thigh_r", "ball_r"),
]


def extract_source(actor, archive_name):
    archive = os.path.join(LIBRARY_DIR, archive_name)
    if not os.path.isfile(archive):
        raise RuntimeError(f"Missing replacement archive: {archive}")
    target = os.path.join(EXTRACT_DIR, actor)
    if os.path.isdir(target):
        shutil.rmtree(target)
    os.makedirs(target, exist_ok=True)
    with zipfile.ZipFile(archive, "r") as bundle:
        bundle.extractall(target)
    files = []
    for root, _, names in os.walk(target):
        files.extend(os.path.join(root, name) for name in names if name.lower().endswith(".fbx"))
    if len(files) != 1:
        raise RuntimeError(f"Expected one FBX for {actor}, found {len(files)}")
    return files[0]


def find_assets(root, asset_type):
    result = []
    for path in unreal.EditorAssetLibrary.list_assets(root, recursive=True, include_folder=False):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(asset, asset_type):
            result.append(asset)
    return result


def create_ik_rig(name, path, mesh, root_bone, chains):
    rig = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, path, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory()
    )
    if not rig:
        raise RuntimeError(f"Unable to create IK rig {path}/{name}")
    controller = unreal.IKRigController.get_controller(rig)
    if not controller.set_skeletal_mesh(mesh):
        raise RuntimeError(f"Unable to assign skeletal mesh to {name}")
    if not controller.set_retarget_root(root_bone):
        raise RuntimeError(f"Unable to set retarget root {root_bone} on {name}")
    for chain_name, start, end in chains:
        created = controller.add_retarget_chain(chain_name, start, end, "")
        if str(created) in ("None", ""):
            raise RuntimeError(f"Unable to add {chain_name} ({start}->{end}) to {name}")
    return rig


def retarget_legacy_animations(actor, target_root, source_mesh, target_mesh):
    source_rig = create_ik_rig(f"IK_Source_{actor}", target_root, source_mesh, "Hip", SOURCE_CHAINS)
    target_rig = create_ik_rig(f"IK_Target_{actor}", target_root, target_mesh, "pelvis", TARGET_CHAINS)

    retargeter = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        f"RTG_{actor}", target_root, unreal.IKRetargeter, unreal.IKRetargetFactory()
    )
    if not retargeter:
        raise RuntimeError(f"Unable to create retargeter for {actor}")
    controller = unreal.IKRetargeterController.get_controller(retargeter)
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, source_rig)
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, target_rig)
    controller.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, source_mesh)
    controller.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, target_mesh)
    controller.add_default_ops()

    source_root = f"/Game/Characters/{actor}"
    sequences = find_assets(source_root, unreal.AnimSequence)
    if not sequences:
        raise RuntimeError(f"No legacy animation sequences found for {actor}")
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    asset_data = [registry.get_asset_by_object_path(sequence.get_path_name()) for sequence in sequences]

    inputs = unreal.IKRetargetBatchOperationInputs()
    inputs.set_editor_properties({
        "assets_to_retarget": asset_data,
        "source_mesh": source_mesh,
        "target_mesh": target_mesh,
        "ik_retarget_asset": retargeter,
        "prefix": "RT_",
        "target_path": f"{target_root}/Retargeted",
        "use_source_path": False,
        "include_referenced_assets": False,
        "overwrite_existing_files": True,
    })
    created = unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
    if not created:
        raise RuntimeError(f"Batch retarget produced no animations for {actor}")
    unreal.log(f"LALALAND_RETARGET_OK {actor} animations={len(created)}")


def import_actor(actor, fbx_path):
    destination = f"/Game/Characters/{actor}_2026"
    mesh_path = f"{destination}/SK_{actor}_2026"
    # This folder is generated only by this importer. Clearing it makes the
    # command idempotent and removes any invalid skeleton from an older run.
    if unreal.EditorAssetLibrary.does_directory_exist(destination):
        unreal.EditorAssetLibrary.delete_directory(destination)

    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", True)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("skeleton", None)
    skeletal_data = options.get_editor_property("skeletal_mesh_import_data")
    skeletal_data.set_editor_property("import_morph_targets", True)
    skeletal_data.set_editor_property("use_t0_as_ref_pose", False)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", fbx_path)
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    meshes = find_assets(destination, unreal.SkeletalMesh)
    if len(meshes) != 1:
        raise RuntimeError(f"Expected one replacement SkeletalMesh for {actor}, found {len(meshes)}")
    imported_path = meshes[0].get_path_name()
    if imported_path != mesh_path:
        if not unreal.EditorAssetLibrary.rename_asset(imported_path, mesh_path):
            raise RuntimeError(f"Unable to rename {imported_path} to {mesh_path}")
    mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
    skeleton = mesh.get_editor_property("skeleton")
    source_mesh = unreal.EditorAssetLibrary.load_asset(f"/Game/Characters/{actor}/SK_{actor}")
    if not skeleton or not source_mesh:
        raise RuntimeError(f"Missing source or target skeleton data for {actor}")
    if skeleton == source_mesh.get_editor_property("skeleton"):
        raise RuntimeError(f"Replacement {actor} incorrectly reused the legacy skeleton")

    retarget_legacy_animations(actor, destination, source_mesh, mesh)
    unreal.EditorAssetLibrary.save_directory(destination, only_if_is_dirty=False, recursive=True)
    invalid_legacy_destination = f"/Game/Characters/{actor}_New"
    if unreal.EditorAssetLibrary.does_directory_exist(invalid_legacy_destination):
        unreal.EditorAssetLibrary.delete_directory(invalid_legacy_destination)
    unreal.log(f"LALALAND_REPLACEMENT_IMPORT_OK {actor} {mesh_path}")


for actor, archive in ARCHIVES.items():
    import_actor(actor, extract_source(actor, archive))
unreal.log("LALALAND_REPLACEMENT_IMPORT_COMPLETE B-D")
