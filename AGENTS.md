# AGENTS.md - BearSurvior UE5 开发指南

## 项目概览

BearSurvior 是基于 **Unreal Engine 5.7** 的 C++ 动作生存游戏项目。项目包含多个玩法变体模块：
- **Variant_Combat** — 近战战斗系统（连击、蓄力攻击、AI 敌人）
- **Variant_Platforming** — 平台跳跃系统
- **Variant_SideScrolling** — 横版过关系统

依赖模块：`Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `AIModule`, `StateTreeModule`, `GameplayStateTreeModule`, `UMG`

插件：`ModelingToolsEditorMode`, `StateTree`, `GameplayStateTree`

## 构建与编译命令

本项目使用 UnrealBuildTool (UBT) 构建，无 Makefile 或 npm 脚本。

```bash
# 全量编译（Development 配置）
# 在 UE5 Editor 中按 Ctrl+Shift+B，或通过命令行：
"<UE5安装路径>\Engine\Build\BatchFiles\Build.bat" BearSurviorEditor Win64 Development "D:\epic\Project\BearSurvior_UE5\BearSurvior.uproject"

# 编译游戏目标
"<UE5安装路径>\Engine\Build\BatchFiles\Build.bat" BearSurvior Win64 Development "D:\epic\Project\BearSurvior_UE5\BearSurvior.uproject"

# 清理并重新生成 Visual Studio 项目文件
# 右键 .uproject 文件 → "Generate Visual Studio project files"
```

### 测试

项目当前没有独立的自动化测试框架。验证逻辑通过以下方式执行：
- UE5 Editor 中 **Play In Editor (PIE)** 模式进行功能测试
- 战斗模块通过 PIE 运行时手动验证连击、伤害、AI 行为

### 代码检查

```bash
# 无独立 lint 命令。代码质量通过以下方式保证：
# 1. 编译器严格模式（PCHUsageMode.UseExplicitOrSharedPCHs）
# 2. UE5 编辑器内 Live Coding 实时检查
# 3. UBT 编译时的静态分析告警
```

## 目录结构约定

```
Source/BearSurvior/
├── Public/               # 自定义公开头文件（EnemyBase, AttackPlayer, MainGameUserSetting 等）
├── Private/              # 对应实现文件
├── Variant_Combat/       # 战斗玩法模块
│   ├── AI/               # 战斗 AI 控制器、敌人生成器、EQS
│   └── *.h / *.cpp       # 战斗角色、伤害、UI
├── Variant_Platforming/  # 平台跳跃模块
├── Variant_SideScrolling/# 横版模块
│   ├── AI/               # 横版 AI
│   ├── Gameplay/         # 交互物件（跳板、移动平台、拾取物）
│   └── *.h / *.cpp
├── BearSurvior.Build.cs  # 模块依赖定义
├── BearSurvior.h/cpp     # 模块主头文件
├── BearSurviorCharacter.* # 玩家角色基类
├── BearSurviorGameMode.*  # 游戏模式基类
└── BearSurviorPlayerController.* # 玩家控制器
```

## 代码风格规范

### 1. 注释要求

- **每个类** 的开头必须有中文注释说明其职责
- **每个函数和变量** 都需要注释说明作用（除非命名已足够自解释）
- **复杂函数** 需要函数级注释说明其逻辑意图
- 每次更新后检查类注释是否仍然准确
- 用中文编写注释和回答问题

```cpp
/**
 * 游戏用户设置类：负责保存与读取玩家本地设置。
 */
UCLASS(Config=GameUserSettings)
class BEARSURVIOR_API UMainGameUserSetting : public UGameUserSettings
{
    /** 获取当前鼠标灵敏度，供角色与 UI 初始化时读取。 */
    UFUNCTION(BlueprintPure, Category = "Settings|Input")
    float GetMouseSensitivity() const;
};
```

### 2. 命名约定

| 类型 | 规则 | 示例 |
|------|------|------|
| 类名 | `A` 前缀 (Actor)、`U` 前缀 (UObject)、`F` 前缀 (Struct)、`I` 前缀 (Interface) | `ACombatCharacter`, `UMainGameUserSetting`, `FStateTreeAttackPlayerInstanceData` |
| 函数 | PascalCase，动词开头 | `DoAttackTrace()`, `ApplyMouseSensitivityFromSettings()` |
| UFUNCTION | Blueprint 交互函数使用 `Do` + 动作名 | `DoMove()`, `DoJumpStart()`, `DoAimEnd()` |
| 变量 | camelCase，布尔值以 `b` 前缀 | `bIsAiming`, `MouseSensitivity`, `ComboCount` |
| UPROPERTY | PascalCase | `MaxHP`, `CurrentHP`, `MeleeTraceRadius` |
| 常量 | 大写或 UE 宏 | `KINDA_SMALL_NUMBER` |
| 模块前缀 | 文件名以模块名开头 | `CombatCharacter.h`, `SideScrollingPlayerController.h` |

### 3. 头文件组织

```cpp
// Copyright Epic Games, Inc. All Rights Reserved.
// 类的中文说明注释

