// 近战武器数据资产。每种近战武器对应一个 .uasset 实例。
// 继承自 UItemDataAsset，在基类公共字段基础上扩展近战专属战斗字段。
// 替代旧的 DataTable + FDataTableRowHandle 模式。

#pragma once

#include "CoreMinimal.h"
#include "Weapon/ItemDataAsset.h"
#include "MeleeWeaponDataAsset.generated.h"

/**
 * 近战武器数据资产。
 * 在 Content Browser 中右键 → Miscellaneous → DataAsset → 选 UMeleeWeaponDataAsset 创建。
 * 每个资产实例对应一种近战武器（如铁剑、战斧），编辑时直接在属性面板配置所有字段。
 */
UCLASS(BlueprintType, Const)
class BEARSURVIOR_API UMeleeWeaponDataAsset : public UItemDataAsset
{
	GENERATED_BODY()

public:
	// 基础伤害值，单次近战命中的基础伤害。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BaseDamage = 25.0f;

	// 攻击间隔，两次近战攻击之间的最短等待时间（秒）。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackInterval = 1.0f;

	// 每次近战攻击消耗的耐久值。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Durability", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DurabilityCostPerAttack = 1.0f;

	// 近战攻击范围（射线最大距离，厘米）。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackRange = 200.0f;

	// 近战扫描球体半径（厘米），越大越容易命中但手感越"粘"。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackRadius = 50.0f;

	// 攻击窗口内是否允许命中多个目标。为 false 时单次窗口只命中第一个目标。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	bool bCanHitMultipleTargets = false;
};
