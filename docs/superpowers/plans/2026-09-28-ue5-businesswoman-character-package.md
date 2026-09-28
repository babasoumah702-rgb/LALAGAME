# UE5 Businesswoman Character Package Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Add a self-contained UE 5.8 example project to the LALAGAME repository so developers can play, inspect, modify, and later migrate the completed businesswoman character.

**Architecture:** Copy the verified runtime project assets while preserving their `/Game` package paths, then place them behind a clean project descriptor and minimal default-map configuration. Keep editable source art in a separate `SourceAssets` folder and validate both filesystem completeness and UE asset references before committing.

**Tech Stack:** Unreal Engine 5.8, UE Blueprints and Enhanced Input, PowerShell, Git, Blender 4.5 source assets.

---

## File map

- Create `UE5/BusinesswomanCharacterDemo/BusinesswomanCharacterDemo.uproject`: clean UE 5.8 project entry point.
- Create `UE5/BusinesswomanCharacterDemo/Config/DefaultEngine.ini`: startup map, game map, renderer and input defaults.
- Create `UE5/BusinesswomanCharacterDemo/Config/DefaultGame.ini`: project identity.
- Create `UE5/BusinesswomanCharacterDemo/Config/DefaultInput.ini`: Enhanced Input classes.
- Copy `UE5/BusinesswomanCharacterDemo/Content/**`: verified model, materials, animations, Blueprints, map, input and referenced template assets, preserving current package paths.
- Create `UE5/BusinesswomanCharacterDemo/SourceAssets/`: original GLB, final editable Blender file and final rigged FBX.
- Create `UE5/BusinesswomanCharacterDemo/打开角色示例.cmd`: locate the configured UE 5.8 editor and open the demo map.
- Create `UE5/BusinesswomanCharacterDemo/README_角色使用说明.md`: beginner and developer usage, migration and component checklist.
- Create `UE5/BusinesswomanCharacterDemo/Tools/verify_character_package.py`: in-editor asset and graph audit.
- Create `UE5/BusinesswomanCharacterDemo/Tools/Verify-Package.ps1`: filesystem and UE commandlet verification entry point.
- Modify `.gitattributes`: classify UE binary assets.
- Modify `.gitignore`: exclude UE generated directories.

### Task 1: Create the clean UE project shell

**Files:**
- Create: `UE5/BusinesswomanCharacterDemo/BusinesswomanCharacterDemo.uproject`
- Create: `UE5/BusinesswomanCharacterDemo/Config/DefaultEngine.ini`
- Create: `UE5/BusinesswomanCharacterDemo/Config/DefaultGame.ini`
- Create: `UE5/BusinesswomanCharacterDemo/Config/DefaultInput.ini`

- [x] **Step 1: Create the project descriptor**

```json
{
  "FileVersion": 3,
  "EngineAssociation": "5.8",
  "Category": "LALAGAME Character Demo",
  "Description": "Playable businesswoman character delivery project",
  "Plugins": [
    { "Name": "ControlRig", "Enabled": true },
    { "Name": "EnhancedInput", "Enabled": true }
  ]
}
```

- [x] **Step 2: Configure the default map and game mode**

Write `DefaultEngine.ini` with:

```ini
[/Script/EngineSettings.GameMapsSettings]
EditorStartupMap=/Game/orc_character/Women_Motified_Ultimate/FinalGameTest/L_BusinesswomanGameTest.L_BusinesswomanGameTest
GameDefaultMap=/Game/orc_character/Women_Motified_Ultimate/FinalGameTest/L_BusinesswomanGameTest.L_BusinesswomanGameTest
GlobalDefaultGameMode=/Game/orc_character/Women_Motified_Ultimate/FinalGameTest/BP_BusinesswomanGameMode.BP_BusinesswomanGameMode_C

[/Script/Engine.Engine]
+ActiveGameNameRedirects=(OldGameName="我的项目2",NewGameName="/Script/BusinesswomanCharacterDemo")

[/Script/Engine.InputSettings]
DefaultPlayerInputClass=/Script/EnhancedInput.EnhancedPlayerInput
DefaultInputComponentClass=/Script/EnhancedInput.EnhancedInputComponent
DefaultTouchInterface=None
```

- [x] **Step 3: Configure project identity and input defaults**

Write `DefaultGame.ini`:

```ini
[/Script/EngineSettings.GeneralProjectSettings]
ProjectID=AB6E7C594F5E4D8B8CE7778CE5B42025
ProjectName=Businesswoman Character Demo
Description=Playable UE5 businesswoman character for LALAGAME
```

Write `DefaultInput.ini`:

```ini
[/Script/Engine.InputSettings]
DefaultPlayerInputClass=/Script/EnhancedInput.EnhancedPlayerInput
DefaultInputComponentClass=/Script/EnhancedInput.EnhancedInputComponent
DefaultTouchInterface=None
```

- [x] **Step 4: Verify the descriptor and configs are parseable**

Run:

