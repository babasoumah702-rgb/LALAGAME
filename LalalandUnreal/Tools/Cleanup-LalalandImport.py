import unreal

for actor in ("A", "B", "C", "D"):
    path = f"/Game/Characters/{actor}"
    if unreal.EditorAssetLibrary.does_directory_exist(path):
        if not unreal.EditorAssetLibrary.delete_directory(path):
            raise RuntimeError(f"Unable to delete {path}")

unreal.log("LALALAND_IMPORT_CLEANUP_OK")
