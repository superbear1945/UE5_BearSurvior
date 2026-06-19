// 远程武器数据资产。每种远程武器对应一个 .uasset 实例。
// 继承自 UItemDataAsset，在基类公共字段基础上扩展远程专属战斗字段。
// 替代旧的 DataTable + FDataTableRowHandle 模式。

#pragma once

#include "CoreMinimal.h"
#include "Weapon/ItemDataAsset.h"
#include "RangedWeaponDataAsset.generated.h"

class USoundBase;
class UStaticMesh;

/**
 * 远程武器数据资产。
 * 在 Content Browser 中右键 → Miscellaneous → DataAsset → 选 URangedWeaponDataAsset 创建。
 * 每个资产实例对应一种远程武器（如 AK47、手枪），编辑时直接在属性面板配置所有字段。
 */
UCLASS(BlueprintType, Const)
class BEARSURVIOR_API URangedWeaponDataAsset : public UItemDataAsset
{
	GENERATED_BODY()

public:
	// 基础伤害值，单次射击命中的基础伤害。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged|Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BaseDamage = 20.0f;

	// 攻击间隔，两次远程攻击入口之间的最短等待时间（秒）。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged|Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackInterval = 0.1f;

	// 每分钟射击次数（RPM, Rounds Per Minute），即射速。例如 600 = 每分钟 600 发 = 每秒 10 发。
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

	// 武器弹匣的世界显示网格，用于在世界中显示弹匣外观（可选）。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged")
	TSoftObjectPtr<UStaticMesh> MagazineMesh = nullptr;

	// 弹匣的偏移量，默认附着点可能和弹匣位置不完全匹配时使用。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged")
	FVector MagazineOffset = FVector::ZeroVector;

	// 枪声资源，用于远程武器开火时提供对应音效资源引用。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged|Audio")
	TSoftObjectPtr<USoundBase> GunshotSound;

	// 开火时的后坐力配置（可选），以旋转形式表示，默认无后坐力。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranged|Combat")
	FRotator RecoilRotation = FRotator::ZeroRotator;
};
