# LALAGAME · Last Call

这是新增的关系体验 MVP。原酒吧漫游的场景和运行包仍保留。

## 启动

双击 Builds/LastCall-Windows/LastCall.exe，或运行同目录的「启动LastCall.cmd」。
必须保留整个运行目录；游戏会隐藏启动自己的本地服务，退出时保存并关闭服务。
玩家不需要安装 Node、Unity 或登录团结引擎。

第一次建议选「熟人来客 → 随便看看 → 自然聊天」，在线和离线都可玩。
五种入口不会重置你与 A、B、C 的历史。所有角色均为虚构成年人。

## 操作

- WASD / 方向键：按屏幕方向移动；Shift：快走。
- 鼠标点击角色或右侧名字：选择对象；E：选择最近的人。
- 「走近她」：自动寻路，手动移动可取消。
- 先选社交意图，再点击一条推荐表达，或输入最多 200 字，点击「说给她听」。
- 输入文字、等待关键回复、打开线索册和暂停菜单时，夜晚时间停止。
- 「去哪里」：自动前往场景区域；「观察」：记录附近可见信息。
- Esc：暂停 / 继续。「离场并回看」可以提前结束；拒绝牌局不会卡住进程。
- 窗口失去焦点会自动暂停。恢复游戏后点「继续今晚」。

一晚约 12 分钟有效世界时间，对应 22:35—02:00；等待回复、暂停等不计时。
默认 1280×720 可调整窗口。锁定的情境牌需要加入牌局，Last Call 要到闭店前才开放。

## 演示路线

1. 入场看到「有人已经替你点了」。选 B → 走近她 → 表达 → 第一条推荐 → 说给她听。
2. 留在附近观察 B 与调酒师的反应。她们只能使用自己听到或看到的信息，可能误解，也可能沉默。
3. 23:25 左右出现牌局邀请（社交顾客或「认识新人」更早）。参加或拒绝均可继续。
4. A 稳定到场。可以转向 A、继续与 B 相处，或设边界；没有强制争吵或最终选择。
5. 想遇见 C：加入牌局、使用「点酒」点旧时光，00:20 后到 13 号座。
6. 想遇见 D：01:15 后在 13 号座独处，最近三分钟至少三次观察或设界限，最近 90 秒没有推进亲近。
7. Last Call 后再表达一次，或从暂停菜单离场。回看展示可观察的变化、关键片段和已知传播链。

C/D 是条件机会，不保证每晚全部出现。去门外不是换地图。照片仅是游戏内邀请，不会拍摄或发布真实照片。

## 模型与隐私

使用你指定的 https://api.openai-next.com/v1 和 gpt-4.1-mini。
在线会把本局玩家表达及该角色有权知道的上下文发送到此第三方网关。
它不是 OpenAI 官方域名；兼容性以实际网关返回为准。

你提供的密钥已仅配置在本机：

    %LOCALAPPDATA%\LALAGAME\private\model.env

文件键名为 LASTCALL_API_BASE、LASTCALL_MODEL、LASTCALL_API_KEY。
不要把真实密钥复制到源码、场景、日志或运行包。给别人分发时，对方默认可离线游玩，在线需自行配置。

每局最多 80 次请求、12 万 token；重试计入预算，最多两个并发，单次 8 秒、最多一次重试。
预算不是费用保证。无权限、模型错误、网络失败或非法结果会明确降级，不自动改用其他模型。

本机存档：%LOCALAPPDATA%\LALAGAME\last-call.db，包含事件、记忆、快照和已返回的决策。
每个玩家标识和每个世界独立。选择「继续上次的夜晚」恢复；回看页可带着记忆开始下一晚，或换入口独立新开局。
下一晚是同一套场景的关系延续，不是第二套剧情。

## 编辑与重建

用团结引擎 2022.3.62t14 打开本工程，场景为 Assets/Scenes/LastCall.unity。
原版是 Assets/Scenes/AmberRoom.unity。不要用两个编辑器同时打开同一工程；内存紧张时先自行释放内存。

后端源码在 Server/src，角色、身份、卡牌、对白风格、关系与时间机会在 Server/scenarios/last_call.json。
条件只接受已定义的声明式名称，不执行配置代码。导航网格在 Server/scenarios/navigation.json。
更改家具碰撞后须重新导出导航；修改剧情后须重启本地服务。

在工程父目录 PowerShell 运行：

    .\Tools\Build-LastCall.ps1

首次还原后端依赖：进入 Server 执行 npm ci，再 npm run build。需要 Node 24.12.x。
本机编辑器运行默认使用 D:\node.exe，也可以把对应运行时放到 Server/node.exe。

Build-LastCall.ps1 -Prepare 会从原漫游场景重新生成 Last Call 场景并导出导航，会覆盖 LastCall 场景的手工编辑。普通打包不要加此参数。

## 测试

后端：在 Server 运行 npm test。
真实模型单次探测：npm run test:live（会产生实际网关调用）。
完整夜晚服务端模拟：node dist/live-night.js；加 --online 会实际调用网关。
Unity Test Runner 的 EditMode 测试同时检查原版和 Last Call。

实际窗口自动检查需要显式参数 -lastCallVerify -lastCallArtifacts "绝对输出目录"；
默认离线，加 -lastCallOnline 才联网。仅测试时使用，期间保持游戏聚焦，不要按键。
测试存档单独位于 %LOCALAPPDATA%\LALAGAME\Verification，不污染正常进度。

具体通过项、采样结果和未覆盖项见 Verification/LASTCALL_测试报告.md。

## API 实现参考

接入时使用了 OpenAI Docs 技能核对 [gpt-4.1-mini](https://developers.openai.com/api/docs/models/gpt-4.1-mini) 和 [结构化输出说明](https://developers.openai.com/api/docs/guides/structured-outputs)。
为了兼容指定网关，本项目使用 Chat Completions 的 JSON mode，并在本地另外进行 JSON Schema 和行动权限校验；JSON mode 本身不保证字段合法。
