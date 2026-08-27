# LALAGAME
LALA game

## 琥珀酒馆 / The Amber Room

固定斜俯视、低多边形 2.5D 酒吧漫游原型。WASD / 方向键移动，Shift 快走，Esc 暂停 / 继续。

![酒吧实机截图](BarPrototype/Verification/bar-1920x1080.png)

- [工程与中文说明](BarPrototype/README.md)
- [测试报告与验收边界](BarPrototype/Verification/STATUS.md)
- 编辑器版本：团结引擎 **2022.3.62t14**；URP **14.2.0-t1**。
- 在团结 Hub 中添加仓库内的 `BarPrototype` 文件夹，打开 `Assets/Scenes/AmberRoom.unity`。
- 编辑器菜单 `Amber Room > 2 - Build Windows game` 可生成 Windows 程序。

仓库包含源代码、场景、角色 Prefab、材质、截图及测试结果，不含 Library 缓存、本地日志和 Windows 二进制构建产物。

在仓库根目录也可通过 PowerShell 运行重建脚本；路径与本机不同时指定编辑器位置：

```powershell
.\Tools\Build-Bar.ps1 -Action Test -EditorPath 'D:\unity cn\Editor\Tuanjie.exe'
.\Tools\Build-Bar.ps1 -Action Build -EditorPath 'D:\unity cn\Editor\Tuanjie.exe'
.\Tools\Build-Bar.ps1 -Action Smoke
```

已有场景可直接编辑，无需执行 Prepare。Prepare 会覆盖自动生成资产，手工修改后请先备份。
