import json
import os
import unreal


ASSETS = {
    "skeletal_mesh": "/Game/orc_character/Women_Motified_Ultimate/tripo_convert_990a7ce5-2cf1-4992-8c37-2335c7420d46",
    "animation_blueprint": "/Game/orc_character/Women_Motified_Ultimate/A/ABP_Unarmed",
    "blend_space": "/Game/orc_character/Women_Motified_Ultimate/A/BS_Idle_Walk_Run",
    "playable_character": "/Game/orc_character/Women_Motified_Ultimate/FinalGameTest/BP_BusinesswomanPlayable",
    "game_mode": "/Game/orc_character/Women_Motified_Ultimate/FinalGameTest/BP_BusinesswomanGameMode",
    "test_map": "/Game/orc_character/Women_Motified_Ultimate/FinalGameTest/L_BusinesswomanGameTest",
}


def pin_named(node, name):
    for pin in node.list_all_pins():
        if str(pin.get_pin_name()) == name:
            return pin
    raise RuntimeError(f"Pin {name} was not found on {node.get_name()}")


report = {"assets": {}, "animation_graph": {}, "map_check": {}, "success": False, "errors": []}

try:
    loaded = {}
    for label, path in ASSETS.items():
        asset = unreal.load_asset(path)
        loaded[label] = asset
        report["assets"][label] = {"path": path, "loaded": asset is not None}
        if asset is None:
            raise RuntimeError(f"Required asset did not load: {path}")

    abp = loaded["animation_blueprint"]
    graph = unreal.BlueprintEditorLibrary.find_graph(abp, "AnimGraph")
    if graph is None:
        raise RuntimeError("ABP_Unarmed has no AnimGraph")

    roots = graph.get_graph_nodes_of_class(unreal.AnimGraphNode_Root)
    controls = graph.get_graph_nodes_of_class(unreal.AnimGraphNode_ControlRig)
    if len(roots) != 1:
        raise RuntimeError(f"Expected one AnimGraph root, found {len(roots)}")
    if len(controls) != 1:
        raise RuntimeError(f"Expected one retained Control Rig node, found {len(controls)}")

    root_result = pin_named(roots[0], "Result")
    root_links = list(root_result.list_connected_pins())
    root_titles = [
        str(unreal.BlueprintEditorLibrary.get_node_title(pin.get_owning_node()))
        for pin in root_links
    ]

    control_source = pin_named(controls[0], "Source")
    control_pose = pin_named(controls[0], "Pose")
    control_source_links = len(list(control_source.list_connected_pins()))
    control_pose_links = len(list(control_pose.list_connected_pins()))

    report["animation_graph"] = {
        "root_links": root_titles,
        "control_rig_source_links": control_source_links,
        "control_rig_pose_links": control_pose_links,
    }

    if root_titles != ["Slot 'DefaultSlot'"]:
        raise RuntimeError(f"Unexpected final pose source: {root_titles}")
    if control_source_links or control_pose_links:
        raise RuntimeError("The incompatible mannequin Foot IK Control Rig is connected")

    for key in ("animation_blueprint", "playable_character", "game_mode"):
        unreal.BlueprintEditorLibrary.compile_blueprint(loaded[key])

    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    map_loaded = bool(level_editor.load_level(ASSETS["test_map"]))
    report["map_check"]["map_loaded"] = map_loaded
    if not map_loaded:
        raise RuntimeError("The packaged test map could not be opened in the level editor")
    editor_world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    unreal.SystemLibrary.execute_console_command(editor_world, "MAP CHECK")
    report["map_check"]["command_issued"] = True

    report["success"] = True
except Exception as exc:
    report["errors"].append(str(exc))
    unreal.log_error(str(exc))
finally:
    saved_dir = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())
    os.makedirs(saved_dir, exist_ok=True)
    report_path = os.path.join(saved_dir, "CharacterPackageAudit.json")
    with open(report_path, "w", encoding="utf-8") as handle:
        json.dump(report, handle, ensure_ascii=False, indent=2)

if not report["success"]:
    raise RuntimeError("Businesswoman character package audit failed")
