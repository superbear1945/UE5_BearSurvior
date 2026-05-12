# AGENTS.md - BearSurvior UE5 Agent 工作指南

## 项目定位

- 本仓库是基于 **Unreal Engine 5.7** 的 C++ 项目，主模块为 `BearSurvior`。
- 默认只关注 `Source/BearSurvior/Public/`、`Source/BearSurvior/Private/` 和用户明确点名的目录。
- `Source/BearSurvior/Variant_Combat/`、`Source/BearSurvior/Variant_Platforming/`、`Source/BearSurvior/Variant_SideScrolling/` 是 **UE 自带示例**，**若用户没有特别提到则不需要理会，也不要主动修改、分析或纳入方案**。
- 主要模块依赖：`EnhancedInput`、`AIModule`、`NavigationSystem`、`StateTreeModule`、`GameplayStateTreeModule`、`UMG`。
- `.uproject` 已启用插件：`PythonScriptPlugin`、`ModelingToolsEditorMode`、`StateTree`、`GameplayStateTree`、`UE_MCP_Bridge`。

## 规则来源

- 仓库根目录已有 `AGENTS.md`，本文件是在原基础上整理增强后的统一规范。
- 未发现 `.cursorrules`。
- 未发现 `.cursor/rules/`。
- 未发现 `.github/copilot-instructions.md`。
- 因此无需额外合并 Cursor 或 Copilot 规则；按本文件和仓库现状执行即可。

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

- 本项目没有 `Makefile`、`package.json`、`CMakeLists.txt` 或独立脚本，标准构建方式是 **UnrealBuildTool**。
- 优先使用 Unreal 官方批处理脚本，不要自行拼装 MSBuild 命令。

```bat
"<UE5安装路径>\Engine\Build\BatchFiles\Build.bat" BearSurviorEditor Win64 Development "D:\epic\Project\BearSurvior_UE5\BearSurvior.uproject"
```

- 游戏目标编译：

```bat
"<UE5安装路径>\Engine\Build\BatchFiles\Build.bat" BearSurvior Win64 Development "D:\epic\Project\BearSurvior_UE5\BearSurvior.uproject"
```

- 重新生成工程文件：右键 `BearSurvior.uproject` -> `Generate Visual Studio project files`。
- 小改动可用 Live Coding 快速验证，但最终仍以完整 UBT 编译结果为准。

## 测试命令

- 当前 `Source/` 内**未发现** `IMPLEMENT_SIMPLE_AUTOMATION_TEST`、`IMPLEMENT_COMPLEX_AUTOMATION_TEST`、`DEFINE_SPEC`、`BEGIN_DEFINE_SPEC`。
- 因此当前主要验证方式是：
  - UBT 成功编译
  - UE5 Editor 中 PIE 手动验证
  - 查看日志确认输入、UI、AI、StateTree 行为

### 运行全部自动化测试（若后续新增）

```bat
"<UE5安装路径>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\epic\Project\BearSurvior_UE5\BearSurvior.uproject" -unattended -nop4 -NullRHI -ExecCmds="Automation RunTests BearSurvior; Quit"
```

### 运行单个自动化测试

- Unreal 单测通常按**测试名**过滤，不按文件路径执行。
- 先在 Session Frontend / Automation 面板确认测试名，再执行：

```bat
"<UE5安装路径>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\epic\Project\BearSurvior_UE5\BearSurvior.uproject" -unattended -nop4 -NullRHI -ExecCmds="Automation RunTests BearSurvior.YourTestName; Quit"
```

- 如果测试层级更细，直接传完整名称，例如 `Project.Input.MouseSensitivity`。
- 后续若新增测试，优先在相邻代码或文档中写清测试名和运行方式。

## Lint / 静态检查

- 当前仓库没有独立 `lint` 命令，也没有 Clang-Tidy/CppCheck 配置。
- 实际检查手段是：UBT 编译警告、UHT 反射检查、编辑器加载/Live Coding、运行时 `UE_LOG`、临时屏幕调试、断言。
- 如果用户要求“lint”，默认理解为：做一次干净的 UBT 编译并处理编译器/UHT 警告。

## Agent 工作边界

- 不要把 `Variant_*` 示例目录当成正式需求来源。
- 未被点名时，不要为了“参考模式”而大范围阅读或修改 `Variant_*`。
- 参考现有写法时，优先查看：
  - `Source/BearSurvior/BearSurviorCharacter.h`
  - `Source/BearSurvior/BearSurviorCharacter.cpp`
  - `Source/BearSurvior/Public/MainGameUserSetting.h`
  - `Source/BearSurvior/Public/PauseMenu.h`
  - `Source/BearSurvior/Private/PauseMenu.cpp`

## C++ 代码规范

### 注释与文档

- **新增或修改代码后，必须使用简体中文编写详细注释。**
- **每个文件最上方都要写清该文件作用。**
- **每个函数定义上方都要写清该函数作用。**
- **若有函数内有较复杂大块逻辑需要写下清晰注释解释逻辑间的跳转和作用**
- 类、复杂函数、关键状态字段、蓝图绑定字段都要有中文说明。
- 简单 getter/setter 可简洁，但蓝图暴露接口仍应说明用途。
- **修改代码时必须同步检查旧注释是否已落后于实现；若过时，必须一并更新。**

### 头文件与 include

