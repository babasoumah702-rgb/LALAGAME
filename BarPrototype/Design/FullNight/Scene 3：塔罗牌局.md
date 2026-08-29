# Scene 3：塔罗牌局

# Scene 3：塔罗牌局

# La La Land｜Scene 3 开发稿

## Scene 3｜闭店前最后一局：塔罗

**时间**：约 00:10–00:35
**建议时长**：5–8 分钟
**视角**：Player 第一人称
**空间**：主桌 / 吧台活动区
**在场**：Player、A、B、C、D、Bartender，普通顾客逐渐减少
**固定锚点**：闭店前最后一个社交活动——La La Land Social Tarot
**核心目标**：

> 用“随机问题 → 自由回答 → 观察与误读 → A2A Ripple”，第一次真正把 A/B/C/D 原本藏在工作与社交下面的关系暗线撬开。
> 
> 

---

# 1\. 场景背景

La La Land 不只是商业社交酒吧，也长期存在偏娱乐、心理探索和女性社群性质的活动：

- 塔罗；

- 社交卡牌；

- 匿名问题；

- 情绪小游戏；

- 心理体验；

- 女性创作者活动。

前面的热场游戏已经进行过一段时间。

此时夜越来越深：

- 人开始减少；

- 音乐降低；

- 桌上的空杯越来越多；

- A/B/C/D 第一次重新坐到同一张桌上。

Bartender 收掉前一轮游戏道具，只留下一副 **La La Land Social Tarot**。

视觉上完成第一次明显转变：

> **流动社交 → 一张桌子的关系局。**
> 
> 

---

# 2\. Tarot Reader 动态产生

不是固定 Bartender 主持。

```text
tarot_reader =
A | B | C | D | Bartender
```

根据人物状态加权：

- **B**：主动性高，最容易接牌主持；

- **D**：兴致高时可能主动玩；

- **A**：通常不抢主持，但被起哄后可能接；

- **C**：更容易先旁观；

- **Bartender**：无人主动时兜底。

第一局 Player 不作为 Tarot Reader，后续可开放。

Tarot Reader 的职责只有：

> 抽牌、念问题、维持轮次。
> 
> 

她不是全知主持人，也没有权限逼任何人回答。

---

# 3\. 开场画面

Bartender 把音乐向下压一点。

前一轮游戏牌被推开。

塔罗牌落在桌面中央。

镜头第一视角环视：

- A 的镜片映着桌灯；

- B 还保持着半社交状态；

- C 低头转着杯子；

- D 手指敲着桌沿；

- Bartender 在远处继续收杯。

有人伸手把牌拉过来。

洗牌。

抽出第一张。

没有魔法粒子，没有“命运降临”特效。

只有：

> 纸牌划过桌面的声音。
> 
> 

---

# 4\. 核心玩法

每轮运行：

```text
Question Selection
↓
Tarot Reader 翻牌
↓
决定首答者
↓
首答 / 拒答 / 回避 / 玩笑
↓
其他 Agent 自主反应
↓
Player 参与
↓
Visibility & Perception
↓
Belief / Relationship Update
↓
A2A Ripple
↓
下一轮
```

每局只玩 **3–5 张**。

不是把题库刷完。

---

# 5\. 情绪节奏

问题不能纯随机。

建议一局遵循：

```text
Round 1
轻松 / 吐槽
↓
Round 2
暧昧 / 第一印象
↓
Round 3
关系 / 误解
↓
Round 4
高张力
↓
Comedy Break
↓
Round 5（可选）
未来 / 温柔收束
```

核心情绪不是一直越来越沉重，而是：

> **笑 → 暧昧 → 不对劲 → 真话 → 尴尬 → 突然破功 → 柔软。**
> 
> 

---

# 6\. Social Tarot 基础题库

题库建议至少 12 张，后台做标签而不是简单 `random()`。

## Q01｜不该遇见的人

**今晚，你最不想再遇见的人是谁？**

标签：

```text
past
relationship
high_tension
```

适合撬开旧关系，但不要求点名。

---

## Q02｜最想留下的人

**如果今晚只能留下一个人，你希望是谁？**

