// 物品公共数据资产基类。所有可拾取/可装备物品的通用数据基类。
// 继承链：UItemDataAsset（基类，Abstract）→ UMeleeWeaponDataAsset / URangedWeaponDataAsset
// 替代旧的 DataTable + FDataTableRowHandle 模式，每个具体物品对应一个 .uasset 实例。
// 字段直接定义在 DataAsset 类体上，不嵌套 Struct。

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "Weapon/ItemEnums.h"
#include "ItemDataAsset.generated.h"

/**
 * 物品公共数据基类。所有可拾取/可装备物品的通用数据。
 * 每个资产实例对应一种物品的完整公共数据配置。
 * 武器专属数据由派生类 UMeleeWeaponDataAsset / URangedWeaponDataAsset 定义。
 * 在 Content Browser 中右键 → Miscellaneous → DataAsset → 选派生类创建。
 */
UCLASS(BlueprintType, Abstract, Const)
class BEARSURVIOR_API UItemDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
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
	TSoftObjectPtr<UStaticMesh> WeaponMesh = nullptr;

	// 最大耐久度，耐久度上限。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxDurability = 100.0f;

	// 握持插槽名称。角色左手 IK 目标将跟随此插槽的世界变换。
	// 在武器网格体上创建此名称的插槽，控制绑定会驱动左手骨骼匹配该位置。
	// 例如 "GripSocket"、"WeaponGrip" 等。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|IK")
	FName GripSocketName = TEXT("GripSocket");
};
