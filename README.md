# Lalaland

第一人称 AI 社交叙事原型。玩家从电梯进入深夜酒吧，与 A–D 及调酒师自由观察、移动和交流，经历第三杯、动态社交、塔罗、走廊、断电散场与屋顶收尾。

> 当前仓库保存源码、场景、模型、剧情文档与验收材料。玩家运行包发布在 [192tt/Lalaland](https://github.com/192tt/Lalaland)。

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
- **玩家模型配置**：首页可填写 OpenAI 兼容接口地址、模型 ID 和 API Key；密钥只写入当前用户的私有配置，不进入游戏目录、ZIP、存档或日志。

## 玩家包

- Windows：[Release 下载页](https://github.com/192tt/Lalaland/releases/tag/windows-bgm-api-20260831)，约 193 MB。
- SHA-256：`bdf4ef72223a523afda031ad72c9b8e55814e585abfc7bc98f908a59d4ee9e59`。
- 玩家解压整个 ZIP 后双击 `启动游戏.cmd`；不需要安装团结引擎、Unity、Node.js 或开发工具。
- ZIP 内有一个完整运行文件夹，共 894 个文件；归档内容已逐文件对照白名单运行目录校验。
- 最终目录 1280×720 启动验收通过 13 / 13；模型 API 首页表单在 1920×1080 通过 35 项可见检查。后端 151 / 151、编辑器 28 / 28 通过。
- 独立 Scene 2→塔罗旧夹具仍未纳入本次门禁，相关主流程由整晚 57 项用例覆盖。BGM 对外发布前还需确认素材授权。

- macOS：[Release 下载页](https://github.com/192tt/Lalaland/releases/tag/macos-bgm-api-20260831)，Intel 与 Apple Silicon 双架构。
- SHA-256：`48deefe33294eac1edb95d354f12f1d0938f97dbecc72d822ac16f95cd226eb1`。
- Mac ZIP 共 906 个文件，逐项内容校验通过；6 个启动文件保留 Unix 可执行权限。
- macOS 包由 Windows 交叉编译，尚未进行 Apple 签名、公证或真实 Mac 窗口验收，首次打开需按发布说明处理 Gatekeeper 提示。

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
- Windows x64 / Mono；macOS Universal / Mono。
- 主场景：`BarPrototype/Assets/Scenes/LastCall.unity`。

首次运行后端：

```powershell
cd .\BarPrototype\Server
npm ci
npm run build
```

在团结 Hub 中添加 `BarPrototype`，使用对应版本打开主场景即可编辑或 Play。不要同时用两个编辑器打开同一工程。

## 测试与构建

```powershell
# 服务端
cd .\BarPrototype\Server
npm test

# 回到仓库根目录后运行编辑器测试或构建
cd ..\..
.\Tools\Build-Scene0.ps1 -Action Test
.\Tools\Build-Scene0.ps1 -Action Build
```

可见窗口整晚验收由 `Tools/Verify-FullNight-Windows.ps1` 执行。运行目录由 `Package-FullNight-Exe.ps1` 生成，玩家 ZIP 由 `Package-Player-Zip.ps1` 逐文件校验后生成。

macOS Universal Player 由 `LastCall.Editor.LalalandReleaseBuilder.BuildMacOS` 构建，`Tools/package_lalaland_macos.py` 在 Windows 上生成带 Unix 权限元数据的 ZIP，并逐文件复核归档内容。

当前模型配置改动完成服务端 151 项、编辑器 28 项、1080p 35 项和最终目录 13 项检查；既有整晚 57 项流程报告继续作为 Scene 0–6 主路线证据。剩余边界见[未完成工作文档](BarPrototype/未完成工作_前端交互改造.md)。

## 工程结构

- `BarPrototype/Assets/LastCall`：角色、动作、Scene 0 资产与 Last Call 客户端代码。
- `BarPrototype/Assets/Scenes/LastCall.unity`：当前连续夜晚主场景。
- `BarPrototype/Server`：本地权威状态、AI 请求、角色感知、剧情推进、存档与测试。
- `BarPrototype/Design`：Scene 0–6 原始剧情与设计材料。
- `BarPrototype/Verification`：测试报告、截图与诊断材料；日志和本地运行数据不纳入交付。
- `Tools`：构建、可见窗口验收、白名单打包与审计脚本。
- `新置换模型素材库`：A–D 原始置换模型素材。

## 隐私与模型配置

在线模型配置可在游戏首页填写，也可从本机私有配置读取。远程接口必须使用 HTTPS，本机回环接口可使用 HTTP。仓库和玩家包不得包含 API 密钥、玩家存档、数据库、日志或私人附件。配置文件不会回显密钥，但它不是加密保险库，建议玩家使用有限额、可撤销的独立密钥。没有模型配置时可以明确选择离线规则模式；程序不会把规则台词标成 AI 回复。

## 保留的旧原型

仓库仍保留固定斜俯视的“琥珀酒馆 / The Amber Room”原型。旧场景与构建方式见 [BarPrototype/README.md](BarPrototype/README.md)，当前开发主线以 `LastCall.unity` 为准。