重点在于：

- 可以点名；

- 可以说没人；

- 可以说不在场的人；

- 可以故意回避。

本身就是误读发动机。

---

## Q03｜如果早点认识

**在场有没有一个人，你偶尔会想：如果我们早几年认识，会不会不一样？**

适合：

- A/B；

- B/D；

- Player × 任意 Agent。

同时带出人物处于不同人生阶段。

---

## Q04｜记忆里的她

**有没有一个人，现在已经和你记忆里的她完全不一样了？**

核心：

> 现实中的她
> VS
> 自己记忆里的她。
> 
> 

为后面的倒叙与旧人投射做准备。

---

## Q05｜没说出口的话

**最近一次，你明明想说，最后却没说出口的话是什么？**

不要求说对象。

一句：

> “留下来。”
> 
> 

就可能同时击中几个人。

---

## Q06｜误会

**在场有没有一个人，你后来才发现自己可能一直误会了她？**

这是最典型的 A2A 问题。

回答：

> “有。”
> 
> 

已经足够。

---

## Q07｜真心还是利益

**你有没有过一次，自己都分不清：靠近一个人，到底因为她有价值，还是因为真的喜欢她？**

La La Land 商业场景的标志性问题。

尤其容易触发：

- A；

- B；

- D。

---

## Q08｜被看穿

**如果今晚有人真的看穿你，你最不希望她看见哪一部分？**

可指向：

- 野心；

- 嫉妒；

- 依赖；

- 不确定；

- 需要；

- 害怕失去。

不做心理诊断。

---

## Q09｜再来一次

**如果能回到你和某个人关系发生变化的那一天，你还会做同样的选择吗？**

为 Scene 4 的 Memory Flashback 埋触发点。

---

## Q10｜今晚以后

**这一桌有没有一个人，你希望今晚之后，你们的关系和现在不一样？**

适合作为最后一张之一。

把视角从：

> “以前发生了什么”
> 
> 

推向：

> “以后怎么办。”
> 
> 

---

## Q11｜第一印象

**在场谁最不像你第一次认识她时以为的样子？**

偏轻。

特别适合 Player 进入。

---

## Q12｜没有身份以后

**如果今晚不谈职位、钱、项目和过去，你最想以什么身份重新认识这里的某个人？**

可能产生：

> 普通朋友。
> 重新认识。
> 什么都不是。
> 一个没有利益关系的人。
> 
> 

适合后半段。

---

# 7\. Comedy / Roast Card

另外单独建立一个小型 **Joker Pool**。

每局最多出现 1 张，用来在高张力之后“破功”。

例如：

### J01｜约会尽调

**这一桌谁最有可能把约会做成尽调？**

全桌可能同时看 B。

关键不是固定笑话，而是让模型利用当前人物关系实时 Roast。

---

### J02｜创业灾难

**如果这一桌一起创业，谁最可能第一个把公司搞黄？**

适合 D/C/B 之间互相甩锅。

---

### J03｜无意识撩人

**谁最容易让别人以为她在撩人，但本人坚称自己什么都没做？**

非常适合制造：

> 全桌看同一个人 → 当事人抗议 → 另一个 Agent 补刀。
> 
> 

---

### J04｜前任短信

**谁最可能喝多以后给前任发一句“睡了吗”？**

最好放在高张力问题之后。

前一秒大家还沉默。

下一秒抽到这张。

让整个场子突然笑出来。

---

### J05｜职业病

**说一个在场某人的职业病。**

例如 Agent 可以互相 Roast：

> 把所有关系做项目管理。
> 把所有情绪做风险隔离。
> 把照顾别人当成默认权限。
> 
> 

这种笑话同时还能偷偷暴露人物关系。

---

# 8\. Player 参与方式

Player 每轮都可以自由选择：

### Answer

自由文字 / 语音。

### Skip

明确拒答。

### Deflect

让别人先回答。

### Ask Back

把问题直接丢给某个 Agent。

### Observe

不回答，只看其他人的反应。

### Joke

用玩笑化解。

这些全部属于有效 Social Move。

---

# 9\. 沉默也是 Event

例如：

