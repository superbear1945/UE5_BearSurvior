# AGENTS.md - BearSurvior UE5 Agent 工作指南

## 项目定位

- 本仓库是基于 **Unreal Engine 5.7** 的 C++ 项目，主模块为 `BearSurvior`。
- 当前真实开发范围优先关注 `Source/BearSurvior/Public/`、`Source/BearSurvior/Private/` 以及用户明确点名的功能目录。
- `Source/BearSurvior/Variant_Combat/`、`Source/BearSurvior/Variant_Platforming/`、`Source/BearSurvior/Variant_SideScrolling/` 是 UE 示例内容，**除非用户明确提到，否则不要主动修改、分析或纳入方案**。
- 项目启用了 `EnhancedInput`、`AIModule`、`StateTreeModule`、`GameplayStateTreeModule`、`UMG`，并在 `.uproject` 中启用了 `StateTree`、`GameplayStateTree`、`ModelingToolsEditorMode`、`PythonScriptPlugin`、`UE_MCP_Bridge`。

## 仓库规则来源

- 当前仓库内存在已有 `AGENTS.md`，本文件是在其基础上整理与增强后的统一指南。
- 未发现 `.cursorrules`。
- 未发现 `.cursor/rules/` 下的规则文件。
- 未发现 `.github/copilot-instructions.md`。
- 因此，agent 主要遵循本文件与代码现状，不需要额外合并 Cursor/Copilot 专用规则。

## 目录重点

```text
BearSurvior.uproject
Source/BearSurvior/
  BearSurvior.Build.cs
  BearSurviorCharacter.h/.cpp
  Public/
  Private/
  Variant_Combat/            # 默认忽略，除非用户明确要求
  Variant_Platforming/       # 默认忽略，除非用户明确要求
  Variant_SideScrolling/     # 默认忽略，除非用户明确要求
```

## 构建命令

- 本项目没有 `Makefile`、`package.json`、`CMakeLists.txt` 或独立 lint/test 脚本，标准构建方式是 **UnrealBuildTool**。
- 推荐优先使用 Unreal 官方批处理脚本构建，而不是自行拼装 MSBuild 命令。

```bat
"<UE5安装路径>\Engine\Build\BatchFiles\Build.bat" BearSurviorEditor Win64 Development "D:\epic\Project\BearSurvior_UE5\BearSurvior.uproject"
```

- 编译 Editor 目标时使用上面的 `BearSurviorEditor`。
- 如需编译游戏运行目标，可使用：

```bat
"<UE5安装路径>\Engine\Build\BatchFiles\Build.bat" BearSurvior Win64 Development "D:\epic\Project\BearSurvior_UE5\BearSurvior.uproject"
```

- 如需重新生成工程文件，优先在资源管理器中右键 `BearSurvior.uproject` 执行 `Generate Visual Studio project files`。
- 若只是快速验证少量 C++ 变更，可在编辑器内使用 Live Coding，但最终仍应以完整 UBT 编译结果为准。

## 测试命令

- 当前 `Source/` 内**未发现** `IMPLEMENT_SIMPLE_AUTOMATION_TEST`、`IMPLEMENT_COMPLEX_AUTOMATION_TEST`、`DEFINE_SPEC` 等自动化测试定义。
- 因此，当前项目的主要验证方式仍是：
  - UBT 成功编译
  - UE5 Editor 中 PIE 手动验证
  - 必要时查看输出日志确认输入、UI、AI、StateTree 行为

### 运行所有自动化测试（若后续新增）

```bat
"<UE5安装路径>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\epic\Project\BearSurvior_UE5\BearSurvior.uproject" -unattended -nop4 -NullRHI -ExecCmds="Automation RunTests BearSurvior; Quit"
```

### 运行单个自动化测试（重点）

- Unreal 的“单测”通常是按测试名过滤，而不是按文件路径执行。
- 先在编辑器 Session Frontend / Automation 面板中确认测试名，再执行：

```bat
"<UE5安装路径>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\epic\Project\BearSurvior_UE5\BearSurvior.uproject" -unattended -nop4 -NullRHI -ExecCmds="Automation RunTests BearSurvior.YourTestName; Quit"
```

- 若测试名层级更细，可以直接传完整名称，例如 `Project.Input.MouseSensitivity`。
- 如果仓库后续新增测试，优先在新代码附近补充“如何跑该测试”的注释或文档。

## Lint / 静态检查

- 当前仓库没有独立 `lint` 命令，也没有 Clang-Tidy/CppCheck 配置文件。
- 实际代码检查手段是：
  - UBT 编译错误与警告
  - UE Header Tool 反射检查
  - 编辑器内 Live Coding / 启动时加载检查
  - 运行时 `UE_LOG`、屏幕调试信息、断言
- 如果用户要求“lint”，默认解释为：完成一次干净的 UBT 编译并处理编译器/反射警告。

## Agent 工作边界

- 不要假设 Variant 示例目录是正式需求来源。
- 未被点名时，不要为了“找模式”而大范围修改 `Variant_*` 文件。
- 若需要参考已有写法，优先看：
  - `Source/BearSurvior/BearSurviorCharacter.h`
  - `Source/BearSurvior/BearSurviorCharacter.cpp`
  - `Source/BearSurvior/Public/MainGameUserSetting.h`
  - `Source/BearSurvior/Public/PauseMenu.h`
  - `Source/BearSurvior/Private/PauseMenu.cpp`

## C++ 代码风格

### 注释与语言

