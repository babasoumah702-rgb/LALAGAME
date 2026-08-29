# Scene 0 · 电梯上升：今晚见

## 玩这一版

在完整运行目录中双击 LastCall.exe。不需要安装 Node 或编辑器；不要只复制 exe。
选择「进入电梯」，不操作约七秒后到达酒吧。

- 按住鼠标右键：轻微观察四周。
- E 或点击手机：查看 / 收起手机。
- T 或「写一句话」：最多 200 字的当下想法，不会自动发给酒吧里的人。
- 输入时演出暂停，提交或留白后继续；Esc 暂停、保存和退出。
- 入场后恢复 WASD / 方向键、Shift，以及原有酒吧交互。

五种身份和参与意愿保留，新增独自来 / 朋友邀约 / 活动参与及可选背景。
新局 A/B/C 已在场，D 仍为条件入场。旧存档不补播开场，也不改写既有进度。
开场中的模型消息如未及时到达或未通过校验，会明确使用预设文案，不阻塞开门。
在线消息在不含身份和关系秘密的安全短句范围内生成；JSON 格式与业务字段分别校验。
语音仅留转写接口，没有录音按钮，不会打开麦克风。

## 玩牌（卡牌修复版）

进酒吧后点击「牌局」→「请老板娘开局并加入」，无需等待定时邀请。
选人物、选牌、选择推荐表达或输入文字，再点击「出牌」。距离太远会先走近；WASD 或再次点击可取消。
灰色文字的卡牌仍可点击查看解锁／冷却原因。五张基础社交牌始终可用，六张情境牌加入后开放，「最后一次表达」需要等 Last Call。
已加入后再次打开「牌局」可退出、重新加入；这不会结束夜晚。
详见运行包中的 CARDPLAY_修复说明.md；原生截图和测试报告在 Verification 目录。

## 打开工程

使用团结引擎 2022.3.62t14 打开本仓库的 BarPrototype。
编辑 Assets/Scenes/LastCall.unity。Scene 0 在同一场景的酒吧入口外，可直接选中修改。
电梯与手机 Prefab：Assets/LastCall/SceneZero/SceneZero.prefab。
角色 / 贴图引用：Assets/LastCall/SceneZero/ArtCatalog.asset。

首次准备：Last Call → Scene 0 → 1 - Add elevator (preserve bar)。
已有开场时只刷新资源和导航，不重建酒吧；手动移动家具后可用此菜单重新导出导航。
运行包：Last Call → Scene 0 → 2 - Build Windows verification。
不要使用旧的 Create MVP scene 来更新此场景，该旧菜单会从原漫游版本重建。

命令行：在仓库根目录运行 Tools/Build-Scene0.ps1 -Action Test 或 -Action Build。
首次构建需要 Node 24.12.x；运行包内已经包含运行时。

## 隐私与素材

仍使用已有本机私有模型配置，不在源码或包内保存密钥。
首选密钥路径为 %USERPROFILE%\.lalagame\private\model.env，避免 Windows 应用隔离造成的 AppData 重定向；未创建此文件时兼容旧的 AppData 路径。存档位置不变。
当前默认使用 DeepSeek 官方接口 https://api.deepseek.com，模型 deepseek-v4-flash，非思考模式。
在线表达发送到用户已配置的模型服务；无密钥也能使用离线规则模式。更换私有配置后请重启游戏。
玩家存档仍在本机；原始私人场景文档不随包发布。
音频来源与许可位于 Assets/LastCall/SceneZero/Audio/LICENSES.md。
