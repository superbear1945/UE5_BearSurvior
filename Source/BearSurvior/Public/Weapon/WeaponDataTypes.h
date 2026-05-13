// 物品与武器属性数据表行结构体定义。
// FItemData 只保存物品展示与拾取层面的公共字段，FMeleeWeaponData / FRangedWeaponData 独立保存各自攻击组件所需字段。
// 三种结构体都直接继承 FTableRowBase，可分别作为独立 DataTable 行类型，避免武器专属数据和物品公共数据互相耦合。

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WeaponDataTypes.generated.h"

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

/**
 * 物品公共数据：只负责物品自身的展示、背包和世界外观字段。
 * 武器攻击、伤害、弹药、耐久消耗等专属字段由近战/远程数据结构独立维护。
 */
USTRUCT(BlueprintType)
struct BEARSURVIOR_API FItemData : public FTableRowBase
{
	GENERATED_BODY()

	// 物品显示名称，用于背包UI、拾取提示等。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText DisplayName;

	// 物品描述文本，用于Tooltip等详细信息展示。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText Description;

	// 物品图标，用于背包格子、快捷栏等UI。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	// 物品重量，单位自定义。可用于背包容量计算等。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Weight = 0.0f;

	// 物品品质/稀有度。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemRarity Rarity = EItemRarity::Common;

	// 武器世界显示网格，用于在世界中显示武器外观。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TSoftObjectPtr<UStaticMesh> WeaponMesh;

	// 最大耐久度，耐久度上限。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Durability", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxDurability = 100.0f;
};

/**
 * 近战武器数据：独立保存近战攻击组件所需配置，不再继承 FItemData。
 * 用作 DT_MeleeWeapons DataTable 的行类型。
 */
USTRUCT(BlueprintType)
struct BEARSURVIOR_API FMeleeWeaponData : public FTableRowBase
{
	GENERATED_BODY()

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

/**
 * 远程武器数据：独立保存远程攻击组件所需配置，不再继承 FItemData。
 * 用作 DT_RangedWeapons DataTable 的行类型。
 */
USTRUCT(BlueprintType)
struct BEARSURVIOR_API FRangedWeaponData : public FTableRowBase
{
	GENERATED_BODY()

	// 基础伤害值，单次射击命中的基础伤害。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged|Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BaseDamage = 20.0f;

	// 攻击间隔，两次远程攻击入口之间的最短等待时间（秒）。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged|Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackInterval = 0.1f;

	// 每秒射击次数，即射速。例如 600 = 每秒10发。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FireRate = 600.0f;

	// 弹匣容量。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged|Ammo", meta = (ClampMin = "1", UIMin = "1"))
	int32 MagazineCapacity = 30;

	// 装弹所需时间（秒）。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged|Ammo", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ReloadTime = 2.0f;

	// 子弹散布角度（度），0 表示完全精准。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged|Accuracy", meta = (ClampMin = "0.0", ClampMax = "45.0", UIMin = "0.0", UIMax = "45.0"))
	float SpreadAngle = 1.0f;

	// 是否为全自动射击模式。为 false 时为半自动（每次按键只射击一次）。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged")
	bool bAutomaticFire = true;

	// 最大有效射程（厘米）。超过此距离的射线不造成伤害。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxRange = 10000.0f;

	// 每次射击消耗的耐久值。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DurabilityCostPerShot = 0.5f;

	// 储备弹药总数（背包弹药），-1 表示无限弹药。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged|Ammo")
	int32 ReserveAmmo = 90;
};