> “今晚你最不想遇见谁？”
> 
> 

Player 不回答。

不能当作没有输入。

World Event：

```text
Player chose not to answer.
```

然后不同 Agent 可以形成不同 Interpretation。

### A

可能认为 Player 在保护自己的边界。

### B

可能认为 Player 明明有答案，只是不愿公开。

### C

可能怀疑答案与现场某人有关。

### D

可能认为这个问题本身就不值得逼问。

所以：

> **Silence ≠ Null**
> 
> 

---

# 10\. A2A 核心示例

抽到：

> **“在场有没有一个人，你后来才发现自己可能一直误会了她？”**
> 
> 

C 停顿。

回答：

> “有。”
> 
> 

没有点名。

World Kernel 只记录：

```text
C answered yes.
Target not disclosed.
```

但各 Agent 独立理解。

### A

> 她会不会说的是我？
> 
> 

因为旧项目。

### B

> 她是不是说我？
> 
> 

因为一个月前的关系。

### D

> 她应该是在说我。
> 
> 

因为两人的旧关系。

### Player

什么都不能确定。

这就是 Scene 3 最理想的状态：

> **一句话，同时在三段关系里产生不同意义。**
> 
> 

---

# 11\. 高价值视线事件

例如抽到 Q07：

> 靠近一个人，到底因为她有价值，还是因为喜欢？
> 
> 

B 被抽中。

B 根据自身状态可能：

- 玩笑化解；

- 承认有过；

- 不点名；

- 看某个人；

- 先看 A，再看 D；

- 根本不看任何人。

系统必须记录的不只是回答，还包括：

```text
gaze_order
pause_duration
gesture
targeted_response
```

例如：

```text
B looked at A first.
Then briefly looked at D.
```

A、D、C、Player 可能对此产生完全不同解释。

所以这一场：

> **视线本身就是对话。**
> 
> 

---

# 12\. 3D 画面演出

不要一直固定镜头轮流说话。

关键画面包括：

- 洗牌的手；

- 牌角被压住；

- 杯壁水珠；

- A 镜片中的反光；

- B 笑到一半停下来；

- C 手指碰登山扣；

- D 敲桌动作突然停止；

- 某个人回答之前先看另一个人；

- 两个人同时看向第三个人；

- Player 转头时撞上某人的目光；

- 高张力后有人低头喝酒；

- Joker 出现时有人彻底笑破功。

最重要的设计规则：

> **回答之前的 0\.5 秒，往往比回答本身信息量更高。**
> 
> 

---

# 13\. Tarot Card 视觉设计

不使用传统完整 Rider\-Waite 体系。

做成 La La Land 自己的 **Social Tarot**。

结构：

```text
Card Symbol
+
Relationship Theme
+
Dynamic Question
```

例如：

### THE MIRROR｜镜

主题：

> 投射 / 被看见
> 
> 

可生成问题：

> “你现在看见的是她，还是某个过去的人？”
> 
> 

---

### THE EMPTY CHAIR｜空椅

主题：

> 缺席 / 等待
> 
> 

可生成：

> “今晚你最希望出现、却又最怕出现的人是谁？”
> 
> 

还能回调 Scene 1 的第三杯和空椅。

---

### THE CONTRACT｜契约

主题：

> 利益 / 承诺
> 
> 

可生成：

> “有没有一段关系，什么都谈清楚了，唯独没有说清彼此是什么？”
> 
> 

---

### THE EXIT｜出口

主题：

> 离开 / 留下
> 
> 

可生成：

> “如果现在可以不解释地离开，你会走吗？”
> 
> 

这些牌本身可以逐渐成为 La La Land 的 IP 资产。

---

# 14\. 声音设计

Scene 3 声音明显比 Scene 2 安静。

环境：

- 音乐降低；

- 顾客减少；

- 杯子声音变明显；

- 雨 / 城市远声；

- 空调底噪。

### 翻牌

只需要：

> 纸牌摩擦桌面的声音
> 
> - 极轻低频 Cue。
> 
> 

不要魔法音效。

### 高张力回答

背景音乐下降约 1–2 秒。

留出真正的：

