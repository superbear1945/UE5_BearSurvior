# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

A learning-focused Unreal Engine 5.7 C++ game project called **BearSurvior**. Single runtime module `BearSurvior` (LoadingPhase `Default`). The default game mode is a Blueprint class (`BP_ThirdPersonGameMode`), not C++.

## Build Commands

Build the editor target (most common):

```bat
"<UE5安装路径>\Engine\Build\BatchFiles\Build.bat" BearSurviorEditor Win64 Development "D:\epic\Project\BearSurvior_UE5\BearSurvior.uproject"
```

Build the game target:

```bat
"<UE5安装路径>\Engine\Build\BatchFiles\Build.bat" BearSurvior Win64 Development "D:\epic\Project\BearSurvior_UE5\BearSurvior.uproject"
```

To regenerate project files: right-click `BearSurvior.uproject` → `Generate Visual Studio project files`.

No CI, no Makefile, no test framework exists yet. Verification is done by UBT compile success + PIE manual testing.

## Module Dependencies

`BearSurvior.Build.cs` declares these public dependencies: `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `AIModule`, `NavigationSystem`, `StateTreeModule`, `GameplayStateTreeModule`, `UMG`. Private: `Slate`, `SlateCore`.

## Architecture

### Class Hierarchy

```
AActor
├── AItemBase                          (pickup/equip/un-equip/drop, rarity, mesh, stack)
│   └── AWeaponBase                    (damage, durability, attack interval, StartAttack/StopAttack)
│       └── AAK47                      (concrete weapon subclass)
├── ACharacter
│   ├── ABearSurviorCharacter          (player: EnhancedInput, aim camera, pause menu, health)
│   └── AEnemyBase                     (enemy: AI state machine via StateTree, health, UI widget)
└── AController
    └── AAIController
        └── AMyAIController            (StateTreeComponent, AI perception, seeing/forget player)
```

### Component Architecture

- **UHealthComponent** (`Component/HealthComponent.h`): HP management with damage/heal/death events. Power-safe design (dead state prevents further damage/healing). Used by both `ABearSurviorCharacter` and `AEnemyBase`.
- **UMeleeAttackComponent** (`Component/MeleeAttackComponent.h`): Melee combat with sphere trace, attack windows, damage application.
- **URangeAttackComponent** (`Component/RangeAttackComponent.h`): Ranged combat with line trace, magazine system, reload.

### AI System

Enemies use a **StateTree**-driven AI (not Behavior Trees). The `AMyAIController` owns a `UStateTreeComponent` and handles AI perception (seeing/forgetting player). Enemy states are an enum (`EEnemyState`: Idle/Patrol/Chase/Attack/Dead) managed via `SwitchEnemyState()`. There's a custom `FSTPatrolTask` StateTree task node for patrol behavior (random point movement).

### Input System

Uses **EnhancedInput**. `ABearSurviorCharacter` binds `UInputAction` references for Move/Look/Jump/Aim/Back. Exposes `DoMove()`, `DoLook()`, `DoJumpStart()`, etc. as `BlueprintCallable` for both code and UI-driven input.

### UI System

`ABearSurviorCharacter` manages a pause menu via `UPauseMenu` widget, controlling `SetInputMode`, `SetPause`, and cursor visibility. `AEnemyBase` has a `UWidgetComponent` for health bars.

### Key Files to Reference

- `Source/BearSurvior/BearSurviorCharacter.h/.cpp` — player character
- `Source/BearSurvior/Public/Component/HealthComponent.h` — health system
- `Source/BearSurvior/Public/Weapon/WeaponBase.h` — weapon base class
- `Source/BearSurvior/Public/ItemBase.h` — item base class
- `Source/BearSurvior/Public/EnemyBase.h` — enemy character
- `Source/BearSurvior/Public/MyAIController.h` — AI controller
- `Source/BearSurvior/Public/MainGameUserSetting.h` — persistent settings
- `Source/BearSurvior/Public/PauseMenu.h` — pause menu widget

### Directory Rules

- New public headers go in `Source/BearSurvior/Public/`, implementations in `Source/BearSurvior/Private/`.
- `Source/BearSurvior/Variant_Combat/`, `Variant_Platforming/`, `Variant_SideScrolling/` are **UE sample content — ignore them** unless the user explicitly mentions them. Do not read, modify, or reference them for implementation patterns.

## Coding Conventions

- **Comments must be in Simplified Chinese**. Every file needs a header comment describing its purpose. Every function definition needs a comment describing what it does.
- Follow Unreal naming prefixes: `A`/`U`/`F`/`E`/`I`/`T`/`S`. Booleans use `b` prefix. Functions use PascalCase with verb-first for actions.
- Include order: `CoreMinimal.h` → parent class → engine deps → module deps → `*.generated.h` **last**.
- Prefer forward declarations in headers. Use `TObjectPtr<>` for UObject member pointers.
- `UPROPERTY` access: prefer `BlueprintReadOnly` over `BlueprintReadWrite`. Component refs typically use `VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true")`.
- Numeric ranges use both `meta=(ClampMin, ClampMax)` and `FMath::Clamp` for dual enforcement.
- Use guard clauses (early return on null/invalid) instead of deep nesting.
- Always unbind dynamic delegates in `EndPlay`/`NativeDestruct`.
- Log categories: `LogTemplateCharacter`, `LogCombatCharacter`. Avoid `LogTemp` for permanent logs.

## MCP Integration

The `opencode.json` configures a `ue-mcp` MCP bridge that connects to the Unreal Editor process. This allows AI tools to interact with the running editor (query actors, modify properties, etc.).
