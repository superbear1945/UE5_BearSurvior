# VSCode + clangd 无法补全排查结论

## 背景

当前项目使用 VSCode + clangd 开发 UE5 C++。实际现象是：

- VSCode 中代码跳转正常。
- VSCode 中语法错误提示正常。
- 输入 `FV` 后无法自动弹出 `FVector` 等补全候选。
- 手动按 `Ctrl + Space` 后，补全面板卡在“正在加载”。
- VSCode 自定义代码片段也无法正常弹出。
- 同一项目在 Neovim + clangd 中补全正常。

这些现象说明 clangd 对项目的基础解析能力没有根本问题，`compile_commands.json` 也能被找到。问题主要发生在 VSCode 的补全提供链路上，而不是 UE 项目或 clangd 本身无法工作。

## 最终定位

本次问题的核心原因是 VSCode 中 C++ 补全 Provider 存在竞争或阻塞。

项目同时启用了：

- `llvm-vs-code-extensions.vscode-clangd`
- `ms-vscode.cpptools`

虽然 workspace 中原本尝试关闭 Microsoft C/C++ 插件的 IntelliSense，但配置值写成了大写：

```jsonc
"C_Cpp.intelliSenseEngine": "Disabled",
"C_Cpp.autocomplete": "Disabled",
"C_Cpp.errorSquiggles": "Disabled"
```

VSCode/C/C++ 插件设置通常使用小写枚举值。大写 `Disabled` 可能没有按预期生效，导致 Microsoft C/C++ 插件仍然参与 C++ 补全或语言服务注册。

当 VSCode 同时存在 clangd 和 C/C++ 两套 C++ 语言服务时，`Ctrl + Space` 会触发多个补全 Provider。只要其中一个 Provider 长时间未返回，VSCode 补全面板就可能停在“正在加载”，表现为 clangd 补全、普通单词补全和 snippet 都无法稳定出现。

## 已采用的解决方案

在 `BearSurvior.code-workspace` 中完成以下调整。

### 1. 正确禁用 C/C++ 插件的 IntelliSense 能力

将大写枚举值改为小写：

```jsonc
"C_Cpp.intelliSenseEngine": "disabled",
"C_Cpp.autocomplete": "disabled",
"C_Cpp.errorSquiggles": "disabled"
```

这样保留 C/C++ 插件时，也尽量避免它和 clangd 抢 C++ 语言服务。

### 2. 显式打开 VSCode 补全和代码片段

增加以下设置，确保 VSCode 自身补全面板和 snippet 不被隐藏：

```jsonc
"editor.quickSuggestions": {
    "other": true,
    "comments": false,
    "strings": false
},
"editor.suggestOnTriggerCharacters": true,
"editor.snippetSuggestions": "top",
"editor.suggest.showSnippets": true,
"editor.suggest.showWords": true,
"editor.suggestSelection": "first",
"editor.tabCompletion": "on"
```

### 3. 整理 clangd 启动参数

将 clangd 参数整理为单一、无重复、无明显冲突的一组：

```jsonc
"clangd.arguments": [
    "--compile-commands-dir=.vscode",
    "--background-index",
    "--clang-tidy",
    "-j=6",
    "--pch-storage=memory",
    "--completion-style=detailed",
    "--header-insertion=iwyu",
    "--all-scopes-completion",
    "--function-arg-placeholders=false"
]
```

## 验证结果

调整后重新加载 VSCode，并重启 clangd language server，补全恢复正常。

验证过的行为：

- 输入 `FV` 后可以正常出现补全候选。
- `Ctrl + Space` 不再长期卡在“正在加载”。
- VSCode 的代码片段补全恢复正常。

## 推荐的日常使用方式

UE C++ 项目中，clangd 已经可以承担主要代码编辑能力：

- 代码补全
- 跳转定义和声明
- 查找引用
- 语法诊断
- clang-tidy 检查
- include 建议
- 格式化配合 clang-format

Microsoft C/C++ 插件主要价值是 VSCode 内的 C++ 调试能力。如果不在 VSCode 中断点调试，可以直接禁用 C/C++ 插件，只保留 clangd。

推荐分工：

| 功能 | 推荐负责 |
| --- | --- |
| C++ 补全 | clangd |
| 跳转定义/声明 | clangd |
| 查找引用 | clangd |
| 语法诊断 | clangd |
| UE 编译 | UnrealBuildTool / VSCode Task |
| VSCode 内断点调试 | Microsoft C/C++ 插件 |

如果保留 C/C++ 插件用于调试，应继续保持以下 workspace 配置，避免它接管 IntelliSense：

```jsonc
"C_Cpp.intelliSenseEngine": "disabled",
"C_Cpp.autocomplete": "disabled",
"C_Cpp.errorSquiggles": "disabled"
```

## 后续排查建议

如果后续再次出现补全卡在“正在加载”，优先按以下顺序检查：

1. 确认 VSCode 是通过 `BearSurvior.code-workspace` 打开的，而不是直接打开项目文件夹。
2. 执行 `Developer: Reload Window`。
3. 执行 `clangd: Restart language server`。
4. 临时禁用 `ms-vscode.cpptools`，只保留 clangd 测试补全。
5. 查看 `Output -> clangd` 是否有编译数据库、索引或补全相关错误。
6. 查看 `Output -> C/C++`，确认 Microsoft C/C++ 插件是否仍在运行 IntelliSense。

## 结论

本次无法补全不是 clangd 不能解析 UE 项目，也不是 `compile_commands.json` 缺失。根因是 VSCode 侧 C++ 语言服务和补全 Provider 链路不稳定，重点触发因素是 C/C++ 插件与 clangd 并存，以及 workspace 中禁用 C/C++ IntelliSense 的配置值大小写不规范。

修正 workspace 配置后，VSCode + clangd 可以正常用于当前 UE5 C++ 项目开发。