- `.h` 文件通常按顺序组织：`CoreMinimal.h` -> 父类头 -> 引擎依赖 -> 模块内依赖 -> `*.generated.h`。
- `*.generated.h` **必须最后 include**。
- 能前向声明就前向声明，减少耦合。
- `.cpp` 先包含本类头文件，再包含其余依赖。
- 仓库未严格全量执行 IWYU；改动时优先保持“能编译且不额外引入无关 include”。

### 格式与布局

- 使用 UE 常见大括号风格：函数和多数控制块另起一行。
- 缩进保持现有风格；当前仓库主要使用 `tab`。
- **函数中优先使用卫语句（早返回），避免多层嵌套。** 先处理异常/边界情况并立即返回，主线逻辑保持在函数顶层，最大嵌套深度建议不超过 3 层。
  ```cpp
  // 推荐：卫语句写法
  void Process(Foo* InFoo)
  {
  	if (!IsValid(InFoo))
  		return;
  	if (!InFoo->CanProcess())
  		return;
  	// 主线逻辑在此，无需嵌套
  	DoWork(InFoo);
  }
  ```
- `if`、`for`、`while` 等控制块内若**只有一行语句**，省略大括号，仅保留缩进。
  ```cpp
  if (!Ptr)
  	return;
  for (auto& Item : Items)
  	Item.Reset();
  ```
- 修改旧文件时延续局部风格，不要顺手大规模格式化。

### 命名约定

- 遵循 Unreal 前缀：`A/U/F/E/I/T/S`。
- 布尔值使用 `b` 前缀，如 `bIsPauseMenuOpen`、`bIsSeeingPlayer`。
- 函数使用 PascalCase，动作函数优先动词开头。
- 蓝图输入/交互方法优先沿用 `DoXxx`，如 `DoMove()`、`DoAimEnd()`。
- `UPROPERTY` 字段名以 PascalCase 为主；局部变量通常为 camelCase 或具名对象名。
- 文件名通常与主要类名一致。

### 类型与 UE 用法

- `UObject` 成员优先用 `UPROPERTY()` 包裹，避免 GC 丢引用。
- 新代码中的 UObject 成员优先用 `TObjectPtr<>`。
- 蓝图可配置类引用优先用 `TSubclassOf<>`。
- 配置/持久化设置使用 `UCLASS(Config=...)` + `UPROPERTY(Config, ...)`。
- 能 `const` 就 `const`；只读函数加 `const`。
- 数值区间优先同时用 `meta=(ClampMin, ClampMax, UIMin, UIMax)` 和 `FMath::Clamp` 双重约束。

### UPROPERTY / UFUNCTION

- 蓝图权限尽量最小化；能 `BlueprintReadOnly` 就不要放大到 `BlueprintReadWrite`。
- 组件引用常用：`VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true")`。
- 行为函数用 `BlueprintCallable`；纯读取接口用 `BlueprintPure`。
- UI 绑定字段使用 `meta=(BindWidget)`，名称必须和 UMG 变量一致。
- `Category` 命名保持稳定，避免破坏蓝图兼容性。

### 错误处理与健壮性

- 先做空指针/有效性检查，再进入核心逻辑。
- 优先使用卫语句；依赖缺失、输入非法时立即 `return`。
- 非致命问题使用 `UE_LOG(..., Warning, ...)`。
- 关键失败可记 `Error`；仅在确定应中断开发态执行时才用 `check()`。
- 日志应带 `GetNameSafe(this)` 或相关对象名，避免丢失上下文。
- 动态绑定事件后，要在析构或 `EndPlay`/`NativeDestruct` 中对应 `RemoveDynamic`。

### 输入、UI、设置模式

- 输入系统使用 `EnhancedInput`，新增输入逻辑应优先沿用现有绑定方式。
- UI 逻辑尽量放在 `UUserWidget` 子类，角色/控制器负责状态切换和输入模式控制。
- 本地设置集中到 `UMainGameUserSetting` 之类的设置类，不要把持久化散落在多个 Widget/Character。
- 处理暂停菜单时，同时考虑 Widget 生命周期、`SetInputMode`、`SetPause`、`bShowMouseCursor`。

### 日志与调试

- 优先使用明确日志类别；当前已有 `LogTemplateCharacter`、`LogCombatCharacter`。
- `LogTemp` 只适合临时调试，长期保留的核心日志应使用自定义分类。
- `GEngine->AddOnScreenDebugMessage` 仅适合临时验证，不要长期滥用。

## 新文件与模块变更

- 新增类通常同时创建 `.h` 与 `.cpp`。
- 通用公开头文件放 `Source/BearSurvior/Public/`，实现放 `Source/BearSurvior/Private/`。
- 只有在用户明确要求时才扩展示例 `Variant_*` 目录。
- 新增模块依赖时更新 `Source/BearSurvior/BearSurvior.Build.cs`，并保持依赖最小化。

## 提交前自检

- 能否完成一次 `BearSurviorEditor` 的 UBT 编译。
- 是否破坏 `generated.h` include 顺序。
- 是否遗漏 `UPROPERTY`/`UFUNCTION` 反射标记。
- 是否补齐文件头注释、函数注释、中文说明，并检查旧注释是否已过时。
- 是否为新增委托绑定补上了解绑逻辑。
- 是否误改了未被要求处理的 `Variant_*` 示例目录。
