# Lalaland Unreal

Lalaland 的 Unreal Engine 5.8 客户端。该目录与 `BarPrototype` 并列，保留 Node.js/Fastify 权威叙事后端，通过认证的 Protocol v1 通信。

## 当前酒吧首夜

- 独立的 Unreal 存档与模型配置目录：项目／玩家包的 `Saved/LalalandData`。
- 玩家自填 OpenAI 兼容 API；密钥不进入存档、日志或打包资源。
- 第一人称 WASD，按住鼠标右键转头。
- 最多三个情境动作和始终可用的定向自由输入，不使用几十个常驻按钮。
- 新版首夜内容：自由认识四人、点酒／请客／饮用、五人弹球入杯、奖励、流星雨、露台邀请与见闻结算。
- 弹球使用 Chaos 刚体，必须真实落桌后入杯；人物、桌椅、楼梯与屋顶使用 Capsule／碰撞体和 CharacterMovement。
- A 沿用既有模型；B（X）、C（万塞）、D（一桐）采用 2026-09 新 FBX。新模型使用自己的 61 骨骼，导入工具自动生成 IK Rig／Retargeter，并分别生成 64、100、84 段重定向动作。运行时只加载与当前 Mesh Skeleton 一致的动作，避免倒地、T Pose 和半身陷入地面；头顶气泡使用真实头骨锚点。
- Windows 使用认证 HTTP 以 5 Hz 同步本地状态，避免系统代理影响 WebSocket；其他平台保留 WebSocket。
- 自由输入支持 Enter 发送；模型生成期间只锁定当前对话，失败时保留原话并提供幂等重试。
- Lounge BGM 会根据入场、弹球、赛后选歌、流星雨、露台和结算阶段动态调整，酒杯、门、电梯、手机与游戏事件有独立音效。

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

默认输出到 `D:\LalalandBuilds\Windows`。2026-09-28 新模型验收包位于 `D:\LalalandBuilds\Windows-NewModels-20260928`。玩家从归档目录双击 `Lalaland.exe`；运行包自带 Node 24 和本地叙事服务，不需要 Unreal Editor、Unity 或另行安装 Node.js。

当前首夜输出目录和实测结果见仓库根目录 README。玩家包已完成 Shipping 独立启动、随包 Node 子进程、物理审计、密钥扫描及存档／日志／PDB 排除检查；SHA-256 写入运行包根目录的 `SHA256SUMS.txt`。

## 验证结果

- Node 叙事后端：179 / 179 通过。
- Unreal 协议与音频自动化：3 / 3 通过。
- 新模型导入与动作：B 64、C 100、D 84 段重定向动作生成成功；1280×720 可见窗口确认人物站立且脚底落地。
- 定向 A2A：真实在线接口完成 A→B、B→C、C→D、D→A 的发起与回应，共 8 次 AI 生成；旁观者只感知，不抢答。
- 完整首夜：干净存档从入场跑到物理楼梯、屋顶和 `settled`，最终记录 5 个关键行为与 4 份角色评估。
- Chaos 弹球审计：真实落桌一次并进入杯体触发区。
- 楼梯审计：CharacterMovement 从主厅连续步行到屋顶层。
- 发布包审计：未发现 API Key；不含 `.log`、`.db`、`.sav` 或 `.pdb`。

## 安全边界

- 本地服务只监听 `127.0.0.1` 动态端口。
- 每次启动使用新的 bearer token，通过子进程环境传递。
- API Key 仅发送到本地服务，日志中不打印请求密钥或模型供应商响应体。
- Unreal 版不读取、转换或覆盖 Unity/Tuanjie 存档。