#pragma once

#include "CoreMinimal.h"                              // 始终第一个
#include "父类头文件路径.h"                              // 父类
#include "需要的引擎模块.h"                              // 引擎依赖
#include "本模块内相对依赖.h"                             // 模块内依赖
#include "本类.generated.h"                             // 始终最后一个

// 前向声明（class + struct 分组）
class USpringArmComponent;
class UCameraComponent;
struct FInputActionValue;
```

### 4. UPROPERTY / UFUNCTION 宏

```cpp
// 属性暴露到编辑器/蓝图
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Melee Attack|Damage", meta=(ClampMin=0, ClampMax=100))
float MeleeDamage = 1.0f;

// 只读暴露
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
USpringArmComponent* CameraBoom;

// Blueprint 可调用函数
UFUNCTION(BlueprintCallable, Category="Input")
virtual void DoMove(float Right, float Forward);

// Blueprint 纯函数（无副作用）
UFUNCTION(BlueprintPure, Category = "Settings|Input")
float GetMouseSensitivity() const;

// Blueprint 可实现事件
UFUNCTION(BlueprintImplementableEvent, Category="Combat")
void DealtDamage(float Damage, const FVector& ImpactPoint);
```

### 5. 函数与控制流

- **使用卫语句** 防止深层嵌套，优先 early-return
- if / for 等语句中只有一行语句时，省略大括号但保留缩进
- 构造函数中使用 `CreateDefaultSubobject` 创建子组件
- 覆写函数使用 `override` 关键字

```cpp
void ABearSurviorCharacter::TogglePauseMenu()
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController || !PlayerController->IsLocalController())
    {
        UE_LOG(LogTemplateCharacter, Warning, TEXT("..."));
        return;
    }

    if (bIsPauseMenuOpen)
    {
        if (PauseMenuWidgetInstance && PauseMenuWidgetInstance->IsInViewport())
            PauseMenuWidgetInstance->RemoveFromParent();

        PlayerController->SetPause(false);
        FInputModeGameOnly InputMode;
        PlayerController->SetInputMode(InputMode);
        bIsPauseMenuOpen = false;
        return;
    }
    // ...
}
```

### 6. 接口实现

使用 `~begin InterfaceName interface` / `~end InterfaceName interface` 注释块标记接口实现区域：

```cpp
// ~begin CombatAttacker interface
virtual void DoAttackTrace(FName DamageSourceBone) override;
virtual void CheckCombo() override;
// ~end CombatAttacker interface
```

### 7. 日志与错误处理

- 使用 `UE_LOG` 进行日志输出，分类日志使用 `DECLARE_LOG_CATEGORY_EXTERN`
- 空指针检查用卫语句 + early-return 处理
- 对蓝图暴露的函数要处理无效输入（如 `FMath::Clamp` 限制范围）

```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogCombatCharacter, Log, All);
DEFINE_LOG_CATEGORY(LogCombatCharacter);

UE_LOG(LogTemplateCharacter, Warning, TEXT("PauseMenuWidgetClass is not set on %s."), *GetNameSafe(this));
```

### 8. 访问控制

- 子组件（CameraBoom、FollowCamera）设为 `private`，通过 `FORCEINLINE` 公开 getter
- 输入动作属性放在 `protected` 区域供子类扩展
- Blueprint 交互函数放在 `public` 区域
- 内部逻辑函数放在 `protected` 区域

## 新增文件清单

添加新类时，创建 `.h` 和 `.cpp` 文件对，按以下位置放置：
- 公开头文件 → `Source/BearSurvior/Public/`
- 实现文件 → `Source/BearSurvior/Private/`
- Variant 子模块内类 → 对应 Variant 目录下

如需新增模块依赖，在 `BearSurvior.Build.cs` 中添加到对应的 `DependencyModuleNames` 数组。
