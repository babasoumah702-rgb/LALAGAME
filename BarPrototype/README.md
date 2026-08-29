# BarPrototype｜Last Call 与琥珀酒馆

当前主线是第一人称 AI 社交叙事 **LALAGAME / Last Call**，主场景为 `Assets/Scenes/LastCall.unity`。它已经串联 Scene 0–6，并加入三页新游戏入口、观察／移动／互动两层菜单、章节下一步、定向 AI 回复、A–D Humanoid 动作、头顶气泡、存档和屋顶收尾。

Windows 最终运行目录仍在收尾，当前 `Builds/Scene0-Windows` 只是候选构建源，不能直接视为正式交付。剩余测试与打包事项见[未完成工作_前端交互改造.md](未完成工作_前端交互改造.md)。按要求最终只交付完整 exe 运行目录，不生成 ZIP。

以下内容主要记录仍保留的原版“琥珀酒馆”漫游。

可玩的固定斜俯视、低多边形 2.5D 酒吧漫游原型。场景、角色 Prefab、材质及 Windows 程序均已生成。

## 直接游玩

打开 `Builds/Windows/AmberRoom.exe`，或解压 `Builds/AmberRoom-Windows.zip` 后运行其中的 `AmberRoom.exe`。运行游戏不需要安装编辑器或登录账号。

- WASD / 方向键：按屏幕方向移动。
- 左右 Shift：快走。
- Esc：暂停 / 继续；暂停菜单可以退出游戏。
- 窗口失去焦点时自动暂停，回到游戏后按 Esc 或点击继续。

默认 1280×720 可调整窗口，支持 16:9 / 16:10。`启动游戏.cmd` 可按 1280×720 窗口启动。必须保留完整运行目录，不能只复制 exe。

## 编辑工程

本工程使用用户已安装并激活的**团结引擎 2022.3.62t14**（revision `1f04f7aba499`），并非最初规划的 Unity 6.3。请优先用此版本重新打开，跨版本升级前先备份。

- 编辑器：`D:\unity cn\Editor\Tuanjie.exe`。
- 工程目录：`D:\工作\项目\黑客松\LALAGAME\LALAGAME\BarPrototype`。
- 在团结 Hub 中添加现有工程，打开 `Assets/Scenes/AmberRoom.unity`，点击 Play。
- 场景内 Player 的 PlayerMotor 可调整速度和转向；Main Camera 的 FixedRoomCamera 可调整构图参数。
- 主角源资产为 `Assets/Prefabs/Player.prefab`。场景家具、灯光、材质均可直接编辑。

启动编辑器前建议释放内存；不要同时用两个编辑器打开同一工程。编辑器需要有效许可证，独立运行程序不需要。

依赖已锁定于 `Packages/packages-lock.json`：URP 14.2.0-t1、Input System 1.14.4-t1、Test Framework 1.1.33、Burst 1.8.30-t2、Mathematics 1.3.2。Windows x64 / Mono，无 Android、iOS 或 IL2CPP 要求。

## 重建与测试

普通编辑不需要重新生成场景。需要从代码重新生成或打包时，在父目录 `D:\工作\项目\黑客松` 打开 PowerShell：

```powershell
.\Tools\Build-Bar.ps1 -Action Prepare
.\Tools\Build-Bar.ps1 -Action Test
.\Tools\Build-Bar.ps1 -Action Build
.\Tools\Build-Bar.ps1 -Action Smoke
```

Prepare / Test / Build 运行前关闭正在打开本工程的编辑器。编辑器位置不同时增加 `-EditorPath '实际路径\Editor\Tuanjie.exe'`。Smoke 会打开真实游戏窗口并在结束后自动退出，期间请保持窗口聚焦，不要按键。

也可使用编辑器菜单 `Amber Room > 1 - Create or rebuild the prototype scene` 和 `Amber Room > 2 - Build Windows game`。

**重新生成会覆盖自动生成场景、角色 Prefab 和生成资产；有手工修改时请先备份或另存。** Build 只更新 Windows 运行目录，不会自动更新发布 ZIP。

## 工程组织

- `Assets/Scripts`：移动、程序化角色动画、固定镜头、暂停 UI、显式启用的运行测试。
- `Assets/Editor`：场景生成、URP 设置与 Windows 打包。
- `Assets/Tests/EditMode`：方向、速度归一化、场景资产和镜头构图测试。
- `Assets/Generated`、`Assets/Prefabs`、`Assets/Scenes`：持久化的可编辑资产。
- `Builds/Windows`：完整可运行程序；发布 ZIP 不包含调试符号目录。
- `Verification`：测试结果、运行截图及本地诊断日志。验收范围见 `Verification/STATUS.md`。

## 已验证结果

2026-08-27：编辑器编译和 Windows 构建成功；10 项 EditMode 测试、37 项独立程序运行检查全部通过。包含八方向、输入绑定、加速、松键停止、帧率一致性、边界/部分家具碰撞、贴墙滑动、暂停恢复和材质检查。

RTX 4070 Laptop GPU 上 1920×1080、5 秒采样平均 59.98 FPS；这是当前场景短时测量，不代表所有设备或长期运行表现。16:9、16:10 截图已检查；用户已确认退出按钮有效。交互式编辑器 Play 和全场所有家具缝隙/角落的人工遍历未逐一验收。

## 素材与范围

几何体、配色和动画由项目代码生成，无付费美术素材或外部 API。中文 UI 使用系统字体，不重新分发微软雅黑。

首版仅单房间键盘漫游，不包含跳跃、NPC、对话、经营、战斗、联网或存档。
