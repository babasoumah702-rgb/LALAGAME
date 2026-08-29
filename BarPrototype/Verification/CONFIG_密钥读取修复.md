# 密钥读取修复

2026-08-28：用户正常启动的后台报告 `modelConfigured=false`，因此未发起模型调用就切入规则模式。开发环境却能读到同名配置。文件 ID 检查确认，开发环境中的 AppData 路径实际指向应用 LocalCache 内的同一文件，而不是可供普通启动流程共享的私有文件。

修复为优先读取 `%USERPROFILE%\.lalagame\private\model.env`。该目录不在 AppData 内，密钥只保存在用户私有目录，未放到工程、构建包或日志。保留旧路径兼容；显式 `LASTCALL_CONFIG_DIR` 继续严格隔离，测试不能意外加载真实密钥。

配置在打开入口、恢复世界和尝试联网时重新读取。旧存档若仅因缺少密钥而降级，找到新配置后恢复在线；玩家主动选择离线的存档仍保持离线。所有存档文件均未迁移或删除。

当前已经运行的旧后台仍持有启动时的空配置，必须保存并退出游戏后再启动。随后可以继续原来的夜晚，不需要重开世界。

回归记录：`config-path-backend.xml`。其中覆盖重定向路径／正常路径一致、显式目录隔离、旧配置兼容、空配置不误用旧钥匙，以及旧存档降级恢复和主动离线保留。

Windows 依据：[微软关于打包桌面应用 AppData 重定向的说明](https://learn.microsoft.com/en-us/windows/msix/desktop/desktop-to-uwp-behind-the-scenes)。
