// 物品基类。作为项目中所有可拾取/可装备物品的公共父类。
// 提供物品通用属性（名称、描述、重量、品质等）和基础行为虚函数（拾取、装备、卸下、丢弃）。
// 物品公共配置由 UItemDataAsset 派生类提供，通过 ItemDataAsset 字段直接引用 DataAsset 资产。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon/ItemDataAsset.h"
#include "Weapon/ItemEnums.h"
#include "ItemBase.generated.h"

/**
 * 物品基类：所有可拾取物品的公共父类。
 * 封装物品数据、可视化组件，以及物品生命周期中的虚函数入口。
 * 物品拾取通过射线检测实现，物品的 StaticMesh 本身作为射线目标。
 */
UCLASS(Abstract)
class BEARSURVIOR_API AItemBase : public AActor
{
	GENERATED_BODY()

public:
	/** 构造函数，初始化物品默认属性和组件。 */
	AItemBase();

// ────────────────────────────────────────── 组件 ──────────────────────────────────────────

protected:

	// 物品的可视化网格组件，用于在世界中显示物品外观。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Component")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

// ────────────────────────────────────────── 数据 ──────────────────────────────────────────

public:

	// 物品公共数据资产引用。该数据只负责物品展示、背包字段和世界外观，不包含近战/远程攻击参数。
	// 在编辑器中直接拖入 UItemDataAsset 资产进行赋值，不再需要通过 DataTable 行引用。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|DataAsset")
	TObjectPtr<UItemDataAsset> ItemDataAsset;

	// 物品显示名称，用于背包UI、拾取提示等。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ItemDisplayName;

	// 物品描述文本，用于Tooltip等详细信息展示。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FText ItemDescription;

	// 物品图标，用于背包格子、快捷栏等UI。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	TObjectPtr<UTexture2D> ItemIcon;

	// 物品重量，单位自定义。可用于背包容量计算等。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Weight;

	// 物品品质/稀有度。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	EItemRarity Rarity;

	// 物品最大堆叠数量，为1表示不可堆叠。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxStackCount;

	// 当前堆叠数量。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1", UIMin = "1"))
	int32 StackCount;

protected:

	// 物品公共数据资产引用，由 BeginPlay 中的 ResolveItemData 校验非空。
	// 所有公共字段（DisplayName、Weight、Rarity 等）直接从 ItemDataAsset 读取。
	// 攻击组件通过 Owner 的 GetItemDataAsset() 获取 DataAsset 并 Cast 为派生类型。

// ────────────────────────────────────────── 行为 ──────────────────────────────────────────

public:

	// 拾取物品。当角色拾取此物品时调用，子类可覆盖实现拾取逻辑。
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void PickUp(AActor* NewOwner);

	// 装备物品。当角色装备此物品时调用，子类可覆盖实现装备效果。
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void Equip(AActor* CharacterOwner);

	// 卸下物品。当角色卸下此物品时调用，子类可覆盖实现卸下效果。
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void UnEquip(AActor* CharacterOwner);

	// 丢弃物品。将物品丢弃到世界中时调用，子类可覆盖实现丢弃逻辑。
	UFUNCTION(BlueprintCallable, Category = "Item")
	virtual void Drop();

protected:

	/** 在游戏开始或生成时调用。 */
	virtual void BeginPlay() override;

	/** 解析 ItemDataAsset 指向的公共物品数据。 */
	virtual void ResolveItemData();

	/** 将公共物品数据同步到 AItemBase 的运行时字段和显示组件。 */
	virtual void SyncItemProperties();

public:

	/** 返回物品公共数据资产指针。数据未加载时返回 nullptr。 */
	UFUNCTION(BlueprintPure, Category = "Item|DataAsset")
	virtual UItemDataAsset* GetItemDataAsset() const;
};
