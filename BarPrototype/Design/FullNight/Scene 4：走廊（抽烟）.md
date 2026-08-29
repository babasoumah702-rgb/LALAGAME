# Scene 4：走廊（抽烟）

# Scene 4：走廊（抽烟）

# **La La Land｜Scene 4 开发稿**

## **Scene 4｜走廊透气：巧克力烟**

**时间**：塔罗局后，约 00:35
**建议时长**：3–6 分钟
**视角**：Player 第一人称
**空间**：La La Land 酒吧外侧走廊 / 电梯厅 / 通往露台的过渡空间
**人物**：Player \+ A/B/C/D 中动态选出的 1–3 人
**固定锚点**：有人离开牌桌透气 → 禁烟空间里用巧克力棒代替烟 → 一段半私密谈话 → 过去记忆碎片介入 → 第三人可能出现 → 关系状态改变。

这一场真正承担的是：

**把塔罗桌上的公共张力，拆成一段半私密关系。**

不是揭露大秘密，而是第一次让玩家明显感觉到：

她们现在说的每一句话，后面都压着以前发生过的事。

---

# **1\. Scene 4 如何触发**

Scene 3 塔罗局结束时，系统根据本轮关系状态自动选择最适合离桌的 Agent。

优先读取：

```text
tension
mutual_attention
unfinished_topic
interaction_frequency
avoidance
relationship_history
boundary_trigger
```

不是单纯随机两个人。

选择逻辑主要有三类：

### **① 聊得最多 \+ 张力最高**

例如 A × B、C × D。

说明这组关系已经被牌桌明显撬开。

### **② 聊得最少 \+ 相互关注最高**

两个人可能整晚几乎没有直接说话，却一直：

- 偷看；

- 对对方回答有反应；

- 刻意回避。

这种组合反而非常适合走廊。

### **③ 某人主动离席**

如果某 Agent 被上一张牌明显触发：

她主动说“我出去一下”。

另一个与她关系最相关的 Agent 可以自主选择是否跟出去。

最终形成：

```text
corridor_group =
solo
duo
trio
```

---

# **2\. 场景开场**

塔罗局刚结束一个高张力问题。

牌桌上短暂安静。

有人笑着把牌推回去，但某个 Agent 没有跟着笑。

她拿起自己的杯子，又放下。

随后起身。

具体离席文案由角色模型实时生成，只固定语义：

**“出去透一下气。”**

Player 看见她推开酒吧侧门。

门一关。

里面的音乐和人声立刻被压成模糊的低频。

---

# **3\. 空间画面**

走廊和酒吧内部形成非常强的反差。

### **酒吧内**

暖、热闹、多人、酒红、深绿、音乐、人声。

### **走廊**

冷静、空、半私密。

画面元素：

- 深灰墙面；

- 暖黄壁灯；

- 电梯金属门；

- 玻璃反射；

- 远端城市灯光；

- 指示牌；

- 酒吧门缝透出的暖光。

第一人称出来以后，可以先不靠太近。

前面的 Agent 靠在墙边。

她低头摸口袋。

像是准备拿烟。

然后——

掏出一根巧克力棒。

---

# **4\. 固定 Comedy Beat｜巧克力烟**

这是 Scene 4 的固定视觉梗。

因为楼内全面禁烟。

角色拆开巧克力包装，把巧克力像烟一样夹在指间，或者直接叼在嘴边。

镜头甚至可以先故意拍得很酷：

- 手部特写；

- 侧脸；

- 走廊冷光；

- 靠墙；

- 指尖夹着“烟”。

再切近一点才发现：

是巧克力。

形成一个很轻的反差笑点。

具体对白不写死，只定义语义：

“这里不能抽。”
“所以只能这样。”
“至少合法。”

如果第二个 Agent 已经在场，可以顺势吐槽。

这样刚才塔罗局过高的张力会先被放下来一点。

---

# **5\. Player 是否跟出去**

Player 不强制进入。

Scene 3 结束后可以：

```text
跟出去
留在牌桌
晚一点出去
去找另一个人
```

如果不出去：

Scene 4 可以作为后台 A2A Scene 运行，Player 之后只看到结果或部分回声。

如果跟出去：

Player 成为这个半私密事件的直接观察者。

---

# **6\. Scene 4 主体结构**

整场固定按照这条情绪曲线运行：