> 沉默。
> 
> 

### Comedy Break

音乐恢复一点，人物笑声重新进入，缓解高压。

---

# 15\. UI

保持极简。

翻牌：

**THE MIRROR**

> “有没有一个人，你后来才发现自己可能一直误会了她？”
> 
> 

基础操作：

```text
回答
跳过
```

其余：

- Deflect

- Ask Back

- Joke

- Observe

通过自然语言 / Voice 完成。

不显示：

> 好感 \+5
> 嫉妒 \+10
> 
> 

关系变化全部通过人物行为反馈。

---

# 16\. 动态分支

### Branch A｜拒答

另一个 Agent 可能帮忙挡题，也可能尊重沉默。

### Branch B｜直接点名

关系张力明显提高。

### Branch C｜隐瞒 / 说谎

如果其他 Agent 有相反 Memory，可以产生：

> 怀疑，而不是自动揭穿。
> 
> 

### Branch D｜Player 点某人回答

被点名者会记住：

> Player 主动让自己暴露在公共场。
> 
> 

### Branch E｜Boundary 被触发

Agent 可以：

- 停止回答；

- 离桌；

- 去吧台；

- 去走廊；

- 反问；

- 结束这一轮。

### Branch F｜Comedy Recovery

高张力以后 Joker 成功缓和气氛，原本准备离开的 Agent 可能暂时留下。

这也属于真实关系变化。

---

# 17\. Scene 3 → Scene 4

塔罗局不需要固定玩满五张。

满足任一条件：

```text
3–5 cards completed
OR
high_tension_event triggered
OR
agent boundary triggered
OR
agent voluntarily leaves
OR
high-value misunderstanding surfaced
```

进入收尾。

某个 Agent 拿起杯子。

停顿。

站起来。

具体台词由模型生成，只固定：

> 她需要暂时离开公共桌面。
> 
> 

例如：

> 出去透气。
> 
> 

另一个与她当前关系张力最高 / 关注最高 / 最缺直接沟通的 Agent，可能自主跟出去。

Player 可以：

- 跟；

- 不跟；

- 留在桌上；

- 过一会再出去；

- 找其他人。

从而进入：

# Scene 4｜走廊透气：巧克力烟

---

# 18\. Scene 3 最终开发结构

```text
Scene 3｜Social Tarot
│
├── Environment
│   └── Late-night Main Table
│
├── Tarot Reader
│   └── Dynamic Agent Selection
│
├── Tarot Deck
│   ├── Light
│   ├── Relationship
│   ├── Past
│   ├── Identity
│   ├── Business Emotion
│   ├── High Tension
│   ├── Future
│   └── Joker / Roast
│
├── Round Director
│   └── Light → Deep → High → Comedy → Soft
│
├── Player
│   ├── Answer
│   ├── Skip
│   ├── Deflect
│   ├── Ask Back
│   ├── Observe
│   └── Joke
│
├── Agent Runtime
│   ├── Answer Decision
│   ├── Gaze
│   ├── Gesture
│   ├── Interpretation
│   ├── Belief Update
│   ├── Follow-up
│   └── Leave
│
├── A2A
│   ├── Multiple Interpretations
│   ├── Misunderstanding
│   ├── Cross-target Reaction
│   └── Information Propagation
│
├── Audio
│   ├── Quiet Bar
│   ├── Card Foley
│   ├── Silence Beat
│   └── Comedy Recovery
│
├── State
│   ├── question_history
│   ├── answers
│   ├── gaze_events
│   ├── exposed_information
│   ├── belief_updates
│   ├── tension
│   └── player_social_moves
│
└── Exit
    └── Dynamic Agent Leaves
        ↓
       Scene 4
```

这一场最终可以用一句话定义：

> **塔罗并不是用来预测未来，而是给所有人一个足够安全的借口，说出那些平时不会主动说的话。**
> 
> 

而真正的 A2A 发生在问题之后：

> **有人回答，有人误会；有人笑，有人突然不笑；有人嘴上说的是一个人，眼睛看的却是另一个人。**
> 
> 

这才是 La La Land 塔罗局最核心的戏。

