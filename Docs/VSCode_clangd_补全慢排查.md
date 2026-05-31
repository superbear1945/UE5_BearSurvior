# VSCode + clangd 补全慢排查记录

## 背景

当前项目使用 VSCode + clangd 开发 UE5 C++。现象是代码跳转正常，但代码补全明显慢；同一项目在 Neovim + clangd_extension 下补全很快。

这个对比说明 `compile_commands.json` 和 clangd 基础解析能力大概率没有根本性问题，问题更可能出在 VSCode 工作区配置、扩展竞争、文件监听、补全 UI 或 clangd 启动参数差异上。

## 已观察到的项目状态

- 项目存在 `.clangd`，其中指定 clangd 到 `.vscode` 下寻找编译数据库。
- 项目存在 `.vscode/compile_commands.json`，代码跳转正常也印证 clangd 能找到编译数据库。
- 项目没有 `.vscode/settings.json`，clangd 参数主要写在 `BearSurvior.code-workspace`。
- `BearSurvior.code-workspace` 曾配置同时打开项目目录和 `C:/D/AboutCreating/Epic/UE_5.7` 引擎目录；当前需要确认 UE5 folder 是否仍处于启用状态。
- `.vscode/compile_commands.json` 包含项目源码、`Variant_*` 示例源码和 `UE_MCP_Bridge` 插件源码。

## 已修复的问题

`BearSurvior.code-workspace` 的 `tasks.tasks` 数组中，`Subtask:MoveCompileCommands` 与 `Gen Compile Commands` 两个任务之间缺少逗号，导致 workspace JSONC 语法不完整。

修复后，VSCode 应能稳定解析 workspace 中的 tasks 和 settings 配置。

## 可能原因排序

### 1. VSCode workspace 同时打开 UE5 Engine 目录

`BearSurvior.code-workspace` 中曾包含或可能包含：

```jsonc
{
    "name": "UE5",
    "path": "C:/D/AboutCreating/Epic/UE_5.7"
}
```

这会让 VSCode 把整个 UE5 引擎源码作为 workspace folder 管理。即使 clangd 只依赖编译数据库，VSCode 自身的文件 watcher、搜索索引、Git 检测和其他扩展仍会扫描大量文件。

Neovim 通常只打开项目目录，不会像 VSCode workspace 那样全量管理 Engine 目录，因此这一点是最可疑差异。

验证方式：

1. 临时从 `BearSurvior.code-workspace` 移除 UE5 folder。
2. 或直接用 VSCode 打开项目文件夹，不打开 `.code-workspace`。
3. 执行 `Clangd: Restart language server`。
4. 对比补全响应时间。

### 2. clangd 参数只写在 code-workspace，打开文件夹时不会生效

当前没有 `.vscode/settings.json`。如果使用 VSCode 的“打开文件夹”方式，而不是打开 `BearSurvior.code-workspace`，这些配置不会生效：

```jsonc
"clangd.arguments": [
    "--compile-commands-dir=.vscode",
    "-pretty",
    "-j=6",
    "--header-insertion=never",
    "--completion-style=detailed",
    "--pch-storage=disk"
]
```

`.clangd` 已经能保证 clangd 找到 `.vscode/compile_commands.json`，所以跳转仍可能正常；但 VSCode clangd 扩展的其他启动参数可能没有应用。

验证方式：

1. 打开 VSCode `Output` 面板。
2. 选择 `clangd` 输出通道。
3. 查看 clangd 启动命令是否包含期望参数。
4. 如果缺少参数，说明当前打开方式没有加载 workspace settings。

### 3. Microsoft C/C++ 扩展可能与 clangd 竞争资源

workspace 中配置了：

```jsonc
"C_Cpp.intelliSenseEngine": "Disabled",
"C_Cpp.autocomplete": "Disabled",
"C_Cpp.errorSquiggles": "Disabled"
```

但如果 workspace settings 未生效，`ms-vscode.cpptools` 仍可能运行 IntelliSense、browse database 或 include 扫描，与 clangd 抢 CPU 和 IO。

验证方式：

1. 临时禁用 `ms-vscode.cpptools` 扩展。
2. 重启 VSCode 和 clangd。
3. 观察补全响应是否改善。
4. 查看 `Output -> C/C++` 是否仍有 IntelliSense 或 browse 日志。

