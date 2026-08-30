# LALAGAME · Last Call

第一人称 AI 社交叙事原型。玩家从电梯进入深夜酒吧，与 A–D 及调酒师自由观察、移动和交流，经历第三杯、动态社交、塔罗、走廊、断电散场与屋顶收尾。

> 当前仓库保存源码、场景、模型、剧情文档与验收材料。2026-08-30 已生成带 BGM 的本地 Windows 候选目录；完整回归的剩余事项仍见[未完成工作](BarPrototype/未完成工作_前端交互改造.md)。旧版 Release 不代表当前源码状态。

## 当前版本

- **Scene 0–6 连续流程**：电梯入场、第三杯、动态社交、Social Tarot、走廊私聊、夜深断电、楼梯与屋顶开放尾声。
- **三页新游戏入口**：身份、今晚目的、聊天风格；每页只回答一个问题，选中后点“下一步”，可返回改选。
- **统一交互语言**：剧情一级入口固定为 **观察 / 移动 / 互动**，线索册与暂停／存档位于独立系统区。
- **章节下一步**：每章显示当前目标和推进方式；自由对话、AI 失败、角色沉默或纯旁观不会被当作唯一通关条件。
- **定向 AI 对话**：玩家选择谁就由谁回应；其他角色只能按实际距离和遮挡旁听，不自动插嘴。
- **来源与重试**：对白记录生成来源；在线失败保留玩家原话并允许重试，不用固定规则句冒充 AI。
- **A–D Humanoid 动作**：行走、转身、交谈、倾听、坐立与剧情手势使用真实骨骼；头部、双手用于气泡和道具锚点。
- **第一人称表现**：NPC 对白在头顶气泡显示，玩家发言位于画面下方；交谈双方自然面对彼此。
- **信息边界**：姓名、私聊、隔墙／跨层声音、回忆与回顾内容按玩家和角色实际感知过滤。
- **循环 BGM**：酒吧音乐从 `Resources/Audio/bgm-jazz-rnb.mp3` 加载，以 20% 音量循环播放，给对白和环境音留出空间。

## 当前 BGM Windows 构建

- 完整运行目录：`BarPrototype/Builds/FullNight-Windows-bgm-20260830-v3`。
- 启动文件：`LastCall.exe`；必须保留目录内的 `LastCall_Data`、`MonoBleedingEdge`、`Server` 和播放器文件。
- exe SHA-256：`61263a2aad52e1be6961139f628d00eb2abaf6799134f69f4062bc5e33178f1b`。
- 1280×720 白名单目录启动验收通过 13 / 13，电梯交接耗时 7.227 秒，平均 59.99 FPS；最终审计未发现密钥、存档、日志或调试产物。
- 本次复用了已通过的整晚 57 项、720p、800p、Scene 0、卡牌、后端与编辑器结果，并重新通过 1080p 33 项检查。独立的 Scene 2→塔罗自动化夹具因过时前置条件和空引用未计入本次打包门禁，相关主流程仍由整晚用例覆盖。
- 没有生成 ZIP。BGM 的来源与再分发授权尚未写入素材记录，对外发布前必须确认。

## 操作

- `WASD`：移动。
- 按住右键或使用画面提示键：转动视角。
- 鼠标点击：选择人物、一级菜单、二级动作与系统功能。
- 文字交流：选择 **互动 → 文字交流**，输入内容后发送。
- `Esc`：关闭输入／面板或打开暂停菜单。

游戏运行时必须保留完整目录，不能只复制 `LastCall.exe`。

## 开发环境

- 团结引擎 `2022.3.62t14`，revision `1f04f7aba499`。
- Node.js `24.12.x`。
- Windows x64 / Mono。
- 主场景：`BarPrototype/Assets/Scenes/LastCall.unity`。

首次运行后端：

```powershell
cd .\BarPrototype\Server
npm ci
npm run build
```

在团结 Hub 中添加 `BarPrototype`，使用对应版本打开主场景即可编辑或 Play。不要同时用两个编辑器打开同一工程。

## 测试与 Windows 构建

```powershell
# 服务端
cd .\BarPrototype\Server
npm test

# 回到仓库根目录后运行编辑器测试或构建
cd ..\..
.\Tools\Build-Scene0.ps1 -Action Test
.\Tools\Build-Scene0.ps1 -Action Build
```

可见窗口整晚验收由 `Tools/Verify-FullNight-Windows.ps1` 执行。最终仅生成 Windows 完整运行目录，不生成 ZIP。

当前代码曾完成服务端 153 项、编辑器 28 项以及整晚 57 项可见流程检查；最后一次交互保护修改后的完整复跑尚未全部完成，因此交付前仍须按[未完成工作文档](BarPrototype/未完成工作_前端交互改造.md)重新确认。

## 工程结构

- `BarPrototype/Assets/LastCall`：角色、动作、Scene 0 资产与 Last Call 客户端代码。
- `BarPrototype/Assets/Scenes/LastCall.unity`：当前连续夜晚主场景。
- `BarPrototype/Server`：本地权威状态、AI 请求、角色感知、剧情推进、存档与测试。
- `BarPrototype/Design`：Scene 0–6 原始剧情与设计材料。
- `BarPrototype/Verification`：测试报告、截图与诊断材料；日志和本地运行数据不纳入交付。
- `Tools`：构建、可见窗口验收、白名单打包与审计脚本。
- `新置换模型素材库`：A–D 原始置换模型素材。

## 隐私与模型配置

在线模型配置只从本机私有配置读取。仓库与 Windows 运行目录不得包含 API 密钥、玩家存档、数据库、日志或私人附件。没有模型配置时，玩家可以明确选择离线规则模式；程序不会把规则台词标成 AI 回复。

## 保留的旧原型

仓库仍保留固定斜俯视的“琥珀酒馆 / The Amber Room”原型。旧场景与构建方式见 [BarPrototype/README.md](BarPrototype/README.md)，当前开发主线以 `LastCall.unity` 为准。
