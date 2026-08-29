# Agent 动态身份人设机制 · 交付说明

日期：2026-08-28

本次按 `（最新）8.28agent 人设机制 设定.md`（v0.6 基线）安装动态身份人设机制。核心变化：**废弃"角色围绕玩家建关系"的旧前提，改为四人彼此之间有戏、开局都不认识玩家**，并装上 §4–§9 的动态身份包引擎。

## 一、已确认的五个决定

1. 范围＝人设 + 完整动态身份包引擎（§4–§9 全装）。
2. 玩家旧关系＝照文档全部废弃（A 不再是玩家伴侣、C 不再是旧情人、B 不再对玩家有好感）。
3. 背景输入＝三道轻量选择题 + 现有 200 字自由输入并存，选择题优先。
4. 姓名＝kiko(A) / X(B) / 万塞(C) / 一桐(D)；文档正文里 C 的 "Nora" 视为笔误统一为万塞。
5. 开场前提＝按入口分流（solo / friend_invited / event_guest 各有自己的开场理由）。

## 二、改动清单

### 服务端（全部完成并测试通过）

| 文件 | 改动 |
|---|---|
| `scenarios/last_call.json` | 改名；重建 facts（四人彼此关系）；`initialRelations` 改为四人之间的有向关系；改写 `voices` 台词库；新增 `personas`（四人内核+声口+80/20）、`identityPacks`（五个身份包）、`choices`（三道选择题）；`roles` 描述与 `arrival`/`signal`/`a_arrival`/`c_window` 开场文案去玩家关系断言 |
| `src/types.ts` | 新增 `Persona` / `IdentityPack` / `Choice` / `ContextProfile` / `PackRevision` 等类型；`World` 加 `identityPack`、`contextProfile`、`pendingPackRevision` |
| `src/identity.ts`（新） | `buildProfile`（只提取明确内容）、`selectPack`（纯函数、可复现）、`identityBrief`（压缩摘要） |
| `src/config.ts` | 校验 personas 覆盖 A/B/C/D、身份包覆盖四角色且 sceneSkin 键合法、voices 五键齐全、initialRelations 引用真实角色 |
| `src/world.ts` | 开局算 profile + 选包，落到 world |
| `src/engine.ts` | 选项透传 choices；`nextNight` 保留身份包与 context profile |
| `src/intro.ts` | `IntroState` 加 `choiceAnswers`；开场后台事件改为 B 等 A（不是等玩家） |
| `src/decisions.ts` | `agentContext` 注入 `identity` 摘要；删掉假设玩家关系的 interpretation 文案与 A 的 arrival 分支 |
| `src/beats.ts` | 开场文案按 entryMode 分流；场景皮肤按 beat 覆盖；在 beat 边界消费 `pendingPackRevision` |
| `src/commands.ts` | 新增 `revise_context` 命令（中途改背景，延迟到下一 beat 边界生效） |
| `src/model.ts` | 系统指令补充：身份外壳可换、内核不可换；单场最多 1–2 个新概念 |
| `src/server.ts` | `/api/bootstrap` 返回 `choices`；`/api/session` 接收并透传 `choices` |
| `src/view.ts` | 复盘页四个 `initial` 标签改为"开局都不认识玩家"的中性说法 |
| `src/tests/identity.test.ts`（新） | 复现性、模糊回退、隐私不泄露、中途改包保留状态、包覆盖校验等 9 项 |
| `src/tests/runtime.test.ts` | 两处 fact key 由 `user_b_attraction` 改为新的四人关系 key（断言语义不变） |
| `ARCHITECTURE.md` | 补身份包说明 |

### 客户端（Unity，已改，需编辑器验证）

| 文件 | 改动 |
|---|---|
| `Assets/LastCall/Runtime/Contracts.cs` | 新增 `ChoiceDto` / `ChoiceOptionDto` / `ChoiceAnswersDto`；`BootstrapDto` 加 `choices`；`SessionRequest` 加 `choices` |
| `Assets/LastCall/Runtime/LastCallEntry.cs` | 入口界面加"04 关于你 · 可跳过"三道循环选择题，随 session 提交 |

客户端**不改任何人设内容**：角色名字全部来自服务端 payload，改名零客户端改动；按 actor id 的美术与六人 roster 本次不增删，未动。

## 三、机制要点

- **两层隐私**：系统用清洗后的 Context Profile 选身份包；原始玩家输入**绝不进入任何 Character Agent**，Agent 只在玩家场内说出口后才能引用。
- **选包是纯函数**：关键词打分 + 阈值，同 (选择题答案, 自由文本, seed) 必得同包，不花模型调用，满足 §12 复现。
- **身份不变量**：只换职业外壳（publicRole / position / corpus / 场景皮肤），不换人格内核、性取向、关系边界与主要矛盾。
- **场景皮肤**：五个身份包各带 `arrival`/`signal`/`last_call` 皮肤，换皮不换命运问题；不新增 beat effect（八个 effect 名仍是不可扩展的契约）。
- **中途修改**：`revise_context` 挂起 `pendingPackRevision`，在下一 beat 边界切换，保留关系状态与本局记忆。

## 四、验证结果

- `npm test`：**80 / 80 通过**（原 71 项 + 新增 identity 9 项），全部离线，含五个入口 × 720 秒离线夜。
- HTTP 冒烟：`/api/bootstrap` 返回 `choices`（domain 5 / career_stage 4 / preferred_topic_density 4）；`/api/session` 接受 `choices` 并成功开局。
- 联网语气验收（§12 的"同行语言但不炫术语"）**未跑**：本机 DeepSeek 配置已确认可用（密钥与模型 deepseek-v4-flash 均就绪），`npm run test:live` 可随时真跑，会消耗真实额度。

## 五、待验证 / 说明

1. **Unity 端未编译验证**：本环境无 Unity。入口界面的三道选择题 UI 和 `choices` 序列化需在编辑器里打开确认（`LastCallEntry.cs` 里新增了循环选择按钮，表单高度 654→720）。
2. **戏剧张力已变化**：开局冲突从"玩家被追问隐瞒"变成"玩家旁观四人关系场"，这是文档要的方向，但手感差别明显，建议第 1 步改完就先试玩一轮确认。
3. **`nextNight` 会保留身份包**：同一存档续夜时玩家的背景不丢，回到 default 的只有全新开局。
4. **密钥安全**：DeepSeek 密钥只存于 `%USERPROFILE%\.lalagame\private\model.env`，未写入工程或提交。因密钥在对话里明文出现过，建议之后在 DeepSeek 控制台轮换一次。