### 4. VSCode 文件监听和搜索未排除 UE 生成目录

UE 项目会频繁产生大量中间文件。若 VSCode 没有排除这些目录，会增加文件监听和搜索索引负担。

建议排除：

```jsonc
"files.watcherExclude": {
    "**/Binaries/**": true,
    "**/DerivedDataCache/**": true,
    "**/Intermediate/**": true,
    "**/Saved/**": true,
    "**/.vscode/.cache/**": true
},
"search.exclude": {
    "**/Binaries/**": true,
    "**/DerivedDataCache/**": true,
    "**/Intermediate/**": true,
    "**/Saved/**": true,
    "**/.vscode/.cache/**": true
}
```

如果继续把 Engine 放进 workspace，还应排除 Engine 的输出目录。

### 5. `--completion-style=detailed` 增加 VSCode 补全 UI 负担

UE 类型、宏、模板和 UObject API 数量很大。`--completion-style=detailed` 会让补全项携带更多信息，VSCode 的补全面板渲染和排序成本可能高于 Neovim 补全 UI。

验证方式：

1. 临时改为 `--completion-style=bundled`。
2. 或直接移除该参数使用默认值。
3. 重启 clangd 后测试常见补全场景。

### 6. clangd 后台索引并发过高或索引范围偏大

`-j` 控制 clangd 后台索引并发。并发过高会让首次打开项目时 CPU 被索引任务占满，前台补全请求排队变慢。

当前参数已是 `-j=6`，比此前 `-j=12` 更稳妥。如果机器 CPU 或内存压力仍较高，可以继续尝试 `-j=4`。

验证方式：

1. 打开项目后观察 `clangd.exe` CPU 使用率。
2. 等待后台索引完成后再次测试补全。
3. 如果首次慢、稍后变快，说明主要是后台索引抢占资源。

### 7. `compile_commands.json` 包含额外源码，扩大后台索引量

当前编译数据库不仅包含主要项目源码，还包含：

- `Plugins/UE_MCP_Bridge/Source/...`
- `Source/BearSurvior/Variant_Combat/...`
- `Source/BearSurvior/Variant_Platforming/...`
- `Source/BearSurvior/Variant_SideScrolling/...`

这些条目会增加 clangd 后台索引量。它们不一定是错误，但会扩大首次索引成本。

验证方式：

1. 查看 `Output -> clangd` 是否持续索引这些目录。
2. 对比索引完成前后的补全速度。
3. 如确认影响明显，再考虑生成更小范围的 clang database。

### 8. PCH 存储策略与机器资源有关

`--pch-storage=memory` 通常会增加内存占用；UE 项目头文件体量大，内存紧张时会导致 Windows 换页，补全反而变慢。

如果当前 workspace 使用 `--pch-storage=memory`，建议在补全卡顿时对比 `--pch-storage=disk`。`disk` 更适合内存压力较大的环境；若机器内存非常充足，也可以对比 `memory` 和 `disk` 的实际体验。

## 推荐排查顺序

1. 修复 workspace JSONC 语法问题，确保 VSCode 能解析 settings 和 tasks。
2. 确认 VSCode 是否真的打开了 `BearSurvior.code-workspace`。
3. 在 `Output -> clangd` 中确认 clangd 启动参数是否生效。
4. 临时移除 workspace 中的 UE5 Engine folder，或只打开项目目录进行对比。
5. 临时禁用 Microsoft C/C++ 扩展，只保留 clangd。
6. 增加 `files.watcherExclude` 和 `search.exclude`。
7. 对比 `--completion-style=detailed` 与 `--completion-style=bundled`。
8. 根据 CPU 和内存占用调整 `-j` 与 `--pch-storage`。

## 当前判断

最高概率根因是 VSCode 侧额外负载，而不是 clangd 对 UE 项目无法胜任。最可疑组合是：

1. workspace 打开或曾打开整个 UE5 Engine 源码目录。
2. clangd 参数依赖 `BearSurvior.code-workspace`，打开方式不对时可能不生效。
3. VSCode 文件监听、搜索索引、C/C++ 扩展和补全 UI 共同增加延迟。

Neovim + clangd_extension 补全快，说明可以继续使用 clangd 路线；重点应放在减少 VSCode workspace 负载和确保 VSCode clangd 配置真正生效。
