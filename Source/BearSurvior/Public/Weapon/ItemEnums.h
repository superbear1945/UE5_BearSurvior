// 物品与武器枚举类型定义。
// 所有物品/武器相关的枚举集中在此文件中管理。
// 物品/武器数据字段已迁移到 DataAsset 继承链（UItemDataAsset → UMeleeWeaponDataAsset / URangedWeaponDataAsset），
// 不再使用 DataTable + FTableRowBase 结构体模式。

#pragma once

#include "CoreMinimal.h"
#include "ItemEnums.generated.h"

// 物品品质/稀有度枚举，用于背包分类、UI显示、掉落权重等。
UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	Common       UMETA(DisplayName = "普通"),
	Uncommon     UMETA(DisplayName = "优秀"),
	Rare         UMETA(DisplayName = "稀有"),
	Epic         UMETA(DisplayName = "史诗"),
	Legendary    UMETA(DisplayName = "传说")
};

// 武器类型枚举，用于区分近战与远程武器。
UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	Melee    UMETA(DisplayName = "近战"),
	Ranged   UMETA(DisplayName = "远程")
};
