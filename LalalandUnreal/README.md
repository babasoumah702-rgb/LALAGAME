# Lalaland Unreal

Lalaland 的 Unreal Engine 5.8 客户端。该目录与 `BarPrototype` 并列，保留原 Node.js/Fastify 叙事后端，通过认证 HTTP 与 WebSocket 使用 Protocol v1。

## 当前竖切片

- 独立的 Unreal 存档与模型配置目录：`%LOCALAPPDATA%/LalalandUnreal`。
- 三页单题入口和玩家自填 OpenAI 兼容 API。
- 第一人称 WASD，按住鼠标右键转头。
- 观察／移动／互动三级 UI、固定选项状态和定向文字交流。
- 程序化电影感夜间酒吧、角色 Capsule、第三杯和 D 入场可见状态。
- A–D 原始 FBX、材质与内嵌动作已导入；运行时按移动、姿态和剧情手势选择动作，头顶气泡使用真实头骨锚点。
- 本地事件优先使用认证 WebSocket；Windows 上握手失败时自动切换到认证 HTTP 状态同步，不阻塞电梯和章节时间。

## 开发环境

1. 在 Epic Games Launcher 把 Unreal Engine 5.8 安装到 `D:\Epic Games\UE_5.8`。
2. Visual Studio Installer 安装“使用 C++ 的游戏开发”和 Unreal Engine 工具。
3. 确保 `D:\node.exe` 存在，或让 Node 24 的 `node.exe` 可从 `PATH` 找到。
4. 构建后端：

```powershell
cd ..\BarPrototype\Server
npm ci
npm run build
```

5. 运行 `Tools\Setup-Unreal.ps1 -ImportCharacters` 生成 VS 工程、导入人物并暂存本地服务，随后打开 `LalalandUnreal.uproject`。

## Windows Shipping

```powershell
.\Tools\Build-Unreal-Windows.ps1
```

默认输出到 `D:\LalalandBuilds\Windows`。玩家从归档目录双击 `Lalaland.exe`；运行包自带 Node 24 和本地叙事服务，不需要 Unreal Editor、Unity 或另行安装 Node.js。

2026-09-19 竖切片实际输出：`D:\LalalandBuilds\Windows-UE-Scene01-20260919`。该目录已完成 Shipping 独立启动、随包 Node 子进程、1280×720 可见截图、密钥扫描及存档／日志／PDB 排除检查。SHA-256 见运行包根目录的 `SHA256SUMS.txt`。

## 验证结果

- Node 叙事后端：151 / 151 通过。
- Unreal 协议自动化：2 / 2 通过。
- Win64 Shipping：编译、Cook 841 个包、IoStore、Stage 和 Archive 通过。
- 发布包审计：未发现 API Key；不含 `.log`、`.db`、`.sav` 或 `.pdb`。

## 安全边界

- 本地服务只监听 `127.0.0.1` 动态端口。
- 每次启动使用新的 bearer token，通过子进程环境传递。
- API Key 仅发送到本地服务，日志中不打印请求密钥或模型供应商响应体。
- Unreal 版不读取、转换或覆盖 Unity/Tuanjie 存档。