- **用中文写注释、说明和文档**。
- 每个类开头都应有中文职责说明，且修改后要检查注释是否仍准确。
- 复杂函数、关键状态字段、蓝图绑定字段需要补充中文注释。
- 简单 getter/setter 不要堆砌无意义注释，但对蓝图暴露接口应说明用途。

### 头文件与 include 顺序

- `.h` 文件通常按以下顺序组织：
  1. `CoreMinimal.h`
  2. 父类头文件
  3. 需要的引擎头
  4. 本模块其他依赖
  5. `*.generated.h` 且**必须最后一个 include**
- 能前向声明就前向声明，减少头文件耦合。
- `.cpp` 先包含本类头文件，再包含其余依赖。
- 现有代码未严格执行全仓 IWYU；改动时优先保持“能编译 + 不额外引入无关 include”。

### 格式与布局

- 使用 UE/C++ 常见大括号风格：函数与多数控制块另起一行。
- 缩进保持与现有文件一致；仓库当前主要为 `tab` 缩进。
- 单行 `if` 在现有代码中允许省略大括号，但仅限非常简单且无歧义的语句。
- 优先使用早返回，避免多层嵌套。
- 修改旧文件时以“延续现有局部风格”为先，不要顺手做大规模格式化。

### 命名约定

- 遵循 Unreal 前缀约定：`A/U/F/E/I/T/S`。
- 布尔值使用 `b` 前缀，例如 `bIsPauseMenuOpen`、`bIsSeeingPlayer`。
- 函数使用 PascalCase，动作函数优先动词开头。
- 面向蓝图输入/交互的方法，项目内常用 `DoXxx` 命名，如 `DoMove()`、`DoAimEnd()`。
- `UPROPERTY` 字段名以 PascalCase 为主，局部变量通常是 camelCase 或具名对象名。
- 文件名通常与主要类名一致。

### 类型与 UE 习惯用法

- `UObject` 引用成员优先使用 `UPROPERTY()` 包裹，避免 GC 丢失引用。
- 在新代码中，`UObject*` 成员优先使用 `TObjectPtr<>`，与当前代码趋势保持一致。
- `TSubclassOf<>` 用于蓝图可配置类引用，例如 Widget Class。
- 配置/持久化设置使用 `UCLASS(Config=...)` + `UPROPERTY(Config, ...)`。
- 能用 `const` 的地方要用 `const`；只读函数加 `const`。
- 数值合法区间优先通过 `meta=(ClampMin, ClampMax, UIMin, UIMax)` 和 `FMath::Clamp` 双重约束。

### UPROPERTY / UFUNCTION 规范

- 蓝图读写权限最小化；能 `BlueprintReadOnly` 就不要随意 `BlueprintReadWrite`。
- 组件引用常见写法：`VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true")`。
- 蓝图可调用行为函数使用 `BlueprintCallable`；纯读取接口使用 `BlueprintPure`。
- UI 绑定字段使用 `meta=(BindWidget)`，命名必须与 UMG 中绑定变量一致。
- 反射宏、类声明、成员分类应清晰，`Category` 名称保持稳定，不要随意改坏蓝图兼容性。

### 错误处理与健壮性

- 先做空指针/有效性检查，再做核心逻辑。
- 优先使用卫语句：判空、非法输入、缺少依赖时立即 `return`。
- 对运行时缺失但不致命的问题，使用 `UE_LOG(..., Warning, ...)`。
- 对关键失败可使用 `Error` 日志；只有在确定应中止开发态执行时再用 `check()`。
- 不要吞掉失败原因；日志里要带上 `GetNameSafe(this)` 或相关对象名，方便定位。
- 动态绑定事件时，析构/结束生命周期中要对应 `RemoveDynamic`。

### 输入、UI、设置相关模式

- 输入系统使用 `EnhancedInput`；新增输入行为应优先沿用现有绑定方式。
- UI 逻辑尽量放在 `UUserWidget` 子类中，角色/控制器负责状态切换与输入模式控制。
- 本地设置优先集中到 `UMainGameUserSetting` 这类设置类，不要把持久化逻辑散落在多个 Widget/Character 中。
- 暂停菜单、输入模式、鼠标显示等逻辑需要同时考虑：Widget 生命周期、`SetInputMode`、`SetPause`、`bShowMouseCursor`。

### 日志与调试

- 优先使用自定义日志类别；当前项目已有 `LogTemplateCharacter`、`LogCombatCharacter` 示例。
- 临时调试可用 `LogTemp`，但长期保留的核心系统日志建议使用明确分类。
- 屏幕调试信息 `GEngine->AddOnScreenDebugMessage` 只适合临时验证，不要无节制保留。

## 新文件与模块变更

- 新增类通常同时创建 `.h` 与 `.cpp`。
- 通用公开头文件放 `Source/BearSurvior/Public/`，实现放 `Source/BearSurvior/Private/`。
- 若确实要扩展示例 Variant 目录，必须先确认用户明确要求该目录。
- 新增模块依赖时，更新 `Source/BearSurvior/BearSurvior.Build.cs`，并保持依赖最小化。

## 提交前自检

- 能否完成一次 `BearSurviorEditor` 的 UBT 编译。
- 是否破坏了 `generated.h` include 顺序。
- 是否遗漏 `UPROPERTY`/`UFUNCTION` 反射标记。
- 是否补齐中文类注释与必要成员注释。
- 是否为新增委托绑定补充了解绑逻辑。
- 是否误改了未被要求处理的 `Variant_*` 示例目录。