```text
塔罗高潮
↓
离席
↓
巧克力烟破功
↓
轻松几句
↓
沉默
↓
一句普通的话碰到旧事
↓
Flashback 01
↓
回到现在
↓
真正的私聊
↓
关系靠近 / 拉远
↓
第三人可能出现
↓
Partial Perception
↓
Flashback 02（条件触发）
↓
新的误解 / 修正
↓
返回酒吧或进入下一个空间
```

---

# **7\. 第一阶段｜轻松破冰**

巧克力烟之后不要马上谈过去。

先允许 15–30 秒自然互动。

例如可以聊：

- 刚才的塔罗太缺德；

- 谁的问题最像故意设计的；

- 谁喝多了；

- 为什么这里连烟都不让抽；

- 某个人刚才的答案明显在骗人；

- 某个人笑得太假。

这一段由 Agent 根据当前 Scene 3 的 Memory 动态生成。

目的只有一个：

**让人物从“戏剧角色”重新变成活人。**

---

# **8\. 第二阶段｜旧事触发**

真正的关系内容不要凭空开始。

一定通过一个**当前动作 / 物件 / 句子**触发。

例如：

- 巧克力；

- 外套；

- 电梯；

- 一句“你还是这样”；

- 某个称呼；

- 某种习惯；

- 工作问题；

- 对方的一个动作。

然后进入 Memory Cut。

---

# **9\. Flashback 系统**

倒叙不是完整剧情。

每次只出现：

**2–5 秒过去碎片。**

结构固定：

```text
Current Trigger
↓
Private Memory Match
↓
2–5s Memory Fragment
↓
Hard Cut Back
↓
Current Agent Reaction
```

不显示：

“三年前，北京。”

让观众自己拼。

过去不是独立章节，而是不断寄生在现在的动作里。

---

# **10\. 不同组合的 Flashback 内容**

## **A × B｜“你听见的和我说的不是一回事”**

### **当前**

两人在走廊谈刚才塔罗的问题。

B 可能再次把某句话解释成：

判断、投资、策略。

A 明显停了一下。

### **Flashback**

过去某次会议结束。

会议室只剩两个人。

B：

“所以，你投吗？”

A：

“你问哪一个？”

B 没回答。

CUT。

### **回到现在**

A 看着 B。

核心语义：

**“我以前就已经说过，只是你一直听成另外一种意思。”**

这一组的核心：

感情已经发生，但双方一直拿职业语言解释它。

---

## **B × C｜“我说没事，你真的就走了”**

### **当前**

C 对 B：

“你还是这样。”

### **Flashback**

过去酒店 / 出差夜晚。

B 穿好外套。

C 坐在一边。

B 问：

“怎么了？”

C：

“没事。”

B真的离开。

CUT。

### **回到现在**

C：

“你看。就是这个。”

核心不是：

B 无情。

而是：

C 认为“没事”应该被听懂；B 认为尊重对方说出的边界才是正确的。

同一件事，两套完全合理的理解。

---

## **C × D｜“你还记得”**

### **当前**

C 把巧克力递给 D。

D：

“你还记得我吃这个？”

### **Flashback**

过去深夜。

D 趴在电脑前工作。

C 把巧克力和外套一起放到旁边。

D：

“你怎么什么都记得？”

C：

“也没什么难记的。”

CUT。

### **回到现在**

同样的巧克力。

但关系已经结束。

D 核心表达：

**“你以前总是用照顾代替说你需要我。”**

这一组不是激烈争吵，而是温柔里带刺。

---

## **B × D｜“你最早看见的是我，还是潜力？”**

### **当前**

B下意识开始问 D 的项目。

D：

“你又开始了。”

### **Flashback**

第一次项目会。

所有人都在说市场。

年轻一点的 D 打断：

“这个系统撑不到上线。”

会议室安静。

B 抬头：

“为什么？”

D 开始讲。

B一直看着她。

CUT。

### **回到现在**

D：

“你第一次看我，就是这个表情。”

B：

“什么表情？”

核心：

**像看到一个值得下注的东西。**

D 不确定后来有没有变成：

看见她本人。

---

## **A × C｜“我们都以为自己是在帮对方”**

### **当前**

C问：

“当时你到底知不知道？”

### **Flashback**

签字桌。

A把文件推过去。

A：

“你可以晚一点签。”

C：

“不用。”

签字。

CUT。

### **回到现在**

C：

“我那时候以为你是在帮我。”

A：

“我也是。”

这条线强化：