```powershell
Get-Content UE5/BusinesswomanCharacterDemo/BusinesswomanCharacterDemo.uproject -Raw | ConvertFrom-Json | Select-Object EngineAssociation
Select-String UE5/BusinesswomanCharacterDemo/Config/DefaultEngine.ini -Pattern 'L_BusinesswomanGameTest','BP_BusinesswomanGameMode'
```

Expected: engine association `5.8`; both startup assets are printed.

### Task 2: Copy runtime assets without generated files

**Files:**
- Copy: `E:/LaLaGame/Test/我的项目2/Content/**`
- Create: `UE5/BusinesswomanCharacterDemo/Content/**`

- [x] **Step 1: Close the source editor before copying**

Run:

```powershell
Get-Process UnrealEditor -ErrorAction SilentlyContinue
```

Expected: no `UnrealEditor` process. If one exists, close it normally before copying.

- [x] **Step 2: Copy the complete verified Content directory**

Run:

```powershell
robocopy "E:\LaLaGame\Test\我的项目2\Content" "E:\LaLaGame\LALAGAME-main\LALAGAME-main\UE5\BusinesswomanCharacterDemo\Content" /E /COPY:DAT /DCOPY:DAT /R:2 /W:1
if ($LASTEXITCODE -gt 7) { exit $LASTEXITCODE }
```

Expected: all `Content` files copied; robocopy exit code 0–7.

- [x] **Step 3: Remove editor-only empty personal folders**

Remove empty `Content/Collections` and `Content/Developers` folders if present. Do not remove `__ExternalActors__` or `__ExternalObjects__`; the world-partition map may reference them.

- [x] **Step 4: Check the required runtime assets**

Run `Test-Path` for:

```text
Content/orc_character/Women_Motified_Ultimate/tripo_convert_990a7ce5-2cf1-4992-8c37-2335c7420d46.uasset
Content/orc_character/Women_Motified_Ultimate/A/ABP_Unarmed.uasset
Content/orc_character/Women_Motified_Ultimate/A/BS_Idle_Walk_Run.uasset
Content/orc_character/Women_Motified_Ultimate/FinalGameTest/BP_BusinesswomanPlayable.uasset
Content/orc_character/Women_Motified_Ultimate/FinalGameTest/BP_BusinesswomanGameMode.uasset
Content/orc_character/Women_Motified_Ultimate/FinalGameTest/L_BusinesswomanGameTest.umap
```

Expected: all values are `True`.

- [x] **Step 5: Prune the copied template to the real UE dependency closure**

Use the UE Asset Registry to recursively collect hard, soft and management dependencies from the skeletal mesh, animation blueprint, blend space, playable character, game mode and test map. Keep the resulting 66 `/Game` packages and remove the other copied `.uasset`/`.umap` files. Run the complete audit and standalone game again after pruning.

Expected: 66 runtime packages remain; core assets load; Map Check reports zero errors; the standalone game starts with `BP_BusinesswomanGameMode_C` and no Blueprint runtime errors.

### Task 3: Add editable source art

**Files:**
- Copy: `E:/LaLaGame/新置换模型素材库/businesswoman suit 3d model.glb`
- Copy: `E:/LaLaGame/CharacterLab/SourceAssets/Businesswoman/businesswoman_work_v08_glasses.blend`
- Copy: `E:/LaLaGame/Test/W0men_Modified.fbx`
- Create: `UE5/BusinesswomanCharacterDemo/SourceAssets/README.md`

- [x] **Step 1: Copy the three source assets with stable names**

```text
SourceAssets/businesswoman_original_tripo.glb
SourceAssets/businesswoman_final_editable.blend
SourceAssets/businesswoman_rigged.fbx
```

- [x] **Step 2: Document what each source file represents**

Write `SourceAssets/README.md` stating that the GLB is the original generated model, the `.blend` file is the latest editable art file with glasses, and the FBX is the rigged interchange model. Explain that runtime truth is the UE skeletal mesh and that reimport must be done in a copy or branch because it can change skeleton/material references.

- [x] **Step 3: Enforce GitHub's individual-file limit**

Run:

```powershell
Get-ChildItem UE5/BusinesswomanCharacterDemo -Recurse -File |
  Where-Object Length -ge 100MB |
  Select-Object FullName,Length
```

Expected: no output.

### Task 4: Add developer entry points and documentation

**Files:**
- Create: `UE5/BusinesswomanCharacterDemo/打开角色示例.cmd`
- Create: `UE5/BusinesswomanCharacterDemo/README_角色使用说明.md`

- [x] **Step 1: Create a launcher that resolves the repository-relative UE install**

```bat
@echo off
setlocal
set "PROJECT=%~dp0BusinesswomanCharacterDemo.uproject"
set "EDITOR=%~dp0..\..\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
if not exist "%EDITOR%" (
  echo UE 5.8 not found: %EDITOR%
  echo Open the .uproject manually with Unreal Engine 5.8.
  pause
  exit /b 1
)
start "" "%EDITOR%" "%PROJECT%" "/Game/orc_character/Women_Motified_Ultimate/FinalGameTest/L_BusinesswomanGameTest"
```

- [x] **Step 2: Write the complete-use README**

Document:

