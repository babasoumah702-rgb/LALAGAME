# Businesswoman Character Demo 使用说明

这是 LALAGAME 的 UE 5.8 可游玩女角色交付工程。它包含运行角色所需的模型、贴图、材质、骨骼、动画、操控蓝图、输入、摄像机、游戏模式和测试地图，也附带可继续修改的模型源文件。

## 打开和游玩

1. 安装 Unreal Engine 5.8。
2. 双击 `打开角色示例.cmd`。如果你的 UE 不在仓库旁边的 `UE_5.8` 或 Epic 默认目录，双击 `BusinesswomanCharacterDemo.uproject` 并选择 UE 5.8。
3. 编辑器会打开 `L_BusinesswomanGameTest`。
4. 点击顶部绿色播放按钮。

操作：

- `WASD`：移动；动画按移动速度在待机、走路和跑步间混合。
- 鼠标：旋转第三人称视角。
- `Space`：跳跃，并进入起跳、下落和落地动画。
- `Esc`：退出编辑器中的运行模式。

## 主要资产

| 内容 | UE 路径 | 用途 |
|---|---|---|
| 骨骼网格体 | `/Game/orc_character/Women_Motified_Ultimate/tripo_convert_990a7ce5-2cf1-4992-8c37-2335c7420d46` | 角色形状、骨骼和蒙皮权重 |
| 动画蓝图 | `/Game/orc_character/Women_Motified_Ultimate/A/ABP_Unarmed` | 根据速度和空中状态选择、混合动画 |
| 移动混合空间 | `/Game/orc_character/Women_Motified_Ultimate/A/BS_Idle_Walk_Run` | 平滑混合待机、各方向走路和跑步 |
| 玩家角色 | `/Game/orc_character/Women_Motified_Ultimate/FinalGameTest/BP_BusinesswomanPlayable` | 碰撞、移动、输入、摄像机和角色显示 |
| 游戏模式 | `/Game/orc_character/Women_Motified_Ultimate/FinalGameTest/BP_BusinesswomanGameMode` | 指定默认玩家角色 |
| 测试地图 | `/Game/orc_character/Women_Motified_Ultimate/FinalGameTest/L_BusinesswomanGameTest` | 出生、光照、地面和操控测试 |

## 一个完整可游玩人物需要什么

1. **网格（Mesh）**：决定人物外形。
2. **UV**：规定二维贴图如何包到三维表面。
3. **贴图与材质**：基础颜色、法线、粗糙度、金属度等决定表面外观。
4. **骨骼（Skeleton）**：人体内部的可动画关节层级。
5. **蒙皮权重（Skin Weights）**：规定每块皮肤跟随哪些骨骼以及跟随多少。
6. **动画片段**：待机、走路、跑步、起跳、下落和落地等动作数据。
7. **混合空间与动画蓝图**：按速度、方向和是否在空中自动选择并平滑混合动作。
8. **胶囊体碰撞**：代表角色在游戏世界里占据的空间，负责挡墙和站在地面上。
9. **Character Movement**：计算加速、减速、重力、跳跃、斜坡和碰撞移动。
10. **输入映射**：把键盘、鼠标和手柄输入转换成移动、视角和跳跃命令。
11. **摄像机和弹簧臂**：跟随玩家并在墙边缩短距离，减少穿墙。
12. **玩家角色蓝图**：把模型、动画、碰撞、移动、输入和摄像机组合起来。
13. **游戏模式**：告诉关卡应该生成哪个玩家角色和控制器。
14. **Player Start 与测试地图**：提供出生位置和可以检查全部功能的环境。

正式游戏通常还需要 LOD、物理资产复核、脚步声、交互接口、受击和死亡、网络同步、存档以及素材授权记录。这些属于后续玩法和性能制作，不影响本示例角色的基本操控。

## 为什么关闭了人偶 Foot IK

该角色来自 Tripo 骨架，没有 UE Manny/Quinn 使用的 `ik_foot` 等 IK 骨。原人偶 Foot IK Control Rig 会按不存在的骨骼修正双腿，导致腿部被拉开。因此 `ABP_Unarmed` 的最终姿势从 `Slot 'DefaultSlot'` 直接输出，原 Control Rig 节点保留但已经断开。不要重新连接它，除非先给角色建立匹配的 IK 骨和 Control Rig。

## 迁移到未来的 UE5 主项目

1. 同时关闭正在运行的游戏。
2. 在本工程内容浏览器里找到 `Content/orc_character/Women_Motified_Ultimate/FinalGameTest`。
3. 右键需要的角色资产，选择 **资产操作（Asset Actions）→ 迁移（Migrate）**。
4. 在资产报告中保留 UE 自动勾选的依赖。
5. 选择新项目的 `Content` 文件夹。
6. 打开新项目，检查输入映射、默认游戏模式和关卡中的 Player Start。

不要在资源管理器里只复制单个 `.uasset`。动画蓝图、动画序列、材质、贴图和输入资产互相引用，拆开复制会出现丢贴图、丢动画或蓝图无法编译。

## 修改或替换模型

- 美术源文件位于 `SourceAssets`。
- 建模修改应保留角色朝向、厘米单位、骨骼名称、骨骼层级和材质槽顺序。
- 只改外形时优先保留现有 Skeleton；改变骨骼结构后需要重新绑定、重新检查全部动画。
- 重导入后依次检查材质、参考姿势、肩膀、手指、髋部、膝盖、脚底和全部移动动画。
- 先在分支或副本中重导入，验证后再替换团队资产。

## 验证

在 PowerShell 中运行：

```powershell
.\Tools\Verify-Package.ps1 -EditorPath "E:\LaLaGame\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
```

脚本会检查核心文件、单文件大小、UE 资产加载以及动画蓝图的最终连接，并在完成后清除运行产生的缓存目录。