没有单一反派。

---

# **11\. Player 交互**

Scene 4 的 Player 交互比塔罗局更轻。

可进行：

```text
靠近
保持距离
听
插话
转移话题
询问刚才的塔罗问题
向其中一人提问
要一根巧克力
回酒吧
自由文字 / 语音
```

一个很推荐保留的互动：

**“也给我一根。”**

它既轻松，又能成为关系表现。

Agent 可以根据关系状态：

- 直接递给 Player；

- 把整盒扔过来；

- 让 Player 自己拿；

- 笑一下再递；

- 拒绝；

- 第三个人抢先递。

不需要 UI 告诉用户：

好感 \+2。

动作本身就是反馈。

---

# **12\. 第三人加入**

Scene 4 中后段允许第三个 Agent 动态进入。

不是固定剧情。

触发依据：

```text
relationship_relevance
unfinished_topic
curiosity
tension
location_path
agent_goal
```

例如：

A 和 B 正在谈。

D 本来只是去洗手间。

门打开。

她看到两人。

可以：

- 直接加入；

- 停一下再走；

- 打招呼；

- 假装没看见；

- 改变目的地。

全部由 Agent Runtime 判断。

---

# **13\. 第三人出现最大的作用：信息残缺**

例如 A 对 B 说：

“我不想你再把我的感情解释成策略。”

D 推门出来时，只听到：

“…解释成策略。”

那么系统记录：

### **World Truth**

A/B 正在讨论私人关系。

### **D Perceived Event**

```text
A and B are having a serious private discussion.
Heard keyword: strategy.
Full context unavailable.
```

D 可能理解：

她们还在谈投资。

也可能理解：

她们显然不只是在谈投资。

这就是 Scene 4 最重要的 A2A 价值：

**第三个人永远只拿到一部分。**

---

# **14\. A2A Runtime**

这一场后台运行：

```text
World Event
↓
Visibility / Audibility
↓
Perceived Event
↓
Private Memory Retrieval
↓
Interpretation
↓
Belief Update
↓
Boundary Check
↓
Action Decision
↓
Expression
↓
New Event
```

尤其需要支持：

### **Partial Hearing**

只听到半句话。

### **Delayed Reaction**

当场不说，回去以后再找别人。

### **Memory\-triggered Action**

某个动作让 Agent 想起过去，改变当前行为。

---

# **15\. 记忆不是客观纪录**

同一段过去允许不同 Agent 有不同记忆版本。

例如 B/C 某次分别。

### **C 的记忆**

B 穿上外套，没有回头。

### **B 的记忆**

自己在门口停了几秒，C 一直没有开口。

两段 Memory 都可以是真的。

所以系统区分：

```text
World Past Fact
≠
Agent Memory Meaning
```

这会成为 La La Land 后面非常重要的叙事机制。

---

# **16\. 画面设计**

这一场重点不是脸怼脸聊天。

要用空间距离表达关系。

推荐固定视觉：

### **S4\-01**

门关闭。

酒吧声音突然变闷。

### **S4\-02**

角色靠墙，掏出口袋里的巧克力。

### **S4\-03**

撕包装特写。

### **S4\-04**

巧克力像烟一样夹在手里。

### **S4\-05**

两个人并肩，但都看前方。

### **S4\-06**

一句话触发 Flashback。

### **S4\-07**

Hard Cut 回到现在。

同一个动作重复。

### **S4\-08**

其中一个人终于转头看另外一个。

### **S4\-09**

第三个人推门。

两人同时停下来。

### **S4\-10**

电梯 / 玻璃反射出现三个人的错位倒影。

这非常适合做宣传片素材。

---

# **17\. Comedy 二次破功**

如果谈话再次过于沉重，可以动态触发一个很短的 Comedy Beat。

例如角色正非常认真：

“我那时候是真的——”

“啪。”

巧克力断了。

所有人低头。

停 0\.5 秒。

另一个人笑。

或者 Bartender 推门出来：

“这里不许抽烟。”

角色举起巧克力。

Bartender 看了一眼。

门重新关上。

这种笑点不要多。

一场 1–2 个已经够了。

---

# **18\. 声音 / 音效**

## **酒吧门关闭**

这是第一声重要变化。

从：

人群 \+ 音乐 \+ 碰杯

瞬间变成：

空调 \+ 电梯 \+ 极远的低频。

---

## **走廊核心声音**

- 鞋底；

- 衣服摩擦；