- UE 5.8 requirement and launch steps.
- Play controls: WASD, mouse, run, jump and stop play.
- Exact runtime asset paths and their responsibilities.
- How model, UV, textures, materials, skeleton, skin weights, animation sequences, blend space, animation blueprint, collision capsule, movement component, input mapping, camera, character blueprint, game mode, player start and test map fit together.
- How to migrate the character through UE's Asset Actions → Migrate command.
- How to replace the mesh without breaking skeleton, material and animation references.
- Current limitation: mannequin Foot IK Control Rig is intentionally bypassed because the Tripo skeleton has no matching IK bones.
- Future production additions: LOD, physics asset review, footstep/audio, gameplay interactions, damage/death, networking, save data and asset-license record.

- [x] **Step 3: Check documentation paths against the copied tree**

Run every documented `/Game/...` path through the verification script in Task 6; no documented path may be absent.

### Task 5: Add repository rules for UE assets

**Files:**
- Modify: `.gitattributes`
- Modify: `.gitignore`

- [x] **Step 1: Classify UE packages as binary**

Append to `.gitattributes`:

```gitattributes
*.uasset binary
*.umap binary
*.blend binary
*.fbx binary
*.glb binary
```

- [x] **Step 2: Ignore generated UE directories only**

Append to `.gitignore`:

```gitignore
UE5/**/Binaries/
UE5/**/DerivedDataCache/
UE5/**/Intermediate/
UE5/**/Saved/
UE5/**/*.sln
UE5/**/*.VC.db
```

- [x] **Step 3: Verify runtime content is not ignored**

Run:

```powershell
git check-ignore -v UE5/BusinesswomanCharacterDemo/Content/orc_character/Women_Motified_Ultimate/A/ABP_Unarmed.uasset
```

Expected: no output and a nonzero status, meaning the asset is tracked normally.

### Task 6: Add repeatable package verification

**Files:**
- Create: `UE5/BusinesswomanCharacterDemo/Tools/verify_character_package.py`
- Create: `UE5/BusinesswomanCharacterDemo/Tools/Verify-Package.ps1`

- [x] **Step 1: Write the Unreal Python audit**

The script must load the skeletal mesh, animation blueprint, blend space, playable character, game mode and map with `unreal.load_asset`; fail if any returns `None`; find the AnimGraph root and assert its `Result` pin connects to `Slot 'DefaultSlot'`; assert the Control Rig node's `Source` and `Pose` pins have no connections; run `EditorLoadingAndSavingUtils.save_map` only if a map was modified (the audit normally remains read-only); write `Saved/CharacterPackageAudit.json` with per-asset results and a final `success` boolean.

- [x] **Step 2: Write the PowerShell verification wrapper**

The wrapper must:

1. Resolve the project root from `$PSScriptRoot`.
2. Check all six required runtime files.
3. Reject `Binaries`, `DerivedDataCache`, `Intermediate` and pre-existing `Saved` delivery folders.
4. Reject individual files at or above 100 MB.
5. Locate UE from optional `-EditorPath`, then the repository's `UE_5.8`, then fail with an actionable message.
6. Run `UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript=<audit> -unattended -nop4 -nosplash -nullrhi -NoSound`.
7. Read `Saved/CharacterPackageAudit.json` and fail unless `success` is `true`.

- [x] **Step 3: Run filesystem verification before UE**

Expected: all required files present, zero forbidden delivered directories, zero files at or above 100 MB.

- [x] **Step 4: Run the complete UE audit**

Run:

```powershell
& UE5/BusinesswomanCharacterDemo/Tools/Verify-Package.ps1 -EditorPath "E:\LaLaGame\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
```

Expected: command exits 0; report says every asset loads; root connects to `Slot 'DefaultSlot'`; incompatible Control Rig has no live links.

### Task 7: Open the packaged project and perform final delivery checks

**Files:**
- Verify: `UE5/BusinesswomanCharacterDemo/**`

- [x] **Step 1: Open the copied project and test its map**

Open the demo through `打开角色示例.cmd`. Click Play and confirm the pawn spawns, camera follows, WASD moves, mouse rotates, run changes speed and animation, jump enters airborne animation, and the legs do not stretch.

- [x] **Step 2: Run Map Check**

Expected: 0 errors. Warnings must be reviewed; no warning may report a missing package, missing texture, invalid skeleton or failed Blueprint compilation.

- [x] **Step 3: Inspect repository changes**

Run:

```powershell
git status --short
git diff --check
git ls-files UE5/BusinesswomanCharacterDemo | Measure-Object
```

Expected: only planned UE5 package files, docs and repository rules are included; no cache, logs or private settings.

- [x] **Step 4: Commit the complete delivery**

```powershell
git add .gitattributes .gitignore UE5/BusinesswomanCharacterDemo docs/superpowers/plans/2026-09-28-ue5-businesswoman-character-package.md
git commit -m "feat(ue5): add playable businesswoman demo"
```

- [x] **Step 5: Report the developer handoff**

Provide the project path, launch command, test map, playable controls, migration steps, verification results, known Foot IK limitation and the checklist of components required by a complete playable character.
