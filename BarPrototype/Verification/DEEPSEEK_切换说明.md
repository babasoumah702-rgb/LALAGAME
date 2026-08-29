# DeepSeek 切换记录 · 2026-08-28

- 接口：`https://api.deepseek.com`；模型：`deepseek-v4-flash`。
- 手机邀约与角色 Agent 均使用同一配置。设置 `thinking.type=disabled`，保持原有八秒超时、JSON/字段校验、预算、信息隔离及离线回退。
- 密钥首选本机 `%USERPROFILE%\.lalagame\private\model.env`，不写入工程或运行包。该路径避免 AppData 重定向。未创建首选文件时兼容旧配置路径；存档位置不变。
- 官方模型列表鉴权 HTTP 200；角色回复真实调用约 2430 ms，业务校验与世界应用通过；手机文案约 1487 ms，接受为在线生成。测试仅使用合成场景输入。
- 官方依据：[模型与接口](https://api-docs.deepseek.com/)、[Chat Completions 的非思考参数](https://api-docs.deepseek.com/api/create-chat-completion/)。
- 后端 66 / 66 通过：`deepseek-backend.xml`。Windows 在线开场 14 / 14 通过：`deepseek-online/report.json`，手机文案来自模型，约 7.247 秒进入酒吧，短时采样约 59.99 FPS。

重启更新后的完整运行目录即可生效。若继续旧存档且该存档此前处于规则模式，在暂停菜单点「尝试在线模型」。运行包不含个人密钥，因此拷贝到其他电脑需要另外配置；不会在其他电脑自动使用本机账户。

本次切换不影响已修复的牌局流程、旧存档和其他场景资源。旧 ZIP 保留，不覆盖。