- 呼吸；

- 电梯运行；

- 巧克力包装纸；

- 咬断巧克力；

- 偶尔酒吧门打开时声音突然涌入。

Flashback 时：

当前环境声迅速抽掉。

只留下过去那个场景最重要的一种声音。

例如：

- 键盘；

- 签字笔；

- 车门；

- 包装纸；

- 雨；

- 一声笑。

Hard Cut 回来时，走廊空调声重新出现。

---

# **19\. UI**

Scene 4 继续极简。

只在必要时给非常弱的 Interaction：

靠近
开口
保持距离
回去

文字 / Voice 始终开放。

Flashback：

不显示：

“解锁 C 与 D 回忆 01。”

正式版只让它自然出现。

Debug 才显示：

```text
memory_trigger
memory_owner
source_event
relationship_effect
```

---

# **20\. 状态记录**

Scene 4 结束后至少记录：

```text
corridor_agents
player_followed
player_intervened
private_topic
memory_fragment_triggered
partial_information_heard
third_agent_joined
chocolate_shared
agent_left_first
relationship_shift
new_belief
unresolved_topic
```

其中：

`chocolate_shared`

可以成为一个非常轻的小记忆。

例如后面屋顶上，某人又自然把最后一根递给 Player。

不需要解释。

玩家自己会意识到回调。

---

# **21\. Scene 4 的结束方式**

Scene 4 不以“大和解”结束。

最合适的是：

**说到某个程度以后，停止。**

例如：

某个 Agent 把最后一点巧克力吃掉。

包装纸折起来。

扔进垃圾桶。

停顿。

模型生成一个符合当前关系的结束语义：

回去吧。

酒吧门被推开。

暖光重新照进来。

音乐、人声重新涌入。

但 Player 会发现：

**刚才出去之前的座位关系已经变了。**

有人换了位置。

有人已经不在主桌。

有人正在和另一个人说话。

世界没有停下来等这段私聊结束。

---

# **22\. Scene 4 → Scene 5**

Scene 4 结束后，不固定只进入一个剧情。

根据刚才的结果动态进入：

```text
回到主厅继续社交
OR
第三人触发新的关系事件
OR
某 Agent 转去露台
OR
某 Agent 主动找 Player
OR
新的 A2A Ripple
```

所以 Scene 5 可以正式开始进入：

**“刚才的私聊产生后果。”**

例如：

C/D 在走廊谈完。

D 回去以后没有坐回 C 附近。

反而坐到了 B 身边。

C 看见。

什么都没说。

这就直接成为下一场的种子。

---

# **23\. Scene 4 开发结构**

```text
Scene 4｜Corridor
│
├── Trigger
│   └── Scene 3 high-tension outcome
│
├── Dynamic Casting
│   ├── Most tension
│   ├── Most mutual attention
│   ├── Least direct interaction
│   └── Voluntary exit
│
├── Environment
│   ├── Corridor
│   ├── Elevator Lobby
│   └── Bar Door
│
├── Comedy Anchor
│   └── Chocolate Cigarette
│
├── Player
│   ├── Follow
│   ├── Observe
│   ├── Talk
│   ├── Ask
│   ├── Share Chocolate
│   └── Leave
│
├── Agent Runtime
│   ├── Private Dialogue
│   ├── Boundary
│   ├── Memory Retrieval
│   ├── Belief Update
│   └── Autonomous Action
│
├── Flashback
│   ├── Current Trigger
│   ├── Private Memory
│   ├── 2–5s Fragment
│   └── Hard Cut
│
├── Third-Agent Entry
│   ├── Partial Hearing
│   ├── Misinterpretation
│   └── New Ripple
│
├── Audio
│   ├── Muted Bar
│   ├── Corridor Ambience
│   ├── Chocolate Wrapper
│   └── Flashback Sound Motif
│
├── State Output
│   ├── relationship_shift
│   ├── belief_update
│   ├── memory_trigger
│   └── unresolved_topic
│
└── Exit
    └── Return to World / New Ripple
```

这场最后的核心感觉可以定成一句话：

**刚才在桌上什么都敢说的人，真正只剩两个人的时候，反而不知道该怎么开口；于是只能先叼一根巧克力，假装自己只是出来抽根烟。**

而倒叙让玩家逐渐发现：

**她们不是从今晚才开始喜欢、误会、失望或者错过。今晚只是恰好让过去重新经过了一次。**

